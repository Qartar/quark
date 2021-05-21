// r_font.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_filesystem.h"
#include "win_include.h"
#include "gl/gl_include.h"
#include "r_font.h"
#include "r_glyph.h"
#include "r_image.h"
#include "r_shader.h"

#include <atomic>
#include <numeric> // iota
#include <thread>

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
struct font_sdf
{
    using glyph_info = font::glyph_info;

    //! A contiguous block of characters
    struct block
    {
        char32_t from; //!< First character in block
        char32_t to; //!< Last character in block
        int offset; //!< Number of characters in all preceding blocks
    };

    //! Formatted image data, assumes 32 bpp
    struct image
    {
        std::size_t width;
        std::size_t height;
        std::vector<std::uint32_t> data;
    };

    float line_gap;
    std::vector<block> blocks;
    std::vector<glyph_info> glyphs;

    //! Formatted image data, only used during generation
    image image;

    //! Return the glyph index for the given codepoint
    int codepoint_to_glyph_index(char32_t cp) const {
        for (std::size_t block_index = 0; block_index < blocks.size(); ++block_index) {
            if (cp >= blocks[block_index].from && cp <= blocks[block_index].to) {
                return blocks[block_index].offset + (cp - blocks[block_index].from);
            }
        }
        return 0;
    }
};

//------------------------------------------------------------------------------
void write_sdf(string::view filename, font_sdf const* sdf)
{
    auto f = file::open(filename, file::mode::write);
    int num_blocks = narrow_cast<int>(sdf->blocks.size());
    f.write((file::byte const*)&num_blocks, sizeof(num_blocks));
    for (int ii = 0; ii < num_blocks; ++ii) {
        f.write((file::byte const*)&sdf->blocks[ii].from, sizeof(sdf->blocks[ii].from));
        f.write((file::byte const*)&sdf->blocks[ii].to, sizeof(sdf->blocks[ii].to));
    }
    f.write((file::byte const*)sdf->glyphs.data(), sdf->glyphs.size() * sizeof(font_sdf::glyph_info));
}

//------------------------------------------------------------------------------
bool read_sdf(string::view filename, font_sdf* sdf)
{
    auto f = file::open(filename, file::mode::read);
    if (!f) {
        return false;
    }
    int num_blocks = 0; f.read((file::byte*)&num_blocks, sizeof(num_blocks));
    sdf->blocks.resize(num_blocks);
    int num_glyphs = 0;
    for (int ii = 0; ii < num_blocks; ++ii) {
        f.read((file::byte*)&sdf->blocks[ii].from, sizeof(sdf->blocks[ii].from));
        f.read((file::byte*)&sdf->blocks[ii].to, sizeof(sdf->blocks[ii].to));
        sdf->blocks[ii].offset = num_glyphs;
        num_glyphs += sdf->blocks[ii].to + 1 - sdf->blocks[ii].from;
    }
    sdf->glyphs.resize(num_glyphs);
    f.read((file::byte*)sdf->glyphs.data(), sdf->glyphs.size() * sizeof(font_sdf::glyph_info));
    return true;
}

//------------------------------------------------------------------------------
int pack_rects(int width, std::size_t num_rects, vec2i const* sizes, vec2i* offsets)
{
    std::vector<std::size_t> index(num_rects);
    std::iota(index.begin(), index.end(), 0);
    // sort descending by height, i.e.
    //      sizes[index[N]].y >= sizes[index[N+1]].y
    std::sort(index.begin(), index.end(), [sizes](std::size_t lhs, std::size_t rhs) {
        return (sizes[lhs].y > sizes[rhs].y) || (sizes[lhs].y == sizes[rhs].y && sizes[lhs].x > sizes[rhs].x);
    });

    int pack_height = 0;
    std::deque<vec2i> prev_steps = {vec2i(0, 0), vec2i(width, 0)};
    std::size_t step_index = 0;
    std::deque<vec2i> row_steps;
    bool left_to_right = true;
    int x = 0;

    for (std::size_t ii = 0; ii < num_rects; ++ii) {
        // size of current rect
        std::size_t idx = index[ii];
        vec2i sz = sizes[idx];

        // start a new row
        if ((left_to_right && x + sz.x > width) || (!left_to_right && x - sz.x < 0)) {
            // failed to place any rects on the current row, width is too small
            if (!row_steps.size()) {
                return 0;
            }
            std::swap(row_steps, prev_steps);
            row_steps.clear();
            // extend steps to full width
            prev_steps.front().x = 0;
            prev_steps.back().x = width;

            // determine left_to_right
            left_to_right = prev_steps.front().y < prev_steps.back().y;
            x = left_to_right ? 0 : width;
            step_index = left_to_right ? 0 : prev_steps.size() - 2;
        }

        // place rect

        if (left_to_right) {
            // assume fits on the current step
            offsets[idx] = vec2i(x, prev_steps[step_index].y);

            std::size_t next_index = step_index;
            while (x + sz.x > prev_steps[next_index + 1].x) {
                next_index += 2;
                assert(next_index < prev_steps.size());
            }
            // check if the rect needs to be bumped down
            if (prev_steps[next_index].y > prev_steps[step_index].y) {
                offsets[idx].y += prev_steps[next_index].y - prev_steps[step_index].y;
            }
            // row_steps are always left-to-right
            row_steps.push_back(offsets[idx] + vec2i(0, sz.y));
            row_steps.push_back(offsets[idx] + sz);
            pack_height = max(pack_height, row_steps.back().y);
            x += sz.x;
            // advance step
            step_index = next_index;
        } else {
            // assume fits on the current step
            offsets[idx] = vec2i(x - sz.x, prev_steps[step_index + 1].y);

            std::size_t next_index = step_index;
            while (x - sz.x < prev_steps[next_index].x) {
                next_index -= 2;
                assert(next_index < prev_steps.size()); // i.e. next_index >= 0 via underflow
            }
            // check if the rect needs to be bumped down
            if (prev_steps[next_index + 1].y > prev_steps[step_index + 1].y) {
                offsets[idx].y += prev_steps[next_index + 1].y - prev_steps[step_index + 1].y;
            }
            // row_steps are always left-to-right
            row_steps.push_front(offsets[idx] + sz);
            row_steps.push_front(offsets[idx] + vec2i(0, sz.y));
            pack_height = max(pack_height, row_steps.front().y);
            x -= sz.x;
            // advance step
            step_index = next_index;
        }
    }

    return pack_height;
}

//------------------------------------------------------------------------------
int pack_glyphs(
    glyph const* glyphs,
    std::size_t num_glyphs,
    int width,
    int margin,
    std::vector<std::size_t>& cell_index,
    std::vector<vec2i>& cell_size,
    std::vector<vec2i>& cell_offset)
{
    cell_index.reserve(num_glyphs);
    cell_size.reserve(num_glyphs);

    for (std::size_t ii = 0; ii < num_glyphs; ++ii) {
        cell_index.push_back(cell_size.size());
        // Merge identical glyphs, e.g. non-printable characters
        for (std::size_t jj = 0; jj < ii; ++jj) {
            if (glyphs[ii] == glyphs[jj]) {
                cell_index[ii] = cell_index[jj];
                break;
            }
        }
        // If the current glyph is unique add its rect to the list
        if (cell_index[ii] == cell_size.size()) {
            cell_size.push_back({
                int(glyphs[ii].metrics().gmBlackBoxX + margin * 2),
                int(glyphs[ii].metrics().gmBlackBoxY + margin * 2)
            });
        }
    }

    cell_offset.resize(cell_size.size());
    return pack_rects(width, cell_size.size(), cell_size.data(), cell_offset.data());
}

//------------------------------------------------------------------------------
uint32_t pack_r10g10b10a2(color4 color)
{
    uint32_t r = color.r < 0.f ? 0 : color.r > 1.f ? 0x3ff : uint32_t(color.r * 1023.f + .5f);
    uint32_t g = color.g < 0.f ? 0 : color.g > 1.f ? 0x3ff : uint32_t(color.g * 1023.f + .5f);
    uint32_t b = color.b < 0.f ? 0 : color.b > 1.f ? 0x3ff : uint32_t(color.b * 1023.f + .5f);
    uint32_t a = color.a < 0.f ? 0 : color.a > 1.f ? 0x003 : uint32_t(color.a *    3.f + .5f);
    return (a << 30) | (b << 20) | (g << 10) | (r << 0);
}

//------------------------------------------------------------------------------
void generate_bitmap_r10g10b10a2(glyph const& glyph, mat3 image_to_glyph, std::size_t width, std::size_t height, std::size_t row_stride, uint32_t* data)
{
    for (std::size_t yy = 0; yy < height; ++yy) {
        for (std::size_t xx = 0; xx < width; ++xx) {
            // transform pixel center coordinates in image space to glyph space
            vec2 point = vec2(float(xx) + .5f, float(yy) + .5f) * image_to_glyph;
            vec3 d = glyph.signed_edge_distance_channels(point);
            // discretize d and write to bitmap
            color3 c = color3(d * (1.f / 8.f) + vec3(.5f));
            data[yy * row_stride + xx] = pack_r10g10b10a2(color4(c));
        }
    }
}

//------------------------------------------------------------------------------
void generate_sdf(HDC hdc, font_sdf::block const* blocks, std::size_t num_blocks, int width, font_sdf* sdf)
{
    constexpr int scale = 1;
    constexpr int margin = 2;

    int total_height = 0;
    std::vector<uint32_t> image_data;

    std::size_t num_glyphs = 0;
    for (std::size_t block_index = 0; block_index < num_blocks; ++block_index) {
        num_glyphs += blocks[block_index].to - blocks[block_index].from + 1;
    }

    std::vector<glyph> glyphs;
    glyphs.reserve(num_glyphs);

    // Note: generate_glyph requires a device context for GetGlyphOutlineW
    for (std::size_t block_index = 0; block_index < num_blocks; ++block_index) {
        for (char32_t ii = blocks[block_index].from; ii <= blocks[block_index].to; ++ii) {
            glyphs.push_back(glyph::from_hdc(hdc, ii));
        }
    }

    std::vector<std::size_t> cell_index;
    std::vector<vec2i> cell_size;
    std::vector<vec2i> cell_offset;

    cell_index.reserve(num_glyphs);
    cell_size.reserve(num_glyphs);
    cell_offset.reserve(num_glyphs);

    // Remaining code does not need a device context and could be multithreaded.
    for (std::size_t block_index = 0; block_index < num_blocks; ++block_index) {

        std::vector<std::size_t> block_cell_index;
        std::vector<vec2i> block_cell_size;
        std::vector<vec2i> block_cell_offset;

        int pack_height = pack_glyphs(
            glyphs.data() + blocks[block_index].offset,
            blocks[block_index].to - blocks[block_index].from + 1,
            width,
            margin,
            block_cell_index,
            block_cell_size,
            block_cell_offset);

        // Concatenate block cell information
        for (std::size_t ii = 0; ii < block_cell_index.size(); ++ii) {
            cell_index.push_back(narrow_cast<int>(block_cell_index[ii] + cell_offset.size()));
        }
        for (std::size_t ii = 0; ii < block_cell_offset.size(); ++ii) {
            cell_size.push_back(block_cell_size[ii]);
            cell_offset.push_back(block_cell_offset[ii] + vec2i(0, total_height));
        }

        total_height += pack_height;
    }

    int height = (1 + total_height / width) * width;
    image_data.resize(width * scale * height * scale);
    int row_stride = width * scale;

    sdf->glyphs.resize(glyphs.size());
    std::vector<std::thread> threads;
    std::atomic_size_t index = 0;

    auto thread_fn = [&]() {
        for (std::size_t ii = index++, sz = glyphs.size(); ii < sz; ii = index++) {
            std::size_t jj = cell_index[ii];
            int x0 = cell_offset[jj].x * scale;
            int y0 = cell_offset[jj].y * scale;

            uint32_t* glyph_data = image_data.data() + y0 * row_stride + x0;
            {
                mat3 image_to_glyph = mat3(1.f / float(scale), 0.f, 0.f,
                                            0.f, 1.f / float(scale), 0.f,
                                            float(glyphs[ii].metrics().gmptGlyphOrigin.x - margin),
                                            float(glyphs[ii].metrics().gmptGlyphOrigin.y - cell_size[jj].y + margin),
                                            1.f);

                generate_bitmap_r10g10b10a2(glyphs[ii], image_to_glyph, cell_size[jj].x * scale, cell_size[jj].y * scale, row_stride, glyph_data);
            }

            {
                font_sdf::glyph_info g;
                g.size = vec2((cell_size[jj] - vec2i(2)) * scale);
                g.cell = vec2((cell_offset[jj] + vec2i(1)) * scale);
                g.offset = vec2(float(glyphs[ii].metrics().gmptGlyphOrigin.x) * scale,
                                float(glyphs[ii].metrics().gmptGlyphOrigin.y - (cell_size[jj].y - margin)) * scale);
                g.advance = vec2(float(glyphs[ii].metrics().gmCellIncX * scale), 0);
                sdf->glyphs[ii] = std::move(g);
            }
        }
    };

    // Generate glyph bitmaps using all available hardware threads
    for (unsigned int ii = 0, sz = std::thread::hardware_concurrency(); ii < sz; ++ii) {
        threads.push_back(std::thread(thread_fn));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (std::size_t block_index = 0, block_offset = 0; block_index < num_blocks; ++block_index) {
        font_sdf::block block;
        block.from = blocks[block_index].from;
        block.to = blocks[block_index].to;
        block.offset = narrow_cast<int>(block_offset);
        block_offset += (block.to - block.from + 1);
        sdf->blocks.push_back(std::move(block));
    }

    sdf->image.width = width * scale;
    sdf->image.height = height * scale;
    sdf->image.data = image_data;
}

//------------------------------------------------------------------------------
render::font const* system::load_font(string::view name, int size)
{
    for (auto const& f : _fonts) {
        if (f->compare(name, size)) {
            return f.get();
        }
    }
    _fonts.push_back(std::make_unique<render::font>(this, name, size));
    return _fonts.back().get();
}

font::PFNGLDRAWELEMENTSINSTANCED font::glDrawElementsInstanced = nullptr;

//------------------------------------------------------------------------------
void font::init(get_proc_address_t get_proc_address)
{
    // additional opengl bindings
    glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCED)get_proc_address("glDrawElementsInstanced");
}

//------------------------------------------------------------------------------
font::font(render::system* renderer, string::view name, int size)
    : _name(name)
    , _size(size)
{
    string::buffer data_filename = string::buffer(va("assets/font/atlas_%.*s-%d.dat", name.length(), name.begin(), _size));
    string::buffer image_filename = string::buffer(va("assets/font/atlas_%.*s-%d.dds", name.length(), name.begin(), _size));

    _sdf = std::make_unique<font_sdf>();
    // try to load sdf data from file system
    if (read_sdf(data_filename, _sdf.get())) {
        _image = renderer->load_image(image_filename);
    } else {
#if defined(_WIN32)
        // create font
        HFONT font = CreateFontA(
            _size,                      // cHeight
            0,                          // cWidth
            0,                          // cEscapement
            0,                          // cOrientation
            FW_NORMAL,                  // cWeight
            FALSE,                      // bItalic
            FALSE,                      // bUnderline
            FALSE,                      // bStrikeOut
            ANSI_CHARSET,               // iCharSet
            OUT_TT_PRECIS,              // iOutPrecision
            CLIP_DEFAULT_PRECIS,        // iClipPrecision
            ANTIALIASED_QUALITY,        // iQuality
            FF_DONTCARE|DEFAULT_PITCH,  // iPitchAndFamily
            _name.c_str()               // pszFaceName
        );

        // set our new font to the system
        HFONT prev_font = (HFONT )SelectObject(application::singleton()->window()->hdc(), font);

        const font_sdf::block ascii_block{0, 255, 0};

        generate_sdf(application::singleton()->window()->hdc(), &ascii_block, 1, 512, _sdf.get());
        write_sdf(data_filename, _sdf.get());
        image::write_dds(image_filename, narrow_cast<int>(_sdf->image.width), narrow_cast<int>(_sdf->image.height), image_format::rgb10a2, reinterpret_cast<uint8_t const*>(_sdf->image.data.data()));
        _image = renderer->load_image(image_filename);

        // restore previous font and delete our created font
        SelectObject(application::singleton()->window()->hdc(), prev_font);
        DeleteObject(font);
#endif // defined(_WIN32)
    }

    if (_sdf.get()) {
        _vao = gl::vertex_array({
            gl::vertex_array_binding{1, {
                gl::vertex_array_attrib{2, GL_FLOAT, gl::vertex_attrib_type::float_, offsetof(instance, position)},
                gl::vertex_array_attrib{1, GL_INT, gl::vertex_attrib_type::integer, offsetof(instance, index)},
                gl::vertex_array_attrib{4, GL_UNSIGNED_BYTE, gl::vertex_attrib_type::normalized, offsetof(instance, color)},
            }}
        });

        _ibo = gl::index_buffer<uint16_t>(gl::buffer_usage::static_, gl::buffer_access::draw, {0, 1, 2, 1, 3, 2});
        _vbo = gl::vertex_buffer<instance>(gl::buffer_usage::stream, gl::buffer_access::draw, max_instances);
        _ssbo = gl::shader_storage_buffer<glyph_info>(
            gl::buffer_usage::static_, gl::buffer_access::draw,
            narrow_cast<GLsizei>(_sdf->glyphs.size()), _sdf->glyphs.data());

        _vao.bind_buffer(_ibo);
        _vao.bind_buffer(_vbo, 0);

        gl::index_buffer<int>().bind();
        gl::vertex_buffer<int>().bind();

        _shader = renderer->load_shader("font", "assets/shader/font.vsh", "assets/shader/font.fsh");
    }
}

//------------------------------------------------------------------------------
font::~font()
{
}

//------------------------------------------------------------------------------
bool font::compare(string::view name, int size) const
{
    return _name == name
        && _size == size;
}

//------------------------------------------------------------------------------
void font::draw(string::view string, vec2 position, color4 color, vec2 scale) const
{
    if (!_sdf) {
        return;
    }

    _shader->program().use();
    _vao.bind();
    _ssbo.bind_base(0);
    _image->texture().bind();

    std::vector<instance> instances;

    int xoffs = 0;

    int r = static_cast<int>(color.r * 255.f + .5f);
    int g = static_cast<int>(color.g * 255.f + .5f);
    int b = static_cast<int>(color.b * 255.f + .5f);
    int a = static_cast<int>(color.a * 255.f + .5f);

    char const* cursor = string.begin();
    char const* end = string.end();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glScalef(scale.x, scale.y, 1);
    glTranslatef(position.x / scale.x, position.y / scale.y, 0);

    while (cursor < end) {
        char const* next = find_color(cursor, end);
        if (!next) {
            next = end;
        }

        uint32_t packed_color = (a << 24) | (b << 16) | (g << 8) | r;

        // Convert codepoints to character indices
        while (cursor < next && instances.size() < max_instances) {
            int glyph_index = _sdf->codepoint_to_glyph_index(*cursor++);
            instances.push_back({
                    vec2(float(xoffs), 0),
                    glyph_index,
                    packed_color,
                });
            xoffs += int(_sdf->glyphs[glyph_index].advance.x);
        }

        // Scan color and potentially continue filling instance data
        if (instances.size() < max_instances && cursor < end) {
            if (!get_color(cursor, r, g, b)) {
                r = static_cast<int>(color.r * 255.f + .5f);
                g = static_cast<int>(color.g * 255.f + .5f);
                b = static_cast<int>(color.b * 255.f + .5f);
            }
            cursor += 4;
            if (cursor < end) {
                continue;
            }
        }

        if (instances.size()) {
            _vbo.upload(0, instances.size(), instances.data());

            glDrawElementsInstanced(
                GL_TRIANGLES,
                6,
                GL_UNSIGNED_SHORT,
                nullptr,
                narrow_cast<GLsizei>(instances.size()));

            instances.resize(0);
        }
    }

    glPopMatrix();

    gl::program().use();
    gl::vertex_array().bind();
    gl::texture2d().bind();

    gl::index_buffer<int>().bind();
    gl::vertex_buffer<int>().bind();
}

//------------------------------------------------------------------------------
vec2 font::size(string::view string, vec2 scale) const
{
    if (!_sdf) {
        return vec2_zero;
    }

    vec2i size(0, _size);
    char const* cursor = string.begin();
    char const* end = string.end();

    while (cursor < end) {
        char const* next = find_color(cursor, end);
        if (!next) {
            next = end;
        }

        while (cursor < next) {
            int glyph_index = _sdf->codepoint_to_glyph_index(*cursor++);
            size.x += int(_sdf->glyphs[glyph_index].advance.x);
        }

        if (cursor < end) {
            cursor += 4;
        }
    }

    return vec2(size) * scale;
}

} // namespace render

// r_font.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "win_include.h"
#include <GL/gl.h>

////////////////////////////////////////////////////////////////////////////////
//namespace {

struct unicode_block {
    int from;
    int to;
    char const* name;
};

constexpr unicode_block unicode_blocks[] = {
    { 0x0000, 0x007f, "Basic Latin" },
    { 0x0080, 0x00ff, "Latin-1 Supplement" },
    { 0x0100, 0x017f, "Latin Extended-A" },
    { 0x0180, 0x024f, "Latin Extended-B" },
    // { 0x0250, 0x02af, "IPA Extensions" },
    // { 0x02b0, 0x02ff, "Spacing Modifier Letters" },
    // { 0x0300, 0x036f, "Combining Diacritic Marks" },
    { 0x0370, 0x03ff, "Greek and Coptic" },
    { 0x0400, 0x04ff, "Cyrillic" },
    { 0x0500, 0x052f, "Cyrillic Supplement" },
    { 0x0530, 0x058f, "Armenian" },
    { 0x0590, 0x05ff, "Hebrew" },
    { 0x0600, 0x06ff, "Arabic" },
    // ...
    { 0xfe70, 0xfeff, "Arabic Presentation Forms-B" },
};

template<int size>
constexpr int unicode_block_size(int index, unicode_block const (&blocks)[size])
{
    return blocks[index].to - blocks[index].from + 1;
}

template<int size>
constexpr int unicode_block_offset(int index, unicode_block const (&blocks)[size])
{
    return index > 0
        ? unicode_block_offset(index - 1, blocks) + unicode_block_size(index - 1, blocks)
        : 0;
}

template<int... Is> struct seq{};
template<int N, int... Is> struct gen_seq : gen_seq<N - 1, N - 1, Is...>{};
template<int... Is> struct gen_seq<0, Is...> : seq<Is...>{};

struct unicode_block_info : unicode_block {
    int size;
    int offset;
};

template<int size>
constexpr unicode_block_info make_unicode_block_info(int index)
{
    return {
        unicode_blocks[index].from,
        unicode_blocks[index].to,
        unicode_blocks[index].name,
        unicode_block_size(index, unicode_blocks),
        unicode_block_offset(index, unicode_blocks),
    };
}

template<int size, int... Is>
constexpr std::array<unicode_block_info, size> make_unicode_blocks(seq<Is...>)
{
    return {{ make_unicode_block_info<size>(Is)... }};
}

template<int size>
constexpr std::array<unicode_block_info, size> make_unicode_blocks(unicode_block const (&)[size])
{
    return make_unicode_blocks<size>(gen_seq<size>{});
}

constexpr auto unicode_data_lol = make_unicode_blocks(unicode_blocks);

//------------------------------------------------------------------------------
template<std::size_t S, std::size_t N = 0>
constexpr int character_count(unicode_block const (&c)[S])
{
    if constexpr (N < S) {
        return (c[N].to - c[N].from + 1) + character_count<S, N + 1>(c);
    } else {
        return 0;
    }
}

constexpr int character_max = unicode_blocks[countof(unicode_blocks) - 1].to - unicode_blocks[0].from;
constexpr int character_num = character_count(unicode_blocks);

constexpr int block_search(int min_index, int max_index, int codepoint)
{
    // Binary search
    int mid_index = (max_index + min_index) / 2;
    if (min_index == max_index) {
        return min_index;
    } else {
        if (codepoint >= unicode_blocks[mid_index].to) {
            return block_search(mid_index, max_index, codepoint);
        } else if (codepoint < unicode_blocks[mid_index].from) {
            return block_search(min_index, max_index, codepoint);
        } else {
            return mid_index;
        }
    }
}

constexpr int character_block(int codepoint)
{
    return block_search(0, int(countof(unicode_blocks) - 1), codepoint);
}

constexpr int block_size(int block_index)
{
    return unicode_blocks[block_index].to
         - unicode_blocks[block_index].from
         + 1;
}

constexpr int block_offset(int block_index)
{
    if (block_index > 0) {
        return block_size(block_index - 1) + block_offset(block_index - 1);
    } else {
        return 0;
    }
}

constexpr int character_index(int codepoint)
{
    int block = character_block(codepoint);
    int offset = block_offset(block);
    return codepoint - unicode_blocks[block].from + offset;
}

constexpr int block_search_index(int min_index, int max_index, int index)
{
    // Linear search
    if (index >= block_offset(min_index) && index < block_offset(min_index) + block_size(min_index)) {
        return min_index;
    } else if (min_index < max_index) {
        return block_search_index(min_index + 1, max_index, index);
    }
}

constexpr int character_codepoint(int index)
{
    int block = block_search_index(0, int(countof(unicode_blocks) - 1), index);
    return unicode_blocks[block].from + index - block_offset(block);
}

constexpr int kaf_index = character_index(0x0643);
constexpr int kaf_codepoint = character_codepoint(kaf_index);

//} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
namespace render {

HFONT font::_system_font = NULL;
HFONT font::_active_font = NULL;

//------------------------------------------------------------------------------
render::font const* system::load_font(string::view name, int size)
{
    for (auto const& f : _fonts) {
        if (f->compare(name, size)) {
            return f.get();
        }
    }
    _fonts.push_back(std::make_unique<render::font>(name, size));
    return _fonts.back().get();
}

//------------------------------------------------------------------------------
font::font(string::view name, int size)
    : _name(name)
    , _size(size)
    , _handle(NULL)
    , _list_base(0)
    , _char_width{0}
{
    (void)unicode_data_lol[0];

    GLYPHMETRICS gm;
    MAT2 m;

    // allocate lists
    _list_base = glGenLists(256);

    // create font
    _handle = CreateFontA(
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
    HFONT prev_font = (HFONT )SelectObject(application::singleton()->window()->hdc(), _handle);

    // generate font bitmaps with selected HFONT
    memset( &m, 0, sizeof(m) );
    m.eM11.value = 1;
    m.eM12.value = 0;
    m.eM21.value = 0;
    m.eM22.value = 1;

    wglUseFontBitmapsW(application::singleton()->window()->hdc(), 0xfe2f, 256, _list_base);
    for (int ii = 0; ii < kNumChars; ++ii) {
        GetGlyphOutlineW(application::singleton()->window()->hdc(), ii, GGO_METRICS, &gm, 0, NULL, &m);
        _char_width[ii] = gm.gmCellIncX;
    }

    // restore previous font
    SelectObject(application::singleton()->window()->hdc(), prev_font);
}

//------------------------------------------------------------------------------
font::~font()
{
    // restore system font if this is the active font
    if (_active_font == _handle) {
        glListBase(0);
        SelectObject(application::singleton()->window()->hdc(), _system_font);

        _active_font = _system_font;
        _system_font = NULL;
    }

    // delete from opengl
    if (_list_base) {
        glDeleteLists(_list_base, kNumChars);
    }

    // delete font from gdi
    if (_handle) {
        DeleteObject(_handle);
    }
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
    // activate font if it isn't already
    if (_active_font != _handle) {
        HFONT prev_font = (HFONT )SelectObject(application::singleton()->window()->hdc(), _handle);

        // keep track of the system font so it can be restored later
        if (_system_font == NULL) {
            _system_font = prev_font;
        }

        glListBase(_list_base);
        _active_font = _handle;
    }

    int xoffs = 0;

    int r = static_cast<int>(color.r * 255.5f);
    int g = static_cast<int>(color.g * 255.5f);
    int b = static_cast<int>(color.b * 255.5f);
    int a = static_cast<int>(color.a * 255.5f);

    char const* cursor = string.begin();
    char const* end = string.end();

    while (cursor < end) {
        char const* next = find_color(cursor, end);
        if (!next) {
            next = end;
        }

        glColor4ub(narrow_cast<uint8_t>(r),
                   narrow_cast<uint8_t>(g),
                   narrow_cast<uint8_t>(b),
                   narrow_cast<uint8_t>(a));
        glRasterPos2f(position.x + xoffs * scale.x, position.y);

        char const* mbcs = cursor;
        while (!(*mbcs & 0x80) && mbcs < next) {
            ++mbcs;
        }

        next = mbcs;

        if (*cursor & 0x80) {
            int mb = 0;
            xoffs += _char_width[(uint8_t)*cursor];
            if ((*cursor & 0xf8) == 0xf0) {
                mb = ((*cursor++ & 0x07) << 18)
                   | ((*cursor++ & 0x3f) << 12)
                   | ((*cursor++ & 0x3f) <<  6)
                   | ((*cursor++ & 0x3f) <<  0);
            } else if ((*cursor & 0xf0) == 0xe0) {
                mb = ((*cursor++ & 0x0f) << 12)
                   | ((*cursor++ & 0x3f) <<  6)
                   | ((*cursor++ & 0x3f) <<  0);
            } else if ((*cursor & 0xe0) == 0xc0) {
                mb = ((*cursor++ & 0x1f) <<  6)
                   | ((*cursor++ & 0x3f) <<  0);
            }
            glCallLists(1, GL_INT, &mb);
            next = cursor;
        } else {
            glCallLists(static_cast<GLsizei>(next - cursor), GL_UNSIGNED_BYTE, cursor);
        }

        while (cursor < next) {
            xoffs += _char_width[(uint8_t)*cursor++];
        }

        if (cursor < end && is_color(cursor)) {
            if (!get_color(cursor, r, g, b)) {
                r = static_cast<int>(color.r * 255.5f);
                g = static_cast<int>(color.g * 255.5f);
                b = static_cast<int>(color.b * 255.5f);
            }
            cursor += 4;
        }
    }
}

//------------------------------------------------------------------------------
vec2 font::size(string::view string, vec2 scale) const
{
    vec2i size(0, _size);
    char const* cursor = string.begin();
    char const* end = string.end();

    while (cursor < end) {
        char const* next = find_color(cursor, end);
        if (!next) {
            next = end;
        }

        while (cursor < next) {
            size.x += _char_width[(uint8_t)*cursor++];
        }

        if (cursor < end) {
            cursor += 4;
        }
    }

    return vec2(size) * scale;
}

} // namespace render

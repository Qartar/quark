// r_font.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "win_include.h"
#include <GL/gl.h>

////////////////////////////////////////////////////////////////////////////////
namespace {

struct unicode_block {
    int from;
    int to;
    char const* name;
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

template<int... Is> struct indices{};
template<int N, int... Is> struct build_indices : build_indices<N - 1, N - 1, Is...>{};
template<int... Is> struct build_indices<0, Is...> : indices<Is...>{};

struct unicode_block_layout : unicode_block {
    int size;
    int offset;
};

template<int size>
constexpr unicode_block_layout build_unicode_block_layout(unicode_block const (&blocks)[size], int index)
{
    return {
        blocks[index].from,
        blocks[index].to,
        blocks[index].name,
        unicode_block_size(index, blocks),
        unicode_block_offset(index, blocks),
    };
}

template<int size, int... Is>
constexpr std::array<unicode_block_layout, size> build_unicode_block_layout(unicode_block const (&blocks)[size], indices<Is...>)
{
    return {{ build_unicode_block_layout<size>(blocks, Is)... }};
}

template<int size>
constexpr std::array<unicode_block_layout, size> build_unicode_block_layout(unicode_block const (&blocks)[size])
{
    return build_unicode_block_layout<size>(blocks, build_indices<size>{});
}

template<int sz> class unicode_block_data
{
public:
    static constexpr int invalid_codepoint = 0xffff; // not a character
    static constexpr int unknown_codepoint = 0xfffd; // replacement character
    static constexpr int invalid_index = -1;

public:
    constexpr unicode_block_data(unicode_block const (&blocks)[sz])
        : _blocks(build_unicode_block_layout(blocks))
    {}

    constexpr auto begin() const { return _blocks.begin(); }

    constexpr auto end() const { return _blocks.end(); }

    constexpr int size() const { return sz; }

    constexpr unicode_block_layout operator[](int index) const { return _blocks[index]; }

    constexpr int max_index() const { return _blocks.back().offset + _blocks.back().size - 1; }

    constexpr int codepoint_to_index(int codepoint) const {
        // linear search
        for (int ii = 0; ii < sz && _blocks[ii].from <= codepoint; ++ii) {
            if (_blocks[ii].from <= codepoint && codepoint <= _blocks[ii].to) {
                return codepoint - _blocks[ii].from + _blocks[ii].offset;
            }
        }
        return invalid_index;
    }

    constexpr int index_to_codepoint(int index) const {
        // linear search
        for (int ii = 0; ii < sz && _blocks[ii].offset <= index; ++ii) {
            if (_blocks[ii].offset <= index && index < _blocks[ii].offset + _blocks[ii].size) {
                return _blocks[ii].from + index - _blocks[ii].offset;
            }
        }
        return unknown_codepoint;
    }

    static constexpr bool is_ascii(char const* s) {
        return (*s & 0x80) == 0x00;
    }

    static constexpr bool is_utf8(char const* s) {
        if ((*s & 0xf8) == 0xf0) {
            return ((*++s & 0xc0) == 0x80)
                 | ((*++s & 0xc0) == 0x80)
                 | ((*++s & 0xc0) == 0x80);
        } else if ((*s & 0xf0) == 0xe0) {
            return ((*++s & 0xc0) == 0x80)
                 | ((*++s & 0xc0) == 0x80);
        } else if ((*s & 0xe0) == 0xc0) {
            return ((*++s & 0xc0) == 0x80);
        } else {
            return (*s & 0x80) == 0x00;
        }
    }

    static constexpr int decode(char const*& s) {
        assert(is_utf8(s));
        if ((*s & 0xf8) == 0xf0) {
            return ((*s++ & 0x07) << 18)
                 | ((*s++ & 0x3f) << 12)
                 | ((*s++ & 0x3f) <<  6)
                 | ((*s++ & 0x3f) <<  0);
        } else if ((*s & 0xf0) == 0xe0) {
            return ((*s++ & 0x0f) << 12)
                 | ((*s++ & 0x3f) <<  6)
                 | ((*s++ & 0x3f) <<  0);
        } else if ((*s & 0xe0) == 0xc0) {
            return ((*s++ & 0x1f) <<  6)
                 | ((*s++ & 0x3f) <<  0);
        } else {
            return *s++;
        }
    }

    constexpr int decode_index(char const*& s) const {
        return codepoint_to_index(decode(s));
    }

protected:
    std::array<unicode_block_layout, sz> _blocks;
};

#if defined(__INTELLISENSE__)
constexpr unicode_block_data<12> unicode_data(
#else
constexpr unicode_block_data unicode_data(
#endif
    {
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
        // { 0xff00, 0xffef, "Halfwidth and Fullwidth Forms" },
        { 0xfff0, 0xffff, "Specials" },
    }
);

constexpr int kaf_index = unicode_data.codepoint_to_index(0x0643);
constexpr int kaf_codepoint = unicode_data.index_to_codepoint(kaf_index);
static_assert(unicode_data.index_to_codepoint(unicode_data.codepoint_to_index(0x0643)) == 0x0643, "");

} // anonymous namespace

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
    GLYPHMETRICS gm;
    MAT2 m;

    // allocate lists
    _list_base = glGenLists(unicode_data.max_index() + 1);
    _char_width.resize(unicode_data.max_index() + 1);

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

    for (auto block : unicode_data) {
        wglUseFontBitmapsW(application::singleton()->window()->hdc(), block.from, block.size, _list_base + block.offset);
        for (int ii = 0; ii < block.size; ++ii) {
            GetGlyphOutlineW(application::singleton()->window()->hdc(), block.from + ii, GGO_METRICS, &gm, 0, NULL, &m);
            _char_width[block.offset + ii] = gm.gmCellIncX;
        }
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
        glDeleteLists(_list_base, unicode_data.max_index() + 1);
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

        char const* utf8 = cursor;
        while (unicode_data.is_ascii(utf8) && utf8 < next) {
            ++utf8;
        }

        next = utf8;

        if (!unicode_data.is_ascii(cursor) && unicode_data.is_utf8(cursor)) {
            int ch = unicode_data.decode_index(cursor);
            glCallLists(1, GL_INT, &ch);
            xoffs += _char_width[ch];
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
            int ch = unicode_data.decode_index(cursor);
            size.x += _char_width[ch];
        }

        if (cursor < end) {
            cursor += 4;
        }
    }

    return vec2(size) * scale;
}

} // namespace render

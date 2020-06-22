// r_font.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "gl/gl_include.h"

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
    _list_base = glGenLists(kNumChars);

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

    wglUseFontBitmapsA(application::singleton()->window()->hdc(), 0, kNumChars-1, _list_base);
    for (int ii = 0; ii < kNumChars; ++ii) {
        GetGlyphOutlineA(application::singleton()->window()->hdc(), ii, GGO_METRICS, &gm, 0, NULL, &m);
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
        glCallLists(static_cast<GLsizei>(next - cursor), GL_UNSIGNED_BYTE, cursor);

        while (cursor < next) {
            xoffs += _char_width[(uint8_t)*cursor++];
        }

        if (cursor < end) {
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

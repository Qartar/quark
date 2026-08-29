// r_outline.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "r_outline.h"

#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
outline::outline(vec2 const* vertices, std::size_t num_vertices)
{
    if (!num_vertices) {
        return;
    }

    std::size_t xmin = 0;
    std::vector<vec2f> s(num_vertices);
    for (std::size_t ii = 0; ii < num_vertices; ++ii) {
        s[ii] = vec2f(vertices[ii]);
        if (s[ii].x < s[xmin].x) {
            xmin = ii;
        } else if (s[ii].x == s[xmin].x && s[ii].y >= 0 && s[ii].y < s[xmin].y) {
            xmin = ii;
        }
    }
    // Reverse and rotate to ensure all outlines have the same orientation for
    // triangulation. Eventually triangles should be provided explicitly, making
    // this logic unnecessary.
    if (xmin != 0) {
        std::rotate(s.begin(), s.begin() + xmin, s.end());
    }
    if (s[1].y < 0) {
        std::reverse(s.begin(), s.end());
        std::rotate(s.begin(), s.end() - 1, s.end());
    }

    // Create a single triangle strip. NOTE: some turrets will have triangles that
    // extend outside the outline, eg. angled rangefinders. This is good enough for now.
    std::vector<unsigned short> idx;
    {
        short i0 = 0;
        short i1 = narrow_cast<short>(num_vertices - 1);
        if (vertices[0].y == 0) {
            idx.push_back(0);
            idx.push_back(i1);
            idx.push_back(1);
            ++i0;
        }

        while (i0 * 2 + 2 < num_vertices) {
            idx.push_back(i0);
            idx.push_back(i1);
            idx.push_back(i0 + 1);

            idx.push_back(i0 + 1);
            idx.push_back(i1);
            idx.push_back(i1 - 1);

            ++i0;
            --i1;
        }

        if (vertices[num_vertices / 2].y == 0) {
            idx.push_back(narrow_cast<short>(num_vertices / 2 - 1));
            idx.push_back(narrow_cast<short>(num_vertices / 2));
            idx.push_back(narrow_cast<short>(num_vertices / 2 + 1));
        }
    }

    _vertices = gl::vertex_buffer<vec2f>(
        gl::buffer_usage::static_,
        gl::buffer_access::draw,
        s.size(),
        s.data());

    _indices = gl::index_buffer<unsigned short>(
        gl::buffer_usage::static_,
        gl::buffer_access::draw,
        idx.size(),
        idx.data());
}

} // namespace render

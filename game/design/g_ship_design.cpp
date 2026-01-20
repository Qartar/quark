// g_ship_design.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "design/g_ship_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

#define QBZ(a,b,c,t)        \
    (((1-t)*(1-t)*(a)+2*(1-t)*t*(b)+t*t*(c)))

#define QBZ8(a,b,c)         \
    QBZ(a,b,c,0),           \
    QBZ(a,b,c,0.125f),      \
    QBZ(a,b,c,0.25f),       \
    QBZ(a,b,c,0.375f),      \
    QBZ(a,b,c,0.5f),        \
    QBZ(a,b,c,0.625f),      \
    QBZ(a,b,c,0.75f),       \
    QBZ(a,b,c,0.875)

#define SHIP(L,B) {         \
    QBZ8(vec2(0.5f * L, 0.f), vec2(0.3f * L, 0.5f * B), vec2(0.f, 0.5f * B)),   \
    QBZ8(vec2(0.f, 0.5f * B), vec2(-0.5f * L, 0.5f * B), vec2(-0.5f * L, 0.f)),   \
    QBZ8(vec2(-0.5f * L, 0.f), vec2(-0.5f * L, -0.5f * B), vec2(0.f, -0.5f * B)),   \
    QBZ8(vec2(0.f, -0.5f * B), vec2(0.3f * L, -0.5f * B), vec2(0.5f * L, 0.f)),   }

const vec2 ship_hulls[][32] = {
    // yamato-class battleship
    SHIP(263.f, 39.f),

    // iowa-class battleship
    SHIP(270.f, 33.f),

    // king george v-class battleship
    SHIP(227.f, 31.5f),

    // deutschland-class cruiser
    SHIP(186.f, 21.7f),

    // town-class cruiser
    SHIP(180.f, 19.f),

    // tribal-class destroyer
    SHIP(115.f, 11.f),
};

const vec2 yamato_verts[] = {
    vec2(131.50f, 0.00f), vec2(126.49f, 6.20f), vec2(52.90f, 17.40f), vec2(23.41f, 19.42f), vec2(-73.95f, 19.60f), vec2(-77.85f, 17.52f), vec2(-99.05f, 17.90f), vec2(-101.14f, 16.36f), vec2(-101.15f, 13.87f), vec2(-128.36f, 4.63f), vec2(-131.50f, 0.00f), vec2(-128.36f, -4.63f), vec2(-101.15f, -13.87f), vec2(-101.14f, -16.36f), vec2(-99.05f, -17.90f), vec2(-77.85f, -17.52f), vec2(-73.95f, -19.60f), vec2(23.41f, -19.42f), vec2(52.90f, -17.40f), vec2(126.49f, -6.20f)
};

#define DEGV(x,y) vec2(math::deg2rad(x), math::deg2rad(y))

//------------------------------------------------------------------------------
const ship_design ship_yamato_battleship =
{
    /* name */              string::buffer("Yamato"),
    /* length */            263.f,
    /* beam */              39.f,
    /* displacement */      0.f,

    /* turrets */
    {
        {
            /* position */          vec2(52,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_yamato_46cm,
        },
        {
            /* position */          vec2(30,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_yamato_46cm,
        },
        {
            /* position */          vec2(-65,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_yamato_46cm,
        },
    },

    /* hull_outline */      {yamato_verts, yamato_verts + countof(yamato_verts)},
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(yamato_verts)}}},
};

//------------------------------------------------------------------------------
const ship_design ship_iowa_battleship =
{
    /* name */              string::buffer("Iowa"),
    /* length */            270.f,
    /* beam */              33.f,
    /* displacement */      0.f,

    /* turrets */
    {
        {
            /* position */          vec2(48,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_iowa_16in,
        },
        {
            /* position */          vec2(24,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_iowa_16in,
        },
        {
            /* position */          vec2(-48,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_iowa_16in,
        },
    },

    /* hull_outline */      SHIP(270.f, 33.f),
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(ship_hulls[1])}}},
};

//------------------------------------------------------------------------------
const ship_design ship_king_george_v_battleship =
{
    /* name */              string::buffer("King George V"),
    /* length */            227.f,
    /* beam */              31.5f,
    /* displacement */      0.f,

    /* turrets */
    {
        {
            /* position */          vec2(40,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_kgv_14in_quad,
        },
        {
            /* position */          vec2(16,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_kgv_14in_twin,
        },
        {
            /* position */          vec2(-40,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_kgv_14in_quad,
        },
    },

    /* hull_outline */      SHIP(227.f, 31.5f),
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(ship_hulls[2])}}},
};

//------------------------------------------------------------------------------
const ship_design ship_deutschland_cruiser =
{
    /* name */              string::buffer("Deutschland"),
    /* length */            186.f,
    /* beam */              21.7f,
    /* displacement */      0.f,

    /* turrets */
    {
        {
            /* position */          vec2(32,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_deutschland_28cm,
        },
        {
            /* position */          vec2(-32,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_deutschland_28cm,
        },
    },

    /* hull_outline */      SHIP(186.f, 21.7f),
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(ship_hulls[3])}}},
};

//------------------------------------------------------------------------------
const ship_design ship_town_cruiser =
{
    /* name */              string::buffer("Town"),
    /* length */            180.f,
    /* beam */              19.f,
    /* displacement */      0.f,

    /* turrets */
    {
        {
            /* position */          vec2(32,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_town_6in,
        },
        {
            /* position */          vec2(16,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_town_6in,
        },
        {
            /* position */          vec2(-16,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_town_6in,
        },
        {
            /* position */          vec2(-32,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_town_6in,
        },
    },

    /* hull_outline */      SHIP(180.f, 19.f),
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(ship_hulls[4])}}},
};

//------------------------------------------------------------------------------
const ship_design ship_tribal_destroyer =
{
    /* name */              string::buffer("Tribal"),
    /* length */            115.f,
    /* beam */              11.f,
    /* displacement */      0.f,

    /* turrets */
    {
        {
            /* position */          vec2(24,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_tribal_4_7in,
        },
        {
            /* position */          vec2(12,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_tribal_4_7in,
        },
        {
            /* position */          vec2(-12,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_tribal_4_7in,
        },
        {
            /* position */          vec2(-24,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_tribal_4_7in,
        },
    },

    /* hull_outline */      SHIP(115.f, 11.f),
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(ship_hulls[5])}}},
};

} // namespace game

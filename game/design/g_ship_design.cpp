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
    vec2(131.50f, 0.00f), vec2(131.15f, 2.51f), vec2(130.22f, 4.27f), vec2(128.72f, 5.48f), vec2(126.49f, 6.20f), vec2(107.26f, 8.54f), vec2(82.73f, 12.28f), vec2(52.90f, 17.40f), vec2(37.10f, 18.66f), vec2(23.41f, 19.42f), vec2(-73.95f, 19.60f), vec2(-77.85f, 17.52f), vec2(-99.05f, 17.90f), vec2(-100.21f, 17.72f), vec2(-100.91f, 17.21f), vec2(-101.14f, 16.36f), vec2(-101.15f, 13.87f), vec2(-108.85f, 11.99f), vec2(-115.95f, 9.82f), vec2(-122.45f, 7.37f), vec2(-128.36f, 4.63f), vec2(-130.11f, 3.14f), vec2(-131.15f, 1.59f), vec2(-131.50f, 0.00f), vec2(-131.15f, -1.59f), vec2(-130.11f, -3.14f), vec2(-128.36f, -4.63f), vec2(-122.45f, -7.37f), vec2(-115.95f, -9.82f), vec2(-108.85f, -11.99f), vec2(-101.15f, -13.87f), vec2(-101.14f, -16.36f), vec2(-100.91f, -17.21f), vec2(-100.21f, -17.72f), vec2(-99.05f, -17.90f), vec2(-77.85f, -17.52f), vec2(-73.95f, -19.60f), vec2(23.41f, -19.42f), vec2(37.10f, -18.66f), vec2(52.90f, -17.40f), vec2(82.73f, -12.28f), vec2(107.26f, -8.54f), vec2(126.49f, -6.20f), vec2(128.72f, -5.48f), vec2(130.22f, -4.27f), vec2(131.15f, -2.51f),
};

const vec2 fuso_verts[] = {
    vec2(-105.00f, 0.00f), vec2(-104.59f, 0.94f), vec2(-103.07f, 2.10f), vec2(-99.98f, 3.54f), vec2(-94.86f, 5.25f), vec2(-87.51f, 7.17f), vec2(-78.36f, 9.12f), vec2(-68.77f, 10.78f), vec2(-67.68f, 11.34f), vec2(-63.28f, 12.24f), vec2(-54.38f, 13.40f), vec2(-41.88f, 14.60f), vec2(-30.03f, 15.43f), vec2(-23.10f, 15.64f), vec2(12.72f, 15.61f), vec2(23.41f, 15.18f), vec2(32.76f, 14.38f), vec2(42.12f, 13.08f), vec2(62.25f, 9.22f), vec2(68.65f, 8.47f), vec2(75.04f, 8.19f), vec2(79.61f, 7.85f), vec2(85.95f, 6.72f), vec2(93.42f, 4.81f), vec2(100.07f, 2.60f), vec2(103.90f, 0.89f), vec2(105.00f, 0.00f), vec2(103.90f, -0.89f), vec2(100.07f, -2.60f), vec2(93.42f, -4.81f), vec2(85.95f, -6.72f), vec2(79.61f, -7.85f), vec2(75.04f, -8.19f), vec2(68.65f, -8.47f), vec2(62.25f, -9.22f), vec2(42.12f, -13.08f), vec2(32.76f, -14.38f), vec2(23.41f, -15.18f), vec2(12.72f, -15.61f), vec2(-23.10f, -15.64f), vec2(-30.03f, -15.43f), vec2(-41.88f, -14.60f), vec2(-54.38f, -13.40f), vec2(-63.28f, -12.24f), vec2(-67.68f, -11.34f), vec2(-68.77f, -10.78f), vec2(-78.36f, -9.12f), vec2(-87.51f, -7.17f), vec2(-94.86f, -5.25f), vec2(-99.98f, -3.54f), vec2(-103.07f, -2.10f), vec2(-104.59f, -0.94f),
};

const vec2 iowa_verts[] = {
    vec2(-135.00f, 0.00f), vec2(-134.55f, 1.82f), vec2(-133.27f, 3.63f), vec2(-131.02f, 5.46f), vec2(-127.63f, 7.31f), vec2(-122.88f, 9.18f), vec2(-116.56f, 11.05f), vec2(-108.49f, 12.85f), vec2(-98.79f, 14.50f), vec2(-88.25f, 15.84f), vec2(-78.88f, 16.67f), vec2(-73.39f, 16.93f), vec2(-46.77f, 17.47f), vec2(-20.31f, 17.61f), vec2(6.00f, 17.35f), vec2(21.45f, 16.83f), vec2(36.77f, 15.59f), vec2(53.70f, 13.45f), vec2(80.47f, 9.09f), vec2(100.61f, 6.02f), vec2(116.13f, 4.29f), vec2(130.97f, 3.33f), vec2(132.64f, 3.47f), vec2(133.92f, 2.87f), vec2(134.79f, 1.60f), vec2(135.00f, 0.00f), vec2(134.79f, -1.60f), vec2(133.92f, -2.87f), vec2(132.64f, -3.47f), vec2(130.97f, -3.33f), vec2(116.13f, -4.29f), vec2(100.61f, -6.02f), vec2(80.47f, -9.09f), vec2(53.70f, -13.45f), vec2(36.77f, -15.59f), vec2(21.45f, -16.83f), vec2(6.00f, -17.35f), vec2(-20.31f, -17.61f), vec2(-46.77f, -17.47f), vec2(-73.39f, -16.93f), vec2(-78.88f, -16.67f), vec2(-88.25f, -15.84f), vec2(-98.79f, -14.50f), vec2(-108.49f, -12.85f), vec2(-116.56f, -11.05f), vec2(-122.88f, -9.18f), vec2(-127.63f, -7.31f), vec2(-131.02f, -5.46f), vec2(-133.27f, -3.63f), vec2(-134.55f, -1.82f),
};

const vec2 kgv_verts[] = {
    vec2(-113.00f, 0.00f), vec2(-112.44f, 0.88f), vec2(-110.06f, 2.61f), vec2(-105.43f, 5.03f), vec2(-99.17f, 7.58f), vec2(-92.14f, 9.82f), vec2(-84.97f, 11.51f), vec2(-78.02f, 12.58f), vec2(-60.41f, 14.13f), vec2(-41.40f, 15.12f), vec2(-22.66f, 15.47f), vec2(15.01f, 15.40f), vec2(28.22f, 15.06f), vec2(43.62f, 14.08f), vec2(58.83f, 12.56f), vec2(72.83f, 10.61f), vec2(85.28f, 8.32f), vec2(96.14f, 5.76f), vec2(105.51f, 2.97f), vec2(113.50f, 0.00f), vec2(105.51f, -2.97f), vec2(96.14f, -5.76f), vec2(85.28f, -8.32f), vec2(72.83f, -10.61f), vec2(58.83f, -12.56f), vec2(43.62f, -14.08f), vec2(28.22f, -15.06f), vec2(15.01f, -15.40f), vec2(-22.66f, -15.47f), vec2(-41.40f, -15.12f), vec2(-60.41f, -14.13f), vec2(-78.02f, -12.58f), vec2(-84.97f, -11.51f), vec2(-92.14f, -9.82f), vec2(-99.17f, -7.58f), vec2(-105.43f, -5.03f), vec2(-110.06f, -2.61f), vec2(-112.44f, -0.88f),
};

const vec2 richelieu_verts[] = {
    vec2(-123.93f, 0.00f), vec2(-123.55f, 1.85f), vec2(-122.37f, 3.51f), vec2(-120.16f, 5.05f), vec2(-116.55f, 6.49f), vec2(-111.00f, 7.85f), vec2(-102.54f, 9.18f), vec2(-63.15f, 13.34f), vec2(-46.15f, 14.70f), vec2(-27.56f, 15.67f), vec2(-16.00f, 16.00f), vec2(2.73f, 15.66f), vec2(23.66f, 14.63f), vec2(45.46f, 13.03f), vec2(66.80f, 10.97f), vec2(91.99f, 7.68f), vec2(105.11f, 5.66f), vec2(117.80f, 3.48f), vec2(121.46f, 2.48f), vec2(123.03f, 1.67f), vec2(123.72f, 0.88f), vec2(123.93f, 0.00f), vec2(123.72f, -0.88f), vec2(123.03f, -1.67f), vec2(121.46f, -2.48f), vec2(117.80f, -3.48f), vec2(105.11f, -5.66f), vec2(91.99f, -7.68f), vec2(66.80f, -10.97f), vec2(45.46f, -13.03f), vec2(23.66f, -14.63f), vec2(2.73f, -15.66f), vec2(-16.00f, -16.00f), vec2(-27.56f, -15.67f), vec2(-46.15f, -14.70f), vec2(-63.15f, -13.34f), vec2(-102.54f, -9.18f), vec2(-111.00f, -7.85f), vec2(-116.55f, -6.49f), vec2(-120.16f, -5.05f), vec2(-122.37f, -3.51f), vec2(-123.55f, -1.85f),
};

const vec2 bismarck_verts[] = {
    vec2(-125.50f, 0.00f), vec2(-125.08f, 1.01f), vec2(-123.48f, 2.28f), vec2(-120.19f, 3.81f), vec2(-114.76f, 5.59f), vec2(-106.93f, 7.58f), vec2(-96.63f, 9.70f), vec2(-84.10f, 11.86f), vec2(-70.05f, 13.91f), vec2(-55.64f, 15.67f), vec2(-42.25f, 16.98f), vec2(-30.99f, 17.76f), vec2(-22.49f, 17.99f), vec2(11.99f, 18.01f), vec2(24.66f, 17.58f), vec2(40.57f, 16.22f), vec2(60.31f, 13.86f), vec2(84.47f, 10.40f), vec2(96.72f, 7.98f), vec2(111.71f, 4.53f), vec2(123.96f, 1.33f), vec2(125.27f, 0.57f), vec2(125.50f, 0.00f), vec2(125.27f, -0.57f), vec2(123.96f, -1.33f), vec2(111.71f, -4.53f), vec2(96.72f, -7.98f), vec2(84.47f, -10.40f), vec2(60.31f, -13.86f), vec2(40.57f, -16.22f), vec2(24.66f, -17.58f), vec2(11.99f, -18.01f), vec2(-22.49f, -17.99f), vec2(-30.99f, -17.76f), vec2(-42.25f, -16.98f), vec2(-55.64f, -15.67f), vec2(-70.05f, -13.91f), vec2(-84.10f, -11.86f), vec2(-96.63f, -9.70f), vec2(-106.93f, -7.58f), vec2(-114.76f, -5.59f), vec2(-120.19f, -3.81f), vec2(-123.48f, -2.28f), vec2(-125.08f, -1.01f),
};

const vec2 littorio_verts[] = {
    vec2(-118.88f, 0.00f), vec2(-118.50f, 1.12f), vec2(-117.16f, 2.44f), vec2(-114.48f, 3.97f), vec2(-110.10f, 5.68f), vec2(-103.73f, 7.52f), vec2(-95.27f, 9.40f), vec2(-84.76f, 11.22f), vec2(-72.41f, 12.87f), vec2(-58.51f, 14.27f), vec2(-43.42f, 15.35f), vec2(-27.47f, 16.04f), vec2(-11.00f, 16.34f), vec2(7.03f, 16.34f), vec2(20.69f, 16.00f), vec2(36.53f, 15.07f), vec2(53.44f, 13.55f), vec2(70.00f, 11.56f), vec2(84.95f, 9.24f), vec2(97.43f, 6.80f), vec2(107.13f, 4.38f), vec2(114.15f, 2.10f), vec2(118.88f, 0.00f), vec2(114.15f, -2.10f), vec2(107.13f, -4.38f), vec2(97.43f, -6.80f), vec2(84.95f, -9.24f), vec2(70.00f, -11.56f), vec2(53.44f, -13.55f), vec2(36.53f, -15.07f), vec2(20.69f, -16.00f), vec2(7.03f, -16.34f), vec2(-11.00f, -16.34f), vec2(-27.47f, -16.04f), vec2(-43.42f, -15.35f), vec2(-58.51f, -14.27f), vec2(-72.41f, -12.87f), vec2(-84.76f, -11.22f), vec2(-95.27f, -9.40f), vec2(-103.73f, -7.52f), vec2(-110.10f, -5.68f), vec2(-114.48f, -3.97f), vec2(-117.16f, -2.44f), vec2(-118.50f, -1.12f),
};

#define DEG(a) math::deg2rad(a)
#define DEGV(x,y) vec2(math::deg2rad(x), math::deg2rad(y))
#define KNOTS(kn) (kn * 0.5144447f)
#define SHP(shp) (shp * 0.7456999f)

//------------------------------------------------------------------------------
const ship_design ship_yamato_battleship =
{
    /* name */              string::buffer("Yamato"),
    /* length */            263.f,
    /* beam */              39.f,
    /* draft */             10.4f,
    /* displacement */      65027000.f,

    /* speed */             KNOTS(27.f),
    /* power */             SHP(150000),

    /* rudder_angle */      DEG(36.f),
    /* rudder_speed */      DEG(1.f),

    /* minimum_turning_radius */    500.f,
    /* optimal_turning_radius */    750.f,

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
const ship_design ship_fuso_battleship =
{
    /* name */              string::buffer("Fuso"),
    /* length */            210.3f,
    /* beam */              33.1f,
    /* draft */             8.7f,
    /* displacement */      29797000.f,

    /* speed */             KNOTS(23.f),
    /* power */             SHP(40000),

    /* rudder_angle */      DEG(36.f),
    /* rudder_speed */      DEG(1.f),

    /* minimum_turning_radius */    500.f,
    /* optimal_turning_radius */    750.f,

    /* turrets */
    {
        {
            /* position */          vec2(56.5f,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-150.f, 150.f),
            /* design */            &turret_fuso_36cm,
        },
        {
            /* position */          vec2(43.5f,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-150.f, 150.f),
            /* design */            &turret_fuso_36cm_rf,
        },
        {
            /* position */          vec2(5.25f,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-150.f, 150.f),
            /* design */            &turret_fuso_36cm_rf,
        },
        {
            /* position */          vec2(-24.25f,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-150.f, 150.f),
            /* design */            &turret_fuso_36cm_rf,
        },
        {
            /* position */          vec2(-55.f,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-150.f, 150.f),
            /* design */            &turret_fuso_36cm_rf,
        },
        {
            /* position */          vec2(-67.25f,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-150.f, 150.f),
            /* design */            &turret_fuso_36cm,
        },
    },

    /* hull_outline */      {fuso_verts, fuso_verts + countof(fuso_verts)},
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(fuso_verts)}}},
};

//------------------------------------------------------------------------------
const ship_design ship_iowa_battleship =
{
    /* name */              string::buffer("Iowa"),
    /* length */            270.f,
    /* beam */              33.f,
    /* draft */             11.33f,
    /* displacement */      48880000.f,

    /* speed */             KNOTS(33.f),
    /* power */             SHP(212000),

    /* rudder_angle */      DEG(36.f),
    /* rudder_speed */      DEG(1.f),

    /* minimum_turning_radius */    500.f,
    /* optimal_turning_radius */    750.f,

    /* turrets */
    {
        {
            /* position */          vec2(59,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_iowa_16in,
        },
        {
            /* position */          vec2(38,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_iowa_16in,
        },
        {
            /* position */          vec2(-67,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_iowa_16in,
        },
    },

    /* hull_outline */      {iowa_verts, iowa_verts + countof(iowa_verts)},
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(iowa_verts)}}},
};

//------------------------------------------------------------------------------
const ship_design ship_king_george_v_battleship =
{
    /* name */              string::buffer("King George V"),
    /* length */            227.f,
    /* beam */              31.5f,
    /* draft */             10.2f,
    /* displacement */      37316000.f,

    /* speed */             KNOTS(28.f),
    /* power */             SHP(110000),

    /* rudder_angle */      DEG(36.f),
    /* rudder_speed */      DEG(1.f),

    /* minimum_turning_radius */    500.f,
    /* optimal_turning_radius */    750.f,

    /* turrets */
    {
        {
            /* position */          vec2(49.5f,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_kgv_14in_quad,
        },
        {
            /* position */          vec2(34.25f,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_kgv_14in_twin,
        },
        {
            /* position */          vec2(-62,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_kgv_14in_quad,
        },
    },

    /* hull_outline */      {kgv_verts, kgv_verts + countof(kgv_verts)},
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(kgv_verts)}}},
};

//------------------------------------------------------------------------------
const ship_design ship_richelieu_battleship =
{
    /* name */              string::buffer("Richelieu"),
    /* length */            247.85f,
    /* beam */              33.1f,
    /* draft */             9.9f,
    /* displacement */      37850000.f,

    /* speed */             KNOTS(32.f),
    /* power */             SHP(155000),

    /* rudder_angle */      DEG(36.f),
    /* rudder_speed */      DEG(1.f),

    /* minimum_turning_radius */    500.f,
    /* optimal_turning_radius */    750.f,

    /* turrets */
    {
        // main battery
        {
            /* position */          vec2(51.f,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_richelieu_380mm,
        },
        {
            /* position */          vec2(18.f,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_richelieu_380mm,
        },
        // secondary battery
        {
            /* position */          vec2(-53.f,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-155.f, 155.f),
            /* design */            &turret_richelieu_152mm,
        },
        {
            /* position */          vec2(-67.6f, -8.55f),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(0.f, 170.f),
            /* design */            &turret_richelieu_152mm,
        },
        {
            /* position */          vec2(-67.6f, 8.55f),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-170.f, 0.f),
            /* design */            &turret_richelieu_152mm,
        },
    },

    /* hull_outline */      {richelieu_verts, richelieu_verts + countof(richelieu_verts)},
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(richelieu_verts)}}},
};

//------------------------------------------------------------------------------
const ship_design ship_bismarck_battleship =
{
    /* name */              string::buffer("Bismarck"),
    /* length */            251.f,
    /* beam */              36.f,
    /* draft */             9.3f,
    /* displacement */      41000000.f,

    /* speed */             KNOTS(30.f),
    /* power */             SHP(148120),

    /* rudder_angle */      DEG(36.f),
    /* rudder_speed */      DEG(1.f),

    /* minimum_turning_radius */    500.f,
    /* optimal_turning_radius */    750.f,

    /* turrets */
    {
        // main battery
        {
            /* position */          vec2(69.75f,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_bismarck_38cm,
        },
        {
            /* position */          vec2(52.f,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_bismarck_38cm,
        },
        {
            /* position */          vec2(-59.5f,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_bismarck_38cm,
        },
        {
            /* position */          vec2(-78.f,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_bismarck_38cm,
        },
        // secondary battery
        {
            /* position */          vec2(27.85f, 9.6f),
            /* orientation */       0,
            /* train_limit */       DEGV(0.f, 135.f),
            /* design */            &turret_bismarck_15cm,
        },
        {
            /* position */          vec2(27.85f, -9.6f),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 0.f),
            /* design */            &turret_bismarck_15cm,
        },
        {
            /* position */          vec2(8.3f, 14.5f),
            /* orientation */       0,
            /* train_limit */       DEGV(0.f, 180.f),
            /* design */            &turret_bismarck_15cm_rf,
        },
        {
            /* position */          vec2(8.3f, -14.5f),
            /* orientation */       0,
            /* train_limit */       DEGV(-180.f, 0.f),
            /* design */            &turret_bismarck_15cm_rf,
        },
        {
            /* position */          vec2(-24.7f, 14.15f),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 0.f),
            /* design */            &turret_bismarck_15cm,
        },
        {
            /* position */          vec2(-24.7f, -14.15f),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(0.f, 135.f),
            /* design */            &turret_bismarck_15cm,
        },
    },

    /* hull_outline */      {bismarck_verts, bismarck_verts + countof(bismarck_verts)},
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(bismarck_verts)}}},
};

//------------------------------------------------------------------------------
const ship_design ship_littorio_battleship =
{
    /* name */              string::buffer("Littorio"),
    /* length */            237.76f,
    /* beam */              32.82f,
    /* draft */             9.6f,
    /* displacement */      40724000.f,

    /* speed */             KNOTS(30.f),
    /* power */             SHP(128200),

    /* rudder_angle */      DEG(36.f),
    /* rudder_speed */      DEG(1.f),

    /* minimum_turning_radius */    500.f,
    /* optimal_turning_radius */    750.f,

    /* turrets */
    {
        {
            /* position */          vec2(56.f,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_littorio_381mm,
        },
        {
            /* position */          vec2(34.5f,0),
            /* orientation */       0,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_littorio_381mm,
        },
        {
            /* position */          vec2(-56.f,0),
            /* orientation */       math::pi,
            /* train_limit */       DEGV(-135.f, 135.f),
            /* design */            &turret_littorio_381mm,
        },
    },

    /* hull_outline */      {littorio_verts, littorio_verts + countof(littorio_verts)},
    /* hull_shape */        {{{std::make_unique<physics::convex_shape>(littorio_verts)}}},
};

//------------------------------------------------------------------------------
const ship_design ship_deutschland_cruiser =
{
    /* name */              string::buffer("Deutschland"),
    /* length */            186.f,
    /* beam */              21.7f,
    /* draft */             7.25f,
    /* displacement */      10800000.f,

    /* speed */             KNOTS(26.f),
    /* power */             SHP(53260),

    /* rudder_angle */      DEG(36.f),
    /* rudder_speed */      DEG(1.f),

    /* minimum_turning_radius */    400.f,
    /* optimal_turning_radius */    600.f,

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
    /* draft */             6.1f,
    /* displacement */      11730000.f,

    /* speed */             KNOTS(32.f),
    /* power */             SHP(75000),

    /* rudder_angle */      DEG(36.f),
    /* rudder_speed */      DEG(1.f),

    /* minimum_turning_radius */    350.f,
    /* optimal_turning_radius */    525.f,

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
    /* draft */             3.43f,
    /* displacement */      1884000.f,

    /* speed */             KNOTS(36.f),
    /* power */             SHP(44000),

    /* rudder_angle */      DEG(36.f),
    /* rudder_speed */      DEG(1.f),

    /* minimum_turning_radius */    250.f,
    /* optimal_turning_radius */    375.f,

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

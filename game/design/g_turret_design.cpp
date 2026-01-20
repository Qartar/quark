// g_turret_design.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "design/g_turret_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

#define DEG(x) math::deg2rad(x)
#define DEGV(x,y) vec2(math::deg2rad(x), math::deg2rad(y))

const turret_design turret_yamato_46cm =
{
    /* radius */            7.f,
    /* train_speed */       DEG(2.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-5.f, 45.f),
    /* num_guns */          3,
    /* spacing */           2.9f,
    /* reload_time */       time_delta::from_seconds(24.f),
    /* gun_design */        &gun_46cm_45_Type_94,
};

const turret_design turret_iowa_16in =
{
    /* radius */            6.5f,
    /* train_speed */       DEG(4.f),
    /* elevation_speed */   DEG(12.f),
    /* elevation_limit */   DEGV(-5.f, 45.f),
    /* num_guns */          3,
    /* spacing */           2.25f,
    /* reload_time */       time_delta::from_seconds(30.f),
    /* gun_design */        &gun_16in_50_caliber_Mark_7,
};

const turret_design turret_kgv_14in_quad =
{
    /* radius */            6.f,
    /* train_speed */       DEG(4.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-5.f, 41.f),
    /* num_guns */          4,
    /* spacing */           2.f,
    /* reload_time */       time_delta::from_seconds(30.f),
    /* gun_design */        &gun_BL_14_inch_Mk_VII,
};

const turret_design turret_kgv_14in_twin =
{
    /* radius */            6.f,
    /* train_speed */       DEG(4.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-5.f, 41.f),
    /* num_guns */          2,
    /* spacing */           2.f,
    /* reload_time */       time_delta::from_seconds(30.f),
    /* gun_design */        &gun_BL_14_inch_Mk_VII,
};

const turret_design turret_deutschland_28cm =
{
    /* radius */            5.5f,
    /* train_speed */       DEG(5.f),
    /* elevation_speed */   DEG(15.f),
    /* elevation_limit */   DEGV(-8.f, 40.f),
    /* num_guns */          3,
    /* spacing */           1.75f,
    /* reload_time */       time_delta::from_seconds(24.f),
    /* gun_design */        &gun_28cm_SK_C_28,
};

const turret_design turret_town_6in =
{
    /* radius */            3.f,
    /* train_speed */       DEG(6.f),
    /* elevation_speed */   DEG(20.f),
    /* elevation_limit */   DEGV(-5.f, 45.f),
    /* num_guns */          3,
    /* spacing */           1.f,
    /* reload_time */       time_delta::from_seconds(8.f),
    /* gun_design */        &gun_BL_6_inch_Mk_XXIII,
};

const turret_design turret_tribal_4_7in =
{
    /* radius */            2.f,
    /* train_speed */       DEG(7.f),
    /* elevation_speed */   DEG(25.f),
    /* elevation_limit */   DEGV(-5.f, 40.f),
    /* num_guns */          2,
    /* spacing */           0.75f,
    /* reload_time */       time_delta::from_seconds(4.f),
    /* gun_design */        &gun_QF_4_7_inch_Mark_IX,
};

} // namespace game

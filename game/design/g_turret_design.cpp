// g_turret_design.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "design/g_turret_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

#define DEG(x) math::deg2rad(x)
#define DEGV(x,y) vec2(math::deg2rad(x), math::deg2rad(y))

#define OUTLINE(r) {            \
    vec2(r, .6f * r),           \
    vec2(.3f * r, r),           \
    vec2(-.3f * r, r),          \
    vec2(-2.f * r, .8f * r),    \
    vec2(-2.f * r, -.8f * r),   \
    vec2(-.3f * r, -r),         \
    vec2(.3f * r, -r),          \
    vec2(r, -.6f * r),          \
}

const turret_design turret_yamato_46cm =
{
    /* radius */            6.5f,
    /* train_speed */       DEG(2.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-5.f, 45.f),
    /* num_guns */          3,
    /* spacing */           2.9f,
    /* reload_time */       time_delta::from_seconds(24.f),
    /* gun_design */        &gun_46cm_45_Type_94,
    /* outline */           {
        vec2(6.5f, 4.75f),
        vec2(3.5f, 6.5f),
        vec2(1.75f, 7.f),
        vec2(0.75f, 7.f),
        vec2(-6.75f, 6.f),
        vec2(-6.75f, 8.f),
        vec2(-9.75f, 8.f),
        vec2(-9.5f, 5.75f),
        vec2(-10.25f, 5.75f),
        vec2(-10.75f, 4.f),
        vec2(-11.f, 0.f),
        vec2(-10.75f, -4.f),
        vec2(-10.25f, -5.75f),
        vec2(-9.5f, -5.75f),
        vec2(-9.75f, -8.f),
        vec2(-6.75f, -8.f),
        vec2(-6.75f, -6.f),
        vec2(0.75f, -7.f),
        vec2(1.75f, -7.f),
        vec2(3.5f, -6.5f),
        vec2(6.5f, -4.75f),
    }
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
    /* outline */           {
        vec2(6.5f, 5.f),
        vec2(1.f, 6.5f),
        vec2(-5.25f, 5.75f),
        vec2(-5.f, 7.5f),
        vec2(-7.75f, 7.5f),
        vec2(-7.5f, 5.5f),
        vec2(-8.5f, 5.5f),
        vec2(-9.f, 0.f),
        vec2(-8.5f, -5.5f),
        vec2(-7.5f, -5.5f),
        vec2(-7.75f, -7.5f),
        vec2(-5.f, -7.5f),
        vec2(-5.25f, -5.75f),
        vec2(1.f, -6.5f),
        vec2(6.5f, -5.f),
    }
};

const turret_design turret_kgv_14in_quad =
{
    /* radius */            4.75f,
    /* train_speed */       DEG(4.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-5.f, 41.f),
    /* num_guns */          4,
    /* spacing */           2.5f,
    /* reload_time */       time_delta::from_seconds(30.f),
    /* gun_design */        &gun_BL_14_inch_Mk_VII,
    /* outline */           {
        vec2(4.75f, 5.75f),
        vec2(0.5f, 6.25f),
        vec2(-3.25f, 5.75f),
        vec2(-3.f, 7.25f),
        vec2(-5.25f, 7.25f),
        vec2(-5.f, 5.5f),
        vec2(-7.25f, 5.f),
        vec2(-8.25f, 2.25f),
        vec2(-8.5f, 0.f),
        vec2(-8.25f, -2.25f),
        vec2(-7.25f, -5.f),
        vec2(-5.f, -5.5f),
        vec2(-5.25f, -7.25f),
        vec2(-3.f, -7.25f),
        vec2(-3.25f, -5.75f),
        vec2(0.5f, -6.25f),
        vec2(4.75f, -5.75f),
    }
};

const turret_design turret_kgv_14in_twin =
{
    /* radius */            .65f * 4.75f,
    /* train_speed */       DEG(4.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-5.f, 41.f),
    /* num_guns */          2,
    /* spacing */           2.5f,
    /* reload_time */       time_delta::from_seconds(30.f),
    /* gun_design */        &gun_BL_14_inch_Mk_VII,
    /* outline */           {
        // Don't have a good reference for 'B' turret so just scale down A/X
        .65f * vec2(4.75f, 5.75f),
        .65f * vec2(0.5f, 6.25f),
        .65f * vec2(-3.25f, 5.75f),
        .65f * vec2(-3.f, 7.25f),
        .65f * vec2(-5.25f, 7.25f),
        .65f * vec2(-5.f, 5.5f),
        .65f * vec2(-7.25f, 5.f),
        .65f * vec2(-8.25f, 2.25f),
        .65f * vec2(-8.5f, 0.f),
        .65f * vec2(-8.25f, -2.25f),
        .65f * vec2(-7.25f, -5.f),
        .65f * vec2(-5.f, -5.5f),
        .65f * vec2(-5.25f, -7.25f),
        .65f * vec2(-3.f, -7.25f),
        .65f * vec2(-3.25f, -5.75f),
        .65f * vec2(0.5f, -6.25f),
        .65f * vec2(4.75f, -5.75f),
    }
};

const turret_design turret_richelieu_380mm =
{
    /* radius */            5.f,
    /* train_speed */       DEG(5.f),
    /* elevation_speed */   DEG(6.f),
    /* elevation_limit */   DEGV(-5.f, 35.f),
    /* num_guns */          4,
    /* spacing */           2.25f, // guns are not actually evenly spaced
    /* reload_time */       time_delta::from_seconds(33.f),
    /* gun_design */        &gun_380mm_45_Modele_1935,
    /* outline */           {
        vec2(5.f, 5.f),
        vec2(1.25f, 6.f),
        vec2(-6.f, 6.f),
        vec2(-6.f, 7.125f),
        vec2(-8.75f, 7.125f),
        vec2(-8.75f, 4.125f),
        vec2(-9.125f, 1.5f),
        vec2(-9.125f, -1.5f),
        vec2(-8.75f, -4.125f),
        vec2(-8.75f, -7.125f),
        vec2(-6.f, -7.125f),
        vec2(-6.f, -6.f),
        vec2(1.25f, -6.f),
        vec2(5.f, -5.f),
    }
};

const turret_design turret_bismarck_38cm =
{
    /* radius */            4.875f,
    /* train_speed */       DEG(5.f),
    /* elevation_speed */   DEG(6.f),
    /* elevation_limit */   DEGV(-5.5f, 30.f),
    /* num_guns */          2,
    /* spacing */           3.5f,
    /* reload_time */       time_delta::from_seconds(24.f),
    /* gun_design */        &gun_38cm_SK_C_34,
    /* outline */           {
        vec2(4.875f, 3.5f),
        vec2(1.875f, 4.f),
        vec2(-3.875f, 4.f),
        vec2(-3.875f, 5.25f),
        vec2(-6.125f, 5.f),
        vec2(-6.125f, 4.f),
        vec2(-7.f, 4.f),
        vec2(-8.f, 1.875f),
        vec2(-8.25f, 0.625f),
        vec2(-8.25f, -0.625f),
        vec2(-8.f, -1.875f),
        vec2(-7.f, -4.f),
        vec2(-6.125f, -4.f),
        vec2(-6.125f, -5.f),
        vec2(-3.875f, -5.25f),
        vec2(-3.875f, -4.f),
        vec2(1.875f, -4.f),
        vec2(4.875f, -3.5f),
    }
};

const turret_design turret_littorio_381mm =
{
    /* radius */            4.875f,
    /* train_speed */       DEG(4.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-5.f, 35.f),
    /* num_guns */          3,
    /* spacing */           2.5f,
    /* reload_time */       time_delta::from_seconds(45.f),
    /* gun_design */        &gun_381_50_ansaldo_m1934,
    /* outline */           {
        // Don't have a good reference for Littorio so use Bismarck vertices
        vec2(4.875f, 3.5f),
        vec2(1.875f, 4.f),
        vec2(-3.875f, 4.f),
        vec2(-3.875f, 5.25f),
        vec2(-6.125f, 5.f),
        vec2(-6.125f, 4.f),
        vec2(-7.f, 4.f),
        vec2(-8.f, 1.875f),
        vec2(-8.25f, 0.625f),
        vec2(-8.25f, -0.625f),
        vec2(-8.f, -1.875f),
        vec2(-7.f, -4.f),
        vec2(-6.125f, -4.f),
        vec2(-6.125f, -5.f),
        vec2(-3.875f, -5.25f),
        vec2(-3.875f, -4.f),
        vec2(1.875f, -4.f),
        vec2(4.875f, -3.5f),
    }
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
    /* outline */           OUTLINE(5.5f)
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
    /* outline */           OUTLINE(3.f)
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
    /* outline */           OUTLINE(2.f)
};

} // namespace game

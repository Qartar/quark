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
    /* position */
    {
        vec3(6.5, -2.9, 1),
        vec3(6.5, 0, 1),
        vec3(6.5, 2.9, 1),
    },
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

const turret_design turret_yamato_15_5cm =
{
    /* radius */            2.9f,
    /* train_speed */       DEG(6.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-7.f, 55.f),
    /* num_guns */          3,
    /* position */
    {
        vec3(2.9, -1.5, 1),
        vec3(2.9, 0, 1),
        vec3(2.9, 1.5, 1),
    },
    /* reload_time */       time_delta::from_seconds(12.f),
    /* gun_design */        &gun_15_5cm_60_3rd_Year_Type,
    /* outline */           {
        vec2(-4.45f, 0.00f),
        vec2(-4.01f, 1.89f),
        vec2(-3.45f, 2.70f),
        vec2(-3.45f, 4.40f),
        vec2(-1.65f, 4.40f),
        vec2(-1.65f, 2.70f),
        vec2(2.93f, 2.70f),
        vec2(2.93f, -2.70f),
        vec2(2.93f, -2.70f),
        vec2(-1.65f, -2.70f),
        vec2(-1.65f, -4.40f),
        vec2(-3.45f, -4.40f),
        vec2(-3.45f, -2.70f),
        vec2(-4.01f, -1.89f),
    }
};

const turret_design turret_fuso_36cm =
{
    /* radius */            4.75f,
    /* train_speed */       DEG(2.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-3.f, 43.f),
    /* num_guns */          2,
    /* position */
    {
        vec3(4.75, -1, 1),
        vec3(4.75, 1, 1),
    },
    /* reload_time */       time_delta::from_seconds(30.f),
    /* gun_design */        &gun_36cm_41st_Year_Type,
    /* outline */           {
        vec2(4.75f, 1.5f),
        vec2(3.f, 3.4f),
        vec2(-1.f, 4.4f),
        vec2(-6.25f, 3.4f),
        vec2(-7.f, 1.5f),
        vec2(-7.f, -1.5f),
        vec2(-6.25f, -3.4f),
        vec2(-1.f, -4.4f),
        vec2(3.f, -3.4f),
        vec2(4.75f, -1.5f)
    }
};

const turret_design turret_fuso_36cm_rf =
{
    /* radius */            4.75f,
    /* train_speed */       DEG(2.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-3.f, 43.f),
    /* num_guns */          2,
    /* position */
    {
        vec3(4.75, -1, 1),
        vec3(4.75, 1, 1),
    },
    /* reload_time */       time_delta::from_seconds(30.f),
    /* gun_design */        &gun_36cm_41st_Year_Type,
    /* outline */           {
        vec2(4.75f, 1.5f),
        vec2(3.f, 3.4f),
        vec2(-1.f, 4.4f),
        vec2(-4.75f, 3.75f),
        vec2(-4.75f, 4.75f),
        vec2(-6.6f, 4.75f),
        vec2(-6.6f, 2.5f),
        vec2(-7.f, 1.5f),
        vec2(-7.f, -1.5f),
        vec2(-6.6f, -2.5f),
        vec2(-6.6f, -4.75f),
        vec2(-4.75f, -4.75f),
        vec2(-4.75f, -3.75f),
        vec2(-1.f, -4.4f),
        vec2(3.f, -3.4f),
        vec2(4.75f, -1.5f)
    }
};

const turret_design turret_iowa_16in =
{
    /* radius */            6.5f,
    /* train_speed */       DEG(4.f),
    /* elevation_speed */   DEG(12.f),
    /* elevation_limit */   DEGV(-5.f, 45.f),
    /* num_guns */          3,
    /* position */
    {
        vec3(6.5, -2.25, 1),
        vec3(6.5, 0, 1),
        vec3(6.5, 2.25, 1),
    },
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

const turret_design turret_north_carolina_16in =
{
    /* radius */            5.f,
    /* train_speed */       DEG(4.f),
    /* elevation_speed */   DEG(12.f),
    /* elevation_limit */   DEGV(-2.f, 45.f),
    /* num_guns */          3,
    /* position */
    {
        vec3(5, -3, 1),
        vec3(5, 0, 1),
        vec3(5, 3, 1),
    },
    /* reload_time */       time_delta::from_seconds(30.f),
    /* gun_design */        &gun_16in_45_caliber_Mark_6,
    /* outline */           {
        vec2(-8.00f, 0.00f),
        vec2(-7.75f, 2.45f),
        vec2(-7.00f, 4.50f),
        vec2(-7.00f, 7.00f),
        vec2(-5.00f, 7.00f),
        vec2(-5.00f, 5.00f),
        vec2(2.00f, 5.50f),
        vec2(5.00f, 4.50f),
        vec2(5.00f, -4.50f),
        vec2(5.00f, -4.50f),
        vec2(2.00f, -5.50f),
        vec2(-5.00f, -5.00f),
        vec2(-5.00f, -7.00f),
        vec2(-7.00f, -7.00f),
        vec2(-7.00f, -4.50f),
        vec2(-7.75f, -2.45f),
    }
};

const turret_design turret_north_carolina_5in =
{
    /* radius */            2.f,
    /* train_speed */       DEG(10.f),
    /* elevation_speed */   DEG(12.f),
    /* elevation_limit */   DEGV(-15.f, 85.f),
    /* num_guns */          2,
    /* position */
    {
        vec3(2, -1.25, 1),
        vec3(2, 1.25, 1),
    },
    /* reload_time */       time_delta::from_seconds(4.f),
    /* gun_design */        &gun_5in_38_caliber_Mark_12,
    /* outline */           {
        vec2(-3.00f, 0.00f),
        vec2(-2.78f, 1.11f),
        vec2(-2.11f, 1.78f),
        vec2(-1.00f, 2.00f),
        vec2(2.00f, 2.00f),
        vec2(2.00f, -2.00f),
        vec2(2.00f, -2.00f),
        vec2(-1.00f, -2.00f),
        vec2(-2.11f, -1.78f),
        vec2(-2.78f, -1.11f),
    }
};

const turret_design turret_kgv_14in_quad =
{
    /* radius */            4.75f,
    /* train_speed */       DEG(4.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-5.f, 41.f),
    /* num_guns */          4,
    /* position */
    {
        vec3(4.75, -3.75, 1),
        vec3(4.75, -1.25, 1),
        vec3(4.75, 1.25, 1),
        vec3(4.75, 3.75, 1),
    },
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
    /* position */
    {
        vec3(.65 * 4.75, -1.25, 1),
        vec3(.65 * 4.75, 1.25, 1),
    },
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

const turret_design turret_kgv_5_25in =
{
    /* radius */            1.f,
    /* train_speed */       DEG(10.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-5.f, 70.f),
    /* num_guns */          2,
    /* position */
    {
        vec3(1, -1.25, 1),
        vec3(1, 1.25, 1),
    },
    /* reload_time */       time_delta::from_seconds(7.5f),
    /* gun_design */        &gun_QF_5_25_inch_Mk_I,
    /* outline */           {
        vec2(-2.00f, 0.50f),
        vec2(-1.75f, 1.25f),
        vec2(-1.25f, 1.75f),
        vec2(-0.50f, 2.00f),
        vec2(0.75f, 2.00f),
        vec2(1.00f, 1.75f),
        vec2(1.00f, 1.00f),
        //vec2(2.25f, 1.00f),
        //vec2(2.25f, 0.00f),
        //vec2(2.25f, -1.00f),
        vec2(1.00f, -1.00f),
        vec2(1.00f, -1.75f),
        vec2(0.75f, -2.00f),
        vec2(-0.50f, -2.00f),
        vec2(-1.25f, -1.75f),
        vec2(-1.75f, -1.25f),
        vec2(-2.00f, -0.50f),
    }
};

const turret_design turret_richelieu_380mm =
{
    /* radius */            5.f,
    /* train_speed */       DEG(5.f),
    /* elevation_speed */   DEG(6.f),
    /* elevation_limit */   DEGV(-5.f, 35.f),
    /* num_guns */          4,
    /* position */
    {
        vec3(5, -3.375, 1),
        vec3(5, -1.125, 1),
        vec3(5, 1.125, 1),
        vec3(5, 3.375, 1),
    },
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

const turret_design turret_richelieu_152mm =
{
    /* radius */            3.2f,
    /* train_speed */       DEG(8.f),
    /* elevation_speed */   DEG(8.f),
    /* elevation_limit */   DEGV(-10.f, 85.f),
    /* num_guns */          3,
    /* position */
    {
        vec3(3.2, -1.85, 1),
        vec3(3.2, 0, 1),
        vec3(3.2, 1.85, 1),
    },
    /* reload_time */       time_delta::from_seconds(12.f),
    /* gun_design */        &gun_152mm_55_Modele_1930,
    /* outline */           {
        vec2(-4.70f, 0.00f),
        vec2(-4.63f, 1.60f),
        vec2(-4.40f, 2.89f),
        vec2(-3.95f, 3.06f),
        vec2(-3.95f, 4.59f),
        vec2(-3.50f, 4.59f),
        vec2(-3.49f, 3.19f),
        vec2(-1.58f, 3.35f),
        vec2(0.68f, 3.37f),
        vec2(3.20f, 2.75f),
        vec2(3.20f, -2.75f),
        vec2(3.20f, -2.75f),
        vec2(0.68f, -3.37f),
        vec2(-1.58f, -3.35f),
        vec2(-3.49f, -3.19f),
        vec2(-3.50f, -4.59f),
        vec2(-3.95f, -4.59f),
        vec2(-3.95f, -3.06f),
        vec2(-4.40f, -2.89f),
        vec2(-4.63f, -1.60f),
    }
};

const turret_design turret_bismarck_38cm =
{
    /* radius */            5.15f,
    /* train_speed */       DEG(5.f),
    /* elevation_speed */   DEG(6.f),
    /* elevation_limit */   DEGV(-5.5f, 30.f),
    /* num_guns */          2,
    /* position */
    {
        vec3(5.15, -1.8, 1),
        vec3(5.15, 1.8, 1),
    },
    /* reload_time */       time_delta::from_seconds(24.f),
    /* gun_design */        &gun_38cm_SK_C_34,
    /* outline */           {
        vec2(-8.7f, -0.00f),
        vec2(-8.25f, 2.15f),
        vec2(-7.25f, 4.20f),
        vec2(-6.3f, 4.20f),
        vec2(-6.3f, 5.25f),
        vec2(-4.f, 5.4f),
        vec2(-4.f, 4.2f),
        vec2(1.9f, 4.2f),
        vec2(5.15f, 3.67f),
        vec2(5.15f, -3.67f),
        vec2(1.9f, -4.2f),
        vec2(-4.f, -4.2f),
        vec2(-4.f, -5.4f),
        vec2(-6.3f, -5.25f),
        vec2(-6.3f, -4.20f),
        vec2(-7.25f, -4.20f),
        vec2(-8.25f, -2.15f),
    }
};

const turret_design turret_bismarck_15cm_rf =
{
    /* radius */            2.f,
    /* train_speed */       DEG(9.f),
    /* elevation_speed */   DEG(8.f),
    /* elevation_limit */   DEGV(-10.f, 40.f),
    /* num_guns */          2,
    /* position */
    {
        vec3(2, -0.75, 1),
        vec3(2, 0.75, 1),
    },
    /* reload_time */       time_delta::from_seconds(7.5f),
    /* gun_design */        &gun_15cm_SK_C_28,
    /* outline */           {
        vec2(-4.23f, 0.00f),
        vec2(-4.16f, 1.08f),
        vec2(-3.94f, 1.95f),
        vec2(-3.50f, 2.04f),
        vec2(-3.48f, 3.61f),
        vec2(-2.14f, 3.62f),
        vec2(-2.14f, 2.18f),
        vec2(-1.31f, 2.25f),
        vec2(2.00f, 2.25f),
        vec2(2.00f, -2.25f),
        vec2(2.00f, -2.25f),
        vec2(-1.31f, -2.25f),
        vec2(-2.14f, -2.18f),
        vec2(-2.14f, -3.62f),
        vec2(-3.48f, -3.61f),
        vec2(-3.50f, -2.04f),
        vec2(-3.94f, -1.95f),
        vec2(-4.16f, -1.08f),
    }
};

const turret_design turret_bismarck_15cm =
{
    /* radius */            2.f,
    /* train_speed */       DEG(9.f),
    /* elevation_speed */   DEG(8.f),
    /* elevation_limit */   DEGV(-10.f, 40.f),
    /* num_guns */          2,
    /* position */
    {
        vec3(2, -0.75, 1),
        vec3(2, 0.75, 1),
    },
    /* reload_time */       time_delta::from_seconds(7.5f),
    /* gun_design */        &gun_15cm_SK_C_28,
    /* outline */           {
        vec2(-4.25f, 0.00f),
        vec2(-4.18f, 1.15f),
        vec2(-3.95f, 1.95f),
        vec2(-2.93f, 2.19f),
        vec2(-1.75f, 2.27f),
        vec2(2.00f, 2.27f),
        vec2(2.00f, -2.27f),
        vec2(2.00f, -2.27f),
        vec2(-1.75f, -2.27f),
        vec2(-2.93f, -2.19f),
        vec2(-3.95f, -1.95f),
        vec2(-4.18f, -1.15f),
    }
};

const turret_design turret_littorio_381mm =
{
    /* radius */            5.15f,
    /* train_speed */       DEG(4.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-5.f, 35.f),
    /* num_guns */          3,
    /* position */
    {
        vec3(6, -2.75, 1),
        vec3(6, 0, 1),
        vec3(6, 2.75, 1),
    },
    /* reload_time */       time_delta::from_seconds(45.f),
    /* gun_design */        &gun_381_50_ansaldo_m1934,
    /* outline */           {
        vec2(-9.50f, 0.00f),
        vec2(-9.25f, 2.75f),
        vec2(-8.88f, 4.38f),
        vec2(-9.12f, 7.00f),
        vec2(-5.25f, 7.00f),
        vec2(-5.38f, 5.12f),
        vec2(-0.12f, 5.12f),
        vec2(6.25f, 4.38f),
        vec2(6.25f, -4.38f),
        vec2(6.25f, -4.38f),
        vec2(-0.12f, -5.12f),
        vec2(-5.38f, -5.12f),
        vec2(-5.25f, -7.00f),
        vec2(-9.12f, -7.00f),
        vec2(-8.88f, -4.38f),
        vec2(-9.25f, -2.75f),
    }
};

const turret_design turret_littorio_152mm =
{
    /* radius */            2.f,
    /* train_speed */       DEG(8.f),
    /* elevation_speed */   DEG(10.f),
    /* elevation_limit */   DEGV(-5.f, 35.f),
    /* num_guns */          3,
    /* position */
    {
        vec3(3.375, -1.125, 1),
        vec3(3.375, 0, 1),
        vec3(3.375, 1.125, 1),
    },
    /* reload_time */       time_delta::from_seconds(6.f),
    /* gun_design */        &gun_152_55_ansaldo_m1934,
    /* outline */           {
        vec2(-5.50f, 0.00f),
        vec2(-5.38f, 1.38f),
        vec2(-5.25f, 2.25f),
        vec2(-5.12f, 3.00f),
        vec2(-2.62f, 3.50f),
        vec2(-2.62f, 2.50f),
        vec2(-1.50f, 2.75f),
        vec2(3.38f, 2.12f),
        vec2(3.38f, -2.12f),
        vec2(3.38f, -2.12f),
        vec2(-1.50f, -2.75f),
        vec2(-2.62f, -2.50f),
        vec2(-2.62f, -3.50f),
        vec2(-5.12f, -3.00f),
        vec2(-5.25f, -2.25f),
        vec2(-5.38f, -1.38f),
    }
};

const turret_design turret_deutschland_28cm =
{
    /* radius */            5.5f,
    /* train_speed */       DEG(5.f),
    /* elevation_speed */   DEG(15.f),
    /* elevation_limit */   DEGV(-8.f, 40.f),
    /* num_guns */          3,
    /* position */
    {
        vec3(5.5, -1.75, 1),
        vec3(5.5, 0, 1),
        vec3(5.5, 1.75, 1),
    },
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
    /* position */
    {
        vec3(3, -1, 1),
        vec3(3, 0, 1),
        vec3(3, 1, 1),
    },
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
    /* position */
    {
        vec3(2, -0.375, 1),
        vec3(2, 0.375, 1),
    },
    /* reload_time */       time_delta::from_seconds(4.f),
    /* gun_design */        &gun_QF_4_7_inch_Mark_IX,
    /* outline */           OUTLINE(2.f)
};

} // namespace game

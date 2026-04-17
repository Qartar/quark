// g_gun_design.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "design/g_gun_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const gun_design gun_46cm_45_Type_94 =
{
    /* name */              string::buffer("46 cm/45 Type 94"),
    /* caliber */           0.46f,
    /* length */            20.7f,
    /* shell_mass */        1460.f,
    /* shell_velocity */    780.f,
    /* shell_coefficient */ 0.f,
};

const gun_design gun_36cm_41st_Year_Type =
{
    /* name */              string::buffer("36 cm 41st Year Type"),
    /* caliber */           0.36f,
    /* length */            16.f,
    /* shell_mass */        673.5f,
    /* shell_velocity */    775.f,
    /* shell_coefficient */ 0.f,
};

const gun_design gun_16in_50_caliber_Mark_7 =
{
    /* name */              string::buffer("16\"/50 caliber Mark 7"),
    /* caliber */           .406f,
    /* length */            20.f,
    /* shell_mass */        1225.f,
    /* shell_velocity */    762.f,
    /* shell_coefficient */ 0.f,
};

const gun_design gun_BL_14_inch_Mk_VII =
{
    /* name */              string::buffer("BL 14-inch Mk VII"),
    /* caliber */           .3556f,
    /* length */            16.f,
    /* shell_mass */        721.2f,
    /* shell_velocity */    757.f,
    /* shell_coefficient */ 0.f,
};

const gun_design gun_380mm_45_Modele_1935 =
{
    /* name */              string::buffer("380mm/45 Modèle 1935"),
    /* caliber */           .38f,
    /* length */            17.257f,
    /* shell_mass */        884.f,
    /* shell_velocity */    830.f,
    /* shell_coefficient */ 0.f,
};

const gun_design gun_152mm_55_Modele_1930 =
{
    /* name */              string::buffer("152mm/55 Modèle 1930"),
    /* caliber */           .152f,
    /* length */            8.39f,
    /* shell_mass */        56.f,
    /* shell_velocity */    870.f,
    /* shell_coefficient */ 0.f,
};

const gun_design gun_38cm_SK_C_34 =
{
    /* name */              string::buffer("38 cm SK C/34"),
    /* caliber */           .38f,
    /* length */            18.405f,
    /* shell_mass */        800.f,
    /* shell_velocity */    820.f,
    /* shell_coefficient */ 0.f,
};

const gun_design gun_15cm_SK_C_28 =
{
    /* name */              string::buffer("15 cm SK C/28"),
    /* caliber */           .15f,
    /* length */            7.815f,
    /* shell_mass */        45.3f,
    /* shell_velocity */    875.f,
    /* shell_coefficient */ 0.f,
};

const gun_design gun_381_50_ansaldo_m1934 =
{
    /* name */              string::buffer("Cannone da 381/50 Ansaldo M1934"),
    /* caliber */           .381f,
    /* length */            19.05f,
    /* shell_mass */        885.f,
    /* shell_velocity */    850.f,
    /* shell_coefficient */ 0.f,
};

const gun_design gun_28cm_SK_C_28 =
{
    /* name */              string::buffer("28 cm SK C/28"),
    /* caliber */           .283f,
    /* length */            14.815f,
    /* shell_mass */        300.f,
    /* shell_velocity */    910.f,
    /* shell_coefficient */ 0.f,
};

const gun_design gun_BL_6_inch_Mk_XXIII =
{
    /* name */              string::buffer("BL 6-inch Mk XIII"),
    /* caliber */           .1524f,
    /* length */            7.6f,
    /* shell_mass */        51.f,
    /* shell_velocity */    840.f,
    /* shell_coefficient */ 0.f,
};

const gun_design gun_QF_4_7_inch_Mark_IX =
{
    /* name */              string::buffer("QF 4.7-inch Mk IX"),
    /* caliber */           .12f,
    /* length */            5.4f,
    /* shell_mass */        22.7f,
    /* shell_velocity */    810.f,
    /* shell_coefficient */ 0.f,
};

} // namespace game

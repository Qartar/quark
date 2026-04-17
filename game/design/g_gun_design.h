// g_gun_design.h
//

#pragma once

#include "cm_string.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
struct gun_design
{
    string::buffer name;

    float caliber; //!< Internal diameter of gun barrels
    float length; //!< Length of gun barrels

    float shell_mass; //!< Mass of projectile
    float shell_velocity; //!< Muzzle velocity of projectile

    float shell_coefficient; //!< Ballistic coefficient of projectile
};

extern const gun_design gun_46cm_45_Type_94;
extern const gun_design gun_15_5cm_60_3rd_Year_Type;
extern const gun_design gun_36cm_41st_Year_Type;
extern const gun_design gun_16in_50_caliber_Mark_7;
extern const gun_design gun_16in_45_caliber_Mark_6;
extern const gun_design gun_5in_38_caliber_Mark_12;
extern const gun_design gun_BL_14_inch_Mk_VII;
extern const gun_design gun_380mm_45_Modele_1935;
extern const gun_design gun_152mm_55_Modele_1930;
extern const gun_design gun_38cm_SK_C_34;
extern const gun_design gun_15cm_SK_C_28;
extern const gun_design gun_381_50_ansaldo_m1934;
extern const gun_design gun_28cm_SK_C_28;
extern const gun_design gun_BL_6_inch_Mk_XXIII;
extern const gun_design gun_QF_4_7_inch_Mark_IX;

} // namespace game

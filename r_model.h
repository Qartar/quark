/*
===============================================================================

Name    :   r_model.h

Purpose :   Compound rectangle model class

===============================================================================
*/

#pragma once

struct sRect
{
    int     nPosX;
    int     nPosY;
    int     nSizeX;
    int     nSizeY;
    float   flGamma;
};

struct sRectList
{
    int     numRects;
    sRect   *lpRects;
};

class cModel
{
public:
    cModel () {}
    ~cModel () {};

    cModel (sRectList *lpList);

    bool    Clip (cModel *lpOther, vec2 vPos, float flAngle);
    bool    ClipPoint (vec2 vPos);

    sRectList   m_List;
    vec2        m_AbsMin;   // absolute size, early out clipping detection
    vec2        m_AbsMax;
};

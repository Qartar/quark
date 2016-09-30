/*
===============================================================================

Name	:	r_main.h

Purpose	:	Rendering controller

Date	:	10/19/2004

===============================================================================
*/

#define MAX_FONTS	16
#define NUM_CHARS	256

#include "r_model.h"
#include "r_particle.h"

/*
===========================================================

Name	:	cFont (r_font.cpp)

Purpose	:	OpenGL Font Encapsulation

===========================================================
*/

typedef int	rfont_t;
class cFont
{
public:
	cFont () {}
	~cFont () {}

	int		Init (char *szName, int nSize, unsigned int bitFlags);
	int		Shutdown ();

	bool	Compare (char *szName, int nSize, unsigned int bitFlags);
	HFONT	Activate ();

	bool	is_inuse () { return (m_hFont != NULL); }

private:
	HFONT		m_hFont;
	unsigned 	m_listBase;

	char		m_szName[64];
	int			m_nSize;
	unsigned int	m_bitFlags;
};

/*
===========================================================

Name	:	cRender (r_main.cpp)

Purpose	:	Rendering controller object

===========================================================
*/

class cRender
{
public:
	cRender () {}
	~cRender () {}

	int		Init ();
	int		Shutdown ();

	void	BeginFrame ();
	void	EndFrame ();

	void	Resize () { m_setDefaultState( ) ; }

	// Font Interface (r_font.cpp)

	rfont_t	AddFont (char *szName, int nSize, unsigned int bitFlags);
	int		RemoveFont (rfont_t hFont);
	rfont_t	UseFont (rfont_t hFont);

	// Drawing Functions (r_draw.cpp)

	void	DrawString (char *szString, vec2 vPos, vec4 vColor);
	void	DrawLine (vec2 vOrg, vec2 vEnd, vec4 vColorO, vec4 vColorE);
	void	DrawBox (vec2 vSize, vec2 vPos, float flAngle, vec4 vColor);
	void	DrawModel (cModel *pModel, vec2 vPos, float flAngle, vec4 vColor);
	void	DrawParticles (cParticle *pHead);

	void	SetViewOrigin (vec2 vPos) { m_viewOrigin = vPos ; m_setDefaultState( ) ; }

private:

	// More font stuff (r_font.cpp)

	void	m_InitFonts ();
	void	m_ClearFonts ();
	HFONT	m_sysFont;
	cFont	m_Fonts[MAX_FONTS];
	rfont_t	m_activeFont;

	// Internal stuff

	void	m_setDefaultState ();

	vec2	m_viewOrigin;
};

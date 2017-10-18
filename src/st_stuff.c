// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  st_stuff.c
/// \brief Status bar code
///        Does the face/direction indicator animatin.
///        Does palette indicators as well (red pain/berserk, bright pickup)

#include "doomdef.h"
#include "g_game.h"
#include "r_local.h"
#include "p_local.h"
#include "f_finale.h"
#include "st_stuff.h"
#include "i_video.h"
#include "v_video.h"
#include "z_zone.h"
#include "hu_stuff.h"
#include "s_sound.h"
#include "i_system.h"
#include "m_menu.h"
#include "m_cheat.h"
#include "p_setup.h" // NiGHTS grading

//random index
#include "m_random.h"

// item finder
#include "m_cond.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#ifdef HAVE_BLUA
#include "lua_hud.h"
#endif

UINT16 objectsdrawn = 0;

//
// STATUS BAR DATA
//

patch_t *faceprefix[MAXSKINS]; // face status patches
patch_t *superprefix[MAXSKINS]; // super face status patches

// ------------------------------------------
//             status bar overlay
// ------------------------------------------

// icons for overlay
patch_t *sboscore; // Score logo
patch_t *sbotime; // Time logo
patch_t *sbocolon; // Colon for time
patch_t *sboperiod; // Period for time centiseconds
patch_t *livesback; // Lives icon background
static patch_t *nrec_timer; // Timer for NiGHTS records
static patch_t *sborings;
static patch_t *sboover;
static patch_t *timeover;
static patch_t *stlivex;
static patch_t *sboredrings;
static patch_t *sboredtime;
static patch_t *getall; // Special Stage HUD
static patch_t *timeup; // Special Stage HUD
static patch_t *hunthoming[6];
static patch_t *itemhoming[6];
static patch_t *race1;
static patch_t *race2;
static patch_t *race3;
static patch_t *racego;
static patch_t *ttlnum;
static patch_t *nightslink;
static patch_t *curweapon;
static patch_t *normring;
static patch_t *bouncering;
static patch_t *infinityring;
static patch_t *autoring;
static patch_t *explosionring;
static patch_t *scatterring;
static patch_t *grenadering;
static patch_t *railring;
static patch_t *jumpshield;
static patch_t *forceshield;
static patch_t *ringshield;
static patch_t *watershield;
static patch_t *bombshield;
static patch_t *pityshield;
static patch_t *flameshield;
static patch_t *bubbleshield;
static patch_t *thundershield;
static patch_t *invincibility;
static patch_t *sneakers;
static patch_t *gravboots;
static patch_t *nonicon;
static patch_t *bluestat;
static patch_t *byelstat;
static patch_t *orngstat;
static patch_t *redstat;
static patch_t *yelstat;
static patch_t *nbracket;
static patch_t *nhud[12];
static patch_t *nsshud;
static patch_t *narrow[9];
static patch_t *nredar[8]; // Red arrow
static patch_t *drillbar;
static patch_t *drillfill[3];
static patch_t *capsulebar;
static patch_t *capsulefill;
patch_t *ngradeletters[7];
static patch_t *minus5sec;
static patch_t *minicaps;
static patch_t *gotrflag;
static patch_t *gotbflag;

static boolean facefreed[MAXPLAYERS];

hudinfo_t hudinfo[NUMHUDITEMS] =
{
	{  34, 176}, // HUD_LIVESNAME
	{  16, 176}, // HUD_LIVESPIC
	{  74, 184}, // HUD_LIVESNUM
	{  38, 186}, // HUD_LIVESX

	{  16,  42}, // HUD_RINGS
	{ 220,  10}, // HUD_RINGSSPLIT
	{ 120,  42}, // HUD_RINGSNUM
	{ 296,  10}, // HUD_RINGSNUMSPLIT

	{  16,  10}, // HUD_SCORE
	{ 120,  10}, // HUD_SCORENUM

	{  16,  26}, // HUD_TIME
	{ 128,  10}, // HUD_TIMESPLIT
	{  96,  26}, // HUD_MINUTES
	{ 188,  10}, // HUD_MINUTESSPLIT
	{  96,  26}, // HUD_TIMECOLON
	{ 188,  10}, // HUD_TIMECOLONSPLIT
	{ 120,  26}, // HUD_SECONDS
	{ 212,  10}, // HUD_SECONDSSPLIT
	{ 120,  26}, // HUD_TIMETICCOLON
	{ 144,  26}, // HUD_TICS

	{ 120,  56}, // HUD_SS_TOTALRINGS
	{ 296,  40}, // HUD_SS_TOTALRINGS_SPLIT

	{ 110,  93}, // HUD_GETRINGS
	{ 160,  93}, // HUD_GETRINGSNUM
	{ 124, 160}, // HUD_TIMELEFT
	{ 168, 176}, // HUD_TIMELEFTNUM
	{ 130,  93}, // HUD_TIMEUP
	{ 152, 168}, // HUD_HUNTPICS
	{ 152,  24}, // HUD_GRAVBOOTSICO
	{ 240, 160}, // HUD_LAP
};

//
// STATUS BAR CODE
//

boolean ST_SameTeam(player_t *a, player_t *b)
{
	// Just pipe team messages to everyone in co-op or race.
	if (!G_RingSlingerGametype())
		return true;

	// Spectator chat.
	if (a->spectator && b->spectator)
		return true;

	// Team chat.
	if (G_GametypeHasTeams())
		return a->ctfteam == b->ctfteam;

	if (G_TagGametype())
		return ((a->pflags & PF_TAGIT) == (b->pflags & PF_TAGIT));

	return false;
}

static boolean st_stopped = true;

void ST_Ticker(void)
{
	if (st_stopped)
		return;
}

// 0 is default, any others are special palettes.
static INT32 st_palette = 0;

void ST_doPaletteStuff(void)
{
	INT32 palette;

	if (paused || P_AutoPause())
		palette = 0;
	else if (stplyr && stplyr->flashcount)
		palette = stplyr->flashpal;
	else
		palette = 0;

	palette = min(max(palette, 0), 13);

	if (palette != st_palette)
	{
		st_palette = palette;

#ifdef HWRENDER
		if (rendermode == render_opengl)
			HWR_SetPaletteColor(0);
		else
#endif
		if (rendermode != render_none)
		{
			V_SetPaletteLump(GetPalette()); // Reset the palette
			if (!splitscreen)
				V_SetPalette(palette);
		}
	}
}

void ST_UnloadGraphics(void)
{
	Z_FreeTags(PU_HUDGFX, PU_HUDGFX);
}

void ST_LoadGraphics(void)
{
	int i;

	// SRB2 border patch
	st_borderpatchnum = W_GetNumForName("GFZFLR01");
	scr_borderpatch = W_CacheLumpNum(st_borderpatchnum, PU_HUDGFX);

	// the original Doom uses 'STF' as base name for all face graphics
	// Graue 04-08-2004: face/name graphics are now indexed by skins
	//                   but load them in R_AddSkins, that gets called
	//                   first anyway
	// cache the status bar overlay icons (fullscreen mode)

	// Prefix "STT" is whitelisted (doesn't trigger ISGAMEMODIFIED), btw
	sborings = W_CachePatchName("STTRINGS", PU_HUDGFX);
	sboredrings = W_CachePatchName("STTRRING", PU_HUDGFX);
	sboscore = W_CachePatchName("STTSCORE", PU_HUDGFX);
	sbotime = W_CachePatchName("STTTIME", PU_HUDGFX); // Time logo
	sboredtime = W_CachePatchName("STTRTIME", PU_HUDGFX);
	sbocolon = W_CachePatchName("STTCOLON", PU_HUDGFX); // Colon for time
	sboperiod = W_CachePatchName("STTPERIO", PU_HUDGFX); // Period for time centiseconds

	sboover = W_CachePatchName("SBOOVER", PU_HUDGFX);
	timeover = W_CachePatchName("TIMEOVER", PU_HUDGFX);
	stlivex = W_CachePatchName("STLIVEX", PU_HUDGFX);
	livesback = W_CachePatchName("STLIVEBK", PU_HUDGFX);
	nrec_timer = W_CachePatchName("NGRTIMER", PU_HUDGFX); // Timer for NiGHTS
	getall = W_CachePatchName("GETALL", PU_HUDGFX); // Special Stage HUD
	timeup = W_CachePatchName("TIMEUP", PU_HUDGFX); // Special Stage HUD
	race1 = W_CachePatchName("RACE1", PU_HUDGFX);
	race2 = W_CachePatchName("RACE2", PU_HUDGFX);
	race3 = W_CachePatchName("RACE3", PU_HUDGFX);
	racego = W_CachePatchName("RACEGO", PU_HUDGFX);
	nightslink = W_CachePatchName("NGHTLINK", PU_HUDGFX);

	for (i = 0; i < 6; ++i)
	{
		hunthoming[i] = W_CachePatchName(va("HOMING%d", i+1), PU_HUDGFX);
		itemhoming[i] = W_CachePatchName(va("HOMITM%d", i+1), PU_HUDGFX);
	}

	curweapon = W_CachePatchName("CURWEAP", PU_HUDGFX);
	normring = W_CachePatchName("RINGIND", PU_HUDGFX);
	bouncering = W_CachePatchName("BNCEIND", PU_HUDGFX);
	infinityring = W_CachePatchName("INFNIND", PU_HUDGFX);
	autoring = W_CachePatchName("AUTOIND", PU_HUDGFX);
	explosionring = W_CachePatchName("BOMBIND", PU_HUDGFX);
	scatterring = W_CachePatchName("SCATIND", PU_HUDGFX);
	grenadering = W_CachePatchName("GRENIND", PU_HUDGFX);
	railring = W_CachePatchName("RAILIND", PU_HUDGFX);
	jumpshield = W_CachePatchName("TVWWC0", PU_HUDGFX);
	forceshield = W_CachePatchName("TVFOC0", PU_HUDGFX);
	ringshield = W_CachePatchName("TVATC0", PU_HUDGFX);
	watershield = W_CachePatchName("TVELC0", PU_HUDGFX);
	bombshield = W_CachePatchName("TVARC0", PU_HUDGFX);
	pityshield = W_CachePatchName("TVPIC0", PU_HUDGFX);
	flameshield = W_CachePatchName("TVFLC0", PU_HUDGFX);
	bubbleshield = W_CachePatchName("TVBBC0", PU_HUDGFX);
	thundershield = W_CachePatchName("TVZPC0", PU_HUDGFX);
	invincibility = W_CachePatchName("TVIVC0", PU_HUDGFX);
	sneakers = W_CachePatchName("TVSSC0", PU_HUDGFX);
	gravboots = W_CachePatchName("TVGVC0", PU_HUDGFX);

	tagico = W_CachePatchName("TAGICO", PU_HUDGFX);
	rflagico = W_CachePatchName("RFLAGICO", PU_HUDGFX);
	bflagico = W_CachePatchName("BFLAGICO", PU_HUDGFX);
	rmatcico = W_CachePatchName("RMATCICO", PU_HUDGFX);
	bmatcico = W_CachePatchName("BMATCICO", PU_HUDGFX);
	gotrflag = W_CachePatchName("GOTRFLAG", PU_HUDGFX);
	gotbflag = W_CachePatchName("GOTBFLAG", PU_HUDGFX);
	nonicon = W_CachePatchName("NONICON", PU_HUDGFX);

	// NiGHTS HUD things
	bluestat = W_CachePatchName("BLUESTAT", PU_HUDGFX);
	byelstat = W_CachePatchName("BYELSTAT", PU_HUDGFX);
	orngstat = W_CachePatchName("ORNGSTAT", PU_HUDGFX);
	redstat = W_CachePatchName("REDSTAT", PU_HUDGFX);
	yelstat = W_CachePatchName("YELSTAT", PU_HUDGFX);
	nbracket = W_CachePatchName("NBRACKET", PU_HUDGFX);
	for (i = 0; i < 12; ++i)
		nhud[i] = W_CachePatchName(va("NHUD%d", i+1), PU_HUDGFX);
	nsshud = W_CachePatchName("NSSHUD", PU_HUDGFX);
	minicaps = W_CachePatchName("MINICAPS", PU_HUDGFX);

	for (i = 0; i < 8; ++i)
	{
		narrow[i] = W_CachePatchName(va("NARROW%d", i+1), PU_HUDGFX);
		nredar[i] = W_CachePatchName(va("NREDAR%d", i+1), PU_HUDGFX);
	}

	// non-animated version
	narrow[8] = W_CachePatchName("NARROW9", PU_HUDGFX);

	drillbar = W_CachePatchName("DRILLBAR", PU_HUDGFX);
	for (i = 0; i < 3; ++i)
		drillfill[i] = W_CachePatchName(va("DRILLFI%d", i+1), PU_HUDGFX);
	capsulebar = W_CachePatchName("CAPSBAR", PU_HUDGFX);
	capsulefill = W_CachePatchName("CAPSFILL", PU_HUDGFX);
	minus5sec = W_CachePatchName("MINUS5", PU_HUDGFX);

	for (i = 0; i < 7; ++i)
		ngradeletters[i] = W_CachePatchName(va("GRADE%d", i), PU_HUDGFX);
}

// made separate so that skins code can reload custom face graphics
void ST_LoadFaceGraphics(char *facestr, char *superstr, INT32 skinnum)
{
	faceprefix[skinnum] = W_CachePatchName(facestr, PU_HUDGFX);
	superprefix[skinnum] = W_CachePatchName(superstr, PU_HUDGFX);
	facefreed[skinnum] = false;
}

void ST_ReloadSkinFaceGraphics(void)
{
	INT32 i;

	for (i = 0; i < numskins; i++)
		ST_LoadFaceGraphics(skins[i].face, skins[i].superface, i);
}

static inline void ST_InitData(void)
{
	// 'link' the statusbar display to a player, which could be
	// another player than consoleplayer, for example, when you
	// change the view in a multiplayer demo with F12.
	stplyr = &players[displayplayer];

	st_palette = -1;
}

static inline void ST_Stop(void)
{
	if (st_stopped)
		return;

	V_SetPalette(0);

	st_stopped = true;
}

void ST_Start(void)
{
	if (!st_stopped)
		ST_Stop();

	ST_InitData();
	st_stopped = false;
}

//
// Initializes the status bar, sets the defaults border patch for the window borders.
//

// used by OpenGL mode, holds lumpnum of flat used to fill space around the viewwindow
lumpnum_t st_borderpatchnum;

void ST_Init(void)
{
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
		facefreed[i] = true;

	if (dedicated)
		return;

	ST_LoadGraphics();
}

// change the status bar too, when pressing F12 while viewing a demo.
void ST_changeDemoView(void)
{
	// the same routine is called at multiplayer deathmatch spawn
	// so it can be called multiple times
	ST_Start();
}

// =========================================================================
//                         STATUS BAR OVERLAY
// =========================================================================

boolean st_overlay;

static INT32 SCZ(INT32 z)
{
	return FixedInt(FixedMul(z<<FRACBITS, vid.fdupy));
}

static INT32 SCY(INT32 y)
{
	//31/10/99: fixed by Hurdler so it _works_ also in hardware mode
	// do not scale to resolution for hardware accelerated
	// because these modes always scale by default
	y = SCZ(y); // scale to resolution
	if (splitscreen)
	{
		y >>= 1;
		if (stplyr != &players[displayplayer])
			y += vid.height / 2;
	}
	return y;
}

static INT32 STRINGY(INT32 y)
{
	//31/10/99: fixed by Hurdler so it _works_ also in hardware mode
	// do not scale to resolution for hardware accelerated
	// because these modes always scale by default
	if (splitscreen)
	{
		y >>= 1;
		if (stplyr != &players[displayplayer])
			y += BASEVIDHEIGHT / 2;
	}
	return y;
}

static INT32 SPLITFLAGS(INT32 f)
{
	// Pass this V_SNAPTO(TOP|BOTTOM) and it'll trim them to account for splitscreen! -Red
	if (splitscreen)
	{
		if (stplyr != &players[displayplayer])
			f &= ~V_SNAPTOTOP;
		else
			f &= ~V_SNAPTOBOTTOM;
	}
	return f;
}

static INT32 SCX(INT32 x)
{
	return FixedInt(FixedMul(x<<FRACBITS, vid.fdupx));
}

#if 0
static INT32 SCR(INT32 r)
{
	fixed_t y;
		//31/10/99: fixed by Hurdler so it _works_ also in hardware mode
	// do not scale to resolution for hardware accelerated
	// because these modes always scale by default
	y = FixedMul(r*FRACUNIT, vid.fdupy); // scale to resolution
	if (splitscreen)
	{
		y >>= 1;
		if (stplyr != &players[displayplayer])
			y += vid.height / 2;
	}
	return FixedInt(FixedDiv(y, vid.fdupy));
}
#endif

// =========================================================================
//                          INTERNAL DRAWING
// =========================================================================
#define ST_DrawOverlayNum(x,y,n)           V_DrawTallNum(x, y, V_NOSCALESTART|V_HUDTRANS, n)
#define ST_DrawPaddedOverlayNum(x,y,n,d)   V_DrawPaddedTallNum(x, y, V_NOSCALESTART|V_HUDTRANS, n, d)
#define ST_DrawOverlayPatch(x,y,p)         V_DrawScaledPatch(x, y, V_NOSCALESTART|V_HUDTRANS, p)
#define ST_DrawMappedOverlayPatch(x,y,p,c) V_DrawMappedScaledPatch(x, y, V_NOSCALESTART|V_HUDTRANS, p, c)
#define ST_DrawNumFromHud(h,n,f)        V_DrawTallNum(SCX(hudinfo[h].x), SCY(hudinfo[h].y), V_NOSCALESTART|f, n)
#define ST_DrawPadNumFromHud(h,n,q,f)   V_DrawPaddedTallNum(SCX(hudinfo[h].x), SCY(hudinfo[h].y), V_NOSCALESTART|f, n, q)
#define ST_DrawPatchFromHud(h,p,f)      V_DrawScaledPatch(SCX(hudinfo[h].x), SCY(hudinfo[h].y), V_NOSCALESTART|f, p)
#define ST_DrawNumFromHudWS(h,n,f)      V_DrawTallNum(SCX(hudinfo[h+!!splitscreen].x), SCY(hudinfo[h+!!splitscreen].y), V_NOSCALESTART|f, n)
#define ST_DrawPadNumFromHudWS(h,n,q,f) V_DrawPaddedTallNum(SCX(hudinfo[h+!!splitscreen].x), SCY(hudinfo[h+!!splitscreen].y), V_NOSCALESTART|f, n, q)
#define ST_DrawPatchFromHudWS(h,p,f)    V_DrawScaledPatch(SCX(hudinfo[h+!!splitscreen].x), SCY(hudinfo[h+!!splitscreen].y), V_NOSCALESTART|f, p)

// Draw a number, scaled, over the view, maybe with set translucency
// Always draw the number completely since it's overlay
//
// Supports different colors! woo!
static void ST_DrawNightsOverlayNum(INT32 x /* right border */, INT32 y, INT32 a,
	INT32 num, patch_t **numpat, skincolors_t colornum)
{
	INT32 w = SHORT(numpat[0]->width);
	const UINT8 *colormap;

	// I want my V_SNAPTOx flags. :< -Red
	//a &= V_ALPHAMASK;

	if (colornum == 0)
		colormap = colormaps;
	else // Uses the player colors.
		colormap = R_GetTranslationColormap(TC_DEFAULT, colornum, GTC_CACHE);

	I_Assert(num >= 0); // this function does not draw negative numbers

	// draw the number
	do
	{
		x -= w;
		V_DrawTranslucentMappedPatch(x, y, a, numpat[num % 10], colormap);
		num /= 10;
	} while (num);

	// Sorry chum, this function only draws UNSIGNED values!
}

// Devmode information
static void ST_drawDebugInfo(void)
{
	INT32 height = 192;
	INT32 dist = 8;

	if (!(stplyr->mo && cv_debug))
		return;

	if (cv_ticrate.value)
	{
		height -= 12;
		dist >>= 1;
	}

	if (cv_debug & DBG_BASIC)
	{
		const fixed_t d = AngleFixed(stplyr->mo->angle);
		V_DrawRightAlignedString(320, height - 24, V_MONOSPACE, va("X: %6d", stplyr->mo->x>>FRACBITS));
		V_DrawRightAlignedString(320, height - 16, V_MONOSPACE, va("Y: %6d", stplyr->mo->y>>FRACBITS));
		V_DrawRightAlignedString(320, height - 8,  V_MONOSPACE, va("Z: %6d", stplyr->mo->z>>FRACBITS));
		V_DrawRightAlignedString(320, height,      V_MONOSPACE, va("A: %6d", FixedInt(d)));

		height -= (32+dist);
	}

	if (cv_debug & DBG_DETAILED)
	{
		V_DrawRightAlignedString(320, height - 104, V_MONOSPACE, va("SHIELD: %5x", stplyr->powers[pw_shield]));
		V_DrawRightAlignedString(320, height - 96,  V_MONOSPACE, va("SCALE: %5d%%", (stplyr->mo->scale*100)/FRACUNIT));
		V_DrawRightAlignedString(320, height - 88,  V_MONOSPACE, va("CARRY: %5x", stplyr->powers[pw_carry]));
		V_DrawRightAlignedString(320, height - 80,  V_MONOSPACE, va("AIR: %4d, %3d", stplyr->powers[pw_underwater], stplyr->powers[pw_spacetime]));

		// Flags
		V_DrawRightAlignedString(304-92, height - 72, V_MONOSPACE, "PF:");
		V_DrawString(304-90,             height - 72, (stplyr->pflags & PF_STARTJUMP) ? V_GREENMAP : V_REDMAP, "SJ");
		V_DrawString(304-72,             height - 72, (stplyr->pflags & PF_JUMPED) ? V_GREENMAP : V_REDMAP, "JD");
		V_DrawString(304-54,             height - 72, (stplyr->pflags & PF_SPINNING) ? V_GREENMAP : V_REDMAP, "SP");
		V_DrawString(304-36,             height - 72, (stplyr->pflags & PF_STARTDASH) ? V_GREENMAP : V_REDMAP, "ST");
		V_DrawString(304-18,             height - 72, (stplyr->pflags & PF_THOKKED) ? V_GREENMAP : V_REDMAP, "TH");
		V_DrawString(304,                height - 72, (stplyr->pflags & PF_SHIELDABILITY) ? V_GREENMAP : V_REDMAP, "SH");

		V_DrawRightAlignedString(320, height - 64, V_MONOSPACE, va("CEILZ: %6d", stplyr->mo->ceilingz>>FRACBITS));
		V_DrawRightAlignedString(320, height - 56, V_MONOSPACE, va("FLOORZ: %6d", stplyr->mo->floorz>>FRACBITS));

		V_DrawRightAlignedString(320, height - 48, V_MONOSPACE, va("CNVX: %6d", stplyr->cmomx>>FRACBITS));
		V_DrawRightAlignedString(320, height - 40, V_MONOSPACE, va("CNVY: %6d", stplyr->cmomy>>FRACBITS));
		V_DrawRightAlignedString(320, height - 32, V_MONOSPACE, va("PLTZ: %6d", stplyr->mo->pmomz>>FRACBITS));

		V_DrawRightAlignedString(320, height - 24, V_MONOSPACE, va("MOMX: %6d", stplyr->rmomx>>FRACBITS));
		V_DrawRightAlignedString(320, height - 16, V_MONOSPACE, va("MOMY: %6d", stplyr->rmomy>>FRACBITS));
		V_DrawRightAlignedString(320, height - 8,  V_MONOSPACE, va("MOMZ: %6d", stplyr->mo->momz>>FRACBITS));
		V_DrawRightAlignedString(320, height,      V_MONOSPACE, va("SPEED: %6d", stplyr->speed>>FRACBITS));

		height -= (112+dist);
	}

	if (cv_debug & DBG_RANDOMIZER) // randomizer testing
	{
		fixed_t peekres = P_RandomPeek();
		peekres *= 10000;     // Change from fixed point
		peekres >>= FRACBITS; // to displayable decimal

		V_DrawRightAlignedString(320, height - 16, V_MONOSPACE, va("Init: %08x", P_GetInitSeed()));
		V_DrawRightAlignedString(320, height - 8,  V_MONOSPACE, va("Seed: %08x", P_GetRandSeed()));
		V_DrawRightAlignedString(320, height,      V_MONOSPACE, va("==  :    .%04d", peekres));

		height -= (24+dist);
	}

	if (cv_debug & DBG_MEMORY)
	{
		V_DrawRightAlignedString(320, height,     V_MONOSPACE, va("Heap: %7sKB", sizeu1(Z_TagsUsage(0, INT32_MAX)>>10)));
	}
}

static void ST_drawScore(void)
{
	// SCORE:
	ST_DrawPatchFromHud(HUD_SCORE, sboscore, V_HUDTRANS);
	if (objectplacing)
	{
		if (op_displayflags > UINT16_MAX)
			ST_DrawOverlayPatch(SCX(hudinfo[HUD_SCORENUM].x-tallminus->width), SCY(hudinfo[HUD_SCORENUM].y), tallminus);
		else
			ST_DrawNumFromHud(HUD_SCORENUM, op_displayflags, V_HUDTRANS);
	}
	else
		ST_DrawNumFromHud(HUD_SCORENUM, stplyr->score,V_HUDTRANS);
}

static void ST_drawTime(void)
{
	INT32 seconds, minutes, tictrn, tics;

	// TIME:
	ST_DrawPatchFromHudWS(HUD_TIME, ((mapheaderinfo[gamemap-1]->countdown && countdowntimer < 11*TICRATE && leveltime/5 & 1) ? sboredtime : sbotime), V_HUDTRANS);

	if (objectplacing)
	{
		tics    = objectsdrawn;
		seconds = objectsdrawn%100;
		minutes = objectsdrawn/100;
		tictrn  = 0;
	}
	else
	{
		tics = (mapheaderinfo[gamemap-1]->countdown ? countdowntimer : stplyr->realtime);
		seconds = G_TicsToSeconds(tics);
		minutes = G_TicsToMinutes(tics, true);
		tictrn  = G_TicsToCentiseconds(tics);
	}

	if (cv_timetic.value == 1) // Tics only -- how simple is this?
		ST_DrawNumFromHudWS(HUD_SECONDS, tics, V_HUDTRANS);
	else
	{
		ST_DrawNumFromHudWS(HUD_MINUTES, minutes, V_HUDTRANS); // Minutes
		ST_DrawPatchFromHudWS(HUD_TIMECOLON, sbocolon, V_HUDTRANS); // Colon
		ST_DrawPadNumFromHudWS(HUD_SECONDS, seconds, 2, V_HUDTRANS); // Seconds

		if (!splitscreen && (cv_timetic.value == 2 || modeattacking)) // there's not enough room for tics in splitscreen, don't even bother trying!
		{
			ST_DrawPatchFromHud(HUD_TIMETICCOLON, sboperiod, V_HUDTRANS); // Period
			ST_DrawPadNumFromHud(HUD_TICS, tictrn, 2, V_HUDTRANS); // Tics
		}
	}
}

static inline void ST_drawRings(void)
{
	INT32 ringnum = max(stplyr->rings, 0);

	ST_DrawPatchFromHudWS(HUD_RINGS, ((!stplyr->spectator && stplyr->rings <= 0 && leveltime/5 & 1) ? sboredrings : sborings), ((stplyr->spectator) ? V_HUDTRANSHALF : V_HUDTRANS));

	if (objectplacing)
		ringnum = op_currentdoomednum;
	else if (!useNightsSS && G_IsSpecialStage(gamemap))
	{
		INT32 i;
		ringnum = 0;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].mo && players[i].rings > 0)
				ringnum += players[i].rings;
	}

	ST_DrawNumFromHudWS(HUD_RINGSNUM, ringnum, ((stplyr->spectator) ? V_HUDTRANSHALF : V_HUDTRANS));
}

static void ST_drawLives(void)
{
	const INT32 v_splitflag = (splitscreen && stplyr == &players[displayplayer] ? V_SPLITSCREEN : 0);
	INT32 livescount;
	boolean notgreyedout;

	if (!stplyr->skincolor)
		return; // Just joined a server, skin isn't loaded yet!

	// face background
	V_DrawSmallScaledPatch(hudinfo[HUD_LIVESPIC].x, hudinfo[HUD_LIVESPIC].y + (v_splitflag ? -12 : 0),
		V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_HUDTRANS|v_splitflag, livesback);

	// face
	if (stplyr->mo && stplyr->mo->color)
	{
		// skincolor face/super
		UINT8 *colormap = R_GetTranslationColormap(stplyr->skin, stplyr->mo->color, GTC_CACHE);
		patch_t *face = faceprefix[stplyr->skin];
		if (stplyr->powers[pw_super])
			face = superprefix[stplyr->skin];
		V_DrawSmallMappedPatch(hudinfo[HUD_LIVESPIC].x, hudinfo[HUD_LIVESPIC].y + (v_splitflag ? -12 : 0),
			V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_HUDTRANS|v_splitflag,face, colormap);
		if (cv_translucenthud.value == 10 && stplyr->powers[pw_super] == 1 && stplyr->mo->tracer)
		{
			INT32 v_supertrans = (stplyr->mo->tracer->frame & FF_TRANSMASK) >> FF_TRANSSHIFT;
			if (v_supertrans < 10)
			{
				v_supertrans <<= V_ALPHASHIFT;
				colormap = R_GetTranslationColormap(stplyr->skin, stplyr->mo->tracer->color, GTC_CACHE);
				V_DrawSmallMappedPatch(hudinfo[HUD_LIVESPIC].x, hudinfo[HUD_LIVESPIC].y + (v_splitflag ? -12 : 0),
					V_SNAPTOLEFT|V_SNAPTOBOTTOM|v_supertrans|v_splitflag,face, colormap);
			}
		}
	}
	else if (stplyr->skincolor)
	{
		// skincolor face
		UINT8 *colormap = R_GetTranslationColormap(stplyr->skin, stplyr->skincolor, GTC_CACHE);
		V_DrawSmallMappedPatch(hudinfo[HUD_LIVESPIC].x, hudinfo[HUD_LIVESPIC].y + (v_splitflag ? -12 : 0),
			V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_HUDTRANS|v_splitflag,faceprefix[stplyr->skin], colormap);
	}

	// name
	if (strlen(skins[stplyr->skin].hudname) > 8)
		V_DrawThinString(hudinfo[HUD_LIVESNAME].x, hudinfo[HUD_LIVESNAME].y + (v_splitflag ? -12 : 0),
			V_HUDTRANS|V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_MONOSPACE|V_YELLOWMAP|v_splitflag, skins[stplyr->skin].hudname);
	else
		V_DrawString(hudinfo[HUD_LIVESNAME].x, hudinfo[HUD_LIVESNAME].y + (v_splitflag ? -12 : 0),
			V_HUDTRANS|V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_MONOSPACE|V_YELLOWMAP|v_splitflag, skins[stplyr->skin].hudname);
	// x
	V_DrawScaledPatch(hudinfo[HUD_LIVESX].x, hudinfo[HUD_LIVESX].y + (v_splitflag ? -4 : 0),
		V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_HUDTRANS|v_splitflag, stlivex);

	// lives number
	if ((netgame || multiplayer) && gametype == GT_COOP && cv_cooplives.value == 3)
	{
		INT32 i;
		livescount = 0;
		notgreyedout = (stplyr->lives > 0);
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			if (players[i].lives < 1)
				continue;

			if (players[i].lives > 1)
				notgreyedout = true;

			if (players[i].lives == 0x7f)
			{
				livescount = 0x7f;
				break;
			}
			else if (livescount < 99)
				livescount += (players[i].lives);
		}
	}
	else
	{
		livescount = stplyr->lives;
		notgreyedout = true;
	}

	if (livescount == 0x7f)
		V_DrawCharacter(hudinfo[HUD_LIVESNUM].x - 8, hudinfo[HUD_LIVESNUM].y + (v_splitflag ? -4 : 0), '\x16' | 0x80 | V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_HUDTRANS|v_splitflag, false);
	else
	{
		if (livescount > 99)
			livescount = 99;
		V_DrawRightAlignedString(hudinfo[HUD_LIVESNUM].x, hudinfo[HUD_LIVESNUM].y + (v_splitflag ? -4 : 0),
			V_SNAPTOLEFT|V_SNAPTOBOTTOM|(notgreyedout ? V_HUDTRANS : V_HUDTRANSHALF)|v_splitflag,
			((livescount > 99) ? "!!" : va("%d",livescount)));
	}
}

static void ST_drawInput(void)
{
	//const INT32 v_splitflag = (splitscreen && stplyr == &players[displayplayer] ? V_SPLITSCREEN : 0); -- no splitscreen support - record attack only for base game
	const UINT8 accent = (stplyr->skincolor ? Color_Index[stplyr->skincolor-1][4] : 0);
	UINT8 col, offs;

	INT32 x = hudinfo[HUD_LIVESPIC].x, y = hudinfo[HUD_LIVESPIC].y;

	if (stplyr->powers[pw_carry] == CR_NIGHTSMODE)
		y -= 16;

	// O backing
	V_DrawFill(x, y-1, 16, 16, 20);
	V_DrawFill(x, y+15, 16, 1, 29);

	if (cv_showinputjoy.value) // joystick render!
	{
		/*V_DrawFill(x   , y   , 16,  1, 16);
		V_DrawFill(x   , y+15, 16,  1, 16);
		V_DrawFill(x   , y+ 1,  1, 14, 16);
		V_DrawFill(x+15, y+ 1,  1, 14, 16); -- red's outline*/
		if (stplyr->cmd.sidemove || stplyr->cmd.forwardmove)
		{
			// joystick hole
			V_DrawFill(x+5, y+4, 6, 6, 29);
			// joystick top
			V_DrawFill(x+3+stplyr->cmd.sidemove/12,
				y+2-stplyr->cmd.forwardmove/12,
				10, 10, 29);
			V_DrawFill(x+3+stplyr->cmd.sidemove/9,
				y+1-stplyr->cmd.forwardmove/9,
				10, 10, accent);
		}
		else
		{
			// just a limited, greyed out joystick top
			V_DrawFill(x+3, y+11, 10, 1, 29);
			V_DrawFill(x+3,
				y+1,
				10, 10, 16);
		}
	}
	else // arrows!
	{
		// <
		if (stplyr->cmd.sidemove < 0)
		{
			offs = 0;
			col = accent;
		}
		else
		{
			offs = 1;
			col = 16;
			V_DrawFill(x- 2, y+10,  6,  1, 29);
			V_DrawFill(x+ 4, y+ 9,  1,  1, 29);
			V_DrawFill(x+ 5, y+ 8,  1,  1, 29);
		}
		V_DrawFill(x- 2, y+ 5-offs,  6,  6, col);
		V_DrawFill(x+ 4, y+ 6-offs,  1,  4, col);
		V_DrawFill(x+ 5, y+ 7-offs,  1,  2, col);

		// ^
		if (stplyr->cmd.forwardmove > 0)
		{
			offs = 0;
			col = accent;
		}
		else
		{
			offs = 1;
			col = 16;
			V_DrawFill(x+ 5, y+ 3,  1,  1, 29);
			V_DrawFill(x+ 6, y+ 4,  1,  1, 29);
			V_DrawFill(x+ 7, y+ 5,  2,  1, 29);
			V_DrawFill(x+ 9, y+ 4,  1,  1, 29);
			V_DrawFill(x+10, y+ 3,  1,  1, 29);
		}
		V_DrawFill(x+ 5, y- 2-offs,  6,  6, col);
		V_DrawFill(x+ 6, y+ 4-offs,  4,  1, col);
		V_DrawFill(x+ 7, y+ 5-offs,  2,  1, col);

		// >
		if (stplyr->cmd.sidemove > 0)
		{
			offs = 0;
			col = accent;
		}
		else
		{
			offs = 1;
			col = 16;
			V_DrawFill(x+12, y+10,  6,  1, 29);
			V_DrawFill(x+11, y+ 9,  1,  1, 29);
			V_DrawFill(x+10, y+ 8,  1,  1, 29);
		}
		V_DrawFill(x+12, y+ 5-offs,  6,  6, col);
		V_DrawFill(x+11, y+ 6-offs,  1,  4, col);
		V_DrawFill(x+10, y+ 7-offs,  1,  2, col);

		// v
		if (stplyr->cmd.forwardmove < 0)
		{
			offs = 0;
			col = accent;
		}
		else
		{
			offs = 1;
			col = 16;
			V_DrawFill(x+ 5, y+17,  6,  1, 29);
		}
		V_DrawFill(x+ 5, y+12-offs,  6,  6, col);
		V_DrawFill(x+ 6, y+11-offs,  4,  1, col);
		V_DrawFill(x+ 7, y+10-offs,  2,  1, col);
	}

#define drawbutt(xoffs, yoffs, butt, symb)\
	if (stplyr->cmd.buttons & butt)\
	{\
		offs = 0;\
		col = accent;\
	}\
	else\
	{\
		offs = 1;\
		col = 16;\
		V_DrawFill(x+16+(xoffs), y+9+(yoffs), 10, 1, 29);\
	}\
	V_DrawFill(x+16+(xoffs), y+(yoffs)-offs, 10, 10, col);\
	V_DrawCharacter(x+16+1+(xoffs), y+1+(yoffs)-offs, symb, false)

	drawbutt( 4,-3, BT_JUMP, 'J');
	drawbutt(15,-3, BT_USE,  'S');

	V_DrawFill(x+16+4, y+8, 21, 10, 20); // sundial backing
	if (stplyr->mo)
	{
		UINT8 i, precision;
		angle_t ang = (stplyr->powers[pw_carry] == CR_NIGHTSMODE)
		? (FixedAngle((stplyr->flyangle-90)<<FRACBITS)>>ANGLETOFINESHIFT)
		: (stplyr->mo->angle - R_PointToAngle(stplyr->mo->x, stplyr->mo->y))>>ANGLETOFINESHIFT;
		fixed_t xcomp = FINESINE(ang)>>13;
		fixed_t ycomp = FINECOSINE(ang)>>14;
		if (ycomp == 4)
			ycomp = 3;

		if (ycomp > 0)
			V_DrawFill(x+16+13-xcomp, y+11-ycomp, 3, 3, accent); // point (behind)

		precision = max(3, abs(xcomp));
		for (i = 0; i < precision; i++) // line
		{
			V_DrawFill(x+16+14-(i*xcomp)/precision,
				y+12-(i*ycomp)/precision,
				1, 1, 16);
		}

		if (ycomp <= 0)
			V_DrawFill(x+16+13-xcomp, y+11-ycomp, 3, 3, accent); // point (in front)
	}

#undef drawbutt

	// text above
	x -= 2;
	y -= 13;
	if (!stplyr->powers[pw_carry] == CR_NIGHTSMODE)
	{
		if (stplyr->pflags & PF_AUTOBRAKE)
		{
			V_DrawThinString(x, y,
				((!stplyr->powers[pw_carry]
				&& (stplyr->pflags & PF_APPLYAUTOBRAKE)
				&& !(stplyr->cmd.sidemove || stplyr->cmd.forwardmove)
				&& (stplyr->rmomx || stplyr->rmomy))
				? 0 : V_GRAYMAP),
				"AUTOBRAKE");
			y -= 8;
		}
		if (stplyr->pflags & PF_ANALOGMODE)
		{
			V_DrawThinString(x, y, 0, "ANALOG");
			y -= 8;
		}
	}
	if (!demosynced) // should always be last, so it doesn't push anything else around
		V_DrawThinString(x, y, ((leveltime & 4) ? V_YELLOWMAP : V_REDMAP), "BAD DEMO!!");
}

static void ST_drawLevelTitle(void)
{
	char *lvlttl = mapheaderinfo[gamemap-1]->lvlttl;
	char *subttl = mapheaderinfo[gamemap-1]->subttl;
	INT32 actnum = mapheaderinfo[gamemap-1]->actnum;
	INT32 lvlttlxpos;
	INT32 subttlxpos = BASEVIDWIDTH/2;
	INT32 ttlnumxpos;
	INT32 zonexpos;

	INT32 lvlttly;
	INT32 zoney;

	if (!(timeinmap > 2 && timeinmap-3 < 110))
		return;

	if (actnum > 0)
	{
		ttlnum = W_CachePatchName(va("TTL%.2d", actnum), PU_CACHE);
		lvlttlxpos = ((BASEVIDWIDTH/2) - (V_LevelNameWidth(lvlttl)/2)) - SHORT(ttlnum->width);
	}
	else
		lvlttlxpos = ((BASEVIDWIDTH/2) - (V_LevelNameWidth(lvlttl)/2));

	ttlnumxpos = lvlttlxpos + V_LevelNameWidth(lvlttl);
	zonexpos = ttlnumxpos - V_LevelNameWidth(M_GetText("ZONE"));

	if (lvlttlxpos < 0)
		lvlttlxpos = 0;

	// There's no consistent algorithm that can accurately define the old positions
	// so I just ended up resorting to a single switct statement to define them
	switch (timeinmap-3)
	{
		case 0:   zoney = 200; lvlttly =   0; break;
		case 1:   zoney = 188; lvlttly =  12; break;
		case 2:   zoney = 176; lvlttly =  24; break;
		case 3:   zoney = 164; lvlttly =  36; break;
		case 4:   zoney = 152; lvlttly =  48; break;
		case 5:   zoney = 140; lvlttly =  60; break;
		case 6:   zoney = 128; lvlttly =  72; break;
		case 105: zoney =  80; lvlttly = 104; break;
		case 106: zoney =  56; lvlttly = 128; break;
		case 107: zoney =  32; lvlttly = 152; break;
		case 108: zoney =   8; lvlttly = 176; break;
		case 109: zoney =   0; lvlttly = 200; break;
		default:  zoney = 104; lvlttly =  80; break;
	}

	if (actnum)
		V_DrawScaledPatch(ttlnumxpos, zoney, 0, ttlnum);

	V_DrawLevelTitle(lvlttlxpos, lvlttly, 0, lvlttl);

	if (!(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
		V_DrawLevelTitle(zonexpos, zoney, 0, M_GetText("ZONE"));

	if (lvlttly+48 < 200)
		V_DrawCenteredString(subttlxpos, lvlttly+48, V_ALLOWLOWERCASE, subttl);
}

static void ST_drawFirstPersonHUD(void)
{
	player_t *player = stplyr;
	patch_t *p = NULL;
	UINT16 invulntime = 0;

	if (player->playerstate != PST_LIVE)
		return;

	// Graue 06-18-2004: no V_NOSCALESTART, no SCX, no SCY, snap to right
	if ((player->powers[pw_shield] & SH_NOSTACK & ~SH_FORCEHP) == SH_FORCE)
	{
		UINT8 i, max = (player->powers[pw_shield] & SH_FORCEHP);
		for (i = 0; i <= max; i++)
		{
			INT32 flags = (V_SNAPTORIGHT|V_SNAPTOTOP)|((i == max) ? V_HUDTRANS : V_HUDTRANSHALF);
			if (splitscreen)
				V_DrawSmallScaledPatch(312-(3*i), STRINGY(24)+(3*i), flags, forceshield);
			else
				V_DrawScaledPatch(304-(3*i), 24+(3*i), flags, forceshield);
		}
	}
	else switch (player->powers[pw_shield] & SH_NOSTACK)
	{
	case SH_WHIRLWIND:   p = jumpshield;    break;
	case SH_ELEMENTAL:   p = watershield;   break;
	case SH_ARMAGEDDON:  p = bombshield;    break;
	case SH_ATTRACT:     p = ringshield;    break;
	case SH_PITY:        p = pityshield;    break;
	case SH_FLAMEAURA:   p = flameshield;   break;
	case SH_BUBBLEWRAP:  p = bubbleshield;  break;
	case SH_THUNDERCOIN: p = thundershield; break;
	default: break;
	}

	if (p)
	{
		if (splitscreen)
			V_DrawSmallScaledPatch(312, STRINGY(24), V_SNAPTORIGHT|V_SNAPTOTOP|V_HUDTRANS, p);
		else
			V_DrawScaledPatch(304, 24, V_SNAPTORIGHT|V_SNAPTOTOP|V_HUDTRANS, p);
	}

	// pw_flashing just sets the icon to flash no matter what.
	invulntime = player->powers[pw_flashing] ? 1 : player->powers[pw_invulnerability];
	if (invulntime > 3*TICRATE || (invulntime && leveltime & 1))
	{
		if (splitscreen)
			V_DrawSmallScaledPatch(312, STRINGY(24) + 14, V_SNAPTORIGHT|V_SNAPTOTOP|V_HUDTRANS, invincibility);
		else
			V_DrawScaledPatch(304, 24 + 28, V_SNAPTORIGHT|V_SNAPTOTOP|V_HUDTRANS, invincibility);
	}

	if (player->powers[pw_sneakers] > 3*TICRATE || (player->powers[pw_sneakers] && leveltime & 1))
	{
		if (splitscreen)
			V_DrawSmallScaledPatch(312, STRINGY(24) + 28, V_SNAPTORIGHT|V_SNAPTOTOP|V_HUDTRANS, sneakers);
		else
			V_DrawScaledPatch(304, 24 + 56, V_SNAPTORIGHT|V_SNAPTOTOP|V_HUDTRANS, sneakers);
	}

	p = NULL;

	{
		UINT32 airtime;
		UINT32 frame = 0;
		spriteframe_t *sprframe;
		// If both air timers are active, use the air timer with the least time left
		if (player->powers[pw_underwater] && player->powers[pw_spacetime])
			airtime = min(player->powers[pw_underwater], player->powers[pw_spacetime]);
		else // Use whichever one is active otherwise
			airtime = (player->powers[pw_spacetime]) ? player->powers[pw_spacetime] : player->powers[pw_underwater];

		if (!airtime)
			return; // No air timers are active, nothing would be drawn anyway

		airtime--; // The original code was all n*TICRATE + 1, so let's remove 1 tic for simplicity

		if (airtime > 11*TICRATE)
			return; // Not time to draw any drown numbers yet
		// Choose which frame to use based on time left
		if (airtime <= 11*TICRATE && airtime >= 10*TICRATE)
			frame = 5;
		else if (airtime <= 9*TICRATE && airtime >= 8*TICRATE)
			frame = 4;
		else if (airtime <= 7*TICRATE && airtime >= 6*TICRATE)
			frame = 3;
		else if (airtime <= 5*TICRATE && airtime >= 4*TICRATE)
			frame = 2;
		else if (airtime <= 3*TICRATE && airtime >= 2*TICRATE)
			frame = 1;
		else if (airtime <= 1*TICRATE && airtime > 0)
			frame = 0;
		else
			return; // Don't draw anything between numbers

		if (player->charflags & SF_MACHINE)
			frame += 6;  // Robots use different drown numbers

		// Get the front angle patch for the frame
		sprframe = &sprites[SPR_DRWN].spriteframes[frame];
		p = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);
	}

	// Display the countdown drown numbers!
	if (p)
		V_DrawScaledPatch(SCX((BASEVIDWIDTH/2) - (SHORT(p->width)/2) + SHORT(p->leftoffset)), SCY(60 - SHORT(p->topoffset)),
			V_NOSCALESTART|V_OFFSET|V_TRANSLUCENT, p);
}

// 2.0-1: [21:42] <+Rob> Beige - Lavender - Steel Blue - Peach - Orange - Purple - Silver - Yellow - Pink - Red - Blue - Green - Cyan - Gold
/*#define NUMLINKCOLORS 14
static skincolors_t linkColor[NUMLINKCOLORS] =
{SKINCOLOR_BEIGE,  SKINCOLOR_LAVENDER, SKINCOLOR_AZURE, SKINCOLOR_PEACH, SKINCOLOR_ORANGE,
 SKINCOLOR_MAGENTA, SKINCOLOR_SILVER, SKINCOLOR_SUPERGOLD4, SKINCOLOR_PINK,  SKINCOLOR_RED,
 SKINCOLOR_BLUE, SKINCOLOR_GREEN, SKINCOLOR_CYAN, SKINCOLOR_GOLD};*/

// 2.2 indev list: (unix time 1470866042) <Rob> Emerald, Aqua, Cyan, Blue, Pastel, Purple, Magenta, Rosy, Red, Orange, Gold, Yellow, Peridot
/*#define NUMLINKCOLORS 13
static skincolors_t linkColor[NUMLINKCOLORS] =
{SKINCOLOR_EMERALD, SKINCOLOR_AQUA, SKINCOLOR_CYAN, SKINCOLOR_BLUE, SKINCOLOR_PASTEL,
 SKINCOLOR_PURPLE, SKINCOLOR_MAGENTA, SKINCOLOR_ROSY, SKINCOLOR_RED,  SKINCOLOR_ORANGE,
 SKINCOLOR_GOLD, SKINCOLOR_YELLOW, SKINCOLOR_PERIDOT};*/

// 2.2+ list: [19:59:52] <baldobo> Ruby > Red > Flame > Sunset > Orange > Gold > Yellow > Lime > Green > Aqua  > cyan > Sky > Blue > Pastel > Purple > Bubblegum > Magenta > Rosy > repeat
// [20:00:25] <baldobo> Also Icy for the link freeze text color
// [20:04:03] <baldobo> I would start it on lime
#define NUMLINKCOLORS 18
static skincolors_t linkColor[NUMLINKCOLORS] =
{SKINCOLOR_LIME, SKINCOLOR_EMERALD, SKINCOLOR_AQUA, SKINCOLOR_CYAN, SKINCOLOR_SKY,
 SKINCOLOR_SAPPHIRE, SKINCOLOR_PASTEL, SKINCOLOR_PURPLE, SKINCOLOR_BUBBLEGUM, SKINCOLOR_MAGENTA,
 SKINCOLOR_ROSY, SKINCOLOR_RUBY, SKINCOLOR_RED, SKINCOLOR_FLAME, SKINCOLOR_SUNSET,
 SKINCOLOR_ORANGE, SKINCOLOR_GOLD, SKINCOLOR_YELLOW};

static void ST_drawNightsRecords(void)
{
	INT32 aflag = 0;

	if (!stplyr->texttimer)
		return;

	if (stplyr->texttimer < TICRATE/2)
		aflag = (9 - 9*stplyr->texttimer/(TICRATE/2)) << V_ALPHASHIFT;

	// A "Bonus Time Start" by any other name...
	if (stplyr->textvar == 1)
	{
		V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(52), V_GREENMAP|aflag, M_GetText("GET TO THE GOAL!"));
		V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(60),            aflag, M_GetText("SCORE MULTIPLIER START!"));

		if (stplyr->finishedtime)
		{
			V_DrawString(BASEVIDWIDTH/2 - 48, STRINGY(140), aflag, "TIME:");
			V_DrawString(BASEVIDWIDTH/2 - 48, STRINGY(148), aflag, "BONUS:");
			V_DrawRightAlignedString(BASEVIDWIDTH/2 + 48, STRINGY(140), V_ORANGEMAP|aflag, va("%d", (stplyr->startedtime - stplyr->finishedtime)/TICRATE));
			V_DrawRightAlignedString(BASEVIDWIDTH/2 + 48, STRINGY(148), V_ORANGEMAP|aflag, va("%d", (stplyr->finishedtime/TICRATE) * 100));
		}
	}

	// Get n [more] Spheres
	else if (stplyr->textvar <= 3 && stplyr->textvar >= 2)
	{
		if (!stplyr->capsule)
			return;

		// Yes, this string is an abomination.
		V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(60), aflag,
		                     va(M_GetText("\x80GET\x82 %d\x80 %s%s%s!"), stplyr->capsule->health,
		                        (stplyr->textvar == 3) ? M_GetText("MORE ") : "",
		                        (G_IsSpecialStage(gamemap)) ? "SPHERE" : "RING",
		                        (stplyr->capsule->health > 1) ? "S" : ""));
	}

	// End Bonus
	else if (stplyr->textvar == 4)
	{
		V_DrawString(BASEVIDWIDTH/2 - 48, STRINGY(140), aflag, (G_IsSpecialStage(gamemap)) ? "ORBS:" : "RINGS:");
		V_DrawString(BASEVIDWIDTH/2 - 48, STRINGY(148), aflag, "BONUS:");
		V_DrawRightAlignedString(BASEVIDWIDTH/2 + 48, STRINGY(140), V_ORANGEMAP|aflag, va("%d", stplyr->finishedrings));
		V_DrawRightAlignedString(BASEVIDWIDTH/2 + 48, STRINGY(148), V_ORANGEMAP|aflag, va("%d", stplyr->finishedrings * 50));
		ST_DrawNightsOverlayNum(BASEVIDWIDTH/2 + 48, STRINGY(160), aflag, stplyr->lastmarescore, nightsnum, SKINCOLOR_AZURE);

		// If new record, say so!
		if (!(netgame || multiplayer) && G_GetBestNightsScore(gamemap, stplyr->lastmare + 1) <= stplyr->lastmarescore)
		{
			if (stplyr->texttimer & 16)
				V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(184), V_YELLOWMAP|aflag, "* NEW RECORD *");
		}

		if (P_HasGrades(gamemap, stplyr->lastmare + 1))
		{
			if (aflag)
				V_DrawTranslucentPatch(BASEVIDWIDTH/2 + 60, STRINGY(160), aflag,
				                       ngradeletters[P_GetGrade(stplyr->lastmarescore, gamemap, stplyr->lastmare)]);
			else
				V_DrawScaledPatch(BASEVIDWIDTH/2 + 60, STRINGY(160), 0,
				                  ngradeletters[P_GetGrade(stplyr->lastmarescore, gamemap, stplyr->lastmare)]);
		}
	}
}

static void ST_drawNiGHTSHUD(void)
{
	INT32 origamount;
	INT32 minlink = 1;
	INT32 total_ringcount;
	boolean nosshack = false;

	// When debugging, show "0 Link".
	if (cv_debug & DBG_NIGHTSBASIC)
		minlink = 0;

	// Cheap hack: don't display when the score is showing (it popping up for a split second when exiting a map is intentional)
	if (stplyr->texttimer && stplyr->textvar == 4)
		minlink = INT32_MAX;

	if (G_IsSpecialStage(gamemap))
	{ // Since special stages share score, time, rings, etc.
		// disable splitscreen mode for its HUD.
		if (stplyr != &players[displayplayer])
			return;
		nosshack = splitscreen;
		splitscreen = false;
	}

	// Link drawing
	if (
#ifdef HAVE_BLUA
	LUA_HudEnabled(hud_nightslink) &&
#endif
	stplyr->linkcount > minlink)
	{
		skincolors_t colornum = linkColor[((stplyr->linkcount-1) / 5) % NUMLINKCOLORS];
		if (stplyr->powers[pw_nights_linkfreeze] && (!(stplyr->powers[pw_nights_linkfreeze] & 2) || (stplyr->powers[pw_nights_linkfreeze] > flashingtics)))
			colornum = SKINCOLOR_ICY;

		if (stplyr->linktimer < 2*TICRATE/3)
		{
			INT32 linktrans = (9 - 9*stplyr->linktimer/(2*TICRATE/3)) << V_ALPHASHIFT;

			if (splitscreen)
			{
				ST_DrawNightsOverlayNum(256, STRINGY(152), SPLITFLAGS(V_SNAPTOBOTTOM)|V_SNAPTORIGHT|linktrans, (stplyr->linkcount-1), nightsnum, colornum);
				V_DrawTranslucentMappedPatch(264, STRINGY(152), SPLITFLAGS(V_SNAPTOBOTTOM)|V_SNAPTORIGHT|linktrans, nightslink,
					colornum == 0 ? colormaps : R_GetTranslationColormap(TC_DEFAULT, colornum, GTC_CACHE));
			}
			else
			{
				ST_DrawNightsOverlayNum(160, 160, V_SNAPTOBOTTOM|linktrans, (stplyr->linkcount-1), nightsnum, colornum);
				V_DrawTranslucentMappedPatch(168, 160, V_SNAPTOBOTTOM|linktrans, nightslink,
					colornum == 0 ? colormaps : R_GetTranslationColormap(TC_DEFAULT, colornum, GTC_CACHE));
			}
		}
		else // normal, fullbright
		{
#if 0 // Cool but silly number effect where the previous link number fades away
			if (stplyr->linkcount > 2 && stplyr->linktimer > (2*TICRATE) - 9)
			{
				INT32 offs = 10 - (stplyr->linktimer - (2*TICRATE - 9));
				INT32 ghosttrans = offs << V_ALPHASHIFT;
				ST_DrawNightsOverlayNum(160, STRINGY(160+offs), SPLITFLAGS(V_SNAPTOBOTTOM)|ghosttrans, (stplyr->linkcount-2),
					nightsnum, colornum);
			}
#endif

			if (splitscreen)
			{
				ST_DrawNightsOverlayNum(256, STRINGY(152), SPLITFLAGS(V_SNAPTOBOTTOM)|V_SNAPTORIGHT, (stplyr->linkcount-1), nightsnum, colornum);
				V_DrawMappedPatch(264, STRINGY(152), SPLITFLAGS(V_SNAPTOBOTTOM)|V_SNAPTORIGHT, nightslink,
					colornum == 0 ? colormaps : R_GetTranslationColormap(TC_DEFAULT, colornum, GTC_CACHE));
			}
			else
			{
				ST_DrawNightsOverlayNum(160, 160, V_SNAPTOBOTTOM, (stplyr->linkcount-1), nightsnum, colornum);
				V_DrawMappedPatch(168, 160, V_SNAPTOBOTTOM, nightslink,
					colornum == 0 ? colormaps : R_GetTranslationColormap(TC_DEFAULT, colornum, GTC_CACHE));
			}
		}

		// Show remaining link time left in debug
		if (cv_debug & DBG_NIGHTSBASIC)
			V_DrawCenteredString(BASEVIDWIDTH/2, 180, V_SNAPTOBOTTOM, va("End in %d.%02d", stplyr->linktimer/TICRATE, G_TicsToCentiseconds(stplyr->linktimer)));
	}

	// Drill meter
	if (
#ifdef HAVE_BLUA
	LUA_HudEnabled(hud_nightsdrill) &&
#endif
	stplyr->powers[pw_carry] == CR_NIGHTSMODE)
	{
		INT32 locx, locy;
		INT32 dfill;
		UINT8 fillpatch;

		if (splitscreen || nosshack)
		{
			locx = 110;
			locy = 188;
		}
		else
		{
			locx = 16;
			locy = 180;
		}

		// Use which patch?
		if (stplyr->pflags & PF_DRILLING)
			fillpatch = (stplyr->drillmeter & 1) + 1;
		else
			fillpatch = 0;

		if (splitscreen)
		{ // 11-5-14 Replaced the old hack with a slightly better hack. -Red
			V_DrawScaledPatch(locx, STRINGY(locy)-3, SPLITFLAGS(V_SNAPTOBOTTOM)|V_HUDTRANS, drillbar);
			for (dfill = 0; dfill < stplyr->drillmeter/20 && dfill < 96; ++dfill)
				V_DrawScaledPatch(locx + 2 + dfill, STRINGY(locy + 3), SPLITFLAGS(V_SNAPTOBOTTOM)|V_HUDTRANS, drillfill[fillpatch]);
		}
		else if (nosshack)
		{ // Even dirtier hack-of-a-hack to draw seperate drill meters in splitscreen special stages but nothing else.
			splitscreen = true;
			V_DrawScaledPatch(locx, STRINGY(locy)-3, V_HUDTRANS, drillbar);
			for (dfill = 0; dfill < stplyr->drillmeter/20 && dfill < 96; ++dfill)
				V_DrawScaledPatch(locx + 2 + dfill, STRINGY(locy + 3), V_HUDTRANS, drillfill[fillpatch]);
			stplyr = &players[secondarydisplayplayer];
			if (stplyr->pflags & PF_DRILLING)
				fillpatch = (stplyr->drillmeter & 1) + 1;
			else
				fillpatch = 0;
			V_DrawScaledPatch(locx, STRINGY(locy-3), V_SNAPTOBOTTOM|V_HUDTRANS, drillbar);
			for (dfill = 0; dfill < stplyr->drillmeter/20 && dfill < 96; ++dfill)
				V_DrawScaledPatch(locx + 2 + dfill, STRINGY(locy + 3), V_SNAPTOBOTTOM|V_HUDTRANS, drillfill[fillpatch]);
			stplyr = &players[displayplayer];
			splitscreen = false;
		}
		else
		{ // Draw normally. <:3
			V_DrawScaledPatch(locx, locy, V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_HUDTRANS, drillbar);
			for (dfill = 0; dfill < stplyr->drillmeter/20 && dfill < 96; ++dfill)
				V_DrawScaledPatch(locx + 2 + dfill, locy + 3, V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_HUDTRANS, drillfill[fillpatch]);
		}

		// Display actual drill amount and bumper time
		if (cv_debug & DBG_NIGHTSBASIC)
		{
			if (stplyr->bumpertime)
				V_DrawString(SCX(locx), SCY(locy - 8), V_NOSCALESTART|V_REDMAP|V_MONOSPACE, va("BUMPER: 0.%02d", G_TicsToCentiseconds(stplyr->bumpertime)));
			else
				V_DrawString(SCX(locx), SCY(locy - 8), V_NOSCALESTART|V_MONOSPACE, va("Drill: %3d%%", (stplyr->drillmeter*100)/(96*20)));
		}
	}

	if (gametype == GT_RACE || gametype == GT_COMPETITION)
	{
		ST_drawScore();
		ST_drawTime();
		return;
	}

	// Begin drawing brackets/chip display
#ifdef HAVE_BLUA
	if (LUA_HudEnabled(hud_nightsrings))
	{
#endif
	ST_DrawOverlayPatch(SCX(16), SCY(8), nbracket);
	if (G_IsSpecialStage(gamemap))
		ST_DrawOverlayPatch(SCX(24), SCY(8) + SCZ(8), nsshud);
	else
		ST_DrawOverlayPatch(SCX(24), SCY(8) + SCZ(8), nhud[(leveltime/2)%12]);

	if (G_IsSpecialStage(gamemap))
	{
		INT32 i;
		total_ringcount = 0;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] /*&& players[i].powers[pw_carry] == CR_NIGHTSMODE*/ && players[i].rings)
				total_ringcount += players[i].rings;
	}
	else
		total_ringcount = stplyr->rings;

	if (stplyr->capsule)
	{
		INT32 amount;
		const INT32 length = 88;

		origamount = stplyr->capsule->spawnpoint->angle;
		I_Assert(origamount > 0); // should not happen now

		ST_DrawOverlayPatch(SCX(72), SCY(8), nbracket);
		ST_DrawOverlayPatch(SCX(74), SCY(8) + SCZ(4), minicaps);

		if (stplyr->capsule->reactiontime != 0)
		{
			INT32 r;
			const INT32 orblength = 20;

			for (r = 0; r < 5; r++)
			{
				ST_DrawOverlayPatch(SCX(230 - (7*r)), SCY(144), redstat);
				ST_DrawOverlayPatch(SCX(188 - (7*r)), SCY(144), orngstat);
				ST_DrawOverlayPatch(SCX(146 - (7*r)), SCY(144), yelstat);
				ST_DrawOverlayPatch(SCX(104 - (7*r)), SCY(144), byelstat);
			}

			amount = (origamount - stplyr->capsule->health);
			amount = (amount * orblength)/origamount;

			if (amount > 0)
			{
				INT32 t;

				// Fill up the bar with blue orbs... in reverse! (yuck)
				for (r = amount; r > 0; r--)
				{
					t = r;

					if (r > 15) ++t;
					if (r > 10) ++t;
					if (r > 5)  ++t;

					ST_DrawOverlayPatch(SCX(69 + (7*t)), SCY(144), bluestat);
				}
			}
		}
		else
		{
			INT32 cfill;

			// Lil' white box!
			V_DrawScaledPatch(15, STRINGY(8) + 34, V_SNAPTOLEFT|V_SNAPTOTOP|V_HUDTRANS, capsulebar);

			amount = (origamount - stplyr->capsule->health);
			amount = (amount * length)/origamount;

			for (cfill = 0; cfill < amount && cfill < 88; ++cfill)
				V_DrawScaledPatch(15 + cfill + 1, STRINGY(8) + 35, V_SNAPTOLEFT|V_SNAPTOTOP|V_HUDTRANS, capsulefill);
		}

		if (total_ringcount >= stplyr->capsule->health)
			ST_DrawOverlayPatch(SCX(40), SCY(8) + SCZ(5), nredar[leveltime%8]);
		else
			ST_DrawOverlayPatch(SCX(40), SCY(8) + SCZ(5), narrow[(leveltime/2)%8]);
	}
	else
		ST_DrawOverlayPatch(SCX(40), SCY(8) + SCZ(5), narrow[8]);

	if (total_ringcount >= 100)
		ST_DrawOverlayNum((total_ringcount >= 1000) ? SCX(76) : SCX(72), SCY(8) + SCZ(11), total_ringcount);
	else
		ST_DrawOverlayNum(SCX(68), SCY(8) + SCZ(11), total_ringcount);
#ifdef HAVE_BLUA
	}
#endif

	// Score
	if (!stplyr->exiting
#ifdef HAVE_BLUA
	&& LUA_HudEnabled(hud_nightsscore)
#endif
	)
	{
		ST_DrawNightsOverlayNum(304, STRINGY(16), SPLITFLAGS(V_SNAPTOTOP)|V_SNAPTORIGHT, stplyr->marescore, nightsnum, SKINCOLOR_AZURE);
	}

	if (!stplyr->exiting
#ifdef HAVE_BLUA
	// TODO give this its own section for Lua
	&& LUA_HudEnabled(hud_nightsscore)
#endif
	)
	{
		if (modeattacking == ATTACKING_NIGHTS)
		{
			INT32 maretime = max(stplyr->realtime - stplyr->marebegunat, 0);
			fixed_t cornerx = vid.width, cornery = vid.height-SCZ(20);

#define ASSISHHUDFIX(n) (n*vid.dupx)
			ST_DrawOverlayPatch(cornerx-ASSISHHUDFIX(22), cornery, W_CachePatchName("NGRTIMER", PU_HUDGFX));
			ST_DrawPaddedOverlayNum(cornerx-ASSISHHUDFIX(22), cornery, G_TicsToCentiseconds(maretime), 2);
			ST_DrawOverlayPatch(cornerx-ASSISHHUDFIX(46), cornery, sboperiod);
			if (maretime < 60*TICRATE)
				ST_DrawOverlayNum(cornerx-ASSISHHUDFIX(46), cornery, G_TicsToSeconds(maretime));
			else
			{
				ST_DrawPaddedOverlayNum(cornerx-ASSISHHUDFIX(46), cornery, G_TicsToSeconds(maretime), 2);
				ST_DrawOverlayPatch(cornerx-ASSISHHUDFIX(70), cornery, sbocolon);
				ST_DrawOverlayNum(cornerx-ASSISHHUDFIX(70), cornery, G_TicsToMinutes(maretime, true));
			}
		}
#undef ASSISHHUDFIX
	}

	// Ideya time remaining
	if (!stplyr->exiting && stplyr->nightstime > 0
#ifdef HAVE_BLUA
	&& LUA_HudEnabled(hud_nightstime)
#endif
	)
	{
		INT32 realnightstime = stplyr->nightstime/TICRATE;
		INT32 numbersize;

		if (G_IsSpecialStage(gamemap))
		{
			tic_t lowest_time = stplyr->nightstime;
			INT32 i;
			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE && players[i].nightstime < lowest_time)
					lowest_time = players[i].nightstime;
			realnightstime = lowest_time/TICRATE;
		}

		if (stplyr->powers[pw_flashing] > TICRATE) // was hit
		{
			UINT16 flashingLeft = stplyr->powers[pw_flashing]-(TICRATE);
			if (flashingLeft < TICRATE/2) // Start fading out
			{
				UINT32 fadingFlag = (9 - 9*flashingLeft/(TICRATE/2)) << V_ALPHASHIFT;
				V_DrawTranslucentPatch(SCX(160 - (minus5sec->width/2)), SCY(28), V_NOSCALESTART|fadingFlag, minus5sec);
			}
			else
				V_DrawScaledPatch(SCX(160 - (minus5sec->width/2)), SCY(28), V_NOSCALESTART, minus5sec);
		}

		if (realnightstime < 10)
			numbersize = 16/2;
		else if (realnightstime < 100)
			numbersize = 32/2;
		else
			numbersize = 48/2;

		if (realnightstime < 10)
			ST_DrawNightsOverlayNum(160 + numbersize, STRINGY(12), SPLITFLAGS(V_SNAPTOTOP), realnightstime,
				nightsnum, SKINCOLOR_RED);
		else
			ST_DrawNightsOverlayNum(160 + numbersize, STRINGY(12), SPLITFLAGS(V_SNAPTOTOP), realnightstime,
				nightsnum, SKINCOLOR_SUPERGOLD4);

		// Show exact time in debug
		if (cv_debug & DBG_NIGHTSBASIC)
			V_DrawString(160 + numbersize + 8, 24, V_SNAPTOTOP|((realnightstime < 10) ? V_REDMAP : V_YELLOWMAP), va("%02d", G_TicsToCentiseconds(stplyr->nightstime)));
	}

	// Show pickup durations
	if (cv_debug & DBG_NIGHTSBASIC)
	{
		UINT16 pwr;

		if (stplyr->powers[pw_nights_superloop])
		{
			pwr = stplyr->powers[pw_nights_superloop];
			V_DrawSmallScaledPatch(SCX(110), SCY(44), V_NOSCALESTART, W_CachePatchName("NPRUA0",PU_CACHE));
			V_DrawThinString(SCX(106), SCY(52), V_NOSCALESTART|V_MONOSPACE, va("%2d.%02d", pwr/TICRATE, G_TicsToCentiseconds(pwr)));
		}

		if (stplyr->powers[pw_nights_helper])
		{
			pwr = stplyr->powers[pw_nights_helper];
			V_DrawSmallScaledPatch(SCX(150), SCY(44), V_NOSCALESTART, W_CachePatchName("NPRUC0",PU_CACHE));
			V_DrawThinString(SCX(146), SCY(52), V_NOSCALESTART|V_MONOSPACE, va("%2d.%02d", pwr/TICRATE, G_TicsToCentiseconds(pwr)));
		}

		if (stplyr->powers[pw_nights_linkfreeze])
		{
			pwr = stplyr->powers[pw_nights_linkfreeze];
			V_DrawSmallScaledPatch(SCX(190), SCY(44), V_NOSCALESTART, W_CachePatchName("NPRUE0",PU_CACHE));
			V_DrawThinString(SCX(186), SCY(52), V_NOSCALESTART|V_MONOSPACE, va("%2d.%02d", pwr/TICRATE, G_TicsToCentiseconds(pwr)));
		}
	}

	// Records/extra text
#ifdef HAVE_BLUA
	if (LUA_HudEnabled(hud_nightsrecords))
#endif
	ST_drawNightsRecords();

	if (nosshack)
		splitscreen = true;
}

static void ST_drawWeaponRing(powertype_t weapon, INT32 rwflag, INT32 wepflag, INT32 xoffs, patch_t *pat)
{
	INT32 txtflags = 0, patflags = 0;

	if (stplyr->powers[weapon])
	{
		if (stplyr->powers[weapon] >= rw_maximums[wepflag])
			txtflags |= V_YELLOWMAP;

		if (weapon == pw_infinityring
		|| (stplyr->ringweapons & rwflag))
			txtflags |= V_20TRANS;
		else
		{
			txtflags |= V_TRANSLUCENT;
			patflags =  V_80TRANS;
		}

		V_DrawScaledPatch(8 + xoffs, STRINGY(162), V_SNAPTOLEFT|patflags, pat);

		if (stplyr->powers[weapon] > 99)
			V_DrawThinString(8 + xoffs + 1, STRINGY(162), V_SNAPTOLEFT|txtflags, va("%d", stplyr->powers[weapon]));
		else
			V_DrawString(8 + xoffs, STRINGY(162), V_SNAPTOLEFT|txtflags, va("%d", stplyr->powers[weapon]));

		if (stplyr->currentweapon == wepflag)
			V_DrawScaledPatch(6 + xoffs, STRINGY(162 - (splitscreen ? 4 : 2)), V_SNAPTOLEFT, curweapon);
	}
	else if (stplyr->ringweapons & rwflag)
		V_DrawScaledPatch(8 + xoffs, STRINGY(162), V_SNAPTOLEFT|V_TRANSLUCENT, pat);
}

static void ST_drawMatchHUD(void)
{
	INT32 offset = (BASEVIDWIDTH / 2) - (NUM_WEAPONS * 10);

	if (!G_RingSlingerGametype())
		return;

	if (G_TagGametype() && !(stplyr->pflags & PF_TAGIT))
		return;

#ifdef HAVE_BLUA
	if (LUA_HudEnabled(hud_weaponrings)) {
#endif

	if (stplyr->powers[pw_infinityring])
		ST_drawWeaponRing(pw_infinityring, 0, 0, offset, infinityring);
	else if (stplyr->rings > 0)
		V_DrawScaledPatch(8 + offset, STRINGY(162), V_SNAPTOLEFT, normring);
	else
		V_DrawTranslucentPatch(8 + offset, STRINGY(162), V_SNAPTOLEFT|V_80TRANS, normring);

	if (!stplyr->currentweapon)
		V_DrawScaledPatch(6 + offset, STRINGY(162 - (splitscreen ? 4 : 2)), V_SNAPTOLEFT, curweapon);

	offset += 20;
	ST_drawWeaponRing(pw_automaticring, RW_AUTO, WEP_AUTO, offset, autoring);
	offset += 20;
	ST_drawWeaponRing(pw_bouncering, RW_BOUNCE, WEP_BOUNCE, offset, bouncering);
	offset += 20;
	ST_drawWeaponRing(pw_scatterring, RW_SCATTER, WEP_SCATTER, offset, scatterring);
	offset += 20;
	ST_drawWeaponRing(pw_grenadering, RW_GRENADE, WEP_GRENADE, offset, grenadering);
	offset += 20;
	ST_drawWeaponRing(pw_explosionring, RW_EXPLODE, WEP_EXPLODE, offset, explosionring);
	offset += 20;
	ST_drawWeaponRing(pw_railring, RW_RAIL, WEP_RAIL, offset, railring);

#ifdef HAVE_BLUA
	}

	if (LUA_HudEnabled(hud_powerstones)) {
#endif

	// Power Stones collected
	offset = 136; // Used for Y now

	if (stplyr->powers[pw_emeralds] & EMERALD1)
		V_DrawScaledPatch(28, STRINGY(offset), V_SNAPTOLEFT, tinyemeraldpics[0]);

	offset += 8;

	if (stplyr->powers[pw_emeralds] & EMERALD2)
		V_DrawScaledPatch(40, STRINGY(offset), V_SNAPTOLEFT, tinyemeraldpics[1]);

	if (stplyr->powers[pw_emeralds] & EMERALD6)
		V_DrawScaledPatch(16, STRINGY(offset), V_SNAPTOLEFT, tinyemeraldpics[5]);

	offset += 16;

	if (stplyr->powers[pw_emeralds] & EMERALD3)
		V_DrawScaledPatch(40, STRINGY(offset), V_SNAPTOLEFT, tinyemeraldpics[2]);

	if (stplyr->powers[pw_emeralds] & EMERALD5)
		V_DrawScaledPatch(16, STRINGY(offset), V_SNAPTOLEFT, tinyemeraldpics[4]);

	offset += 8;

	if (stplyr->powers[pw_emeralds] & EMERALD4)
		V_DrawScaledPatch(28, STRINGY(offset), V_SNAPTOLEFT, tinyemeraldpics[3]);

	offset -= 16;

	if (stplyr->powers[pw_emeralds] & EMERALD7)
		V_DrawScaledPatch(28, STRINGY(offset), V_SNAPTOLEFT, tinyemeraldpics[6]);

#ifdef HAVE_BLUA
	}
#endif
}

static inline void ST_drawRaceHUD(void)
{
	if (leveltime >= TICRATE && leveltime < 5*TICRATE)
	{
		INT32 height = ((3*BASEVIDHEIGHT)>>2) - 8;
		INT32 bounce = (leveltime % TICRATE);
		patch_t *racenum;
		switch (leveltime/TICRATE)
		{
			case 1:
				racenum = race3;
				break;
			case 2:
				racenum = race2;
				break;
			case 3:
				racenum = race1;
				break;
			default:
				racenum = racego;
				break;
		}
		if (bounce < 3)
		{
			height -= (2 - bounce);
			if (!bounce)
					S_StartSound(0, ((racenum == racego) ? sfx_s3kad : sfx_s3ka7));
		}
		V_DrawScaledPatch(SCX((BASEVIDWIDTH - SHORT(racenum->width))/2), (INT32)(SCY(height)), V_NOSCALESTART, racenum);
	}

	if (circuitmap)
	{
		if (stplyr->exiting)
			V_DrawString(hudinfo[HUD_LAP].x, STRINGY(hudinfo[HUD_LAP].y), V_YELLOWMAP, "FINISHED!");
		else
			V_DrawString(hudinfo[HUD_LAP].x, STRINGY(hudinfo[HUD_LAP].y), 0, va("Lap: %u/%d", stplyr->laps+1, cv_numlaps.value));
	}
}

static void ST_drawTagHUD(void)
{
	char pstime[33] = "";
	char pstext[33] = "";

	// Figure out what we're going to print.
	if (leveltime < hidetime * TICRATE) //during the hide time, the seeker and hiders have different messages on their HUD.
	{
		if (hidetime)
			sprintf(pstime, "%d", (hidetime - leveltime/TICRATE)); //hide time is in seconds, not tics.

		if (stplyr->pflags & PF_TAGIT && !stplyr->spectator)
			sprintf(pstext, "%s", M_GetText("WAITING FOR PLAYERS TO HIDE..."));
		else
		{
			if (!stplyr->spectator) //spectators get a generic HUD message rather than a gametype specific one.
			{
				if (gametype == GT_HIDEANDSEEK) //hide and seek.
					sprintf(pstext, "%s", M_GetText("HIDE BEFORE TIME RUNS OUT!"));
				else //default
					sprintf(pstext, "%s", M_GetText("FLEE BEFORE YOU ARE HUNTED!"));
			}
			else
				sprintf(pstext, "%s", M_GetText("HIDE TIME REMAINING:"));
		}
	}
	else
	{
		if (cv_timelimit.value && timelimitintics >= leveltime)
			sprintf(pstime, "%d", (timelimitintics-leveltime)/TICRATE);

		if (stplyr->pflags & PF_TAGIT)
			sprintf(pstext, "%s", M_GetText("YOU'RE IT!"));
		else
		{
			if (cv_timelimit.value)
				sprintf(pstext, "%s", M_GetText("TIME REMAINING:"));
			else //Since having no hud message in tag is not characteristic:
				sprintf(pstext, "%s", M_GetText("NO TIME LIMIT"));
		}
	}

	// Print the stuff.
	if (pstext[0])
	{
		if (splitscreen)
			V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(168), 0, pstext);
		else
			V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(184), 0, pstext);
	}
	if (pstime[0])
	{
		if (splitscreen)
			V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(184), 0, pstime);
		else
			V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(192), 0, pstime);
	}
}

static void ST_drawCTFHUD(void)
{
	INT32 i;
	UINT16 whichflag = 0;

	// Draw the flags
	V_DrawSmallScaledPatch(256, (splitscreen) ? STRINGY(160) : STRINGY(176), V_HUDTRANS, rflagico);
	V_DrawSmallScaledPatch(288, (splitscreen) ? STRINGY(160) : STRINGY(176), V_HUDTRANS, bflagico);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[i].gotflag & GF_REDFLAG) // Red flag isn't at base
			V_DrawScaledPatch(256, (splitscreen) ? STRINGY(156) : STRINGY(174), V_HUDTRANS, nonicon);
		else if (players[i].gotflag & GF_BLUEFLAG) // Blue flag isn't at base
			V_DrawScaledPatch(288, (splitscreen) ? STRINGY(156) : STRINGY(174), V_HUDTRANS, nonicon);

		whichflag |= players[i].gotflag;
		if ((whichflag & (GF_REDFLAG|GF_BLUEFLAG)) == (GF_REDFLAG|GF_BLUEFLAG))
			break; // both flags were found, let's stop early
	}

	// YOU have a flag. Display a monitor-like icon for it.
	if (stplyr->gotflag)
	{
		patch_t *p = (stplyr->gotflag & GF_REDFLAG) ? gotrflag : gotbflag;

		if (splitscreen)
			V_DrawSmallScaledPatch(312, STRINGY(24) + 42, V_SNAPTORIGHT|V_SNAPTOTOP|V_HUDTRANS, p);
		else
			V_DrawScaledPatch(304, 24 + 84, V_SNAPTORIGHT|V_SNAPTOTOP|V_HUDTRANS, p);
	}

	// Display a countdown timer showing how much time left until the flag your team dropped returns to base.
	{
		char timeleft[33];
		if (redflag && redflag->fuse > 1)
		{
			sprintf(timeleft, "%u", (redflag->fuse / TICRATE));
			V_DrawCenteredString(268, STRINGY(184), V_YELLOWMAP|V_HUDTRANS, timeleft);
		}

		if (blueflag && blueflag->fuse > 1)
		{
			sprintf(timeleft, "%u", (blueflag->fuse / TICRATE));
			V_DrawCenteredString(300, STRINGY(184), V_YELLOWMAP|V_HUDTRANS, timeleft);
		}
	}
}

// Draws "Red Team", "Blue Team", or "Spectator" for team gametypes.
static inline void ST_drawTeamName(void)
{
	if (stplyr->ctfteam == 1)
		V_DrawString(256, (splitscreen) ? STRINGY(184) : STRINGY(192), V_HUDTRANSHALF, "RED TEAM");
	else if (stplyr->ctfteam == 2)
		V_DrawString(248, (splitscreen) ? STRINGY(184) : STRINGY(192), V_HUDTRANSHALF, "BLUE TEAM");
	else
		V_DrawString(244, (splitscreen) ? STRINGY(184) : STRINGY(192), V_HUDTRANSHALF, "SPECTATOR");
}

static void ST_drawSpecialStageHUD(void)
{
	if (totalrings > 0)
		ST_DrawNumFromHudWS(HUD_SS_TOTALRINGS, totalrings, V_HUDTRANS);

	if (leveltime < 5*TICRATE && totalrings > 0)
	{
		ST_DrawPatchFromHud(HUD_GETRINGS, getall, V_HUDTRANS);
		ST_DrawNumFromHud(HUD_GETRINGSNUM, totalrings, V_HUDTRANS);
	}

	if (sstimer)
	{
		V_DrawString(hudinfo[HUD_TIMELEFT].x, STRINGY(hudinfo[HUD_TIMELEFT].y), V_HUDTRANS, M_GetText("TIME LEFT"));
		ST_DrawNumFromHud(HUD_TIMELEFTNUM, sstimer/TICRATE, V_HUDTRANS);
	}
	else
		ST_DrawPatchFromHud(HUD_TIMEUP, timeup, V_HUDTRANS);
}

static INT32 ST_drawEmeraldHuntIcon(mobj_t *hunt, patch_t **patches, INT32 offset)
{
	INT32 interval, i;
	UINT32 dist = ((UINT32)P_AproxDistance(P_AproxDistance(stplyr->mo->x - hunt->x, stplyr->mo->y - hunt->y), stplyr->mo->z - hunt->z))>>FRACBITS;

	if (dist < 128)
	{
		i = 5;
		interval = 5;
	}
	else if (dist < 512)
	{
		i = 4;
		interval = 10;
	}
	else if (dist < 1024)
	{
		i = 3;
		interval = 20;
	}
	else if (dist < 2048)
	{
		i = 2;
		interval = 30;
	}
	else if (dist < 3072)
	{
		i = 1;
		interval = 35;
	}
	else
	{
		i = 0;
		interval = 0;
	}

	V_DrawScaledPatch(hudinfo[HUD_HUNTPICS].x+offset, STRINGY(hudinfo[HUD_HUNTPICS].y), V_HUDTRANS, patches[i]);
	return interval;
}

// Separated a few things to stop the SOUND EFFECTS BLARING UGH SHUT UP AAAA
static void ST_doHuntIconsAndSound(void)
{
	INT32 interval = 0, newinterval = 0;

	if (hunt1 && hunt1->health)
		interval = ST_drawEmeraldHuntIcon(hunt1, hunthoming, -20);

	if (hunt2 && hunt2->health)
	{
		newinterval = ST_drawEmeraldHuntIcon(hunt2, hunthoming, 0);
		if (newinterval && (!interval || newinterval < interval))
			interval = newinterval;
	}

	if (hunt3 && hunt3->health)
	{
		newinterval = ST_drawEmeraldHuntIcon(hunt3, hunthoming, 20);
		if (newinterval && (!interval || newinterval < interval))
			interval = newinterval;
	}

	if (!(P_AutoPause() || paused) && interval > 0 && leveltime && leveltime % interval == 0)
		S_StartSound(NULL, sfx_emfind);
}

static void ST_doItemFinderIconsAndSound(void)
{
	INT32 emblems[16];
	thinker_t *th;
	mobj_t *mo2;

	UINT8 stemblems = 0, stunfound = 0;
	INT32 i;
	INT32 interval = 0, newinterval = 0;
	INT32 soffset;

	for (i = 0; i < numemblems; ++i)
	{
		if (emblemlocations[i].type > ET_SKIN || emblemlocations[i].level != gamemap)
			continue;

		emblems[stemblems++] = i;

		if (!emblemlocations[i].collected)
			++stunfound;

		if (stemblems >= 16)
			break;
	}
	// Found all/none exist? Don't waste our time
	if (!stunfound)
		return;

	// Scan thinkers to find emblem mobj with these ids
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;
		mo2 = (mobj_t *)th;

		if (mo2->type == MT_EMBLEM)
		{
			if (!(mo2->flags & MF_SPECIAL))
				continue;

			for (i = 0; i < stemblems; ++i)
			{
				if (mo2->health == emblems[i]+1)
				{
					soffset = (i * 20) - ((stemblems-1) * 10);

					newinterval = ST_drawEmeraldHuntIcon(mo2, itemhoming, soffset);
					if (newinterval && (!interval || newinterval < interval))
						interval = newinterval;

					break;
				}
			}
		}
	}

	if (!(P_AutoPause() || paused) && interval > 0 && leveltime && leveltime % interval == 0)
		S_StartSound(NULL, sfx_emfind);
}

// Draw the status bar overlay, customisable: the user chooses which
// kind of information to overlay
//
static void ST_overlayDrawer(void)
{
	//hu_showscores = auto hide score/time/rings when tab rankings are shown
	if (!(hu_showscores && (netgame || multiplayer)))
	{
		if (maptol & TOL_NIGHTS)
			ST_drawNiGHTSHUD();
		else
		{
#ifdef HAVE_BLUA
			if (LUA_HudEnabled(hud_score))
#endif
			ST_drawScore();
#ifdef HAVE_BLUA
			if (LUA_HudEnabled(hud_time))
#endif
			ST_drawTime();
#ifdef HAVE_BLUA
			if (LUA_HudEnabled(hud_rings))
#endif
			ST_drawRings();
			if (G_GametypeUsesLives()
#ifdef HAVE_BLUA
			&& LUA_HudEnabled(hud_lives)
#endif
			)
				ST_drawLives();
		}
	}

	// GAME OVER pic
	if ((gametype == GT_COOP)
		&& (netgame || multiplayer)
		&& (cv_cooplives.value == 0))
	;
	else if (G_GametypeUsesLives() && stplyr->lives <= 0 && !(hu_showscores && (netgame || multiplayer)))
	{
		patch_t *p;

		if (countdown == 1)
			p = timeover;
		else
			p = sboover;

		if ((gametype == GT_COOP)
		&& (netgame || multiplayer)
		&& (cv_cooplives.value != 1))
		{
			INT32 i;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;

				if (&players[i] == stplyr)
					continue;

				if (players[i].lives > 0)
				{
					p = NULL;
					break;
				}
			}
		}

		if (p)
			V_DrawScaledPatch((BASEVIDWIDTH - SHORT(p->width))/2, STRINGY(BASEVIDHEIGHT/2 - (SHORT(p->height)/2)) - (splitscreen ? 4 : 0), (stplyr->spectator ? V_HUDTRANSHALF : V_HUDTRANS), p);
	}


	if (!hu_showscores) // hide the following if TAB is held
	{
		// Countdown timer for Race Mode
		if (countdown)
			V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(176), 0, va("%d", countdown/TICRATE));

		// If you are in overtime, put a big honkin' flashin' message on the screen.
		if (G_RingSlingerGametype() && cv_overtime.value
			&& (leveltime > (timelimitintics + TICRATE/2)) && cv_timelimit.value && (leveltime/TICRATE % 2 == 0))
		{
			if (splitscreen)
				V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(168), 0, M_GetText("OVERTIME!"));
			else
				V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(184), 0, M_GetText("OVERTIME!"));
		}

		// Draw Match-related stuff
		//\note Match HUD is drawn no matter what gametype.
		// ... just not if you're a spectator.
		if (!stplyr->spectator)
			ST_drawMatchHUD();

		// Race HUD Stuff
		if (gametype == GT_RACE || gametype == GT_COMPETITION)
			ST_drawRaceHUD();
		// Tag HUD Stuff
		else if (gametype == GT_TAG || gametype == GT_HIDEANDSEEK)
			ST_drawTagHUD();
		// CTF HUD Stuff
		else if (gametype == GT_CTF)
			ST_drawCTFHUD();

		// Team names for team gametypes
		if (G_GametypeHasTeams())
			ST_drawTeamName();

		// Special Stage HUD
		if (!useNightsSS && G_IsSpecialStage(gamemap) && stplyr == &players[displayplayer])
			ST_drawSpecialStageHUD();

		// Emerald Hunt Indicators
		if (cv_itemfinder.value && M_SecretUnlocked(SECRET_ITEMFINDER))
			ST_doItemFinderIconsAndSound();
		else
			ST_doHuntIconsAndSound();

		if (stplyr->powers[pw_gravityboots] > 3*TICRATE || (stplyr->powers[pw_gravityboots] && leveltime & 1))
			V_DrawScaledPatch(hudinfo[HUD_GRAVBOOTSICO].x, STRINGY(hudinfo[HUD_GRAVBOOTSICO].y), V_SNAPTORIGHT, gravboots);

		if(!P_IsLocalPlayer(stplyr))
		{
			char name[MAXPLAYERNAME+1];
			// shorten the name if its more than twelve characters.
			strlcpy(name, player_names[stplyr-players], 13);

			// Show name of player being displayed
			V_DrawCenteredString((BASEVIDWIDTH/6), BASEVIDHEIGHT-80, 0, M_GetText("Viewpoint:"));
			V_DrawCenteredString((BASEVIDWIDTH/6), BASEVIDHEIGHT-64, V_ALLOWLOWERCASE, name);
		}

		// This is where we draw all the fun cheese if you have the chasecam off!
		if ((stplyr == &players[displayplayer] && !camera.chase)
			|| ((splitscreen && stplyr == &players[secondarydisplayplayer]) && !camera2.chase))
		{
			ST_drawFirstPersonHUD();
		}
	}

#ifdef HAVE_BLUA
	if (!(netgame || multiplayer) || !hu_showscores)
		LUAh_GameHUD(stplyr);
#endif

	// draw level title Tails
	if (*mapheaderinfo[gamemap-1]->lvlttl != '\0' && !(hu_showscores && (netgame || multiplayer))
#ifdef HAVE_BLUA
	&& LUA_HudEnabled(hud_stagetitle)
#endif
	)
		ST_drawLevelTitle();

	if (!hu_showscores && (netgame || multiplayer) && displayplayer == consoleplayer)
	{
		if (!stplyr->spectator && stplyr->exiting && cv_playersforexit.value && gametype == GT_COOP)
		{
			INT32 i, total = 0, exiting = 0;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator)
					continue;
				if (players[i].lives <= 0)
					continue;

				total++;
				if (players[i].exiting)
					exiting++;
			}

			if (cv_playersforexit.value != 4)
			{
				total *= cv_playersforexit.value;
				if (total % 4) total += 4; // round up
				total /= 4;
			}

			if (exiting >= total)
				;
			else
			{
				total -= exiting;
				V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(124), 0, va(M_GetText("%d more player%s required to exit."), total, ((total == 1) ? "" : "s")));
				if (!splitscreen)
					V_DrawCenteredString(BASEVIDWIDTH/2, 132, 0, M_GetText("Press F12 to watch another player."));
			}
		}
		else if (!splitscreen && gametype != GT_COOP && (stplyr->exiting || (G_GametypeUsesLives() && stplyr->lives <= 0 && countdown != 1)))
			V_DrawCenteredString(BASEVIDWIDTH/2, 132, 0, M_GetText("Press F12 to watch another player."));
		else if (gametype == GT_HIDEANDSEEK &&
		 (!stplyr->spectator && !(stplyr->pflags & PF_TAGIT)) && (leveltime > hidetime * TICRATE))
		{
			V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(116), 0, M_GetText("You cannot move while hiding."));
			if (!splitscreen)
				V_DrawCenteredString(BASEVIDWIDTH/2, 132, 0, M_GetText("Press F12 to watch another player."));
		}
		else if (!G_PlatformGametype() && stplyr->playerstate == PST_DEAD && stplyr->lives) //Death overrides spectator text.
		{
			INT32 respawntime = cv_respawntime.value - stplyr->deadtimer/TICRATE;
			if (respawntime > 0 && !stplyr->spectator)
				V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(132), V_HUDTRANSHALF, va(M_GetText("Respawn in: %d second%s."), respawntime, respawntime == 1 ? "" : "s"));
			else
				V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(132), V_HUDTRANSHALF, M_GetText("Press Jump to respawn."));
		}
		else if (stplyr->spectator && (gametype != GT_COOP || stplyr->playerstate == PST_LIVE)
#ifdef HAVE_BLUA
		&& LUA_HudEnabled(hud_textspectator)
#endif
		)
		{
			V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(60), V_HUDTRANSHALF, M_GetText("You are a spectator."));
			if (G_GametypeHasTeams())
				V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(132), V_HUDTRANSHALF, M_GetText("Press Fire to be assigned to a team."));
			else if (G_IsSpecialStage(gamemap) && useNightsSS)
				V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(132), V_HUDTRANSHALF, M_GetText("You cannot play until the stage has ended."));
			else if (gametype == GT_COOP && stplyr->lives <= 0)
			{
				if (cv_cooplives.value == 1
				&& (netgame || multiplayer))
				{
					INT32 i;
					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i])
							continue;

						if (&players[i] == stplyr)
							continue;

						if (players[i].lives > 1)
							break;
					}

					if (i != MAXPLAYERS)
						V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(132)-(splitscreen ? 12 : 0), V_HUDTRANSHALF, M_GetText("You'll steal a life on respawn."));
				}
			}
			else if (gametype != GT_COOP)
				V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(132), V_HUDTRANSHALF, M_GetText("Press Fire to enter the game."));
			if (!splitscreen)
			{
				V_DrawCenteredString(BASEVIDWIDTH/2, 148, V_HUDTRANSHALF, M_GetText("Press F12 to watch another player."));
				V_DrawCenteredString(BASEVIDWIDTH/2, 164, V_HUDTRANSHALF, M_GetText("Press Jump to float and Spin to sink."));
			}
			else
				V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(136), V_HUDTRANSHALF, M_GetText("Press Jump to float and Spin to sink."));
		}
	}

	if (modeattacking)
		ST_drawInput();

	ST_drawDebugInfo();
}

void ST_Drawer(void)
{
#ifdef SEENAMES
	if (cv_seenames.value && cv_allowseenames.value && displayplayer == consoleplayer && seenplayer && seenplayer->mo)
	{
		if (cv_seenames.value == 1)
			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2 + 15, V_HUDTRANSHALF, player_names[seenplayer-players]);
		else if (cv_seenames.value == 2)
			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2 + 15, V_HUDTRANSHALF,
			va("%s%s", G_GametypeHasTeams() ? ((seenplayer->ctfteam == 1) ? "\x85" : "\x84") : "", player_names[seenplayer-players]));
		else //if (cv_seenames.value == 3)
			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2 + 15, V_HUDTRANSHALF,
			va("%s%s", !G_RingSlingerGametype() || (G_GametypeHasTeams() && players[consoleplayer].ctfteam == seenplayer->ctfteam)
			 ? "\x83" : "\x85", player_names[seenplayer-players]));
	}
#endif

	// Doom's status bar only updated if necessary.
	// However, ours updates every frame regardless, so the "refresh" param was removed
	//(void)refresh;

	// force a set of the palette by using doPaletteStuff()
	if (vid.recalc)
		st_palette = -1;

	// Do red-/gold-shifts from damage/items
#ifdef HWRENDER
	//25/08/99: Hurdler: palette changes is done for all players,
	//                   not only player1! That's why this part
	//                   of code is moved somewhere else.
	if (rendermode == render_soft)
#endif
		if (rendermode != render_none) ST_doPaletteStuff();

	if (st_overlay)
	{
		// No deadview!
		stplyr = &players[displayplayer];
		ST_overlayDrawer();

		if (splitscreen)
		{
			stplyr = &players[secondarydisplayplayer];
			ST_overlayDrawer();
		}
	}
}

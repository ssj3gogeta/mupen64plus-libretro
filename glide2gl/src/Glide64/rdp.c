/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

#include <math.h>
#include "Gfx_1.3.h"
#include "m64p.h"
#include "3dmath.h"
#include "Util.h"
#include "Combine.h"
#include "TexCache.h"
#include "TexBuffer.h"
#include "FBtoScreen.h"
#include "CRC.h"

#ifdef __LIBRETRO__ // Prefix API
#define VIDEO_TAG(X) glide64##X

#define ReadScreen2 VIDEO_TAG(ReadScreen2)
#define PluginStartup VIDEO_TAG(PluginStartup)
#define PluginShutdown VIDEO_TAG(PluginShutdown)
#define PluginGetVersion VIDEO_TAG(PluginGetVersion)
#define CaptureScreen VIDEO_TAG(CaptureScreen)
#define ChangeWindow VIDEO_TAG(ChangeWindow)
#define CloseDLL VIDEO_TAG(CloseDLL)
#define DllTest VIDEO_TAG(DllTest)
#define DrawScreen VIDEO_TAG(DrawScreen)
#define GetDllInfo VIDEO_TAG(GetDllInfo)
#define InitiateGFX VIDEO_TAG(InitiateGFX)
#define MoveScreen VIDEO_TAG(MoveScreen)
#define RomClosed VIDEO_TAG(RomClosed)
#define RomOpen VIDEO_TAG(RomOpen)
#define ShowCFB VIDEO_TAG(ShowCFB)
#define SetRenderingCallback VIDEO_TAG(SetRenderingCallback)
#define UpdateScreen VIDEO_TAG(UpdateScreen)
#define ViStatusChanged VIDEO_TAG(ViStatusChanged)
#define ViWidthChanged VIDEO_TAG(ViWidthChanged)
#define ReadScreen VIDEO_TAG(ReadScreen)
#define FBGetFrameBufferInfo VIDEO_TAG(FBGetFrameBufferInfo)
#define FBRead VIDEO_TAG(FBRead)
#define FBWrite VIDEO_TAG(FBWrite)
#define ProcessDList VIDEO_TAG(ProcessDList)
#define ProcessRDPList VIDEO_TAG(ProcessRDPList)
#define ResizeVideoOutput VIDEO_TAG(ResizeVideoOutput)
#endif

const char *ACmp[] = { "NONE", "THRESHOLD", "UNKNOWN", "DITHER" };

const char *Mode0[] = { "COMBINED",    "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "1",        "NOISE",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0" };

const char *Mode1[] = { "COMBINED",    "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "CENTER",     "K4",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0" };

const char *Mode2[] = { "COMBINED",    "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "SCALE",      "COMBINED_ALPHA",
            "T0_ALPHA",     "T1_ALPHA",
            "PRIM_ALPHA",   "SHADE_ALPHA",
            "ENV_ALPHA",    "LOD_FRACTION",
            "PRIM_LODFRAC",   "K5",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0" };

const char *Mode3[] = { "COMBINED",    "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "1",        "0" };

const char *Alpha0[] = { "COMBINED",   "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "1",        "0" };

#define Alpha1 Alpha0
const char *Alpha2[] = { "LOD_FRACTION", "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "PRIM_LODFRAC",   "0" };
#define Alpha3 Alpha0

const char *FBLa[] = { "G_BL_CLR_IN", "G_BL_CLR_MEM", "G_BL_CLR_BL", "G_BL_CLR_FOG" };
const char *FBLb[] = { "G_BL_A_IN", "G_BL_A_FOG", "G_BL_A_SHADE", "G_BL_0" };
const char *FBLc[] = { "G_BL_CLR_IN", "G_BL_CLR_MEM", "G_BL_CLR_BL", "G_BL_CLR_FOG"};
const char *FBLd[] = { "G_BL_1MA", "G_BL_A_MEM", "G_BL_1", "G_BL_0" };

const char *str_zs[] = { "G_ZS_PIXEL", "G_ZS_PRIM" };

const char *str_yn[] = { "NO", "YES" };
const char *str_offon[] = { "OFF", "ON" };

const char *str_cull[] = { "DISABLE", "FRONT", "BACK", "BOTH" };

// I=intensity probably
const char *str_format[] = { "RGBA", "YUV", "CI", "IA", "I", "?", "?", "?" };
const char *str_size[]   = { "4bit", "8bit", "16bit", "32bit" };
const char *str_cm[]     = { "WRAP/NO CLAMP", "MIRROR/NO CLAMP", "WRAP/CLAMP", "MIRROR/CLAMP" };
const char *str_lod[]    = { "1", "2", "4", "8", "16", "32", "64", "128", "256", "512", "1024", "2048" };
const char *str_aspect[] = { "1x8", "1x4", "1x2", "1x1", "2x1", "4x1", "8x1" };

const char *str_filter[] = { "Point Sampled", "Average (box)", "Bilinear" };

const char *str_tlut[]   = { "TT_NONE", "TT_UNKNOWN", "TT_RGBA_16", "TT_IA_16" };

const char *str_dither[] = { "Pattern", "~Pattern", "Noise", "None" };

const char *CIStatus[]   = { "ci_main", "ci_zimg", "ci_unknown",  "ci_useless",
                            "ci_old_copy", "ci_copy", "ci_copy_self",
                            "ci_zcopy", "ci_aux", "ci_aux_copy" };

//static variables

char out_buf[2048];

uint32_t frame_count;  // frame counter

int ucode_error_report = true;
int wrong_tile = -1;

// ** RDP graphics functions **
static void undef(void);

static void spnoop(void);

static void rdp_noop(void);
static void rdp_texrect(void);
//static void rdp_texrectflip(void);
static void rdp_loadsync(void);
static void rdp_pipesync(void);
static void rdp_tilesync(void);
static void rdp_fullsync(void);
static void rdp_setkeygb(void);
static void rdp_setkeyr(void);
static void rdp_setconvert(void);
static void rdp_setscissor(void);
static void rdp_setprimdepth(void);
static void rdp_setothermode(void);
static void rdp_loadtlut(void);
static void rdp_settilesize(void);
static void rdp_loadblock(void);
static void rdp_loadtile(void);
static void rdp_settile(void);
static void rdp_fillrect(void);
static void rdp_setfillcolor(void);
static void rdp_setfogcolor(void);
static void rdp_setblendcolor(void);
static void rdp_setprimcolor(void);
static void rdp_setenvcolor(void);
static void rdp_setcombine(void);
static void rdp_settextureimage(void);
static void rdp_setdepthimage(void);
static void rdp_setcolorimage(void);
static void rdp_trifill(void);
static void rdp_trishade(void);
static void rdp_tritxtr(void);
static void rdp_trishadetxtr(void);
static void rdp_trifillz(void);
static void rdp_trishadez(void);
static void rdp_tritxtrz(void);
static void rdp_trishadetxtrz(void);
static void rdphalf_1(void);
static void rdphalf_2(void);
static void rdphalf_cont(void);

static void rsp_reserved0(void);
static void rsp_reserved1(void);
static void rsp_reserved2(void);
static void rsp_reserved3(void);

static void ys_memrect(void);

uint8_t microcode[4096];
uint32_t uc_crc;
void microcheck ();

// ** UCODE FUNCTIONS **
#include "ucode00.h"
#include "ucode01.h"
#include "ucode02.h"
#include "ucode03.h"
#include "ucode04.h"
#include "ucode05.h"
#include "ucode06.h"
#include "ucode07.h"
#include "ucode08.h"
#include "ucode09.h"
#include "ucode.h"
#include "ucode09rdp.h"
#include "turbo3D.h"

static int reset = 0;
static int old_ucode = -1;

void rdp_new(void)
{
   int i;
   rdp.vtx1 = (VERTEX*)malloc(256 * sizeof(VERTEX));
   memset(rdp.vtx1, 0, 256 * sizeof(VERTEX));
   rdp.vtx2 = (VERTEX*)malloc(256 * sizeof(VERTEX));
   memset(rdp.vtx2, 0, 256 * sizeof(VERTEX));
   rdp.vtxbuf = rdp.vtxbuf2 = 0;
   rdp.vtx_buffer = rdp.n_global = 0;

   for (i = 0; i < MAX_TMU; i++)
   {
      rdp.cache[i] = (CACHE_LUT*)malloc(MAX_CACHE * sizeof(CACHE_LUT));
      rdp.cur_cache[i]   = 0;
      rdp.cur_cache_n[i] = 0;
   };

   rdp.vtx = (VERTEX*)malloc(MAX_VTX * sizeof(VERTEX));
   memset(rdp.vtx, 0, sizeof(VERTEX)*MAX_VTX);
   rdp.v0 = rdp.vn = 0;

   rdp.frame_buffers = (COLOR_IMAGE*)malloc((NUMTEXBUF+2) * sizeof(COLOR_IMAGE));
}

void rdp_free(void)
{
   int i;
   if (rdp.vtx1)
      free(rdp.vtx1);
   if (rdp.vtx2)
      free(rdp.vtx2);

   for (i = 0; i < MAX_TMU; i++)
   {
      if (rdp.cache[i])
         free(rdp.cache[i]);
   }

   if (rdp.vtx)
      free(rdp.vtx);
   if (rdp.frame_buffers)
      free(rdp.frame_buffers);
}

void rdp_reset(void)
{
   int i;
   reset = 1;

   // set all vertex numbers
   for (i = 0; i < MAX_VTX; i++)
      rdp.vtx[i].number = i;

   rdp.scissor_o.ul_x = 0;
   rdp.scissor_o.ul_y = 0;
   rdp.scissor_o.lr_x = 320;
   rdp.scissor_o.lr_y = 240;

   rdp.vi_org_reg = *gfx.VI_ORIGIN_REG;
   rdp.view_scale[2] = 32.0f * 511.0f;
   rdp.view_trans[2] = 32.0f * 511.0f;
   rdp.clip_ratio = 1.0f;

   rdp.lookat[0][0] = rdp.lookat[1][1] = 1.0f;

   rdp.cycle_mode = 2;
   rdp.allow_combine = 1;
   rdp.update = UPDATE_SCISSOR | UPDATE_COMBINE | UPDATE_ZBUF_ENABLED | UPDATE_CULL_MODE;
   rdp.fog_mode = FOG_MODE_ENABLED;
   rdp.maincimg[0].addr = rdp.maincimg[1].addr = rdp.last_drawn_ci_addr = 0x7FFFFFFF;
}

void microcheck(void)
{
   uint32_t i;
   uc_crc = 0;

   // Check first 3k of ucode, because the last 1k sometimes contains trash
   for (i = 0; i < 3072 >> 2; i++)
      uc_crc += ((uint32_t*)microcode)[i];

   FRDP_E ("crc: %08lx\n", uc_crc);

#ifdef LOG_UCODE
   std::ofstream ucf;
   ucf.open ("ucode.txt", std::ios::out | std::ios::binary);
   char d;
   for (i=0; i<0x400000; i++)
   {
      d = ((char*)gfx.RDRAM)[i^3];
      ucf.write (&d, 1);
   }
   ucf.close ();
#endif

   fprintf(stderr, "ucode = %08lx\n", uc_crc);

   {
      old_ucode = settings.ucode;

      if (
               uc_crc == 0x006bd77f
            || uc_crc == 0x07200895
            || uc_crc == UCODE_GOLDENEYE_007
            || uc_crc == UCODE_DUKE_NUKEM_64
            || uc_crc == UCODE_ROBOTECH_CRYSTAL_DREAMS_PROTO
            || uc_crc == 0xb62f900f
            || uc_crc == 0xbc03e969
            || uc_crc == 0xd5604971
            || uc_crc == 0xd5d68b1f
            || uc_crc == 0xd67c2f8b
            || uc_crc == UCODE_KILLER_INSTINCT_GOLD
            || uc_crc == UCODE_MISCHIEF_MAKERS
            || uc_crc == UCODE_MORTAL_KOMBAT_TRILOGY
            || uc_crc == 0x5182f610
            || uc_crc == UCODE_BLAST_CORPS
            || uc_crc == UCODE_PILOTWINGS_64
            || uc_crc == UCODE_CRUISN_USA
            || uc_crc == UCODE_SUPER_MARIO_64
            || uc_crc == UCODE_TETRISPHERE
            || uc_crc == 0x4165e1fd
            || uc_crc == 0x6e4d50af
         )
      {
         settings.ucode = 0;
         fprintf(stderr, "RSP SW 2.0X (Super Mario 64)\n");
      }
      else if (
               uc_crc == 0x05165579
            || uc_crc == 0x05777c62
            || uc_crc == 0x057e7c62
            || uc_crc == 0x1118b3e0
            || uc_crc == UCODE_MINI_RACERS_CRC1
            || uc_crc == 0x1de712ff
            || uc_crc == 0x24cd885b
            || uc_crc == 0x26a7879a
            || uc_crc == 0xfb816260
            || uc_crc == 0x2c7975d6
            || uc_crc == 0x2d3fe3f1
            || uc_crc == UCODE_FIGHTING_FORCE_64_CRC1
            || uc_crc == 0x339872a6
            || uc_crc == 0x3ff1a4ca
            || uc_crc == 0x4340ac9b
            || uc_crc == 0x440cfad6
            || uc_crc == 0x4fe6df78
            || uc_crc == 0x5257cd2a
            || uc_crc == 0x5414030c
            || uc_crc == 0x5414030d
            || uc_crc == 0x559ff7d4
            || uc_crc == UCODE_YOSHIS_STORY_CRC2
            || uc_crc == 0x5ef4e34a
            || uc_crc == 0x6075e9eb
            || uc_crc == 0x60c1dcc4
            || uc_crc == UCODE_1080_SNOWBOARDING
            || uc_crc == 0x66c0b10a
            || uc_crc == 0x6eaa1da8
            || uc_crc == 0x72a4f34e
            || uc_crc == 0x73999a23
            || uc_crc == 0x7df75834
            || uc_crc == UCODE_DOOM_64
            || uc_crc == UCODE_TUROK_1
            || uc_crc == 0x82f48073
            || uc_crc == UCODE_MINI_RACERS_CRC2
            || uc_crc == 0x841ce10f
            || uc_crc == 0x863e1ca7
            || uc_crc == UCODE_MARIO_KART_64
            || uc_crc == 0x8d5735b2
            || uc_crc == 0x8d5735b3
            || uc_crc == 0x97d1b58a
            || uc_crc == UCODE_QUAKE_64
            || uc_crc == UCODE_WETRIX
            || uc_crc == UCODE_STAR_FOX_64
            || uc_crc == UCODE_FIGHTING_FORCE_64_CRC2
            || uc_crc == 0xb4577b9c
            || uc_crc == 0xbe78677c
            || uc_crc == 0xbed8b069
            || uc_crc == 0xc3704e41
            || uc_crc == UCODE_EXTREME_G
            || uc_crc == 0xc99a4c6c
            || uc_crc == 0xcee7920f
            || uc_crc == 0xd1663234
            || uc_crc == 0xd2a9f59c
            || uc_crc == 0xd41db5f7
            || uc_crc == 0xd57049a5
            || uc_crc == 0xd802ec04
            || uc_crc == UCODE_WIPEOUT_64
            || uc_crc == 0xe9231df2
            || uc_crc == 0xec040469
            || uc_crc == UCODE_DUAL_HEROES
            || uc_crc == UCODE_HEXEN_64
            || uc_crc == UCODE_CHAMELEON_TWIST
            || uc_crc == UCODE_BANJO_KAZOOIE
            || uc_crc == UCODE_MACE_THE_DARK_AGE
            || uc_crc == 0xef54ee35
            )
      {
         settings.ucode = 1;
         fprintf(stderr, "F3DEX 1.XX (Star Fox 64)\n");
      }
      else if (
               uc_crc == 0x03044b84
            || uc_crc == 0x030f4b84
            || uc_crc == 0x0ff79527
            || uc_crc == UCODE_COMMAND_AND_CONQUER
            || uc_crc == 0x1a1e18a0
            || uc_crc == 0x1a1e1920
            || uc_crc == UCODE_DONKEY_KONG_64
            || uc_crc == 0x1a62dc2f
            || uc_crc == UCODE_PAPER_MARIO
            || uc_crc == 0x21f91874
            || uc_crc == UCODE_ZELDA_MAJORAS_MASK
            || uc_crc == UCODE_ZELDA_OOT
            || uc_crc == 0x6124a508
            || uc_crc == 0x630a61fb
            || uc_crc == UCODE_CASTLEVANIA_64
            || uc_crc == UCODE_CASTLEVANIA_64
            || uc_crc == 0x65201a09
            || uc_crc == 0x679e1205
            || uc_crc == 0x6d8f8f8a
            || uc_crc == 0x753be4a5
            || uc_crc == 0xda13ab96
            || uc_crc == 0xe65cb4ad
            || uc_crc == 0xe1290fa2
            || uc_crc == 0xde7d67d4
            || uc_crc == 0x485abff2
            || uc_crc == UCODE_STARCRAFT_64
            || uc_crc == 0x2b291027
            || uc_crc == 0x2f71d1d5
            || uc_crc == 0x2f7dd1d5
            || uc_crc == 0x485abff2
            || uc_crc == 0x93d11f7b
            || uc_crc == 0x93d11ffb
            || uc_crc == 0x93d1ff7b
            || uc_crc == UCODE_FZERO_X
            || uc_crc == 0x955117fb
            || uc_crc == UCODE_BIOHAZARD_2
            || uc_crc == 0xa2d0f88e 
            || uc_crc == 0xaa86cb1d
            || uc_crc == 0xaae4a5b9
            || uc_crc == 0xad0a6292
            || uc_crc == 0xad0a6312
            || uc_crc == 0xb62f900f
            || uc_crc == 0xba65ea1e
            || uc_crc == UCODE_KIRBY_64_CRYSTAL_SHARDS
            || uc_crc == UCODE_SUPER_SMASH_BROS
            || uc_crc == UCODE_MARIO_TENNIS
            || uc_crc == UCODE_MEGA_MAN_64
            || uc_crc == UCODE_RIDGE_RACER_64
            || uc_crc == UCODE_40WINKS
            || uc_crc == 0xcb8c9b6c
            || uc_crc == 0xcfa35a45
            )
      {
         settings.ucode = 2;
         fprintf(stderr, "F3DEX 2.XX (The Legend of Zelda: Ocarina of Time)\n");
      }
      else if (
               uc_crc == UCODE_WAVERACE_64
            )
      {
         settings.ucode = 3;
         fprintf(stderr, "F3DEX ? (WaveRace)\n");
      }
      else if (
               uc_crc == UCODE_STAR_WARS_SHADOW_OF_THE_EMPIRE
            )
      {
         settings.ucode = 4;
         fprintf(stderr, "RSP SW 2.0D EXT (Star Wars: Shadows of the Empire)\n");
      }
      else if (
                  uc_crc == UCODE_JET_FORCE_GEMINI
               || uc_crc == UCODE_MICKEYS_SPEEDWAY_USA
               || uc_crc == UCODE_DIDDY_KONG_RACING
               || uc_crc == 0x63be08b3
            )
      {
         settings.ucode = 5;
         fprintf(stderr, "RSP SW 2.0 (Diddy Kong Racing)\n");
      }
      else if (
                  uc_crc == 0x1ea9e30f
               || uc_crc == 0x74af0a74
               || uc_crc == 0x794c3e28
               || uc_crc == UCODE_BANGAIOH
               || uc_crc == 0x2b5a89c2
               || uc_crc == UCODE_YOSHIS_STORY_CRC1
               || uc_crc == 0xd20dedbf
            )
      {
         settings.ucode = 6;
         fprintf(stderr, "S2DEX 1.XX  (Yoshi's Story - SimCity 2000)\n");
      }
      else if (
               uc_crc == UCODE_PERFECT_DARK
            )
      {
         settings.ucode = 7;
         fprintf(stderr, "RSP SW PD (Perfect Dark)\n");
      }
      else if (
               uc_crc == UCODE_CONKERS_BAD_FUR_DAY
            )
      {
         settings.ucode = 8;
         fprintf(stderr, "F3DEXBG 2.08 (Conker's Bad Fur Day)\n");
      }
      else if (
               uc_crc == 0x0bf36d36
            )
      {
         settings.ucode = 9;
         fprintf(stderr, "(Star Wars: Battle for Naboo)\n");
      }
      else if (
                  uc_crc == 0x1f120bbb
               || uc_crc == 0xf9893f70
               || uc_crc == 0xff372492
            )
      {
         settings.ucode = 21;
      }
      else if (
               uc_crc == 0x0d7bbffb
            || uc_crc == 0x0ff795bf
            || uc_crc == UCODE_STAR_WARS_ROGUE_SQUADRON
            || uc_crc == 0x844b55b5
            || uc_crc == 0x8ec3e124
            || uc_crc == 0xd5c4dc96
            )
      {
         settings.ucode = -1;
         fprintf(stderr, "Unknown Microcode.\n");
      }


      fprintf(stderr, "microcheck: old ucode: %d,  new ucode: %d\n", old_ucode, settings.ucode);
      if (uc_crc == 0x8d5735b2 || uc_crc == 0xb1821ed3 || uc_crc == 0x1118b3e0) //F3DLP.Rej ucode. perspective texture correction is not implemented
      {
         rdp.Persp_en = 1;
         rdp.persp_supported = false;
      }
      else if (settings.texture_correction)
         rdp.persp_supported = true;
   }
}

static uint32_t d_ul_x, d_ul_y, d_lr_x, d_lr_y;

static void DrawPartFrameBufferToScreen(void)
{
   FB_TO_SCREEN_INFO *fb_info = (FB_TO_SCREEN_INFO*)malloc(sizeof(FB_TO_SCREEN_INFO));
   fb_info->addr   = rdp.cimg;
   fb_info->size   = rdp.ci_size;
   fb_info->width  = rdp.ci_width;
   fb_info->height = rdp.ci_height;
   fb_info->ul_x = d_ul_x;
   fb_info->lr_x = d_lr_x;
   fb_info->ul_y = d_ul_y;
   fb_info->lr_y = d_lr_y;
   fb_info->opaque = 0;
   DrawFrameBufferToScreen(fb_info);
   memset(gfx.RDRAM+rdp.cimg, 0, (rdp.ci_width*rdp.ci_height)<<rdp.ci_size>>1);
   free(fb_info);
}

#define RGBA16TO32(color) \
  ((color&1)?0xFF:0) | \
  ((uint32_t)((float)((color&0xF800) >> 11) / 31.0f * 255.0f) << 24) | \
  ((uint32_t)((float)((color&0x07C0) >> 6) / 31.0f * 255.0f) << 16) | \
  ((uint32_t)((float)((color&0x003E) >> 1) / 31.0f * 255.0f) << 8)

static void CopyFrameBuffer (GrBuffer_t buffer)
{
   FRDP ("CopyFrameBuffer: %08lx... ", rdp.cimg);

   // don't bother to write the stuff in asm... the slow part is the read from video card,
   //   not the copy.

   uint32_t width = rdp.ci_width;//*gfx.VI_WIDTH_REG;
   uint32_t height;
   if (fb_emulation_enabled && !(settings.hacks&hack_PPL))
   {
      int ind = (rdp.ci_count > 0)?rdp.ci_count-1:0;
      height = rdp.frame_buffers[ind].height;
   }
   else
   {
      height = rdp.ci_lower_bound;
      if (settings.hacks&hack_PPL)
         height -= rdp.ci_upper_bound;
   }
   FRDP ("width: %d, height: %d...  ", width, height);

   if (rdp.scale_x < 1.1f)
   {
      uint16_t * ptr_src = (uint16_t*)malloc(width * height * sizeof(uint16_t));
      if (grLfbReadRegion(buffer,
               (uint32_t)rdp.offset_x,
               (uint32_t)rdp.offset_y,//rdp.ci_upper_bound,
               width,
               height,
               width<<1,
               ptr_src))
      {
         uint16_t *ptr_dst = (uint16_t*)(gfx.RDRAM+rdp.cimg);
         uint32_t *ptr_dst32 = (uint32_t*)(gfx.RDRAM+rdp.cimg);
         uint16_t c;
         uint32_t y, x;

         for (y = 0; y < height; y++)
         {
            for (x = 0; x < width; x++)
            {
               c = ptr_src[x + y * width];
               if (settings.frame_buffer&fb_read_alpha)
               {
                  if (c > 0)
                     c = (c&0xFFC0) | ((c&0x001F) << 1) | 1;
               }
               else
               {
                  c = (c&0xFFC0) | ((c&0x001F) << 1) | 1;
               }
               if (rdp.ci_size == 2)
                  ptr_dst[(x + y * width)^1] = c;
               else
                  ptr_dst32[x + y * width] = RGBA16TO32(c);
            }
         }
         LRDP("ReadRegion.  Framebuffer copy complete.\n");
      }
      else
      {
         LRDP("Framebuffer copy failed.\n");
      }
      if (ptr_src)
         free(ptr_src);
   }
   else
   {
      if (rdp.motionblur && fb_hwfbe_enabled)
      {
         return;
      }
      else
      {
         float scale_x = (settings.scr_res_x - rdp.offset_x*2.0f)  / max(width, rdp.vi_width);
         float scale_y = (settings.scr_res_y - rdp.offset_y*2.0f) / max(height, rdp.vi_height);

         FRDP("width: %d, height: %d, ul_y: %d, lr_y: %d, scale_x: %f, scale_y: %f, ci_width: %d, ci_height: %d\n",width, height, rdp.ci_upper_bound, rdp.ci_lower_bound, scale_x, scale_y, rdp.ci_width, rdp.ci_height);
         GrLfbInfo_t info;
         info.size = sizeof(GrLfbInfo_t);

         if (grLfbLock (GR_LFB_READ_ONLY,
                  buffer,
                  GR_LFBWRITEMODE_565,
                  GR_ORIGIN_UPPER_LEFT,
                  FXFALSE,
                  &info))
         {
            uint16_t *ptr_src = (uint16_t*)info.lfbPtr;
            uint16_t *ptr_dst = (uint16_t*)(gfx.RDRAM+rdp.cimg);
            uint32_t *ptr_dst32 = (uint32_t*)(gfx.RDRAM+rdp.cimg);
            uint16_t c;
            uint32_t stride = info.strideInBytes>>1;
            int y, x;

            int read_alpha = settings.frame_buffer & fb_read_alpha;
            if ((settings.hacks&hack_PMario) && rdp.frame_buffers[rdp.ci_count-1].status != ci_aux)
               read_alpha = false;
            int x_start = 0, y_start = 0, x_end = width, y_end = height;

            if (settings.hacks&hack_BAR)
               x_start = 80, y_start = 24, x_end = 240, y_end = 86;

            for (y = y_start; y < y_end; y++)
            {
               for (x = x_start; x < x_end; x++)
               {
                  c = ptr_src[(int)(x*scale_x + rdp.offset_x) + (int)(y * scale_y + rdp.offset_y) * stride];
                  c = (c&0xFFC0) | ((c&0x001F) << 1) | 1;
                  if (read_alpha && c == 1)
                     c = 0;
                  if (rdp.ci_size <= 2)
                     ptr_dst[(x + y * width)^1] = c;
                  else
                     ptr_dst32[x + y * width] = RGBA16TO32(c);
               }
            }

            // Unlock the backbuffer
            grLfbUnlock (GR_LFB_READ_ONLY, buffer);
            LRDP("LfbLock.  Framebuffer copy complete.\n");
         }
         else
         {
            LRDP("Framebuffer copy failed.\n");
         }
      }
   }
}

/******************************************************************
Function: ProcessDList
Purpose:  This function is called when there is a Dlist to be
processed. (High level GFX list)
input:    none
output:   none
*******************************************************************/
void DetectFrameBufferUsage(void);
uint32_t fbreads_front = 0;
uint32_t fbreads_back = 0;
int cpu_fb_read_called = false;
int cpu_fb_write_called = false;
int cpu_fb_write = false;
int cpu_fb_ignore = false;
int CI_SET = true;
uint32_t ucode5_texshiftaddr = 0;
uint32_t ucode5_texshiftcount = 0;
uint16_t ucode5_texshift = 0;
int depth_buffer_fog;

EXPORT void CALL ProcessDList(void)
{
  no_dlist = false;
  update_screen_count = 0;
  ChangeSize ();

#ifdef VISUAL_LOGGING
  VLOG ("ProcessDList ()\n");
#endif

  if (reset)
  {
    reset = 0;
    if (settings.autodetect_ucode)
    {
      // Thanks to ZeZu for ucode autodetection!!!
      uint32_t startUcode = *(uint32_t*)(gfx.DMEM+0xFD0);
      memcpy (microcode, gfx.RDRAM+startUcode, 4096);
      microcheck ();
    }
    else
      memset (microcode, 0, 4096);
  }
  else if ( ((old_ucode == ucode_S2DEX) && (settings.ucode == ucode_F3DEX)) || settings.force_microcheck)
  {
    uint32_t startUcode = *(uint32_t*)(gfx.DMEM+0xFD0);
    memcpy (microcode, gfx.RDRAM+startUcode, 4096);
    microcheck ();
  }

  if (exception)
    return;

  // Clear out the RDP log
#ifdef RDP_LOGGING
  if (settings.logging && settings.log_clear)
  {
    CLOSE_RDP_LOG ();
    OPEN_RDP_LOG ();
  }
#endif

#ifdef UNIMP_LOG
  if (settings.log_unk && settings.unk_clear)
  {
    std::ofstream unimp;
    unimp.open("unimp.txt");
    unimp.close();
  }
#endif

  //* Set states *//
  if (settings.swapmode > 0)
    SwapOK = true;
  rdp.updatescreen = 1;

  rdp.tri_n = 0;  // 0 triangles so far this frame
  rdp.debug_n = 0;

  rdp.model_i = 0; // 0 matrices so far in stack
  //stack_size can be less then 32! Important for Silicon Vally. Thanks Orkin!
  rdp.model_stack_size = min(32, (*(uint32_t*)(gfx.DMEM+0x0FE4))>>6);
  if (rdp.model_stack_size == 0)
    rdp.model_stack_size = 32;
  rdp.Persp_en = true;
  rdp.fb_drawn = rdp.fb_drawn_front = false;
  rdp.update = 0x7FFFFFFF;  // All but clear cache
  rdp.geom_mode = 0;
  rdp.acmp = 0;
  rdp.maincimg[1] = rdp.maincimg[0];
  rdp.skip_drawing = false;
  rdp.s2dex_tex_loaded = false;
  rdp.bg_image_height = 0xFFFF;
  fbreads_front = fbreads_back = 0;
  rdp.fog_multiplier = rdp.fog_offset = 0;
  rdp.zsrc = 0;
  if (rdp.vi_org_reg != *gfx.VI_ORIGIN_REG)
    rdp.tlut_mode = 0; //is it correct?
  rdp.scissor_set = false;
  ucode5_texshiftaddr = ucode5_texshiftcount = 0;
  cpu_fb_write = false;
  cpu_fb_read_called = false;
  cpu_fb_write_called = false;
  cpu_fb_ignore = false;
  d_ul_x = 0xffff;
  d_ul_y = 0xffff;
  d_lr_x = 0;
  d_lr_y = 0;
  depth_buffer_fog = true;

  //analize possible frame buffer usage
  if (fb_emulation_enabled)
    DetectFrameBufferUsage();
  if (!(settings.hacks&hack_Lego) || rdp.num_of_ci > 1)
    rdp.last_bg = 0;
  //* End of set states *//

  // Get the start of the display list and the length of it
  uint32_t dlist_start = *(uint32_t*)(gfx.DMEM+0xFF0);
  uint32_t dlist_length = *(uint32_t*)(gfx.DMEM+0xFF4);
  FRDP("--- NEW DLIST --- crc: %08lx, ucode: %d, fbuf: %08lx, fbuf_width: %d, dlist start: %08lx, dlist_length: %d, x_scale: %f, y_scale: %f\n", uc_crc, settings.ucode, *gfx.VI_ORIGIN_REG, *gfx.VI_WIDTH_REG, dlist_start, dlist_length, (*gfx.VI_X_SCALE_REG & 0xFFF)/1024.0f, (*gfx.VI_Y_SCALE_REG & 0xFFF)/1024.0f);
  FRDP_E("--- NEW DLIST --- crc: %08lx, ucode: %d, fbuf: %08lx\n", uc_crc, settings.ucode, *gfx.VI_ORIGIN_REG);

  if (cpu_fb_write == true)
    DrawPartFrameBufferToScreen();
  if ((settings.hacks&hack_Tonic) && dlist_length < 16)
  {
    rdp_fullsync();
    FRDP_E("DLIST is too short!\n");
    return;
  }

  // Start executing at the start of the display list
  rdp.pc_i = 0;
  rdp.pc[rdp.pc_i] = dlist_start;
  rdp.dl_count = -1;
  rdp.halt = 0;
  uint32_t a;

  if (settings.ucode == ucode_Turbo3d)
     Turbo3D();
  else
  {
     // MAIN PROCESSING LOOP
     do {
        // Get the address of the next command
        a = rdp.pc[rdp.pc_i] & BMASK;

        // Load the next command and its input
        rdp.cmd0 = ((uint32_t*)gfx.RDRAM)[a>>2];   // \ Current command, 64 bit
        rdp.cmd1 = ((uint32_t*)gfx.RDRAM)[(a>>2)+1]; // /
        // cmd2 and cmd3 are filled only when needed, by the function that needs them

#ifdef LOG_COMMANDS
        // Output the address before the command
        FRDP ("%08lx (c0:%08lx, c1:%08lx): ", a, rdp.cmd0, rdp.cmd1);
#endif

        // Go to the next instruction
        rdp.pc[rdp.pc_i] = (a+8) & BMASK;

        // Process this instruction
        gfx_instruction[settings.ucode][rdp.cmd0>>24] ();

        // check DL counter
        if (rdp.dl_count != -1)
        {
           rdp.dl_count --;
           if (rdp.dl_count == 0)
           {
              rdp.dl_count = -1;

              LRDP("End of DL\n");
              rdp.pc_i --;
           }
        }
     } while (!rdp.halt);
  }

  if (fb_emulation_enabled)
  {
    rdp.scale_x = rdp.scale_x_bak;
    rdp.scale_y = rdp.scale_y_bak;
  }
  if (settings.frame_buffer & fb_ref)
    CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
  if (rdp.cur_image)
    CloseTextureBuffer(rdp.read_whole_frame && ((settings.hacks&hack_PMario) || rdp.swap_ci_index >= 0));

  if ((settings.hacks&hack_TGR2) && rdp.vi_org_reg != *gfx.VI_ORIGIN_REG && CI_SET)
  {
    newSwapBuffers ();
    CI_SET = false;
  }
  LRDP("ProcessDList end\n");
}

// undef - undefined instruction, always ignore
static void undef()
{
#ifdef _ENDUSER_RELEASE_
  *gfx.MI_INTR_REG |= 0x20;
  gfx.CheckInterrupts();
  rdp.halt = 1;
#else
  FRDP_E("** undefined ** (%08lx)\n", rdp.cmd0);
  FRDP("** undefined ** (%08lx) - IGNORED\n", rdp.cmd0);
#endif
}

// spnoop - no operation, always ignore
static void spnoop()
{
  LRDP("spnoop\n");
}

// noop - no operation, always ignore
static void rdp_noop()
{
  LRDP("noop\n");
}

static void ys_memrect ()
{
  uint32_t tile = (uint16_t)((rdp.cmd1 & 0x07000000) >> 24);

  uint32_t lr_x = (uint16_t)((rdp.cmd0 & 0x00FFF000) >> 14);
  uint32_t lr_y = (uint16_t)((rdp.cmd0 & 0x00000FFF) >> 2);
  uint32_t ul_x = (uint16_t)((rdp.cmd1 & 0x00FFF000) >> 14);
  uint32_t ul_y = (uint16_t)((rdp.cmd1 & 0x00000FFF) >> 2);

  if (lr_y > rdp.scissor_o.lr_y)
    lr_y = rdp.scissor_o.lr_y;
  uint32_t off_x = ((rdp.cmd2 & 0xFFFF0000) >> 16) >> 5;
  uint32_t off_y = (rdp.cmd2 & 0x0000FFFF) >> 5;

#ifndef NDEBUG
  FRDP ("memrect (%d, %d, %d, %d), ci_width: %d", ul_x, ul_y, lr_x, lr_y, rdp.ci_width);
  if (off_x > 0)
    FRDP ("  off_x: %d", off_x);
  if (off_y > 0)
    FRDP ("  off_y: %d", off_y);
  LRDP("\n");
#endif

  uint32_t y, width = lr_x - ul_x;
  uint32_t tex_width = rdp.tiles[tile].line << 3;
  uint8_t * texaddr = gfx.RDRAM + rdp.addr[rdp.tiles[tile].t_mem] + tex_width*off_y + off_x;
  uint8_t * fbaddr = gfx.RDRAM + rdp.cimg + ul_x;

  for (y = ul_y; y < lr_y; y++) {
    uint8_t *src = texaddr + (y - ul_y) * tex_width;
    uint8_t *dst = fbaddr + y * rdp.ci_width;
    memcpy (dst, src, width);
  }
}

static void pm_palette_mod ()
{
   int i;
   uint8_t envr = (uint8_t)((float)((rdp.env_color >> 24)&0xFF)/255.0f*31.0f);
   uint8_t envg = (uint8_t)((float)((rdp.env_color >> 16)&0xFF)/255.0f*31.0f);
   uint8_t envb = (uint8_t)((float)((rdp.env_color >> 8)&0xFF)/255.0f*31.0f);
   uint16_t env16 = (uint16_t)((envr<<11)|(envg<<6)|(envb<<1)|1);
   uint8_t prmr = (uint8_t)((float)((rdp.prim_color >> 24)&0xFF)/255.0f*31.0f);
   uint8_t prmg = (uint8_t)((float)((rdp.prim_color >> 16)&0xFF)/255.0f*31.0f);
   uint8_t prmb = (uint8_t)((float)((rdp.prim_color >> 8)&0xFF)/255.0f*31.0f);
   uint16_t prim16 = (uint16_t)((prmr<<11)|(prmg<<6)|(prmb<<1)|1);
   uint16_t * dst = (uint16_t*)(gfx.RDRAM+rdp.cimg);
   for (i = 0; i < 16; i++)
      dst[i^1] = (rdp.pal_8[i]&1) ? prim16 : env16;

   LRDP("Texrect palette modification\n");
}

static void pd_zcopy ()
{
   int x;
   uint16_t ul_x = (uint16_t)((rdp.cmd1 & 0x00FFF000) >> 14);
   uint16_t lr_x = (uint16_t)((rdp.cmd0 & 0x00FFF000) >> 14) + 1;
   uint16_t ul_u = (uint16_t)((rdp.cmd2 & 0xFFFF0000) >> 21) + 1;
   uint16_t *ptr_dst = (uint16_t*)(gfx.RDRAM+rdp.cimg);
   uint16_t width = lr_x - ul_x;
   uint16_t * ptr_src = ((uint16_t*)rdp.tmem)+ul_u;
   uint16_t c;
   for (x = 0; x < width; x++)
   {
      c = ptr_src[x];
      c = ((c<<8)&0xFF00) | (c >> 8);
      ptr_dst[(ul_x+x)^1] = c;
      //      FRDP("dst[%d]=%04lx \n", (x + ul_x)^1, c);
   }
}

static void DrawDepthBufferFog(void)
{
  if (rdp.zi_width < 200)
    return;

  FB_TO_SCREEN_INFO *fb_info = (FB_TO_SCREEN_INFO*)malloc(sizeof(FB_TO_SCREEN_INFO));
  fb_info->addr   = rdp.zimg;
  fb_info->size   = 2;
  fb_info->width  = rdp.zi_width;
  fb_info->height = rdp.ci_height;
  fb_info->ul_x = rdp.scissor_o.ul_x;
  fb_info->lr_x = rdp.scissor_o.lr_x;
  fb_info->ul_y = rdp.scissor_o.ul_y;
  fb_info->lr_y = rdp.scissor_o.lr_y;
  fb_info->opaque = 0;

  DrawDepthBufferToScreen(fb_info);
  free(fb_info);
}

static void rdp_texrect()
{
  if (!rdp.LLE)
  {
    uint32_t a = rdp.pc[rdp.pc_i];
    uint8_t cmdHalf1 = gfx.RDRAM[a+3];
    uint8_t cmdHalf2 = gfx.RDRAM[a+11];
    a >>= 2;
    if ((cmdHalf1 == 0xE1 && cmdHalf2 == 0xF1) || (cmdHalf1 == 0xB4 && cmdHalf2 == 0xB3) || (cmdHalf1 == 0xB3 && cmdHalf2 == 0xB2))
    {
      //gSPTextureRectangle
      rdp.cmd2 = ((uint32_t*)gfx.RDRAM)[a+1];
      rdp.cmd3 = ((uint32_t*)gfx.RDRAM)[a+3];
      rdp.pc[rdp.pc_i] += 16;
    }
    else
    {
      //gDPTextureRectangle
      if (settings.hacks&hack_ASB)
        rdp.cmd2 = 0;
      else
        rdp.cmd2 = ((uint32_t*)gfx.RDRAM)[a+0];
      rdp.cmd3 = ((uint32_t*)gfx.RDRAM)[a+1];
      rdp.pc[rdp.pc_i] += 8;
    }
  }
  if ((settings.hacks&hack_Yoshi) && settings.ucode == ucode_S2DEX)
  {
    ys_memrect();
    return;
  }

  if (rdp.skip_drawing || (!fb_emulation_enabled && (rdp.cimg == rdp.zimg)))
  {
    if ((settings.hacks&hack_PMario) && rdp.ci_status == ci_useless)
    {
      pm_palette_mod ();
    }
    else
    {
      LRDP("Texrect skipped\n");
    }
    return;
  }

  if ((settings.ucode == ucode_CBFD) && rdp.cur_image && rdp.cur_image->format)
  {
    //FRDP("Wrong Texrect. texaddr: %08lx, cimg: %08lx, cimg_end: %08lx\n", rdp.timg.addr, rdp.maincimg[1].addr, rdp.maincimg[1].addr+rdp.ci_width*rdp.ci_height*rdp.ci_size);
    LRDP("Shadow texrect is skipped.\n");
    rdp.tri_n += 2;
    return;
  }

  if ((settings.ucode == ucode_PerfectDark) && (rdp.frame_buffers[rdp.ci_count-1].status == ci_zcopy))
  {
    pd_zcopy ();
    LRDP("Depth buffer copied.\n");
    rdp.tri_n += 2;
    return;
  }

  if ((rdp.othermode_l >> 16) == 0x3c18 && rdp.cycle1 == 0x03ffffff && rdp.cycle2 == 0x01ff1fff) //depth image based fog
  {
    if (!depth_buffer_fog)
      return;
    if (settings.fog)
      DrawDepthBufferFog();
    depth_buffer_fog = false;
    return;
  }

  //  FRDP ("rdp.cycle1 %08lx, rdp.cycle2 %08lx\n", rdp.cycle1, rdp.cycle2);

  float ul_x, ul_y, lr_x, lr_y;
  if (rdp.cycle_mode == 2)
  {
    ul_x = max(0.0f, (short)((rdp.cmd1 & 0x00FFF000) >> 14));
    ul_y = max(0.0f, (short)((rdp.cmd1 & 0x00000FFF) >> 2));
    lr_x = max(0.0f, (short)((rdp.cmd0 & 0x00FFF000) >> 14));
    lr_y = max(0.0f, (short)((rdp.cmd0 & 0x00000FFF) >> 2));
  }
  else
  {
    ul_x = max(0.0f, ((short)((rdp.cmd1 & 0x00FFF000) >> 12)) / 4.0f);
    ul_y = max(0.0f, ((short)(rdp.cmd1 & 0x00000FFF)) / 4.0f);
    lr_x = max(0.0f, ((short)((rdp.cmd0 & 0x00FFF000) >> 12)) / 4.0f);
    lr_y = max(0.0f, ((short)(rdp.cmd0 & 0x00000FFF)) / 4.0f);
  }

  if (ul_x >= lr_x)
  {
    FRDP("Wrong Texrect: ul_x: %f, lr_x: %f\n", ul_x, lr_x);
    return;
  }

  if (rdp.cycle_mode > 1)
  {
    lr_x += 1.0f;
    lr_y += 1.0f;
  } else if (lr_y - ul_y < 1.0f)
    lr_y = ceil(lr_y);

  if (settings.increase_texrect_edge)
  {
    if (floor(lr_x) != lr_x)
      lr_x = ceil(lr_x);
    if (floor(lr_y) != lr_y)
      lr_y = ceil(lr_y);
  }

  //*
#ifdef HAVE_HWFBE
  if (rdp.tbuff_tex && (settings.frame_buffer & fb_optimize_texrect))
  {
    LRDP("Attempt to optimize texrect\n");
    if (!rdp.tbuff_tex->drawn)
    {
      DRAWIMAGE *d = (DRAWIMAGE*)malloc(sizeof(DRAWIMAGE));
      d->imageX  = 0;
      d->imageW  = (uint16_t)rdp.tbuff_tex->width;
      d->frameX  = (uint16_t)ul_x;
      d->frameW  = (uint16_t)(rdp.tbuff_tex->width);

      d->imageY  = 0;
      d->imageH  = (uint16_t)rdp.tbuff_tex->height;
      d->frameY  = (uint16_t)ul_y;
      d->frameH  = (uint16_t)(rdp.tbuff_tex->height);
      FRDP("texrect. ul_x: %d, ul_y: %d, lr_x: %d, lr_y: %d, width: %d, height: %d\n", ul_x, ul_y, lr_x, lr_y, rdp.tbuff_tex->width, rdp.tbuff_tex->height);
      d->scaleX  = 1.0f;
      d->scaleY  = 1.0f;
      DrawHiresImage(d, rdp.tbuff_tex->width == rdp.ci_width);
      rdp.tbuff_tex->drawn = true;
      free(d);
    }
    return;
  }
#endif
  //*/
  // framebuffer workaround for Zelda: MM LOT
  if ((rdp.othermode_l & 0xFFFF0000) == 0x0f5a0000)
    return;

  /*Gonetz*/
  //hack for Zelda MM. it removes black texrects which cover all geometry in "Link meets Zelda" cut scene
  if ((settings.hacks&hack_Zelda) && rdp.timg.addr >= rdp.cimg && rdp.timg.addr < rdp.ci_end)
  {
    FRDP("Wrong Texrect. texaddr: %08lx, cimg: %08lx, cimg_end: %08lx\n", rdp.cur_cache[0]->addr, rdp.cimg, rdp.cimg+rdp.ci_width*rdp.ci_height*2);
    rdp.tri_n += 2;
    return;
  }
  //*
  //hack for Banjo2. it removes black texrects under Banjo
  if (!fb_hwfbe_enabled && ((rdp.cycle1 << 16) | (rdp.cycle2 & 0xFFFF)) == 0xFFFFFFFF && (rdp.othermode_l & 0xFFFF0000) == 0x00500000)
  {
    rdp.tri_n += 2;
    return;
  }
  //*/
  //*
  //remove motion blur in night vision
  if ((settings.ucode == ucode_PerfectDark) && (rdp.maincimg[1].addr != rdp.maincimg[0].addr) && (rdp.timg.addr >= rdp.maincimg[1].addr) && (rdp.timg.addr < (rdp.maincimg[1].addr+rdp.ci_width*rdp.ci_height*rdp.ci_size)))
  {
    if (fb_emulation_enabled)
      if (rdp.frame_buffers[rdp.ci_count-1].status == ci_copy_self)
      {
        //FRDP("Wrong Texrect. texaddr: %08lx, cimg: %08lx, cimg_end: %08lx\n", rdp.timg.addr, rdp.maincimg[1], rdp.maincimg[1]+rdp.ci_width*rdp.ci_height*rdp.ci_size);
        LRDP("Wrong Texrect.\n");
        rdp.tri_n += 2;
        return;
      }
  }
  //*/

  int i;

  uint32_t tile = (uint16_t)((rdp.cmd1 & 0x07000000) >> 24);

  rdp.texrecting = 1;

  uint32_t prev_tile = rdp.cur_tile;
  rdp.cur_tile = tile;

  const float Z = set_sprite_combine_mode ();

  rdp.texrecting = 0;

  if (!rdp.cur_cache[0])
  {
    rdp.cur_tile = prev_tile;
    rdp.tri_n += 2;
    return;
  }
  // ****
  // ** Texrect offset by Gugaman **
  //
  //integer representation of texture coordinate.
  //needed to detect and avoid overflow after shifting
  int32_t off_x_i = (rdp.cmd2 >> 16) & 0xFFFF;
  int32_t off_y_i = rdp.cmd2 & 0xFFFF;
  float dsdx = (float)((short)((rdp.cmd3 & 0xFFFF0000) >> 16)) / 1024.0f;
  float dtdy = (float)((short)(rdp.cmd3 & 0x0000FFFF)) / 1024.0f;
  if (off_x_i & 0x8000) //check for sign bit
    off_x_i |= ~0xffff; //make it negative
  //the same as for off_x_i
  if (off_y_i & 0x8000)
    off_y_i |= ~0xffff;

  if (rdp.cycle_mode == 2)
    dsdx /= 4.0f;

  float s_ul_x = ul_x * rdp.scale_x + rdp.offset_x;
  float s_lr_x = lr_x * rdp.scale_x + rdp.offset_x;
  float s_ul_y = ul_y * rdp.scale_y + rdp.offset_y;
  float s_lr_y = lr_y * rdp.scale_y + rdp.offset_y;

  FRDP("texrect (%.2f, %.2f, %.2f, %.2f), tile: %d, #%d, #%d\n", ul_x, ul_y, lr_x, lr_y, tile, rdp.tri_n, rdp.tri_n+1);
  FRDP ("(%f, %f) -> (%f, %f), s: (%d, %d) -> (%d, %d)\n", s_ul_x, s_ul_y, s_lr_x, s_lr_y, rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x, rdp.scissor.lr_y);
  FRDP("\toff_x: %f, off_y: %f, dsdx: %f, dtdy: %f\n", off_x_i/32.0f, off_y_i/32.0f, dsdx, dtdy);

  float off_size_x;
  float off_size_y;

  if ( ((rdp.cmd0>>24)&0xFF) == 0xE5 ) //texrectflip
  {
    {
      off_size_x = (lr_y - ul_y - 1) * dsdx;
      off_size_y = (lr_x - ul_x - 1) * dtdy;
    }
  }
  else
  {
    {
      off_size_x = (lr_x - ul_x - 1) * dsdx;
      off_size_y = (lr_y - ul_y - 1) * dtdy;
    }
  }

  struct {
    float ul_u, ul_v, lr_u, lr_v;
  } texUV[2]; //struct for texture coordinates
  //angrylion's macro, helps to cut overflowed values.
  #define SIGN16(x) (((x) & 0x8000) ? ((x) | ~0xffff) : ((x) & 0xffff))

  //calculate texture coordinates
  for (i = 0; i < 2; i++)
  {
    if (rdp.cur_cache[i] && (rdp.tex & (i+1)))
    {
      float sx = 1, sy = 1;
      int x_i = off_x_i, y_i = off_y_i;
      TILE *tile = &rdp.tiles[rdp.cur_tile + i];
      //shifting
      if (tile->shift_s)
      {
        if (tile->shift_s > 10)
        {
          uint8_t iShift = (16 - tile->shift_s);
          x_i <<= iShift;
          sx = (float)(1 << iShift);
        }
        else
        {
          uint8_t iShift = tile->shift_s;
          x_i >>= iShift;
          sx = 1.0f/(float)(1 << iShift);
        }
      }
      if (tile->shift_t)
      {
        if (tile->shift_t > 10)
        {
          uint8_t iShift = (16 - tile->shift_t);
          y_i <<= iShift;
          sy = (float)(1 << iShift);
        }
        else
        {
          uint8_t iShift = tile->shift_t;
          y_i >>= iShift;
          sy = 1.0f/(float)(1 << iShift);
        }
      }

#ifdef HAVE_HWFBE
      if (rdp.aTBuffTex[i]) //hwfbe texture
      {
        float t0_off_x;
        float t0_off_y;
        if (off_x_i + off_y_i == 0)
        {
          t0_off_x = tile->ul_s;
          t0_off_y = tile->ul_t;
        }
        else
        {
          t0_off_x = off_x_i/32.0f;
          t0_off_y = off_y_i/32.0f;
        }
        t0_off_x += rdp.aTBuffTex[i]->u_shift;// + tile->ul_s; //commented for Paper Mario motion blur
        t0_off_y += rdp.aTBuffTex[i]->v_shift;// + tile->ul_t;
        texUV[i].ul_u = t0_off_x * sx;
        texUV[i].ul_v = t0_off_y * sy;

        texUV[i].lr_u = texUV[i].ul_u + off_size_x * sx;
        texUV[i].lr_v = texUV[i].ul_v + off_size_y * sy;

        texUV[i].ul_u *= rdp.aTBuffTex[i]->u_scale;
        texUV[i].ul_v *= rdp.aTBuffTex[i]->v_scale;
        texUV[i].lr_u *= rdp.aTBuffTex[i]->u_scale;
        texUV[i].lr_v *= rdp.aTBuffTex[i]->v_scale;
        FRDP("tbuff_tex[%d] ul_u: %f, ul_v: %f, lr_u: %f, lr_v: %f\n",
          i, texUV[i].ul_u, texUV[i].ul_v, texUV[i].lr_u, texUV[i].lr_v);
      }
      else //common case
#endif
      {
        //kill 10.5 format overflow by SIGN16 macro
        texUV[i].ul_u = SIGN16(x_i) / 32.0f;
        texUV[i].ul_v = SIGN16(y_i) / 32.0f;

        texUV[i].ul_u -= tile->f_ul_s;
        texUV[i].ul_v -= tile->f_ul_t;

        texUV[i].lr_u = texUV[i].ul_u + off_size_x * sx;
        texUV[i].lr_v = texUV[i].ul_v + off_size_y * sy;

        texUV[i].ul_u = rdp.cur_cache[i]->c_off + rdp.cur_cache[i]->c_scl_x * texUV[i].ul_u;
        texUV[i].lr_u = rdp.cur_cache[i]->c_off + rdp.cur_cache[i]->c_scl_x * texUV[i].lr_u;
        texUV[i].ul_v = rdp.cur_cache[i]->c_off + rdp.cur_cache[i]->c_scl_y * texUV[i].ul_v;
        texUV[i].lr_v = rdp.cur_cache[i]->c_off + rdp.cur_cache[i]->c_scl_y * texUV[i].lr_v;
      }
    }
    else
    {
      texUV[i].ul_u = texUV[i].ul_v = texUV[i].lr_u = texUV[i].lr_v = 0;
    }
  }
  rdp.cur_tile = prev_tile;

  // ****

  FRDP ("  scissor: (%d, %d) -> (%d, %d)\n", rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x, rdp.scissor.lr_y);

  CCLIP2 (s_ul_x, s_lr_x, texUV[0].ul_u, texUV[0].lr_u, texUV[1].ul_u, texUV[1].lr_u, (float)rdp.scissor.ul_x, (float)rdp.scissor.lr_x);
  CCLIP2 (s_ul_y, s_lr_y, texUV[0].ul_v, texUV[0].lr_v, texUV[1].ul_v, texUV[1].lr_v, (float)rdp.scissor.ul_y, (float)rdp.scissor.lr_y);

  FRDP ("  draw at: (%f, %f) -> (%f, %f)\n", s_ul_x, s_ul_y, s_lr_x, s_lr_y);

  VERTEX vstd[4] = {
    { s_ul_x, s_ul_y, Z, 1.0f, texUV[0].ul_u, texUV[0].ul_v, texUV[1].ul_u, texUV[1].ul_v, {0, 0, 0, 0}, 255 },
    { s_lr_x, s_ul_y, Z, 1.0f, texUV[0].lr_u, texUV[0].ul_v, texUV[1].lr_u, texUV[1].ul_v, {0, 0, 0, 0}, 255 },
    { s_ul_x, s_lr_y, Z, 1.0f, texUV[0].ul_u, texUV[0].lr_v, texUV[1].ul_u, texUV[1].lr_v, {0, 0, 0, 0}, 255 },
    { s_lr_x, s_lr_y, Z, 1.0f, texUV[0].lr_u, texUV[0].lr_v, texUV[1].lr_u, texUV[1].lr_v, {0, 0, 0, 0}, 255 } };

    if ( ((rdp.cmd0>>24)&0xFF) == 0xE5 ) //texrectflip
    {
      vstd[1].u0 = texUV[0].ul_u;
      vstd[1].v0 = texUV[0].lr_v;
      vstd[1].u1 = texUV[1].ul_u;
      vstd[1].v1 = texUV[1].lr_v;

      vstd[2].u0 = texUV[0].lr_u;
      vstd[2].v0 = texUV[0].ul_v;
      vstd[2].u1 = texUV[1].lr_u;
      vstd[2].v1 = texUV[1].ul_v;
    }

    VERTEX *vptr = vstd;
    int n_vertices = 4;

    AllowShadeMods (vptr, n_vertices);
    for (i=0; i<n_vertices; i++)
    {
      apply_shade_mods (&vptr[i]);
    }

    {
      if (rdp.fog_mode >= FOG_MODE_BLEND)
      {
        float fog;
        if (rdp.fog_mode == FOG_MODE_BLEND)
          fog = 1.0f/max(1, rdp.fog_color&0xFF);
        else
          fog = 1.0f/max(1, (~rdp.fog_color)&0xFF);
        for (i = 0; i < n_vertices; i++)
        {
          vptr[i].f = fog;
        }
        grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
      }

      ConvertCoordsConvert (vptr, n_vertices);

      grDrawVertexArrayContiguous (GR_TRIANGLE_STRIP, n_vertices, vptr, sizeof(VERTEX));

      rdp.tri_n += 2;
    }
}

static void rdp_loadsync()
{
  LRDP("loadsync - ignored\n");
}

static void rdp_pipesync()
{
  LRDP("pipesync - ignored\n");
}

static void rdp_tilesync()
{
  LRDP("tilesync - ignored\n");
}

static void rdp_fullsync()
{
  // Set an interrupt to allow the game to continue
  *gfx.MI_INTR_REG |= 0x20;
  gfx.CheckInterrupts();
  LRDP("fullsync\n");
}

static void rdp_setkeygb()
{
  uint32_t sB = rdp.cmd1&0xFF;
  uint32_t cB = (rdp.cmd1>>8)&0xFF;
  uint32_t sG = (rdp.cmd1>>16)&0xFF;
  uint32_t cG = (rdp.cmd1>>24)&0xFF;
  rdp.SCALE = (rdp.SCALE&0xFF0000FF) | (sG<<16) | (sB<<8);
  rdp.CENTER = (rdp.CENTER&0xFF0000FF) | (cG<<16) | (cB<<8);
  FRDP("setkeygb. cG=%02lx, sG=%02lx, cB=%02lx, sB=%02lx\n", cG, sG, cB, sB);
}

static void rdp_setkeyr()
{
  uint32_t sR = rdp.cmd1&0xFF;
  uint32_t cR = (rdp.cmd1>>8)&0xFF;
  rdp.SCALE = (rdp.SCALE&0x00FFFFFF) | (sR<<24);
  rdp.CENTER = (rdp.CENTER&0x00FFFFFF) | (cR<<24);
  FRDP("setkeyr. cR=%02lx, sR=%02lx\n", cR, sR);
}

static void rdp_setconvert()
{
  /*
  rdp.YUV_C0 = 1.1647f  ;
  rdp.YUV_C1 = 0.79931f ;
  rdp.YUV_C2 = -0.1964f ;
  rdp.YUV_C3 = -0.40651f;
  rdp.YUV_C4 = 1.014f   ;
  */
  rdp.K4 = (uint8_t)(rdp.cmd1>>9)&0x1FF;
  rdp.K5 = (uint8_t)(rdp.cmd1&0x1FF);
  //  RDP_E("setconvert - IGNORED\n");
  FRDP("setconvert. K4=%02lx K5=%02lx\n", rdp.K4, rdp.K5);
}

//
// setscissor - sets the screen clipping rectangle
//

static void rdp_setscissor()
{
  // clipper resolution is 320x240, scale based on computer resolution
  rdp.scissor_o.ul_x = /*min(*/(uint32_t)(((rdp.cmd0 & 0x00FFF000) >> 14))/*, 320)*/;
  rdp.scissor_o.ul_y = /*min(*/(uint32_t)(((rdp.cmd0 & 0x00000FFF) >> 2))/*, 240)*/;
  rdp.scissor_o.lr_x = /*min(*/(uint32_t)(((rdp.cmd1 & 0x00FFF000) >> 14))/*, 320)*/;
  rdp.scissor_o.lr_y = /*min(*/(uint32_t)(((rdp.cmd1 & 0x00000FFF) >> 2))/*, 240)*/;

  rdp.ci_upper_bound = rdp.scissor_o.ul_y;
  rdp.ci_lower_bound = rdp.scissor_o.lr_y;
  rdp.scissor_set = true;

  FRDP("setscissor: (%d,%d) -> (%d,%d)\n", rdp.scissor_o.ul_x, rdp.scissor_o.ul_y,
    rdp.scissor_o.lr_x, rdp.scissor_o.lr_y);

  rdp.update |= UPDATE_SCISSOR;

  if (rdp.view_scale[0] == 0) //viewport is not set?
  {
    rdp.view_scale[0] = (rdp.scissor_o.lr_x>>1)*rdp.scale_x;
    rdp.view_scale[1] = (rdp.scissor_o.lr_y>>1)*-rdp.scale_y;
    rdp.view_trans[0] = rdp.view_scale[0];
    rdp.view_trans[1] = -rdp.view_scale[1];
    rdp.update |= UPDATE_VIEWPORT;
  }
}

static void rdp_setprimdepth()
{
  rdp.prim_depth = (uint16_t)((rdp.cmd1 >> 16) & 0x7FFF);
  rdp.prim_dz = (uint16_t)(rdp.cmd1 & 0x7FFF);

  FRDP("setprimdepth: %d\n", rdp.prim_depth);
}

static void rdp_setothermode()
{
#define F3DEX2_SETOTHERMODE(cmd,sft,len,data) { \
  rdp.cmd0 = (cmd<<24) | ((32-(sft)-(len))<<8) | (((len)-1)); \
  rdp.cmd1 = data; \
  gfx_instruction[settings.ucode][cmd] (); \
}
#define SETOTHERMODE(cmd,sft,len,data) { \
  rdp.cmd0 = (cmd<<24) | ((sft)<<8) | (len); \
  rdp.cmd1 = data; \
  gfx_instruction[settings.ucode][cmd] (); \
}

  LRDP("rdp_setothermode\n");

  if ((settings.ucode == ucode_F3DEX2) || (settings.ucode == ucode_CBFD))
  {
    int cmd0 = rdp.cmd0;
    F3DEX2_SETOTHERMODE(0xE2, 0, 32, rdp.cmd1);         // SETOTHERMODE_L
    F3DEX2_SETOTHERMODE(0xE3, 0, 32, cmd0 & 0x00FFFFFF);    // SETOTHERMODE_H
  }
  else
  {
    int cmd0 = rdp.cmd0;
    SETOTHERMODE(0xB9, 0, 32, rdp.cmd1);            // SETOTHERMODE_L
    SETOTHERMODE(0xBA, 0, 32, cmd0 & 0x00FFFFFF);       // SETOTHERMODE_H
  }
}

void load_palette (uint32_t addr, uint16_t start, uint16_t count)
{
  LRDP("Loading palette... ");
  uint16_t *dpal = rdp.pal_8 + start;
  uint16_t end = start+count;
  uint16_t i, p;

  for (i=start; i<end; i++)
  {
    *(dpal++) = *(uint16_t *)(gfx.RDRAM + (addr^2));
    addr += 2;

#ifdef TLUT_LOGGING
    FRDP ("%d: %08lx\n", i, *(uint16_t *)(gfx.RDRAM + (addr^2)));
#endif
  }
  start >>= 4;
  end = start + (count >> 4);
  if (end == start) // it can be if count < 16
    end = start + 1;
  for (p = start; p < end; p++)
  {
    rdp.pal_8_crc[p] = CRC32( 0xFFFFFFFF, &rdp.pal_8[(p << 4)], 32 );
  }
  rdp.pal_256_crc = CRC32( 0xFFFFFFFF, rdp.pal_8_crc, 64 );
  LRDP("Done.\n");
}

static void rdp_loadtlut()
{
   int i, j;
   uint32_t tile = (rdp.cmd1 >> 24) & 0x07;
   uint16_t start = rdp.tiles[tile].t_mem - 256; // starting location in the palettes
   //  uint16_t start = ((uint16_t)(rdp.cmd1 >> 2) & 0x3FF) + 1;
   uint16_t count = ((uint16_t)(rdp.cmd1 >> 14) & 0x3FF) + 1;    // number to copy

   if (rdp.timg.addr + (count<<1) > BMASK)
      count = (uint16_t)((BMASK - rdp.timg.addr) >> 1);

   if (start+count > 256) count = 256-start;

   FRDP("loadtlut: tile: %d, start: %d, count: %d, from: %08lx\n", tile, start, count,
         rdp.timg.addr);

   load_palette (rdp.timg.addr, start, count);

   rdp.timg.addr += count << 1;

   if (rdp.tbuff_tex) //paranoid check.
   {
      //the buffer is definitely wrong, as there must be no CI frame buffers
      //find and remove it
      for (i = 0; i < NUM_TMU; i++)
      {
         for (j = 0; j < rdp.texbufs[i].count; j++)
         {
            if (&(rdp.texbufs[i].images[j]) == rdp.tbuff_tex)
            {
               rdp.texbufs[i].count--;
               if (j < rdp.texbufs[i].count)
                  memcpy(&(rdp.texbufs[i].images[j]), &(rdp.texbufs[i].images[j+1]), sizeof(TBUFF_COLOR_IMAGE)*(rdp.texbufs[i].count-j));
               return;
            }
         }
      }
   }
}

int tile_set = 0;
static void rdp_settilesize()
{
  uint32_t tile = (rdp.cmd1 >> 24) & 0x07;
  rdp.last_tile_size = tile;

  rdp.tiles[tile].f_ul_s = (float)((rdp.cmd0 >> 12) & 0xFFF) / 4.0f;
  rdp.tiles[tile].f_ul_t = (float)(rdp.cmd0 & 0xFFF) / 4.0f;

  int ul_s = (((uint16_t)(rdp.cmd0 >> 14)) & 0x03ff);
  int ul_t = (((uint16_t)(rdp.cmd0 >> 2 )) & 0x03ff);
  int lr_s = (((uint16_t)(rdp.cmd1 >> 14)) & 0x03ff);
  int lr_t = (((uint16_t)(rdp.cmd1 >> 2 )) & 0x03ff);

  if (lr_s == 0 && ul_s == 0)  //pokemon puzzle league set such tile size
    wrong_tile = tile;
  else if (wrong_tile == (int)tile)
    wrong_tile = -1;

  // coords in 10.2 format
  rdp.tiles[tile].ul_s = ul_s;
  rdp.tiles[tile].ul_t = ul_t;
  rdp.tiles[tile].lr_s = lr_s;
  rdp.tiles[tile].lr_t = lr_t;

  // handle wrapping
  if (rdp.tiles[tile].lr_s < rdp.tiles[tile].ul_s) rdp.tiles[tile].lr_s += 0x400;
  if (rdp.tiles[tile].lr_t < rdp.tiles[tile].ul_t) rdp.tiles[tile].lr_t += 0x400;

  rdp.update |= UPDATE_TEXTURE;

  rdp.first = 1;

  FRDP ("settilesize: tile: %d, ul_s: %d, ul_t: %d, lr_s: %d, lr_t: %d, f_ul_s: %f, f_ul_t: %f\n",
    tile, ul_s, ul_t, lr_s, lr_t, rdp.tiles[tile].f_ul_s, rdp.tiles[tile].f_ul_t);
}

void setTBufTex(uint16_t t_mem, uint32_t cnt)
{
#ifdef HAVE_HWFBE
   int i;
   FRDP("setTBufTex t_mem=%d, cnt=%d\n", t_mem, cnt);
   TBUFF_COLOR_IMAGE * pTbufTex = rdp.tbuff_tex;
   for (i = 0; i < 2; i++)
   {
      LRDP("Before: ");
      if (rdp.aTBuffTex[i])
      {
         FRDP("rdp.aTBuffTex[%d]: tmu=%d t_mem=%d tile=%d\n", i, rdp.aTBuffTex[i]->tmu, rdp.aTBuffTex[i]->t_mem, rdp.aTBuffTex[i]->tile);
      }
      else
      {
         FRDP("rdp.aTBuffTex[%d]=0\n", i);
      }

      if ((rdp.aTBuffTex[i] == 0 && rdp.aTBuffTex[i^1] != pTbufTex) || (rdp.aTBuffTex[i] && rdp.aTBuffTex[i]->t_mem >= t_mem && rdp.aTBuffTex[i]->t_mem < t_mem + cnt))
      {
         if (pTbufTex)
         {
            rdp.aTBuffTex[i] = pTbufTex;
            rdp.aTBuffTex[i]->t_mem = t_mem;
            pTbufTex = 0;
            FRDP("rdp.aTBuffTex[%d] tmu=%d t_mem=%d\n", i, rdp.aTBuffTex[i]->tmu, rdp.aTBuffTex[i]->t_mem);
         }
         else
         {
            rdp.aTBuffTex[i] = 0;
            FRDP("rdp.aTBuffTex[%d]=0\n", i);
         }
      }
   }
#endif
}

static INLINE void loadBlock(uint32_t *src, uint32_t *dst, uint32_t off, int dxt, int cnt)
{
  uint32_t *v5;
  int v6;
  uint32_t *v7;
  uint32_t v8;
  int v9;
  uint32_t v10;
  uint32_t v13;
  uint32_t v14;
  int v16;
  int v18;
  unsigned int nbits;

  nbits = sizeof(unsigned int) * 8;
  v5 = dst;
  v6 = cnt;
  if ( cnt )
  {
    v7 = (uint32_t *)((char *)src + (off & 0xFFFFFFFC));
    v8 = off & 3;
    if ( !(off & 3) )
      goto LABEL_23;
    v9 = 4 - v8;
    v10 = *v7++;
    do
    {
      v10 = __ROL__(v10, 8, nbits);
    }while (--v8);
    do
    {
      *v5++ = __ROL__(v10, 8, nbits);
    }while (--v9);
    *v5++ = bswap32(*v7++);
    v6 = cnt - 1;
    if ( cnt != 1 )
    {
LABEL_23:
      do
      {
        *v5++ = bswap32(*v7++);
        *v5++ = bswap32(*v7++);
      }while (--v6 );
    }
    v13 = off & 3;
    if ( off & 3 )
    {
      v14 = *(uint32_t *)((char *)src + ((8 * cnt + off) & 0xFFFFFFFC));
      do
      {
        *v5++ = __ROL__(v14, 8, nbits);
      }while (--v13);
    }
  }
  v6 = cnt;
  v16 = 0;
  v18 = 0;
dxt_test:
  do
  {
     dst += 2;
     if ( !--v6 )
        break;
     v16 += dxt;
     if ( v16 < 0 )
     {
        do
        {
           ++v18;
           if ( !--v6 )
              goto end_dxt_test;
           v16 += dxt;
           if ( v16 >= 0 )
           {
              do
              {
                 *dst    ^= dst[1];
                 dst[1] ^= *dst;
                 *dst    ^= dst[1];
                 dst += 2;
              }while(--v18);
              goto dxt_test;
           }
        }while(1);
     }
  }while(1);
end_dxt_test:
  while ( v18 )
  {
     *dst    ^= dst[1];
     dst[1] ^= *dst;
     *dst    ^= dst[1];
     dst += 2;
     --v18;
  }
}

void LoadBlock32b(uint32_t tile, uint32_t ul_s, uint32_t ul_t, uint32_t lr_s, uint32_t dxt);
static void rdp_loadblock()
{
  if (rdp.skip_drawing)
  {
    LRDP("loadblock skipped\n");
    return;
  }
  uint32_t tile = (uint32_t)((rdp.cmd1 >> 24) & 0x07);
  uint32_t dxt = (uint32_t)(rdp.cmd1 & 0x0FFF);
  uint16_t lr_s = (uint16_t)(rdp.cmd1 >> 14) & 0x3FF;
  if (ucode5_texshiftaddr)
  {
    if (ucode5_texshift % ((lr_s+1)<<3))
    {
      rdp.timg.addr -= ucode5_texshift;
      ucode5_texshiftaddr = 0;
      ucode5_texshift = 0;
      ucode5_texshiftcount = 0;
    }
    else
      ucode5_texshiftcount++;
  }

  rdp.addr[rdp.tiles[tile].t_mem] = rdp.timg.addr;

  // ** DXT is used for swapping every other line
  /*  double fdxt = (double)0x8000000F/(double)((uint32_t)(2047/(dxt-1))); // F for error
  uint32_t _dxt = (uint32_t)fdxt;*/

  // 0x00000800 -> 0x80000000 (so we can check the sign bit instead of the 11th bit)
  uint32_t _dxt = dxt << 20;

  uint32_t addr = segoffset(rdp.timg.addr) & BMASK;

  // lr_s specifies number of 64-bit words to copy
  // 10.2 format
  uint16_t ul_s = (uint16_t)(rdp.cmd0 >> 14) & 0x3FF;
  uint16_t ul_t = (uint16_t)(rdp.cmd0 >>  2) & 0x3FF;

  rdp.tiles[tile].ul_s = ul_s;
  rdp.tiles[tile].ul_t = ul_t;
  rdp.tiles[tile].lr_s = lr_s;

  rdp.timg.set_by = 0;  // load block

  // do a quick boundary check before copying to eliminate the possibility for exception
  if (ul_s >= 512) {
    lr_s = 1;   // 1 so that it doesn't die on memcpy
    ul_s = 511;
  }
  if (ul_s+lr_s > 512)
    lr_s = 512-ul_s;

  if (addr+(lr_s<<3) > BMASK+1)
    lr_s = (uint16_t)((BMASK-addr)>>3);

  //angrylion's advice to use ul_s in texture image offset and cnt calculations.
  //Helps to fix Vigilante 8 jpeg backgrounds and logos
  uint32_t off = rdp.timg.addr + (ul_s << rdp.tiles[tile].size >> 1);
  unsigned char *dst = ((unsigned char *)rdp.tmem) + (rdp.tiles[tile].t_mem<<3);
  uint32_t cnt = lr_s-ul_s+1;
  if (rdp.tiles[tile].size == 3)
    cnt <<= 1;

  if (rdp.timg.size == 3)
    LoadBlock32b(tile, ul_s, ul_t, lr_s, dxt);
  else
    loadBlock((uint32_t *)gfx.RDRAM, (uint32_t *)dst, off, _dxt, cnt);

  rdp.timg.addr += cnt << 3;
  rdp.tiles[tile].lr_t = ul_t + ((dxt*cnt)>>11);

  rdp.update |= UPDATE_TEXTURE;

  FRDP ("loadblock: tile: %d, ul_s: %d, ul_t: %d, lr_s: %d, dxt: %08lx -> %08lx\n",
    tile, ul_s, ul_t, lr_s,
    dxt, _dxt);

#ifdef HAVE_HWFBE
  if (fb_hwfbe_enabled)
    setTBufTex(rdp.tiles[tile].t_mem, cnt);
#endif
}


static INLINE void loadTile(uint32_t *src, uint32_t *dst, int width, int height, int line, int off, uint32_t *end)
{
  uint32_t *v7;
  int v8;
  uint32_t *v9;
  int v10;
  int v11;
  int v12;
  uint32_t *v13;
  int v14;
  int v15;
  uint32_t v16;
  uint32_t *v17;
  uint32_t v18;
  int v19;
  uint32_t v20;
  int v21;
  int v23;
  uint32_t *v24;
  int v25;
  int v26;
  uint32_t *v27;
  int v28;
  int v29;
  int v30;
  uint32_t *v31;
  unsigned int nbits;

  nbits = sizeof(unsigned int) * 8;
  v7 = dst;
  v8 = width;
  v9 = src;
  v10 = off;
  v11 = 0;
  v12 = height;
  do
  {
    if ( end < v7 )
      break;
    v31 = v7;
    v30 = v8;
    v29 = v12;
    v28 = v11;
    v27 = v9;
    v26 = v10;
    if ( v8 )
    {
      v25 = v8;
      v24 = v9;
      v23 = v10;
      v13 = (uint32_t *)((char *)v9 + (v10 & 0xFFFFFFFC));
      v14 = v10 & 3;
      if ( !(v10 & 3) )
        goto LABEL_20;
      v15 = 4 - v14;
      v16 = *v13;
      v17 = v13 + 1;
      do
      {
        v16 = __ROL__(v16, 8, nbits);
      }while (--v14 );
      do
      {
        v16 = __ROL__(v16, 8, nbits);
        *(uint8_t *)v7 = v16;
        v7 = (uint32_t *)((char *)v7 + 1);
      }while(--v15 );
      v18 = *v17;
      v13 = v17 + 1;
      *v7++ = bswap32(v18);
      if (--v8)
      {
LABEL_20:
        do
        {
          *v7++ = bswap32(*v13++);
          *v7++ = bswap32(*v13++);
        }while (--v8);
      }
      v19 = v23 & 3;
      if ( v23 & 3 )
      {
        v20 = *(uint32_t *)((char *)v24 + ((8 * v25 + v23) & 0xFFFFFFFC));
        do
        {
          v20 = __ROL__(v20, 8, nbits);
          *(uint8_t *)v7 = v20;
          v7 = (uint32_t *)((char *)v7 + 1);
        }
        while (--v19);
      }
    }
    v9 = v27;
    v21 = v29;
    v8 = v30;
    v11 = v28 ^ 1;
    if ( v28 == 1 )
    {
      v7 = v31;
      if ( v30 )
      {
        do
        {
           *v7    ^= v7[1];
           v7[1] ^= *v7;
           *v7    ^= v7[1];
           v7 += 2;
        }while (--v8);
      }
      v21 = v29;
      v8 = v30;
    }
    v10 = line + v26;
    v12 = v21 - 1;
  }while ( v12 );
}

void LoadTile32b (uint32_t tile, uint32_t ul_s, uint32_t ul_t, uint32_t width, uint32_t height);
static void rdp_loadtile()
{
  if (rdp.skip_drawing)
  {
    LRDP("loadtile skipped\n");
    return;
  }
  rdp.timg.set_by = 1;  // load tile

  uint32_t tile = (uint32_t)((rdp.cmd1 >> 24) & 0x07);

  rdp.addr[rdp.tiles[tile].t_mem] = rdp.timg.addr;

  uint16_t ul_s = (uint16_t)((rdp.cmd0 >> 14) & 0x03FF);
  uint16_t ul_t = (uint16_t)((rdp.cmd0 >> 2 ) & 0x03FF);
  uint16_t lr_s = (uint16_t)((rdp.cmd1 >> 14) & 0x03FF);
  uint16_t lr_t = (uint16_t)((rdp.cmd1 >> 2 ) & 0x03FF);

  if (lr_s < ul_s || lr_t < ul_t) return;

  if (wrong_tile >= 0)  //there was a tile with zero length
  {
    rdp.tiles[wrong_tile].lr_s = lr_s;

    if (rdp.tiles[tile].size > rdp.tiles[wrong_tile].size)
      rdp.tiles[wrong_tile].lr_s <<= (rdp.tiles[tile].size - rdp.tiles[wrong_tile].size);
    else if (rdp.tiles[tile].size < rdp.tiles[wrong_tile].size)
      rdp.tiles[wrong_tile].lr_s >>= (rdp.tiles[wrong_tile].size - rdp.tiles[tile].size);
    rdp.tiles[wrong_tile].lr_t = lr_t;
    rdp.tiles[wrong_tile].mask_s = rdp.tiles[wrong_tile].mask_t = 0;
    //     wrong_tile = -1;
  }

  if (rdp.tbuff_tex)// && (rdp.tiles[tile].format == 0))
  {
    FRDP("loadtile: tbuff_tex ul_s: %d, ul_t:%d\n", ul_s, ul_t);
    rdp.tbuff_tex->tile_uls = ul_s;
    rdp.tbuff_tex->tile_ult = ul_t;
  }

  if ((settings.hacks&hack_Tonic) && tile == 7)
  {
    rdp.tiles[0].ul_s = ul_s;
    rdp.tiles[0].ul_t = ul_t;
    rdp.tiles[0].lr_s = lr_s;
    rdp.tiles[0].lr_t = lr_t;
  }

  uint32_t height = lr_t - ul_t + 1;   // get height
  uint32_t width = lr_s - ul_s + 1;

  int line_n = rdp.timg.width << rdp.tiles[tile].size >> 1;
  uint32_t offs = ul_t * line_n;
  offs += ul_s << rdp.tiles[tile].size >> 1;
  offs += rdp.timg.addr;
  if (offs >= BMASK)
    return;

  if (rdp.timg.size == 3)
  {
    LoadTile32b(tile, ul_s, ul_t, width, height);
  }
  else
  {
    // check if points to bad location
    if (offs + line_n*height > BMASK)
      height = (BMASK - offs) / line_n;
    if (height == 0)
      return;

    uint32_t wid_64 = rdp.tiles[tile].line;
    unsigned char *dst = ((unsigned char *)rdp.tmem) + (rdp.tiles[tile].t_mem<<3);
    unsigned char *end = ((unsigned char *)rdp.tmem) + 4096 - (wid_64<<3);
    loadTile((uint32_t *)gfx.RDRAM, (uint32_t *)dst, wid_64, height, line_n, offs, (uint32_t *)end);
  }
  FRDP("loadtile: tile: %d, ul_s: %d, ul_t: %d, lr_s: %d, lr_t: %d\n", tile,
    ul_s, ul_t, lr_s, lr_t);

#ifdef HAVE_HWFBE
  if (fb_hwfbe_enabled)
    setTBufTex(rdp.tiles[tile].t_mem, rdp.tiles[tile].line*height);
#endif
}

static void rdp_settile()
{
   int i;
   tile_set = 1; // used to check if we only load the first settilesize

   rdp.first = 0;

   rdp.last_tile = (uint32_t)((rdp.cmd1 >> 24) & 0x07);
   TILE *tile = &rdp.tiles[rdp.last_tile];

   tile->format = (uint8_t)((rdp.cmd0 >> 21) & 0x07);
   tile->size = (uint8_t)((rdp.cmd0 >> 19) & 0x03);
   tile->line = (uint16_t)((rdp.cmd0 >> 9) & 0x01FF);
   tile->t_mem = (uint16_t)(rdp.cmd0 & 0x1FF);
   tile->palette = (uint8_t)((rdp.cmd1 >> 20) & 0x0F);
   tile->clamp_t = (uint8_t)((rdp.cmd1 >> 19) & 0x01);
   tile->mirror_t = (uint8_t)((rdp.cmd1 >> 18) & 0x01);
   tile->mask_t = (uint8_t)((rdp.cmd1 >> 14) & 0x0F);
   tile->shift_t = (uint8_t)((rdp.cmd1 >> 10) & 0x0F);
   tile->clamp_s = (uint8_t)((rdp.cmd1 >> 9) & 0x01);
   tile->mirror_s = (uint8_t)((rdp.cmd1 >> 8) & 0x01);
   tile->mask_s = (uint8_t)((rdp.cmd1 >> 4) & 0x0F);
   tile->shift_s = (uint8_t)(rdp.cmd1 & 0x0F);

   rdp.update |= UPDATE_TEXTURE;

   FRDP ("settile: tile: %d, format: %s, size: %s, line: %d, "
         "t_mem: %08lx, palette: %d, clamp_t/mirror_t: %s, mask_t: %d, "
         "shift_t: %d, clamp_s/mirror_s: %s, mask_s: %d, shift_s: %d\n",
         rdp.last_tile, str_format[tile->format], str_size[tile->size], tile->line,
         tile->t_mem, tile->palette, str_cm[(tile->clamp_t<<1)|tile->mirror_t], tile->mask_t,
         tile->shift_t, str_cm[(tile->clamp_s<<1)|tile->mirror_s], tile->mask_s, tile->shift_s);

#ifdef HAVE_HWFBE
   if (fb_hwfbe_enabled && rdp.last_tile < rdp.cur_tile + 2)
   {
      for (i = 0; i < 2; i++)
      {
         if (rdp.aTBuffTex[i])
         {
            if (rdp.aTBuffTex[i]->t_mem == tile->t_mem)
            {
               if (rdp.aTBuffTex[i]->size == tile->size)
               {
                  rdp.aTBuffTex[i]->tile = rdp.last_tile;
                  rdp.aTBuffTex[i]->info.format = tile->format == 0 ? GR_TEXFMT_RGB_565 : GR_TEXFMT_ALPHA_INTENSITY_88;
                  FRDP("rdp.aTBuffTex[%d] tile=%d, format=%s\n", i, rdp.last_tile, tile->format == 0 ? "RGB565" : "Alpha88");
               }
               else
                  rdp.aTBuffTex[i] = 0;
               break;
            }
            else if (rdp.aTBuffTex[i]->tile == rdp.last_tile) //wrong! t_mem must be the same
               rdp.aTBuffTex[i] = 0;
         }
      }
   }
#endif
}

//
// fillrect - fills a rectangle
//

static void rdp_fillrect()
{
  uint32_t ul_x = ((rdp.cmd1 & 0x00FFF000) >> 14);
  uint32_t ul_y = (rdp.cmd1 & 0x00000FFF) >> 2;
  uint32_t lr_x = ((rdp.cmd0 & 0x00FFF000) >> 14) + 1;
  uint32_t lr_y = ((rdp.cmd0 & 0x00000FFF) >> 2) + 1;
  if ((ul_x > lr_x) || (ul_y > lr_y))
  {
    LRDP("Fillrect. Wrong coordinates. Skipped\n");
    return;
  }
  int pd_multiplayer = (settings.ucode == ucode_PerfectDark) && (rdp.cycle_mode == 3) && (rdp.fill_color == 0xFFFCFFFC);
  if ((rdp.cimg == rdp.zimg) || (fb_emulation_enabled && rdp.frame_buffers[rdp.ci_count-1].status == ci_zimg) || pd_multiplayer)
  {
    LRDP("Fillrect - cleared the depth buffer\n");
    {
      if (!(settings.hacks&hack_Hyperbike) || rdp.ci_width > 64) //do not clear main depth buffer for aux depth buffers
      {
        update_scissor ();
        grDepthMask (FXTRUE);
        grColorMask (FXFALSE, FXFALSE);
        grBufferClear (0, 0, rdp.fill_color ? rdp.fill_color&0xFFFF : 0xFFFF);
        grColorMask (FXTRUE, FXTRUE);
        rdp.update |= UPDATE_ZBUF_ENABLED;
      }
      //if (settings.frame_buffer&fb_depth_clear)
      {
         uint32_t y, x;
        ul_x = min(max(ul_x, rdp.scissor_o.ul_x), rdp.scissor_o.lr_x);
        lr_x = min(max(lr_x, rdp.scissor_o.ul_x), rdp.scissor_o.lr_x);
        ul_y = min(max(ul_y, rdp.scissor_o.ul_y), rdp.scissor_o.lr_y);
        lr_y = min(max(lr_y, rdp.scissor_o.ul_y), rdp.scissor_o.lr_y);
        uint32_t zi_width_in_dwords = rdp.ci_width >> 1;
        ul_x >>= 1;
        lr_x >>= 1;
        uint32_t * dst = (uint32_t*)(gfx.RDRAM+rdp.cimg);
        dst += ul_y * zi_width_in_dwords;
        for (y = ul_y; y < lr_y; y++)
        {
          for (x = ul_x; x < lr_x; x++)
            dst[x] = rdp.fill_color;
          dst += zi_width_in_dwords;
        }
      }
    }
    return;
  }

  if (rdp.skip_drawing)
  {
    LRDP("Fillrect skipped\n");
    return;
  }

  if (rdp.cur_image && (rdp.cur_image->format != 0) && (rdp.cycle_mode == 3) && (rdp.cur_image->width == lr_x - ul_x) && (rdp.cur_image->height == lr_y - ul_y))
  {
    uint32_t color = rdp.fill_color;
    if (rdp.ci_size < 3)
    {
	  color = ((color&1)?0xFF:0) |
		((uint32_t)((float)((color&0xF800) >> 11) / 31.0f * 255.0f) << 24) |
		((uint32_t)((float)((color&0x07C0) >> 6) / 31.0f * 255.0f) << 16) |
		((uint32_t)((float)((color&0x003E) >> 1) / 31.0f * 255.0f) << 8);
    }
    grDepthMask (FXFALSE);
    grBufferClear (color, 0, 0xFFFF);
    grDepthMask (FXTRUE);
    rdp.update |= UPDATE_ZBUF_ENABLED;
    LRDP("Fillrect - cleared the texture buffer\n");
    return;
  }

  // Update scissor
  update_scissor ();

  if (settings.decrease_fillrect_edge && rdp.cycle_mode == 0)
  {
    lr_x--; lr_y--;
  }
  FRDP("fillrect (%d,%d) -> (%d,%d), cycle mode: %d, #%d, #%d\n", ul_x, ul_y, lr_x, lr_y, rdp.cycle_mode,
    rdp.tri_n, rdp.tri_n+1);

  FRDP("scissor (%d,%d) -> (%d,%d)\n", rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x,
    rdp.scissor.lr_y);

  // KILL the floating point error with 0.01f
  int32_t s_ul_x = (uint32_t)min(max(ul_x * rdp.scale_x + rdp.offset_x + 0.01f, rdp.scissor.ul_x), rdp.scissor.lr_x);
  int32_t s_lr_x = (uint32_t)min(max(lr_x * rdp.scale_x + rdp.offset_x + 0.01f, rdp.scissor.ul_x), rdp.scissor.lr_x);
  int32_t s_ul_y = (uint32_t)min(max(ul_y * rdp.scale_y + rdp.offset_y + 0.01f, rdp.scissor.ul_y), rdp.scissor.lr_y);
  int32_t s_lr_y = (uint32_t)min(max(lr_y * rdp.scale_y + rdp.offset_y + 0.01f, rdp.scissor.ul_y), rdp.scissor.lr_y);

  if (s_lr_x < 0) s_lr_x = 0;
  if (s_lr_y < 0) s_lr_y = 0;
  if ((uint32_t)s_ul_x > settings.res_x) s_ul_x = settings.res_x;
  if ((uint32_t)s_ul_y > settings.res_y) s_ul_y = settings.res_y;

  FRDP (" - %d, %d, %d, %d\n", s_ul_x, s_ul_y, s_lr_x, s_lr_y);

  {
    grFogMode (GR_FOG_DISABLE);

    const float Z = (rdp.cycle_mode == 3) ? 0.0f : set_sprite_combine_mode();

    // Draw the rectangle
    VERTEX v[4] = {
      { (float)s_ul_x, (float)s_ul_y, Z, 1.0f,  0,0,0,0,  {0,0,0,0}, 0,0, 0,0,0,0},
      { (float)s_lr_x, (float)s_ul_y, Z, 1.0f,  0,0,0,0,  {0,0,0,0}, 0,0, 0,0,0,0},
      { (float)s_ul_x, (float)s_lr_y, Z, 1.0f,  0,0,0,0,  {0,0,0,0}, 0,0, 0,0,0,0},
      { (float)s_lr_x, (float)s_lr_y, Z, 1.0f,  0,0,0,0,  {0,0,0,0}, 0,0, 0,0,0,0} };

      if (rdp.cycle_mode == 3)
      {
        uint32_t color = rdp.fill_color;

        if ((settings.hacks&hack_PMario) && rdp.frame_buffers[rdp.ci_count-1].status == ci_aux)
        {
          //background of auxiliary frame buffers must have zero alpha.
          //make it black, set 0 alpha to plack pixels on frame buffer read
          color = 0;
        }
        else if (rdp.ci_size < 3)
        {
          color = ((color&1)?0xFF:0) |
            ((uint32_t)((float)((color&0xF800) >> 11) / 31.0f * 255.0f) << 24) |
            ((uint32_t)((float)((color&0x07C0) >> 6) / 31.0f * 255.0f) << 16) |
            ((uint32_t)((float)((color&0x003E) >> 1) / 31.0f * 255.0f) << 8);
        }

        grConstantColorValue (color);

        grColorCombine (GR_COMBINE_FUNCTION_LOCAL,
          GR_COMBINE_FACTOR_NONE,
          GR_COMBINE_LOCAL_CONSTANT,
          GR_COMBINE_OTHER_NONE,
          FXFALSE);

        grAlphaCombine (GR_COMBINE_FUNCTION_LOCAL,
          GR_COMBINE_FACTOR_NONE,
          GR_COMBINE_LOCAL_CONSTANT,
          GR_COMBINE_OTHER_NONE,
          FXFALSE);

        grAlphaBlendFunction (GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ONE, GR_BLEND_ZERO);

        grAlphaTestFunction (GR_CMP_ALWAYS);
        grStippleMode(GR_STIPPLE_DISABLE);

        grCullMode(GR_CULL_DISABLE);
        grFogMode (GR_FOG_DISABLE);
        grDepthBufferFunction (GR_CMP_ALWAYS);
        grDepthMask (FXFALSE);

        rdp.update |= UPDATE_COMBINE | UPDATE_CULL_MODE | UPDATE_FOG_ENABLED | UPDATE_ZBUF_ENABLED;
      }
      else
      {
        uint32_t cmb_mode_c = (rdp.cycle1 << 16) | (rdp.cycle2 & 0xFFFF);
        uint32_t cmb_mode_a = (rdp.cycle1 & 0x0FFF0000) | ((rdp.cycle2 >> 16) & 0x00000FFF);
        if (cmb_mode_c == 0x9fff9fff || cmb_mode_a == 0x09ff09ff) //shade
        {
           int k;
          AllowShadeMods (v, 4);
          for (k = 0; k < 4; k++)
            apply_shade_mods (&v[k]);
        }
        if ((rdp.othermode_l & 0x4000) && ((rdp.othermode_l >> 16) == 0x0550)) //special blender mode for Bomberman64
        {
          grAlphaCombine (GR_COMBINE_FUNCTION_LOCAL,
            GR_COMBINE_FACTOR_NONE,
            GR_COMBINE_LOCAL_CONSTANT,
            GR_COMBINE_OTHER_NONE,
            FXFALSE);
          grConstantColorValue((cmb.ccolor&0xFFFFFF00)|(rdp.fog_color&0xFF));
          rdp.update |= UPDATE_COMBINE;
        }
      }

      grDrawTriangle(&v[0], &v[2], &v[1]);
      grDrawTriangle(&v[2], &v[3], &v[1]);

      rdp.tri_n += 2;
  }
}

//
// setfillcolor - sets the filling color
//

static void rdp_setfillcolor()
{
  rdp.fill_color = rdp.cmd1;
  rdp.update |= UPDATE_ALPHA_COMPARE | UPDATE_COMBINE;

  FRDP("setfillcolor: %08lx\n", rdp.cmd1);
}

static void rdp_setfogcolor()
{
  rdp.fog_color = rdp.cmd1;
  rdp.update |= UPDATE_COMBINE | UPDATE_FOG_ENABLED;

  FRDP("setfogcolor - %08lx\n", rdp.cmd1);
}

static void rdp_setblendcolor()
{
  rdp.blend_color = rdp.cmd1;
  rdp.update |= UPDATE_COMBINE;

  FRDP("setblendcolor: %08lx\n", rdp.cmd1);
}

static void rdp_setprimcolor()
{
  rdp.prim_color = rdp.cmd1;
  rdp.prim_lodmin = (rdp.cmd0 >> 8) & 0xFF;
  rdp.prim_lodfrac = max(rdp.cmd0 & 0xFF, rdp.prim_lodmin);
  rdp.update |= UPDATE_COMBINE;

  FRDP("setprimcolor: %08lx, lodmin: %d, lodfrac: %d\n", rdp.cmd1, rdp.prim_lodmin,
    rdp.prim_lodfrac);
}

static void rdp_setenvcolor()
{
  rdp.env_color = rdp.cmd1;
  rdp.update |= UPDATE_COMBINE;

  FRDP("setenvcolor: %08lx\n", rdp.cmd1);
}

static void rdp_setcombine()
{
  rdp.c_a0  = (uint8_t)((rdp.cmd0 >> 20) & 0xF);
  rdp.c_b0  = (uint8_t)((rdp.cmd1 >> 28) & 0xF);
  rdp.c_c0  = (uint8_t)((rdp.cmd0 >> 15) & 0x1F);
  rdp.c_d0  = (uint8_t)((rdp.cmd1 >> 15) & 0x7);
  rdp.c_Aa0 = (uint8_t)((rdp.cmd0 >> 12) & 0x7);
  rdp.c_Ab0 = (uint8_t)((rdp.cmd1 >> 12) & 0x7);
  rdp.c_Ac0 = (uint8_t)((rdp.cmd0 >> 9)  & 0x7);
  rdp.c_Ad0 = (uint8_t)((rdp.cmd1 >> 9)  & 0x7);

  rdp.c_a1  = (uint8_t)((rdp.cmd0 >> 5)  & 0xF);
  rdp.c_b1  = (uint8_t)((rdp.cmd1 >> 24) & 0xF);
  rdp.c_c1  = (uint8_t)((rdp.cmd0 >> 0)  & 0x1F);
  rdp.c_d1  = (uint8_t)((rdp.cmd1 >> 6)  & 0x7);
  rdp.c_Aa1 = (uint8_t)((rdp.cmd1 >> 21) & 0x7);
  rdp.c_Ab1 = (uint8_t)((rdp.cmd1 >> 3)  & 0x7);
  rdp.c_Ac1 = (uint8_t)((rdp.cmd1 >> 18) & 0x7);
  rdp.c_Ad1 = (uint8_t)((rdp.cmd1 >> 0)  & 0x7);

  rdp.cycle1 = (rdp.c_a0<<0)  | (rdp.c_b0<<4)  | (rdp.c_c0<<8)  | (rdp.c_d0<<13)|
    (rdp.c_Aa0<<16)| (rdp.c_Ab0<<19)| (rdp.c_Ac0<<22)| (rdp.c_Ad0<<25);
  rdp.cycle2 = (rdp.c_a1<<0)  | (rdp.c_b1<<4)  | (rdp.c_c1<<8)  | (rdp.c_d1<<13)|
    (rdp.c_Aa1<<16)| (rdp.c_Ab1<<19)| (rdp.c_Ac1<<22)| (rdp.c_Ad1<<25);

  rdp.update |= UPDATE_COMBINE;

  FRDP("setcombine\na0=%s b0=%s c0=%s d0=%s\nAa0=%s Ab0=%s Ac0=%s Ad0=%s\na1=%s b1=%s c1=%s d1=%s\nAa1=%s Ab1=%s Ac1=%s Ad1=%s\n",
    Mode0[rdp.c_a0], Mode1[rdp.c_b0], Mode2[rdp.c_c0], Mode3[rdp.c_d0],
    Alpha0[rdp.c_Aa0], Alpha1[rdp.c_Ab0], Alpha2[rdp.c_Ac0], Alpha3[rdp.c_Ad0],
    Mode0[rdp.c_a1], Mode1[rdp.c_b1], Mode2[rdp.c_c1], Mode3[rdp.c_d1],
    Alpha0[rdp.c_Aa1], Alpha1[rdp.c_Ab1], Alpha2[rdp.c_Ac1], Alpha3[rdp.c_Ad1]);
}

//
// settextureimage - sets the source for an image copy
//

static void rdp_settextureimage()
{
  static const char *format[]   = { "RGBA", "YUV", "CI", "IA", "I", "?", "?", "?" };
  static const char *size[]     = { "4bit", "8bit", "16bit", "32bit" };

  rdp.timg.format = (uint8_t)((rdp.cmd0 >> 21) & 0x07);
  rdp.timg.size = (uint8_t)((rdp.cmd0 >> 19) & 0x03);
  rdp.timg.width = (uint16_t)(1 + (rdp.cmd0 & 0x00000FFF));
  rdp.timg.addr = segoffset(rdp.cmd1);
  if (ucode5_texshiftaddr)
  {
    if (rdp.timg.format == 0)
    {
      uint16_t * t = (uint16_t*)(gfx.RDRAM+ucode5_texshiftaddr);
      ucode5_texshift = t[ucode5_texshiftcount^1];
      rdp.timg.addr += ucode5_texshift;
    }
    else
    {
      ucode5_texshiftaddr = 0;
      ucode5_texshift = 0;
      ucode5_texshiftcount = 0;
    }
  }
  rdp.s2dex_tex_loaded = true;
  rdp.update |= UPDATE_TEXTURE;

  if (rdp.frame_buffers[rdp.ci_count-1].status == ci_copy_self && (rdp.timg.addr >= rdp.cimg) && (rdp.timg.addr < rdp.ci_end))
  {
    if (!rdp.fb_drawn)
    {
      if (!rdp.cur_image)
         CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
      else
        CloseTextureBuffer(true);
      rdp.fb_drawn = true;
    }
  }

#ifdef HAVE_HWFBE
  if (fb_hwfbe_enabled) //search this texture among drawn texture buffers
    FindTextureBuffer(rdp.timg.addr, rdp.timg.width);
#endif

  FRDP("settextureimage: format: %s, size: %s, width: %d, addr: %08lx\n",
    format[rdp.timg.format], size[rdp.timg.size],
    rdp.timg.width, rdp.timg.addr);
}

static void rdp_setdepthimage()
{
  rdp.zimg = segoffset(rdp.cmd1) & BMASK;
  rdp.zi_width = rdp.ci_width;
  FRDP("setdepthimage - %08lx\n", rdp.zimg);
}

int SwapOK = true;
static void RestoreScale()
{
  FRDP("Return to original scale: x = %f, y = %f\n", rdp.scale_x_bak, rdp.scale_y_bak);
  rdp.scale_x = rdp.scale_x_bak;
  rdp.scale_y = rdp.scale_y_bak;
  //    update_scissor();
  rdp.view_scale[0] *= rdp.scale_x;
  rdp.view_scale[1] *= rdp.scale_y;
  rdp.view_trans[0] *= rdp.scale_x;
  rdp.view_trans[1] *= rdp.scale_y;
  rdp.update |= UPDATE_VIEWPORT | UPDATE_SCISSOR;
  //*
  {
    grDepthMask (FXFALSE);
    grBufferClear (0, 0, 0xFFFF);
    grDepthMask (FXTRUE);
  }
  //*/
}

static uint32_t swapped_addr = 0;

static void rdp_setcolorimage()
{
   int i;

   if (fb_emulation_enabled && (rdp.num_of_ci < NUMTEXBUF))
   {
      COLOR_IMAGE *cur_fb  = &rdp.frame_buffers[rdp.ci_count];
      COLOR_IMAGE *prev_fb = &rdp.frame_buffers[rdp.ci_count?rdp.ci_count-1:0];
      COLOR_IMAGE *next_fb = &rdp.frame_buffers[rdp.ci_count+1];
      switch (cur_fb->status)
      {
         case ci_main:
            {

               if (rdp.ci_count == 0)
               {
                  if ((rdp.ci_status == ci_aux)) //for PPL
                  {
                     float sx = rdp.scale_x;
                     float sy = rdp.scale_y;
                     rdp.scale_x = 1.0f;
                     rdp.scale_y = 1.0f;
                     CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
                     rdp.scale_x = sx;
                     rdp.scale_y = sy;
                  }
                  if (!fb_hwfbe_enabled)
                  {
                     if ((rdp.num_of_ci > 1) &&
                           (next_fb->status == ci_aux) &&
                           (next_fb->width >= cur_fb->width))
                     {
                        rdp.scale_x = 1.0f;
                        rdp.scale_y = 1.0f;
                     }
                  }
                  else if (rdp.copy_ci_index && (settings.hacks&hack_PMario)) //tidal wave
                     OpenTextureBuffer(&rdp.frame_buffers[rdp.main_ci_index]);
               }
               else if (!rdp.motionblur && fb_hwfbe_enabled && !SwapOK && (rdp.ci_count <= rdp.copy_ci_index))
               {
                  if (next_fb->status == ci_aux_copy)
                     OpenTextureBuffer(&rdp.frame_buffers[rdp.main_ci_index]);
                  else
                     OpenTextureBuffer(&rdp.frame_buffers[rdp.copy_ci_index]);
               }
#ifdef HAVE_HWFBE
               else if (fb_hwfbe_enabled && prev_fb->status == ci_aux)
               {
                  if (rdp.motionblur)
                  {
                     rdp.cur_image = &(rdp.texbufs[rdp.cur_tex_buf].images[0]);
                     grRenderBuffer( GR_BUFFER_TEXTUREBUFFER_EXT );
                     grTextureBufferExt( rdp.cur_image->tmu, rdp.cur_image->tex_addr, rdp.cur_image->info.smallLodLog2, rdp.cur_image->info.largeLodLog2,
                           rdp.cur_image->info.aspectRatioLog2, rdp.cur_image->info.format, GR_MIPMAPLEVELMASK_BOTH );
                  }
                  else if (rdp.read_whole_frame)
                  {
                     OpenTextureBuffer(&rdp.frame_buffers[rdp.main_ci_index]);
                  }
               }
#endif
               //else if (rdp.ci_status == ci_aux && !rdp.copy_ci_index)
               //  CloseTextureBuffer(false);

               rdp.skip_drawing = false;
            }
            break;
         case ci_copy:
            {
               if (!rdp.motionblur || (settings.frame_buffer&fb_motionblur))
               {
                  if (cur_fb->width == rdp.ci_width)
                  {
                     if (CopyTextureBuffer(prev_fb, cur_fb))
                     {
                        //                      if (CloseTextureBuffer(TRUE))
                        //*
                        if ((settings.hacks&hack_Zelda) && (rdp.frame_buffers[rdp.ci_count+2].status == ci_aux) && !rdp.fb_drawn) //hack for photo camera in Zelda MM
                        {
                           CopyFrameBuffer (GR_BUFFER_TEXTUREBUFFER_EXT);
                           rdp.fb_drawn = true;
                           memcpy(gfx.RDRAM+cur_fb->addr,gfx.RDRAM+rdp.cimg, (cur_fb->width*cur_fb->height)<<cur_fb->size>>1);
                        }
                        //*/
                     }
                     else
                     {
                        if (!rdp.fb_drawn || prev_fb->status == ci_copy_self)
                        {
                           CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
                           rdp.fb_drawn = true;
                        }
                        memcpy(gfx.RDRAM+cur_fb->addr,gfx.RDRAM+rdp.cimg, (cur_fb->width*cur_fb->height)<<cur_fb->size>>1);
                     }
                  }
                  else
                     CloseTextureBuffer(true);
               }
               else
                  memset(gfx.RDRAM+cur_fb->addr, 0, cur_fb->width*cur_fb->height*rdp.ci_size);
               rdp.skip_drawing = true;
            }
            break;
         case ci_aux_copy:
            {
               rdp.skip_drawing = false;
               if (CloseTextureBuffer(prev_fb->status != ci_aux_copy))
                  ;
               else if (!rdp.fb_drawn)
               {
                  CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
                  rdp.fb_drawn = true;
               }
#ifdef HAVE_HWFBE
               if (fb_hwfbe_enabled)
                  OpenTextureBuffer(cur_fb);
#endif
            }
            break;
         case ci_old_copy:
            {
               if (!rdp.motionblur || (settings.frame_buffer&fb_motionblur))
               {
                  if (cur_fb->width == rdp.ci_width)
                  {
                     memcpy(gfx.RDRAM+cur_fb->addr,gfx.RDRAM+rdp.maincimg[1].addr, (cur_fb->width*cur_fb->height)<<cur_fb->size>>1);
                  }
                  //rdp.skip_drawing = true;
               }
               else
               {
                  memset(gfx.RDRAM+cur_fb->addr, 0, (cur_fb->width*cur_fb->height)<<rdp.ci_size>>1);
               }
            }
            break;
            /*
               else if (rdp.frame_buffers[rdp.ci_count].status == ci_main_i)
               {
            // CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
            rdp.scale_x = rdp.scale_x_bak;
            rdp.scale_y = rdp.scale_y_bak;
            rdp.skip_drawing = false;
            }
            */
         case ci_aux:
            {
               if (!fb_hwfbe_enabled && cur_fb->format != 0)
                  rdp.skip_drawing = true;
               else
               {
                  rdp.skip_drawing = false;
#ifdef HAVE_HWFBE
                  if (fb_hwfbe_enabled && OpenTextureBuffer(cur_fb))
                     ;
                  else
#endif
                  {
                     if (cur_fb->format != 0)
                        rdp.skip_drawing = true;
                     if (rdp.ci_count == 0)
                     {
                        //           if (rdp.num_of_ci > 1)
                        //           {
                        rdp.scale_x = 1.0f;
                        rdp.scale_y = 1.0f;
                        //           }
                     }
                     else if (!fb_hwfbe_enabled && (prev_fb->status == ci_main) &&
                           (prev_fb->width == cur_fb->width)) // for Pokemon Stadium
                        CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
                  }
               }
               cur_fb->status = ci_aux;
            }
            break;
         case ci_zimg:
#ifdef HAVE_HWFBE
            if (settings.ucode != ucode_PerfectDark)
            {
               if (fb_hwfbe_enabled && !rdp.copy_ci_index && (rdp.copy_zi_index || (settings.hacks&hack_BAR)))
               {
                  GrLOD_t LOD = GR_LOD_LOG2_1024;
                  if (settings.scr_res_x > 1024)
                     LOD = GR_LOD_LOG2_2048;
                  grTextureAuxBufferExt( rdp.texbufs[0].tmu, rdp.texbufs[0].begin, LOD, LOD,
                        GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565, GR_MIPMAPLEVELMASK_BOTH );
                  grAuxBufferExt( GR_BUFFER_TEXTUREAUXBUFFER_EXT );
                  LRDP("rdp_setcolorimage - set texture depth buffer to TMU0\n");
               }
            }
#endif
            rdp.skip_drawing = true;
            break;
         case ci_zcopy:
#ifdef HAVE_HWFBE
            if (settings.ucode != ucode_PerfectDark)
            {
               if (fb_hwfbe_enabled && !rdp.copy_ci_index && rdp.copy_zi_index == rdp.ci_count)
               {
                  CopyDepthBuffer();
               }
               rdp.skip_drawing = true;
            }
#endif
            break;
         case ci_useless:
            rdp.skip_drawing = true;
            break;
         case ci_copy_self:
#ifdef HAVE_HWFBE
            if (fb_hwfbe_enabled && (rdp.ci_count <= rdp.copy_ci_index) && (!SwapOK || settings.swapmode == 2))
               OpenTextureBuffer(cur_fb);
#endif
            rdp.skip_drawing = false;
            break;
         default:
            rdp.skip_drawing = false;
      }

      if ((rdp.ci_count > 0) && (prev_fb->status >= ci_aux)) //for Pokemon Stadium
      {
         if (!fb_hwfbe_enabled && prev_fb->format == 0)
            CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
         else if ((settings.hacks&hack_Knockout) && prev_fb->width < 100)
            CopyFrameBuffer (GR_BUFFER_TEXTUREBUFFER_EXT);
      }
      if (!fb_hwfbe_enabled && cur_fb->status == ci_copy)
      {
         if (!rdp.motionblur && (rdp.num_of_ci > rdp.ci_count+1) && (next_fb->status != ci_aux))
         {
            RestoreScale();
         }
      }
      if (!fb_hwfbe_enabled && cur_fb->status == ci_aux)
      {
         if (cur_fb->format == 0)
         {
            if ((settings.hacks&hack_PPL) && (rdp.scale_x < 1.1f))  //need to put current image back to frame buffer
            {
               int y, x;
               int width = cur_fb->width;
               int height = cur_fb->height;
               uint16_t *ptr_dst = (uint16_t*)malloc(width * height * sizeof(uint16_t));
               uint16_t *ptr_src = (uint16_t*)(gfx.RDRAM+cur_fb->addr);
               uint16_t c;

               for (y = 0; y < height; y++)
               {
                  for (x = 0; x < width; x++)
                  {
                     c = ((ptr_src[(x + y * width)^1]) >> 1) | 0x8000;
                     ptr_dst[x + y * width] = c;
                  }
               }
               grLfbWriteRegion(GR_BUFFER_BACKBUFFER,
                     (uint32_t)rdp.offset_x,
                     (uint32_t)rdp.offset_y,
                     GR_LFB_SRC_FMT_555,
                     width,
                     height,
                     FXFALSE,
                     width<<1,
                     ptr_dst);

               free(ptr_dst);
            }
            /*
               else  //just clear buffer
               {

               grColorMask(FXTRUE, FXTRUE);
               grBufferClear (0, 0, 0xFFFF);
               }
               */
         }
      }

      if ((cur_fb->status == ci_main) && (rdp.ci_count > 0))
      {
         int to_org_res = true;
         for (i = rdp.ci_count + 1; i < rdp.num_of_ci; i++)
         {
            if ((rdp.frame_buffers[i].status != ci_main) && (rdp.frame_buffers[i].status != ci_zimg) && (rdp.frame_buffers[i].status != ci_zcopy))
            {
               to_org_res = false;
               break;
            }
         }
         if (to_org_res)
         {
            LRDP("return to original scale\n");
            rdp.scale_x = rdp.scale_x_bak;
            rdp.scale_y = rdp.scale_y_bak;
#ifdef HAVE_HWFBE
            if (fb_hwfbe_enabled && !rdp.read_whole_frame)
               CloseTextureBuffer(false);
#endif
         }
#ifdef HAVE_HWFBE
         if (fb_hwfbe_enabled && !rdp.read_whole_frame && (prev_fb->status >= ci_aux) && (rdp.ci_count > rdp.copy_ci_index))
            CloseTextureBuffer(false);
#endif

      }
      rdp.ci_status = cur_fb->status;
      rdp.ci_count++;
   }

   rdp.ocimg = rdp.cimg;
   rdp.cimg = segoffset(rdp.cmd1) & BMASK;
   rdp.ci_width = (rdp.cmd0 & 0xFFF) + 1;
   if (fb_emulation_enabled)
      rdp.ci_height = rdp.frame_buffers[rdp.ci_count-1].height;
   else if (rdp.ci_width == 32)
      rdp.ci_height = 32;
   else
      rdp.ci_height = rdp.scissor_o.lr_y;
   if (rdp.zimg == rdp.cimg)
   {
      rdp.zi_width = rdp.ci_width;
      //    int zi_height = min((int)rdp.zi_width*3/4, (int)rdp.vi_height);
      //    rdp.zi_words = rdp.zi_width * zi_height;
   }
   uint32_t format = (rdp.cmd0 >> 21) & 0x7;
   rdp.ci_size = (rdp.cmd0 >> 19) & 0x3;
   rdp.ci_end = rdp.cimg + ((rdp.ci_width*rdp.ci_height)<<(rdp.ci_size-1));
   FRDP("setcolorimage - %08lx, width: %d,  height: %d, format: %d, size: %d\n", rdp.cmd1, rdp.ci_width, rdp.ci_height, format, rdp.ci_size);
   FRDP("cimg: %08lx, ocimg: %08lx, SwapOK: %d\n", rdp.cimg, rdp.ocimg, SwapOK);

   if (format != 0) //can't draw into non RGBA buffer
   {
      if (!rdp.cur_image)
      {
#ifdef HAVE_HWFBE
         if (fb_hwfbe_enabled && rdp.ci_width <= 64)
            OpenTextureBuffer(&rdp.frame_buffers[rdp.ci_count - 1]);
         else
#endif
         if (format > 2)
            rdp.skip_drawing = true;
         return;
      }
   }
   else
   {
      if (!fb_emulation_enabled)
         rdp.skip_drawing = false;
   }

   CI_SET = true;
   if (settings.swapmode > 0)
   {
      if (rdp.zimg == rdp.cimg)
         rdp.updatescreen = 1;

      int viSwapOK = ((settings.swapmode == 2) && (rdp.vi_org_reg == *gfx.VI_ORIGIN_REG)) ? false : true;
      if ((rdp.zimg != rdp.cimg) && (rdp.ocimg != rdp.cimg) && SwapOK && viSwapOK && !rdp.cur_image)
      {
         if (fb_emulation_enabled)
            rdp.maincimg[0] = rdp.frame_buffers[rdp.main_ci_index];
         else
            rdp.maincimg[0].addr = rdp.cimg;
         rdp.last_drawn_ci_addr = (settings.swapmode == 2) ? swapped_addr : rdp.maincimg[0].addr;
         swapped_addr = rdp.cimg;
         newSwapBuffers();
         rdp.vi_org_reg = *gfx.VI_ORIGIN_REG;
         SwapOK = false;
#ifdef HAVE_HWFBE
         if (fb_hwfbe_enabled)
         {
            if (rdp.copy_ci_index && (rdp.frame_buffers[rdp.ci_count-1].status != ci_zimg))
            {
               int idx = (rdp.frame_buffers[rdp.ci_count].status == ci_aux_copy) ? rdp.main_ci_index : rdp.copy_ci_index;
               FRDP("attempt open tex buffer. status: %s, addr: %08lx\n", CIStatus[rdp.frame_buffers[idx].status], rdp.frame_buffers[idx].addr);
               OpenTextureBuffer(&rdp.frame_buffers[idx]);
               if (rdp.frame_buffers[rdp.copy_ci_index].status == ci_main) //tidal wave
                  rdp.copy_ci_index = 0;
            }
            else if (rdp.read_whole_frame && !rdp.cur_image)
            {
               OpenTextureBuffer(&rdp.frame_buffers[rdp.main_ci_index]);
            }
         }
#endif
      }
   }
}

static void rsp_reserved0()
{
  if (settings.ucode == ucode_DiddyKong)
  {
    ucode5_texshiftaddr = segoffset(rdp.cmd1);
    ucode5_texshiftcount = 0;
    FRDP("uc5_texshift. addr: %08lx\n", ucode5_texshiftaddr);
  }
  else
  {
    RDP_E("reserved0 - IGNORED\n");
    LRDP("reserved0 - IGNORED\n");
  }
}

static void rsp_reserved1()
{
  LRDP("reserved1 - ignored\n");
}

static void rsp_reserved2()
{
  LRDP("reserved2\n");
}

static void rsp_reserved3()
{
  LRDP("reserved3 - ignored\n");
}

/******************************************************************
Function: FrameBufferRead
Purpose:  This function is called to notify the dll that the
frame buffer memory is beening read at the given address.
DLL should copy content from its render buffer to the frame buffer
in N64 RDRAM
DLL is responsible to maintain its own frame buffer memory addr list
DLL should copy 4KB block content back to RDRAM frame buffer.
Emulator should not call this function again if other memory
is read within the same 4KB range
input:    addr          rdram address
val                     val
size            1 = uint8_t, 2 = uint16_t, 4 = uint32_t
output:   none
*******************************************************************/

EXPORT void CALL FBRead(uint32_t addr)
{
  LOG ("FBRead ()\n");

  if (cpu_fb_ignore)
    return;
  if (cpu_fb_write_called)
  {
    cpu_fb_ignore = true;
    cpu_fb_write = false;
    return;
  }
  cpu_fb_read_called = true;
  uint32_t a = segoffset(addr);
  FRDP("FBRead. addr: %08lx\n", a);
  if (!rdp.fb_drawn && (a >= rdp.cimg) && (a < rdp.ci_end))
  {
    fbreads_back++;
    //if (fbreads_back > 2) //&& (rdp.ci_width <= 320))
    {
       CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
      rdp.fb_drawn = true;
    }
  }
  if (!rdp.fb_drawn_front && (a >= rdp.maincimg[1].addr) && (a < rdp.maincimg[1].addr + rdp.ci_width*rdp.ci_height*2))
  {
    fbreads_front++;
    //if (fbreads_front > 2)//&& (rdp.ci_width <= 320))
    {
      uint32_t cimg = rdp.cimg;
      rdp.cimg = rdp.maincimg[1].addr;
      if (fb_emulation_enabled)
      {
        rdp.ci_width = rdp.maincimg[1].width;
        rdp.ci_count = 0;
        uint32_t h = rdp.frame_buffers[0].height;
        rdp.frame_buffers[0].height = rdp.maincimg[1].height;
        CopyFrameBuffer(GR_BUFFER_FRONTBUFFER);
        rdp.frame_buffers[0].height = h;
      }
      else
      {
        CopyFrameBuffer(GR_BUFFER_FRONTBUFFER);
      }
      rdp.cimg = cimg;
      rdp.fb_drawn_front = true;
    }
  }
}


/******************************************************************
Function: FrameBufferWrite
Purpose:  This function is called to notify the dll that the
frame buffer has been modified by CPU at the given address.
input:    addr          rdram address
val                     val
size            1 = uint8_t, 2 = uint16_t, 4 = uint32_t
output:   none
*******************************************************************/
EXPORT void CALL FBWrite(uint32_t addr, uint32_t size)
{
  LOG ("FBWrite ()\n");
  if (cpu_fb_ignore)
    return;
  if (cpu_fb_read_called)
  {
    cpu_fb_ignore = true;
    cpu_fb_write = false;
    return;
  }
  cpu_fb_write_called = true;
  uint32_t a = segoffset(addr);
  FRDP("FBWrite. addr: %08lx\n", a);
  if (a < rdp.cimg || a > rdp.ci_end)
    return;
  cpu_fb_write = true;
  uint32_t shift_l = (a-rdp.cimg) >> 1;
  uint32_t shift_r = shift_l+2;

  d_ul_x = min(d_ul_x, shift_l%rdp.ci_width);
  d_ul_y = min(d_ul_y, shift_l/rdp.ci_width);
  d_lr_x = max(d_lr_x, shift_r%rdp.ci_width);
  d_lr_y = max(d_lr_y, shift_r/rdp.ci_width);
}


/************************************************************************
Function: FBGetFrameBufferInfo
Purpose:  This function is called by the emulator core to retrieve frame
buffer information from the video plugin in order to be able
to notify the video plugin about CPU frame buffer read/write
operations

size:
= 1           byte
= 2           word (16 bit) <-- this is N64 default depth buffer format
= 4           dword (32 bit)

when frame buffer information is not available yet, set all values
in the FrameBufferInfo structure to 0

input:    FrameBufferInfo pinfo[6]
pinfo is pointed to a FrameBufferInfo structure which to be
filled in by this function
output:   Values are return in the FrameBufferInfo structure
Plugin can return up to 6 frame buffer info
************************************************************************/
///*
EXPORT void CALL FBGetFrameBufferInfo(void *p)
{
   int i;
#ifdef VISUAL_LOGGING
   VLOG ("FBGetFrameBufferInfo ()\n");
#endif
   FrameBufferInfo * pinfo = (FrameBufferInfo *)p;
   memset(pinfo,0,sizeof(FrameBufferInfo)*6);
   if (!(settings.frame_buffer&fb_get_info))
      return;
   LRDP("FBGetFrameBufferInfo ()\n");
   //*
   if (fb_emulation_enabled)
   {
      pinfo[0].addr   = rdp.maincimg[1].addr;
      pinfo[0].size   = rdp.maincimg[1].size;
      pinfo[0].width  = rdp.maincimg[1].width;
      pinfo[0].height = rdp.maincimg[1].height;
      int info_index = 1;
      for (i = 0; i < rdp.num_of_ci && info_index < 6; i++)
      {
         COLOR_IMAGE *cur_fb = &rdp.frame_buffers[i];
         if (cur_fb->status == ci_main || cur_fb->status == ci_copy_self ||
               cur_fb->status == ci_old_copy)
         {
            pinfo[info_index].addr   = cur_fb->addr;
            pinfo[info_index].size   = cur_fb->size;
            pinfo[info_index].width  = cur_fb->width;
            pinfo[info_index].height = cur_fb->height;
            info_index++;
         }
      }
   }
   else
   {
      pinfo[0].addr   = rdp.maincimg[0].addr;
      pinfo[0].size   = rdp.ci_size;
      pinfo[0].width  = rdp.ci_width;
      pinfo[0].height = rdp.ci_width*3/4;
      pinfo[1].addr   = rdp.maincimg[1].addr;
      pinfo[1].size   = rdp.ci_size;
      pinfo[1].width  = rdp.ci_width;
      pinfo[1].height = rdp.ci_width*3/4;
   }
   //*/
}

#include "ucodeFB.h"

void DetectFrameBufferUsage ()
{
   int i;
   LRDP("DetectFrameBufferUsage\n");

   uint32_t dlist_start = *(uint32_t*)(gfx.DMEM+0xFF0);
   uint32_t a;

   int tidal = false;
   if ((settings.hacks&hack_PMario) && (rdp.copy_ci_index || rdp.frame_buffers[rdp.copy_ci_index].status == ci_copy_self))
      tidal = true;
   uint32_t ci = rdp.cimg, zi = rdp.zimg;
   uint32_t ci_height = rdp.frame_buffers[(rdp.ci_count > 0)?rdp.ci_count-1:0].height;
   rdp.main_ci = rdp.main_ci_end = rdp.main_ci_bg = rdp.ci_count = 0;
   rdp.main_ci_index = rdp.copy_ci_index = rdp.copy_zi_index = 0;
   rdp.zimg_end = 0;
   rdp.tmpzimg = 0;
   rdp.motionblur = false;
   rdp.main_ci_last_tex_addr = 0;
   int previous_ci_was_read = rdp.read_previous_ci;
   rdp.read_previous_ci = false;
   rdp.read_whole_frame = false;
   rdp.swap_ci_index = rdp.black_ci_index = -1;
   SwapOK = true;

   // Start executing at the start of the display list
   rdp.pc_i = 0;
   rdp.pc[rdp.pc_i] = dlist_start;
   rdp.dl_count = -1;
   rdp.halt = 0;
   rdp.scale_x_bak = rdp.scale_x;
   rdp.scale_y_bak = rdp.scale_y;

   // MAIN PROCESSING LOOP
   do {

      // Get the address of the next command
      a = rdp.pc[rdp.pc_i] & BMASK;

      // Load the next command and its input
      rdp.cmd0 = ((uint32_t*)gfx.RDRAM)[a>>2];   // \ Current command, 64 bit
      rdp.cmd1 = ((uint32_t*)gfx.RDRAM)[(a>>2)+1]; // /

      // Output the address before the command

      // Go to the next instruction
      rdp.pc[rdp.pc_i] = (a+8) & BMASK;

      if ((uintptr_t)((void*)(gfx_instruction_lite[settings.ucode][rdp.cmd0>>24])))
         gfx_instruction_lite[settings.ucode][rdp.cmd0>>24] ();

      // check DL counter
      if (rdp.dl_count != -1)
      {
         rdp.dl_count --;
         if (rdp.dl_count == 0)
         {
            rdp.dl_count = -1;

            LRDP("End of DL\n");
            rdp.pc_i --;
         }
      }

   } while (!rdp.halt);
   SwapOK = true;
   if (rdp.ci_count > NUMTEXBUF) //overflow
   {
      rdp.cimg = ci;
      rdp.zimg = zi;
      rdp.num_of_ci = rdp.ci_count;
      rdp.scale_x = rdp.scale_x_bak;
      rdp.scale_y = rdp.scale_y_bak;
      return;
   }

   if (rdp.black_ci_index > 0 && rdp.black_ci_index < rdp.copy_ci_index)
      rdp.frame_buffers[rdp.black_ci_index].status = ci_main;

   if (rdp.frame_buffers[rdp.ci_count-1].status == ci_unknown)
   {
      if (rdp.ci_count > 1)
         rdp.frame_buffers[rdp.ci_count-1].status = ci_aux;
      else
         rdp.frame_buffers[rdp.ci_count-1].status = ci_main;
   }

   if ((rdp.frame_buffers[rdp.ci_count-1].status == ci_aux) &&
         (rdp.frame_buffers[rdp.main_ci_index].width < 320) &&
         (rdp.frame_buffers[rdp.ci_count-1].width > rdp.frame_buffers[rdp.main_ci_index].width))
   {
      for (i = 0; i < rdp.ci_count; i++)
      {
         if (rdp.frame_buffers[i].status == ci_main)
            rdp.frame_buffers[i].status = ci_aux;
         else if (rdp.frame_buffers[i].addr == rdp.frame_buffers[rdp.ci_count-1].addr)
            rdp.frame_buffers[i].status = ci_main;
         //                        FRDP("rdp.frame_buffers[%d].status = %d\n", i, rdp.frame_buffers[i].status);
      }
      rdp.main_ci_index = rdp.ci_count-1;
   }

   int all_zimg = true;
   for (i = 0; i < rdp.ci_count; i++)
   {
      if (rdp.frame_buffers[i].status != ci_zimg)
      {
         all_zimg = false;
         break;
      }
   }
   if (all_zimg)
   {
      for (i = 0; i < rdp.ci_count; i++)
         rdp.frame_buffers[i].status = ci_main;
   }

#ifndef NDEBUG
   LRDP("detect fb final results: \n");
   for (i = 0; i < rdp.ci_count; i++)
   {
      FRDP("rdp.frame_buffers[%d].status = %s, addr: %08lx, height: %d\n", i, CIStatus[rdp.frame_buffers[i].status], rdp.frame_buffers[i].addr, rdp.frame_buffers[i].height);
   }
#endif

   rdp.cimg = ci;
   rdp.zimg = zi;
   rdp.num_of_ci = rdp.ci_count;
   if (rdp.read_previous_ci && previous_ci_was_read)
   {
      if (!fb_hwfbe_enabled || !rdp.copy_ci_index)
         rdp.motionblur = true;
   }
   if (rdp.motionblur || fb_hwfbe_enabled || (rdp.frame_buffers[rdp.copy_ci_index].status == ci_aux_copy))
   {
      rdp.scale_x = rdp.scale_x_bak;
      rdp.scale_y = rdp.scale_y_bak;
   }

   if ((rdp.read_previous_ci || previous_ci_was_read) && !rdp.copy_ci_index)
      rdp.read_whole_frame = true;
   if (rdp.read_whole_frame)
   {
#ifdef HAVE_HWFBE
      if (fb_hwfbe_enabled)
      {
         if (rdp.read_previous_ci && !previous_ci_was_read && (settings.swapmode != 2) && (settings.ucode != ucode_PerfectDark))
         {
            int ind = (rdp.ci_count > 0)?rdp.ci_count-1:0;
            uint32_t height = rdp.frame_buffers[ind].height;
            rdp.frame_buffers[ind].height = ci_height;
            CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
            rdp.frame_buffers[ind].height = height;
         }
         if (rdp.swap_ci_index < 0)
         {
            rdp.texbufs[0].clear_allowed = rdp.texbufs[1].clear_allowed = true;
            OpenTextureBuffer(&rdp.frame_buffers[rdp.main_ci_index]);
         }
      }
      else
#endif
      {
         if (rdp.motionblur)
         {
            if (settings.frame_buffer&fb_motionblur)
               CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
            else
               memset(gfx.RDRAM+rdp.cimg, 0, rdp.ci_width*rdp.ci_height*rdp.ci_size);
         }
         else //if (ci_width == rdp.frame_buffers[rdp.main_ci_index].width)
         {
            if (rdp.maincimg[0].height > 65) //for 1080
            {
               rdp.cimg = rdp.maincimg[0].addr;
               rdp.ci_width = rdp.maincimg[0].width;
               rdp.ci_count = 0;
               uint32_t h = rdp.frame_buffers[0].height;
               rdp.frame_buffers[0].height = rdp.maincimg[0].height;
               CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
               rdp.frame_buffers[0].height = h;
            }
            else //conker
            {
               CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
            }
         }
      }
   }

#ifdef HAVE_HWFBE
   if (fb_hwfbe_enabled)
   {
      for (i = 0; i < NUM_TMU; i++)
      {
         int j;
         rdp.texbufs[i].clear_allowed = true;
         for (j = 0; j < 256; j++)
         {
            rdp.texbufs[i].images[j].drawn = false;
            rdp.texbufs[i].images[j].clear = true;
         }
      }
      if (tidal)
      {
         //LRDP("Tidal wave!\n");
         rdp.copy_ci_index = rdp.main_ci_index;
      }
   }
#endif
   rdp.ci_count = 0;
   if (settings.hacks&hack_Banjo2)
      rdp.cur_tex_buf = 0;
   rdp.maincimg[0] = rdp.frame_buffers[rdp.main_ci_index];
   //    rdp.scale_x = rdp.scale_x_bak;
   //    rdp.scale_y = rdp.scale_y_bak;
   LRDP("DetectFrameBufferUsage End\n");
}

/*******************************************
 *          ProcessRDPList                 *
 *******************************************
 *    based on sources of ziggy's z64      *
 *******************************************/

static uint32_t rdp_cmd_ptr = 0;
static uint32_t rdp_cmd_cur = 0;
static uint32_t rdp_cmd_data[0x1000];

void lle_triangle(uint32_t w1, uint32_t w2, int shade, int texture, int zbuffer,
                  uint32_t * rdp_cmd)
{
  rdp.cur_tile = (w1 >> 16) & 0x7;
  int j;
  int xleft, xright, xleft_inc, xright_inc;
  int r, g, b, a, z, s, t, w;
  int drdx = 0, dgdx = 0, dbdx = 0, dadx = 0, dzdx = 0, dsdx = 0, dtdx = 0, dwdx = 0;
  int drde = 0, dgde = 0, dbde = 0, dade = 0, dzde = 0, dsde = 0, dtde = 0, dwde = 0;
  int flip = (w1 & 0x800000) ? 1 : 0;

  int32_t yl, ym, yh;
  int32_t xl, xm, xh;
  int32_t dxldy, dxhdy, dxmdy;
  uint32_t w3, w4, w5, w6, w7, w8;

  uint32_t * shade_base = rdp_cmd + 8;
  uint32_t * texture_base = rdp_cmd + 8;
  uint32_t * zbuffer_base = rdp_cmd + 8;

  if (shade)
  {
    texture_base += 16;
    zbuffer_base += 16;
  }
  if (texture)
  {
    zbuffer_base += 16;
  }

  w3 = rdp_cmd[2];
  w4 = rdp_cmd[3];
  w5 = rdp_cmd[4];
  w6 = rdp_cmd[5];
  w7 = rdp_cmd[6];
  w8 = rdp_cmd[7];

  yl = (w1 & 0x3fff);
  ym = ((w2 >> 16) & 0x3fff);
  yh = ((w2 >>  0) & 0x3fff);
  xl = (int32_t)(w3);
  xh = (int32_t)(w5);
  xm = (int32_t)(w7);
  dxldy = (int32_t)(w4);
  dxhdy = (int32_t)(w6);
  dxmdy = (int32_t)(w8);

  //TODO: negative overflows - commenting them out seems to cause no regressions
  //if experiencing any regressions to do with triangles, check back here
  //if (yl & (0x800<<2)) yl |= 0xfffff000<<2;
  //if (ym & (0x800<<2)) ym |= 0xfffff000<<2;
  //if (yh & (0x800<<2)) yh |= 0xfffff000<<2;

  yh &= ~3;

  r = 0xff; g = 0xff; b = 0xff; a = 0xff; z = 0xffff0000; s = 0;  t = 0;  w = 0x30000;

  if (shade)
  {
    r    = (shade_base[0] & 0xffff0000) | ((shade_base[+4 ] >> 16) & 0x0000ffff);
    g    = ((shade_base[0 ] << 16) & 0xffff0000) | (shade_base[4 ] & 0x0000ffff);
    b    = (shade_base[1 ] & 0xffff0000) | ((shade_base[5 ] >> 16) & 0x0000ffff);
    a    = ((shade_base[1 ] << 16) & 0xffff0000) | (shade_base[5 ] & 0x0000ffff);
    drdx = (shade_base[2 ] & 0xffff0000) | ((shade_base[6 ] >> 16) & 0x0000ffff);
    dgdx = ((shade_base[2 ] << 16) & 0xffff0000) | (shade_base[6 ] & 0x0000ffff);
    dbdx = (shade_base[3 ] & 0xffff0000) | ((shade_base[7 ] >> 16) & 0x0000ffff);
    dadx = ((shade_base[3 ] << 16) & 0xffff0000) | (shade_base[7 ] & 0x0000ffff);
    drde = (shade_base[8 ] & 0xffff0000) | ((shade_base[12] >> 16) & 0x0000ffff);
    dgde = ((shade_base[8 ] << 16) & 0xffff0000) | (shade_base[12] & 0x0000ffff);
    dbde = (shade_base[9 ] & 0xffff0000) | ((shade_base[13] >> 16) & 0x0000ffff);
    dade = ((shade_base[9 ] << 16) & 0xffff0000) | (shade_base[13] & 0x0000ffff);
  }
  if (texture)
  {
    s    = (texture_base[0 ] & 0xffff0000) | ((texture_base[4 ] >> 16) & 0x0000ffff);
    t    = ((texture_base[0 ] << 16) & 0xffff0000)      | (texture_base[4 ] & 0x0000ffff);
    w    = (texture_base[1 ] & 0xffff0000) | ((texture_base[5 ] >> 16) & 0x0000ffff);
    //    w = abs(w);
    dsdx = (texture_base[2 ] & 0xffff0000) | ((texture_base[6 ] >> 16) & 0x0000ffff);
    dtdx = ((texture_base[2 ] << 16) & 0xffff0000)      | (texture_base[6 ] & 0x0000ffff);
    dwdx = (texture_base[3 ] & 0xffff0000) | ((texture_base[7 ] >> 16) & 0x0000ffff);
    dsde = (texture_base[8 ] & 0xffff0000) | ((texture_base[12] >> 16) & 0x0000ffff);
    dtde = ((texture_base[8 ] << 16) & 0xffff0000)      | (texture_base[12] & 0x0000ffff);
    dwde = (texture_base[9 ] & 0xffff0000) | ((texture_base[13] >> 16) & 0x0000ffff);
  }
  if (zbuffer)
  {
    z    = zbuffer_base[0];
    dzdx = zbuffer_base[1];
    dzde = zbuffer_base[2];
  }

  xh <<= 2;  xm <<= 2;  xl <<= 2;
  r <<= 2;  g <<= 2;  b <<= 2;  a <<= 2;
  dsde >>= 2;  dtde >>= 2;  dsdx >>= 2;  dtdx >>= 2;
  dzdx >>= 2;  dzde >>= 2;
  dwdx >>= 2;  dwde >>= 2;

#define XSCALE(x) ((float)(x)/(1<<18))
#define YSCALE(y) ((float)(y)/(1<<2))
#define ZSCALE(z) ((rdp.zsrc == 1)? (float)(rdp.prim_depth) : (float)((uint32_t)(z))/0xffff0000)
  //#define WSCALE(w) (rdp.Persp_en? (float(uint32_t(w) + 0x10000)/0xffff0000) : 1.0f)
  //#define WSCALE(w) (rdp.Persp_en? 4294901760.0/(w + 65536) : 1.0f)
#define WSCALE(w) (rdp.Persp_en? 65536.0f/ (float)((w+ 0xffff)>>16) : 1.0f)
#define CSCALE(c) (((c)>0x3ff0000? 0x3ff0000:((c)<0? 0 : (c)))>>18)
#define _PERSP(w) ( w )
#define PERSP(s, w) ( ((int64_t)(s) << 20) / (_PERSP(w)? _PERSP(w):1) )
#define SSCALE(s, _w) (rdp.Persp_en? (float)(PERSP(s, _w))/(1 << 10) : (float)(s)/(1<<21))
#define TSCALE(s, w) (rdp.Persp_en? (float)(PERSP(s, w))/(1 << 10) : (float)(s)/(1<<21))

  int nbVtxs = 0;
  VERTEX vtxbuf[12];
  VERTEX * vtx = &vtxbuf[nbVtxs++];

  xleft = xm;
  xright = xh;
  xleft_inc = dxmdy;
  xright_inc = dxhdy;

  while (yh<ym &&
    !((!flip && xleft < xright+0x10000) ||
    (flip && xleft > xright-0x10000))) {
      xleft += xleft_inc;
      xright += xright_inc;
      s += dsde;    t += dtde;    w += dwde;
      r += drde;    g += dgde;    b += dbde;    a += dade;
      z += dzde;
      yh++;
  }

  j = ym-yh;
  if (j > 0)
  {
    int dx = (xleft-xright)>>16;
    if ((!flip && xleft < xright) ||
      (flip/* && xleft > xright*/))
    {
      if (shade) {
        vtx->r = CSCALE(r+drdx*dx);
        vtx->g = CSCALE(g+dgdx*dx);
        vtx->b = CSCALE(b+dbdx*dx);
        vtx->a = CSCALE(a+dadx*dx);
      }
      if (texture) {
        vtx->ou = SSCALE(s+dsdx*dx, w+dwdx*dx);
        vtx->ov = TSCALE(t+dtdx*dx, w+dwdx*dx);
      }
      vtx->x = XSCALE(xleft);
      vtx->y = YSCALE(yh);
      vtx->z = ZSCALE(z+dzdx*dx);
      vtx->w = WSCALE(w+dwdx*dx);
      vtx = &vtxbuf[nbVtxs++];
    }
    if ((!flip/* && xleft < xright*/) ||
      (flip && xleft > xright))
    {
      if (shade) {
        vtx->r = CSCALE(r);
        vtx->g = CSCALE(g);
        vtx->b = CSCALE(b);
        vtx->a = CSCALE(a);
      }
      if (texture) {
        vtx->ou = SSCALE(s, w);
        vtx->ov = TSCALE(t, w);
      }
      vtx->x = XSCALE(xright);
      vtx->y = YSCALE(yh);
      vtx->z = ZSCALE(z);
      vtx->w = WSCALE(w);
      vtx = &vtxbuf[nbVtxs++];
    }
    xleft += xleft_inc*j;  xright += xright_inc*j;
    s += dsde*j;  t += dtde*j;
    if (w + dwde*j) w += dwde*j;
    else w += dwde*(j-1);
    r += drde*j;  g += dgde*j;  b += dbde*j;  a += dade*j;
    z += dzde*j;
    // render ...
  }

  if (xl != xh)
    xleft = xl;

  //if (yl-ym > 0)
  {
    int dx = (xleft-xright)>>16;
    if ((!flip && xleft <= xright) ||
      (flip/* && xleft >= xright*/))
    {
      if (shade) {
        vtx->r = CSCALE(r+drdx*dx);
        vtx->g = CSCALE(g+dgdx*dx);
        vtx->b = CSCALE(b+dbdx*dx);
        vtx->a = CSCALE(a+dadx*dx);
      }
      if (texture) {
        vtx->ou = SSCALE(s+dsdx*dx, w+dwdx*dx);
        vtx->ov = TSCALE(t+dtdx*dx, w+dwdx*dx);
      }
      vtx->x = XSCALE(xleft);
      vtx->y = YSCALE(ym);
      vtx->z = ZSCALE(z+dzdx*dx);
      vtx->w = WSCALE(w+dwdx*dx);
      vtx = &vtxbuf[nbVtxs++];
    }
    if ((!flip/* && xleft <= xright*/) ||
      (flip && xleft >= xright))
    {
      if (shade) {
        vtx->r = CSCALE(r);
        vtx->g = CSCALE(g);
        vtx->b = CSCALE(b);
        vtx->a = CSCALE(a);
      }
      if (texture) {
        vtx->ou = SSCALE(s, w);
        vtx->ov = TSCALE(t, w);
      }
      vtx->x = XSCALE(xright);
      vtx->y = YSCALE(ym);
      vtx->z = ZSCALE(z);
      vtx->w = WSCALE(w);
      vtx = &vtxbuf[nbVtxs++];
    }
  }
  xleft_inc = dxldy;
  xright_inc = dxhdy;

  j = yl-ym;
  //j--; // ?
  xleft += xleft_inc*j;  xright += xright_inc*j;
  s += dsde*j;  t += dtde*j;  w += dwde*j;
  r += drde*j;  g += dgde*j;  b += dbde*j;  a += dade*j;
  z += dzde*j;

  while (yl>ym &&
    !((!flip && xleft < xright+0x10000) ||
    (flip && xleft > xright-0x10000))) {
      xleft -= xleft_inc;    xright -= xright_inc;
      s -= dsde;    t -= dtde;    w -= dwde;
      r -= drde;    g -= dgde;    b -= dbde;    a -= dade;
      z -= dzde;
      j--;
      yl--;
  }

  // render ...
  if (j >= 0) {
    int dx = (xleft-xright)>>16;
    if ((!flip && xleft <= xright) ||
      (flip/* && xleft >= xright*/))
    {
      if (shade) {
        vtx->r = CSCALE(r+drdx*dx);
        vtx->g = CSCALE(g+dgdx*dx);
        vtx->b = CSCALE(b+dbdx*dx);
        vtx->a = CSCALE(a+dadx*dx);
      }
      if (texture) {
        vtx->ou = SSCALE(s+dsdx*dx, w+dwdx*dx);
        vtx->ov = TSCALE(t+dtdx*dx, w+dwdx*dx);
      }
      vtx->x = XSCALE(xleft);
      vtx->y = YSCALE(yl);
      vtx->z = ZSCALE(z+dzdx*dx);
      vtx->w = WSCALE(w+dwdx*dx);
      vtx = &vtxbuf[nbVtxs++];
    }
    if ((!flip/* && xleft <= xright*/) ||
      (flip && xleft >= xright))
    {
      if (shade) {
        vtx->r = CSCALE(r);
        vtx->g = CSCALE(g);
        vtx->b = CSCALE(b);
        vtx->a = CSCALE(a);
      }
      if (texture) {
        vtx->ou = SSCALE(s, w);
        vtx->ov = TSCALE(t, w);
      }
      vtx->x = XSCALE(xright);
      vtx->y = YSCALE(yl);
      vtx->z = ZSCALE(z);
      vtx->w = WSCALE(w);
      vtx = &vtxbuf[nbVtxs++];
    }
  }

  {
     int k;
    update ();
    for (k = 0; k < nbVtxs-1; k++)
    {
      VERTEX * v = &vtxbuf[k];
      v->x = v->x * rdp.scale_x + rdp.offset_x;
      v->y = v->y * rdp.scale_y + rdp.offset_y;
      //    v->z = 1.0f;///v->w;
      v->q = 1.0f/v->w;
      v->u1 = v->u0 = v->ou;
      v->v1 = v->v0 = v->ov;
      if (rdp.tex >= 1 && rdp.cur_cache[0])
      {
        if (rdp.tiles[rdp.cur_tile].shift_s)
        {
          if (rdp.tiles[rdp.cur_tile].shift_s > 10)
            v->u0 *= (float)(1 << (16 - rdp.tiles[rdp.cur_tile].shift_s));
          else
            v->u0 /= (float)(1 << rdp.tiles[rdp.cur_tile].shift_s);
        }
        if (rdp.tiles[rdp.cur_tile].shift_t)
        {
          if (rdp.tiles[rdp.cur_tile].shift_t > 10)
            v->v0 *= (float)(1 << (16 - rdp.tiles[rdp.cur_tile].shift_t));
          else
            v->v0 /= (float)(1 << rdp.tiles[rdp.cur_tile].shift_t);
        }

        v->u0 -= rdp.tiles[rdp.cur_tile].f_ul_s;
        v->v0 -= rdp.tiles[rdp.cur_tile].f_ul_t;
        v->u0 = rdp.cur_cache[0]->c_off + rdp.cur_cache[0]->c_scl_x * v->u0;
        v->v0 = rdp.cur_cache[0]->c_off + rdp.cur_cache[0]->c_scl_y * v->v0;
        v->u0 /= v->w;
        v->v0 /= v->w;
      }

      if (rdp.tex >= 2 && rdp.cur_cache[1])
      {
        if (rdp.tiles[rdp.cur_tile+1].shift_s)
        {
          if (rdp.tiles[rdp.cur_tile+1].shift_s > 10)
            v->u1 *= (float)(1 << (16 - rdp.tiles[rdp.cur_tile+1].shift_s));
          else
            v->u1 /= (float)(1 << rdp.tiles[rdp.cur_tile+1].shift_s);
        }
        if (rdp.tiles[rdp.cur_tile+1].shift_t)
        {
          if (rdp.tiles[rdp.cur_tile+1].shift_t > 10)
            v->v1 *= (float)(1 << (16 - rdp.tiles[rdp.cur_tile+1].shift_t));
          else
            v->v1 /= (float)(1 << rdp.tiles[rdp.cur_tile+1].shift_t);
        }

        v->u1 -= rdp.tiles[rdp.cur_tile+1].f_ul_s;
        v->v1 -= rdp.tiles[rdp.cur_tile+1].f_ul_t;
        v->u1 = rdp.cur_cache[1]->c_off + rdp.cur_cache[1]->c_scl_x * v->u1;
        v->v1 = rdp.cur_cache[1]->c_off + rdp.cur_cache[1]->c_scl_y * v->v1;
        v->u1 /= v->w;
        v->v1 /= v->w;
      }
      apply_shade_mods (v);
    }
    ConvertCoordsConvert (vtxbuf, nbVtxs);
    grCullMode (GR_CULL_DISABLE);
    grDrawVertexArrayContiguous (GR_TRIANGLE_STRIP, nbVtxs-1, vtxbuf, sizeof(VERTEX));
  }
}

static void rdp_triangle(int shade, int texture, int zbuffer)
{
  lle_triangle(rdp.cmd0, rdp.cmd1, shade, texture, zbuffer, rdp_cmd_data + rdp_cmd_cur);
}

static void rdp_trifill()
{
  lle_triangle(rdp.cmd0, rdp.cmd1, 0, 0, 0, rdp_cmd_data + rdp_cmd_cur);
  LRDP("trifill\n");
}

static void rdp_trishade()
{
  lle_triangle(rdp.cmd0, rdp.cmd1, 1, 0, 0, rdp_cmd_data + rdp_cmd_cur);
  LRDP("trishade\n");
}

static void rdp_tritxtr()
{
  lle_triangle(rdp.cmd0, rdp.cmd1, 0, 1, 0, rdp_cmd_data + rdp_cmd_cur);
  LRDP("tritxtr\n");
}

static void rdp_trishadetxtr()
{
  lle_triangle(rdp.cmd0, rdp.cmd1, 1, 1, 0, rdp_cmd_data + rdp_cmd_cur);
  LRDP("trishadetxtr\n");
}

static void rdp_trifillz()
{
  lle_triangle(rdp.cmd0, rdp.cmd1, 0, 0, 1, rdp_cmd_data + rdp_cmd_cur);
  LRDP("trifillz\n");
}

static void rdp_trishadez()
{
  lle_triangle(rdp.cmd0, rdp.cmd1, 1, 0, 1, rdp_cmd_data + rdp_cmd_cur);
  LRDP("trishadez\n");
}

static void rdp_tritxtrz()
{
   lle_triangle(rdp.cmd0, rdp.cmd1, 0, 1, 1, rdp_cmd_data + rdp_cmd_cur);
   LRDP("tritxtrz\n");
}

static void rdp_trishadetxtrz()
{
   lle_triangle(rdp.cmd0, rdp.cmd1, 1, 1, 1, rdp_cmd_data + rdp_cmd_cur);
   LRDP("trishadetxtrz\n");
}

static rdp_instr rdp_command_table[64] =
{
   /* 0x00 */
   spnoop,             undef,                  undef,                  undef,
   undef,              undef,                  undef,                  undef,
   rdp_trifill,        rdp_trifillz,           rdp_tritxtr,            rdp_tritxtrz,
   rdp_trishade,       rdp_trishadez,          rdp_trishadetxtr,       rdp_trishadetxtrz,
   /* 0x10 */
   undef,              undef,                  undef,                  undef,
   undef,              undef,                  undef,                  undef,
   undef,              undef,                  undef,                  undef,
   undef,              undef,                  undef,                  undef,
   /* 0x20 */
   undef,              undef,                  undef,                  undef,
   rdp_texrect,        rdp_texrect,            rdp_loadsync,           rdp_pipesync,
   rdp_tilesync,       rdp_fullsync,           rdp_setkeygb,           rdp_setkeyr,
   rdp_setconvert,     rdp_setscissor,         rdp_setprimdepth,       rdp_setothermode,
   /* 0x30 */
   rdp_loadtlut,           undef,                  rdp_settilesize,        rdp_loadblock,
   rdp_loadtile,           rdp_settile,            rdp_fillrect,           rdp_setfillcolor,
   rdp_setfogcolor,        rdp_setblendcolor,      rdp_setprimcolor,       rdp_setenvcolor,
   rdp_setcombine,         rdp_settextureimage,    rdp_setdepthimage,      rdp_setcolorimage
};

static const uint32_t rdp_command_length[64] =
{
   8,                      // 0x00, No Op
   8,                      // 0x01, ???
   8,                      // 0x02, ???
   8,                      // 0x03, ???
   8,                      // 0x04, ???
   8,                      // 0x05, ???
   8,                      // 0x06, ???
   8,                      // 0x07, ???
   32,                     // 0x08, Non-Shaded Triangle
   32+16,                  // 0x09, Non-Shaded, Z-Buffered Triangle
   32+64,                  // 0x0a, Textured Triangle
   32+64+16,               // 0x0b, Textured, Z-Buffered Triangle
   32+64,                  // 0x0c, Shaded Triangle
   32+64+16,               // 0x0d, Shaded, Z-Buffered Triangle
   32+64+64,               // 0x0e, Shaded+Textured Triangle
   32+64+64+16,            // 0x0f, Shaded+Textured, Z-Buffered Triangle
   8,                      // 0x10, ???
   8,                      // 0x11, ???
   8,                      // 0x12, ???
   8,                      // 0x13, ???
   8,                      // 0x14, ???
   8,                      // 0x15, ???
   8,                      // 0x16, ???
   8,                      // 0x17, ???
   8,                      // 0x18, ???
   8,                      // 0x19, ???
   8,                      // 0x1a, ???
   8,                      // 0x1b, ???
   8,                      // 0x1c, ???
   8,                      // 0x1d, ???
   8,                      // 0x1e, ???
   8,                      // 0x1f, ???
   8,                      // 0x20, ???
   8,                      // 0x21, ???
   8,                      // 0x22, ???
   8,                      // 0x23, ???
   16,                     // 0x24, Texture_Rectangle
   16,                     // 0x25, Texture_Rectangle_Flip
   8,                      // 0x26, Sync_Load
   8,                      // 0x27, Sync_Pipe
   8,                      // 0x28, Sync_Tile
   8,                      // 0x29, Sync_Full
   8,                      // 0x2a, Set_Key_GB
   8,                      // 0x2b, Set_Key_R
   8,                      // 0x2c, Set_Convert
   8,                      // 0x2d, Set_Scissor
   8,                      // 0x2e, Set_Prim_Depth
   8,                      // 0x2f, Set_Other_Modes
   8,                      // 0x30, Load_TLUT
   8,                      // 0x31, ???
   8,                      // 0x32, Set_Tile_Size
   8,                      // 0x33, Load_Block
   8,                      // 0x34, Load_Tile
   8,                      // 0x35, Set_Tile
   8,                      // 0x36, Fill_Rectangle
   8,                      // 0x37, Set_Fill_Color
   8,                      // 0x38, Set_Fog_Color
   8,                      // 0x39, Set_Blend_Color
   8,                      // 0x3a, Set_Prim_Color
   8,                      // 0x3b, Set_Env_Color
   8,                      // 0x3c, Set_Combine
   8,                      // 0x3d, Set_Texture_Image
   8,                      // 0x3e, Set_Mask_Image
   8                       // 0x3f, Set_Color_Image
};

#define rdram ((uint32_t*)gfx.RDRAM)
#define rsp_dmem ((uint32_t*)gfx.DMEM)

#define dp_start (*(uint32_t*)gfx.DPC_START_REG)
#define dp_end (*(uint32_t*)gfx.DPC_END_REG)
#define dp_current (*(uint32_t*)gfx.DPC_CURRENT_REG)
#define dp_status (*(uint32_t*)gfx.DPC_STATUS_REG)

// dp_status & 0x1 = XBUS_DMEM_DMA enabled
#define READ_RDP_DATA(address) ((dp_status & 0x1) ? (rsp_dmem[((address) & 0xfff)>>2]) : (rdram[(address) >> 2]))

static void rdphalf_1(void)
{
   uint32_t cmd = rdp.cmd1 >> 24;
   if (cmd >= 0xc8 && cmd <=0xcf) //triangle command
   {
      LRDP("rdphalf_1 - lle triangle\n");
      rdp_cmd_ptr = 0;
      rdp_cmd_cur = 0;
      uint32_t a;

      do
      {
         rdp_cmd_data[rdp_cmd_ptr++] = rdp.cmd1;
         // check DL counter
         if (rdp.dl_count != -1)
         {
            rdp.dl_count --;
            if (rdp.dl_count == 0)
            {
               rdp.dl_count = -1;

               LRDP("End of DL\n");
               rdp.pc_i --;
            }
         }

         // Get the address of the next command
         a = rdp.pc[rdp.pc_i] & BMASK;

         // Load the next command and its input
         rdp.cmd0 = ((uint32_t*)gfx.RDRAM)[a>>2];   // \ Current command, 64 bit
         rdp.cmd1 = ((uint32_t*)gfx.RDRAM)[(a>>2)+1]; // /

         // Go to the next instruction
         rdp.pc[rdp.pc_i] = (a+8) & BMASK;

      }while ((rdp.cmd0 >> 24) != 0xb3);
      rdp_cmd_data[rdp_cmd_ptr++] = rdp.cmd1;
      cmd = (rdp_cmd_data[rdp_cmd_cur] >> 24) & 0x3f;
      rdp.cmd0 = rdp_cmd_data[rdp_cmd_cur+0];
      rdp.cmd1 = rdp_cmd_data[rdp_cmd_cur+1];
      /*
         uint32_t cmd3 = ((uint32_t*)gfx.RDRAM)[(a>>2)+2];
         if ((cmd3>>24) == 0xb4)
         rglSingleTriangle = true;
         else
         rglSingleTriangle = false;
         */
      rdp_command_table[cmd]();
   }
   else
      LRDP("rdphalf_1 - IGNORED\n");
}

static void rdphalf_2(void)
{
   RDP_E("rdphalf_2 - IGNORED\n");
   LRDP("rdphalf_2 - IGNORED\n");
}

static void rdphalf_cont(void)
{
   RDP_E("rdphalf_cont - IGNORED\n");
   LRDP("rdphalf_cont - IGNORED\n");
}

/******************************************************************
Function: ProcessRDPList
Purpose:  This function is called when there is a Dlist to be
processed. (Low level GFX list)
input:    none
output:   none
*******************************************************************/
EXPORT void CALL ProcessRDPList(void)
{
#ifdef VISUAL_LOGGING
   VLOG ("ProcessRDPList ()\n");
   LRDP("ProcessRDPList ()\n");
#endif

   uint32_t i;
   uint32_t cmd, length, cmd_length;
   rdp_cmd_ptr = 0;
   rdp_cmd_cur = 0;

   if (dp_end <= dp_current) return;
   length = dp_end - dp_current;

   // load command data
   for (i=0; i < length; i += 4)
   {
      rdp_cmd_data[rdp_cmd_ptr++] = READ_RDP_DATA(dp_current + i);
#ifdef VISUAL_LOGGING
      if (rdp_cmd_ptr >= 0x1000)
      {
         FRDP("rdp_process_list: rdp_cmd_ptr overflow %x %x --> %x\n", length, dp_current, dp_end);
      }
#endif
   }

   dp_current = dp_end;

   cmd = (rdp_cmd_data[0] >> 24) & 0x3f;
   cmd_length = (rdp_cmd_ptr + 1) * 4;

   // check if more data is needed
   if (cmd_length < rdp_command_length[cmd])
      return;
   rdp.LLE = true;
   while (rdp_cmd_cur < rdp_cmd_ptr)
   {
      cmd = (rdp_cmd_data[rdp_cmd_cur] >> 24) & 0x3f;

      if (((rdp_cmd_ptr-rdp_cmd_cur) * 4) < rdp_command_length[cmd])
         return;

      // execute the command
      rdp.cmd0 = rdp_cmd_data[rdp_cmd_cur+0];
      rdp.cmd1 = rdp_cmd_data[rdp_cmd_cur+1];
      rdp.cmd2 = rdp_cmd_data[rdp_cmd_cur+2];
      rdp.cmd3 = rdp_cmd_data[rdp_cmd_cur+3];
      rdp_command_table[cmd]();

      rdp_cmd_cur += rdp_command_length[cmd] / 4;
   };

   rdp.LLE = false;
   dp_start = dp_end;
   dp_status &= ~0x0002;

   //}
}

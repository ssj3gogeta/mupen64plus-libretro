/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rom.c                                                   *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Tillin9                                            *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define M64P_CORE_PROTOTYPES 1
#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "api/config.h"
#include "api/m64p_config.h"

#include "md5.h"
#include "rom.h"
#include "main.h"
#include "util.h"

#include "memory/memory.h"
#include "osal/preproc.h"

#include "../r4300/r4300.h"

#define DEFAULT 16

#define CHUNKSIZE 1024*128 /* Read files 128KB at a time. */

static romdatabase_entry* ini_search_by_md5(md5_byte_t* md5);

static _romdatabase g_romdatabase;

/* Global loaded rom memory space. */
unsigned char* rom = NULL;
/* Global loaded rom size. */
int rom_size = 0;

unsigned char isGoldeneyeRom = 0;

m64p_rom_header   ROM_HEADER;
rom_params        ROM_PARAMS;
m64p_rom_settings ROM_SETTINGS;

static m64p_system_type rom_country_code_to_system_type(unsigned short country_code);
static int rom_system_type_to_ai_dac_rate(m64p_system_type system_type);
static int rom_system_type_to_vi_limit(m64p_system_type system_type);

/* Tests if a file is a valid N64 rom by checking the first 4 bytes. */
static int is_valid_rom(const unsigned char *buffer)
{
    /* Test if rom is a native .z64 image with header 0x80371240. [ABCD] */
    if((buffer[0]==0x80)&&(buffer[1]==0x37)&&(buffer[2]==0x12)&&(buffer[3]==0x40))
        return 1;
    /* Test if rom is a byteswapped .v64 image with header 0x37804012. [BADC] */
    else if((buffer[0]==0x37)&&(buffer[1]==0x80)&&(buffer[2]==0x40)&&(buffer[3]==0x12))
        return 1;
    /* Test if rom is a wordswapped .n64 image with header  0x40123780. [DCBA] */
    else if((buffer[0]==0x40)&&(buffer[1]==0x12)&&(buffer[2]==0x37)&&(buffer[3]==0x80))
        return 1;
    else
        return 0;
}

/* If rom is a .v64 or .n64 image, byteswap or wordswap loadlength amount of
 * rom data to native .z64 before forwarding. Makes sure that data extraction
 * and MD5ing routines always deal with a .z64 image.
 */
static void swap_rom(unsigned char* localrom, unsigned char* imagetype, int loadlength)
{
    unsigned char temp;
    int i;

    /* Btyeswap if .v64 image. */
    if(localrom[0]==0x37)
        {
        *imagetype = V64IMAGE;
        for (i = 0; i < loadlength; i+=2)
            {
            temp=localrom[i];
            localrom[i]=localrom[i+1];
            localrom[i+1]=temp;
            }
        }
    /* Wordswap if .n64 image. */
    else if(localrom[0]==0x40)
        {
        *imagetype = N64IMAGE;
        for (i = 0; i < loadlength; i+=4)
            {
            temp=localrom[i];
            localrom[i]=localrom[i+3];
            localrom[i+3]=temp;
            temp=localrom[i+1];
            localrom[i+1]=localrom[i+2];
            localrom[i+2]=temp;
            }
        }
    else
        *imagetype = Z64IMAGE;
}

m64p_error open_rom(const unsigned char* romimage, unsigned int size)
{
    md5_state_t state;
    md5_byte_t digest[16];
    romdatabase_entry* entry;
    char buffer[256];
    unsigned char imagetype;
    int i;

    /* check input requirements */
    if (rom != NULL)
    {
        DebugMessage(M64MSG_ERROR, "open_rom(): previous ROM image was not freed");
        return M64ERR_INTERNAL;
    }
    if (romimage == NULL || !is_valid_rom(romimage))
    {
        DebugMessage(M64MSG_ERROR, "open_rom(): not a valid ROM image");
        return M64ERR_INPUT_INVALID;
    }

    /* Clear Byte-swapped flag, since ROM is now deleted. */
    g_MemHasBeenBSwapped = 0;
    /* allocate new buffer for ROM and copy into this buffer */
    rom_size = size;
    rom = (unsigned char *) malloc(size);
    if (rom == NULL)
        return M64ERR_NO_MEMORY;
    memcpy(rom, romimage, size);
    swap_rom(rom, &imagetype, rom_size);

    memcpy(&ROM_HEADER, rom, sizeof(m64p_rom_header));

    /* Calculate MD5 hash  */
    md5_init(&state);
    md5_append(&state, (const md5_byte_t*)rom, rom_size);
    md5_finish(&state, digest);
    for ( i = 0; i < 16; ++i )
        sprintf(buffer+i*2, "%02X", digest[i]);
    buffer[32] = '\0';
    strcpy(ROM_SETTINGS.MD5, buffer);

    /* add some useful properties to ROM_PARAMS */
    ROM_PARAMS.systemtype = rom_country_code_to_system_type(ROM_HEADER.Country_code);
    ROM_PARAMS.vilimit = rom_system_type_to_vi_limit(ROM_PARAMS.systemtype);
    ROM_PARAMS.aidacrate = rom_system_type_to_ai_dac_rate(ROM_PARAMS.systemtype);

    memcpy(ROM_PARAMS.headername, ROM_HEADER.Name, 20);
    ROM_PARAMS.headername[20] = '\0';
    trim(ROM_PARAMS.headername); /* Remove trailing whitespace from ROM name. */

    if(
             sl(ROM_HEADER.CRC1) == 0xC2E9AA9A && sl(ROM_HEADER.CRC2) == 0x475D70AA
          || sl(ROM_HEADER.CRC1) == 0xC9176D39 && sl(ROM_HEADER.CRC2) == 0xEA4779D1
          || sl(ROM_HEADER.CRC1) == 0x155B7CDF && sl(ROM_HEADER.CRC2) == 0xF0DA7325
      )
    {
       strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
       ROM_SETTINGS.savetype = EEPROM_16KB;
       ROM_SETTINGS.players = 1;
       DebugMessage(M64MSG_INFO, "Banjo Tooie INI patches applied.");
    }
    else if(
             (sl(ROM_HEADER.CRC1) == 0x2337d8e8 && sl(ROM_HEADER.CRC2) == 0x6b8e7cec)
             || (sl(ROM_HEADER.CRC1) == 0x2DCFCA60 && sl(ROM_HEADER.CRC2) == 0x8354B147)
             || (sl(ROM_HEADER.CRC1) == 0xD3F97D49 && sl(ROM_HEADER.CRC2) == 0x6924135B)
          )
    {
       strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
       ROM_SETTINGS.savetype = EEPROM_16KB;
       ROM_SETTINGS.players = 1;
       DebugMessage(M64MSG_INFO, "Yoshi's Story INI patches applied.");
    }
    else if(
          (sl(ROM_HEADER.CRC1) == 0xFEE97010 && sl(ROM_HEADER.CRC2) == 0x4E94A9A0)
          || (sl(ROM_HEADER.CRC1) == 0x2500267E && sl(ROM_HEADER.CRC2) == 0x2A7EC3CE)
          )
    {
       strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
       ROM_SETTINGS.savetype = EEPROM_16KB;
       ROM_SETTINGS.players = 4;
       ROM_SETTINGS.rumble = 1;
       DebugMessage(M64MSG_INFO, "RR64 Ridge Racer 64 INI patches applied.");
    }
    else if(
          (sl(ROM_HEADER.CRC1) == 0x839F3AD5 && sl(ROM_HEADER.CRC2) == 0x406D15FA)
          || (sl(ROM_HEADER.CRC1) == 0x5001CF4F  && sl(ROM_HEADER.CRC2) == 0xF30CB3BD)
          || (sl(ROM_HEADER.CRC1) == 0x3A6C42B5  && sl(ROM_HEADER.CRC2) == 0x1ACADA1B)
          )
    {
       strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
       ROM_SETTINGS.savetype = EEPROM_16KB;
       ROM_SETTINGS.players = 4;
       DebugMessage(M64MSG_INFO, "Mario Tennis INI patches applied.");
    }
    else if(
          (sl(ROM_HEADER.CRC1) == 0x147E0EDB && sl(ROM_HEADER.CRC2) == 0x36C5B12C)
          || (sl(ROM_HEADER.CRC1) == 0xAC000A2B && sl(ROM_HEADER.CRC2) == 0x38E3A55C)
          )
    {
       strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
       ROM_SETTINGS.savetype = EEPROM_16KB;
       ROM_SETTINGS.players = 1;
       ROM_SETTINGS.rumble = 1;
       DebugMessage(M64MSG_INFO, "Neon Genesis Evangelion INI patches applied.");
    }
    /* Look up this ROM in the .ini file and fill in goodname, etc */
    else if ((entry=ini_search_by_md5(digest)) != NULL ||
        (entry=ini_search_by_crc(sl(ROM_HEADER.CRC1),sl(ROM_HEADER.CRC2))) != NULL)
    {
        strncpy(ROM_SETTINGS.goodname, entry->goodname, 255);
        ROM_SETTINGS.goodname[255] = '\0';
        ROM_SETTINGS.savetype = entry->savetype;
        ROM_SETTINGS.status = entry->status;
        ROM_SETTINGS.players = entry->players;
        ROM_SETTINGS.rumble = entry->rumble;
    }
    else
    {
        strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
        strcat(ROM_SETTINGS.goodname, " (unknown rom)");
        ROM_SETTINGS.savetype = NONE;
        ROM_SETTINGS.status = 0;
        ROM_SETTINGS.players = 0;
        ROM_SETTINGS.rumble = 0;
    }

    /* count_per_op tweaks */
    // see - https://github.com/paulscode/mupen64plus-ae/commit/5d5ff6af92d035eb66ee281239b9f10c9ce0e866
    if(
             (sl(ROM_HEADER.CRC1) == 0xB58B8CD  && sl(ROM_HEADER.CRC2) == 0xB7B291D2) /* Body Harvest */
          || (sl(ROM_HEADER.CRC1) == 0x6F66B92D && sl(ROM_HEADER.CRC2) == 0x80B9E520) /* Body Harvest */
          || (sl(ROM_HEADER.CRC1) == 0x5326696F && sl(ROM_HEADER.CRC2) == 0xFE9A99C3) /* Body Harvest */
          || (sl(ROM_HEADER.CRC1) == 0xC535091F && sl(ROM_HEADER.CRC2) == 0xD60CCF6C) /* Body Harvest */
          || (sl(ROM_HEADER.CRC1) == 0xA46EE3   && sl(ROM_HEADER.CRC2) == 0x554158C6) /* Body Harvest */
          || (sl(ROM_HEADER.CRC1) == 0x95B2B30B   && sl(ROM_HEADER.CRC2) == 0x2B6415C1) /* Hexen (E) [!] */
          || (sl(ROM_HEADER.CRC1) == 0x9AB3B50A   && sl(ROM_HEADER.CRC2) == 0xBC666105) /* Hexen (G) [!] */
          || (sl(ROM_HEADER.CRC1) == 0x9CAB6AEA   && sl(ROM_HEADER.CRC2) == 0x87C61C00) /* Hexen (U) [!] */
          || (sl(ROM_HEADER.CRC1) == 0xE6BA0A06   && sl(ROM_HEADER.CRC2) == 0x8A3D2C2F) /* Hexen (U) [t1] */
          || (sl(ROM_HEADER.CRC1) == 0x7EAE2488   && sl(ROM_HEADER.CRC2) == 0x9D40A35A) /* Biohazard 2 (J) [!] */
          || (sl(ROM_HEADER.CRC1) == 0x9B500E8E   && sl(ROM_HEADER.CRC2) == 0xE90550B3) /* Resident Evil 2 (E) (M2) [!] */
          || (sl(ROM_HEADER.CRC1) == 0xAA18B1A5   && sl(ROM_HEADER.CRC2) == 0x7DB6AEB)  /* Resident Evil 2 (U) [!] */
          )
    {
       count_per_op = 1;
    }

    if(
          (sl(ROM_HEADER.CRC1) == 0x6AA4DDE7  && sl(ROM_HEADER.CRC2) == 0xE3E2F4E7) /* BattleTanx (U) [!] */
          || (sl(ROM_HEADER.CRC1) == 0x3D615CF5 && sl(ROM_HEADER.CRC2) == 0x6984930A)  /* BattleTanx (U) [b1][t1] [!] */
          || (sl(ROM_HEADER.CRC1) == 0x9A75C9C2 && sl(ROM_HEADER.CRC2) == 0xA4488353)  /* BattleTanx (U) [f1] (PAL) */
          || (sl(ROM_HEADER.CRC1) == 0x535DF3E2 && sl(ROM_HEADER.CRC2) == 0x609789F1)  /* Wave Race 64 - Shindou Edition (J) (V1.2) [!] */
      )
    {
       count_per_op = 3;
    }

    delay_si = 1; /* default */

    if(
             sl(ROM_HEADER.CRC1) == 0xC2E9AA9A && sl(ROM_HEADER.CRC2) == 0x475D70AA
          || sl(ROM_HEADER.CRC1) == 0xC9176D39 && sl(ROM_HEADER.CRC2) == 0xEA4779D1
          || sl(ROM_HEADER.CRC1) == 0x155B7CDF && sl(ROM_HEADER.CRC2) == 0xF0DA7325
          //|| (sl(ROM_HEADER.CRC1) == 0xB58B8CD  && sl(ROM_HEADER.CRC2) == 0xB7B291D2) /* Body Harvest */
          //|| (sl(ROM_HEADER.CRC1) == 0x6F66B92D && sl(ROM_HEADER.CRC2) == 0x80B9E520) /* Body Harvest */
          //|| (sl(ROM_HEADER.CRC1) == 0x5326696F && sl(ROM_HEADER.CRC2) == 0xFE9A99C3) /* Body Harvest */
          //|| (sl(ROM_HEADER.CRC1) == 0xC535091F && sl(ROM_HEADER.CRC2) == 0xD60CCF6C) /* Body Harvest */
          //|| (sl(ROM_HEADER.CRC1) == 0xA46EE3   && sl(ROM_HEADER.CRC2) == 0x554158C6) /* Body Harvest */
      )
    {
       delay_si = 0;
    }

    /* print out a bunch of info about the ROM */
    DebugMessage(M64MSG_INFO, "Goodname: %s", ROM_SETTINGS.goodname);
    DebugMessage(M64MSG_INFO, "Headername: %s", ROM_PARAMS.headername);
    DebugMessage(M64MSG_INFO, "Name: %s", ROM_HEADER.Name);
    imagestring(imagetype, buffer);
    DebugMessage(M64MSG_INFO, "MD5: %s", ROM_SETTINGS.MD5);
    DebugMessage(M64MSG_INFO, "CRC: %x %x", sl(ROM_HEADER.CRC1), sl(ROM_HEADER.CRC2));
    DebugMessage(M64MSG_INFO, "Imagetype: %s", buffer);
    DebugMessage(M64MSG_INFO, "Rom size: %d bytes (or %d Mb or %d Megabits)", rom_size, rom_size/1024/1024, rom_size/1024/1024*8);
    DebugMessage(M64MSG_VERBOSE, "ClockRate = %x", sl(ROM_HEADER.ClockRate));
    DebugMessage(M64MSG_INFO, "Version: %x", sl(ROM_HEADER.Release));
    if(sl(ROM_HEADER.Manufacturer_ID) == 'N')
        DebugMessage(M64MSG_INFO, "Manufacturer: Nintendo");
    else
        DebugMessage(M64MSG_INFO, "Manufacturer: %x", sl(ROM_HEADER.Manufacturer_ID));
    DebugMessage(M64MSG_VERBOSE, "Cartridge_ID: %x", ROM_HEADER.Cartridge_ID);
    countrycodestring(ROM_HEADER.Country_code, buffer);
    DebugMessage(M64MSG_INFO, "Country: %s", buffer);
    DebugMessage(M64MSG_VERBOSE, "PC = %x", sl((unsigned int)ROM_HEADER.PC));
    DebugMessage(M64MSG_VERBOSE, "Save type: %d", ROM_SETTINGS.savetype);

    //Prepare Hack for GOLDENEYE
    isGoldeneyeRom = 0;
    if(strcmp(ROM_PARAMS.headername, "GOLDENEYE") == 0)
       isGoldeneyeRom = 1;

    return M64ERR_SUCCESS;
}

m64p_error close_rom(void)
{
    if (rom == NULL)
        return M64ERR_INVALID_STATE;

    free(rom);
    rom = NULL;

    /* Clear Byte-swapped flag, since ROM is now deleted. */
    g_MemHasBeenBSwapped = 0;
    DebugMessage(M64MSG_STATUS, "Rom closed.");

    return M64ERR_SUCCESS;
}

/********************************************************************************************/
/* ROM utility functions */

// Get the system type associated to a ROM country code.
static m64p_system_type rom_country_code_to_system_type(unsigned short country_code)
{
    switch (country_code)
    {
        // PAL codes
        case 0x44:
        case 0x46:
        case 0x49:
        case 0x50:
        case 0x53:
        case 0x55:
        case 0x58:
        case 0x59:
            return SYSTEM_PAL;

        // NTSC codes
        case 0x37:
        case 0x41:
        case 0x45:
        case 0x4a:
        default: // Fallback for unknown codes
            return SYSTEM_NTSC;
    }
}

// Get the VI (vertical interrupt) limit associated to a ROM system type.
static int rom_system_type_to_vi_limit(m64p_system_type system_type)
{
    switch (system_type)
    {
        case SYSTEM_PAL:
        case SYSTEM_MPAL:
            return 50;

        case SYSTEM_NTSC:
        default:
            return 60;
    }
}

static int rom_system_type_to_ai_dac_rate(m64p_system_type system_type)
{
    switch (system_type)
    {
        case SYSTEM_PAL:
            return 49656530;
        case SYSTEM_MPAL:
            return 48628316;
        case SYSTEM_NTSC:
        default:
            return 48681812;
    }
}

/********************************************************************************************/
/* INI Rom database functions */

void romdatabase_open(void)
{
    FILE *fPtr;
    char buffer[256];
    romdatabase_search* search = NULL;
    romdatabase_search** next_search;

    int counter, value, lineno;
    unsigned char index;
    const char *pathname = ConfigGetSharedDataFilepath("mupen64plus.ini");
    DebugMessage(M64MSG_WARNING, "ROM Database: %s", pathname);

    if(g_romdatabase.have_database)
        return;

    /* Open romdatabase. */
    if (pathname == NULL || (fPtr = fopen(pathname, "rb")) == NULL)
    {
        DebugMessage(M64MSG_ERROR, "Unable to open rom database file '%s'.", pathname);
        return;
    }

    g_romdatabase.have_database = 1;

    /* Clear premade indices. */
    for(counter = 0; counter < 255; ++counter)
        g_romdatabase.crc_lists[counter] = NULL;
    for(counter = 0; counter < 255; ++counter)
        g_romdatabase.md5_lists[counter] = NULL;
    g_romdatabase.list = NULL;

    next_search = &g_romdatabase.list;

    /* Parse ROM database file */
    for (lineno = 1; fgets(buffer, 255, fPtr) != NULL; lineno++)
    {
        char *line = buffer;
        ini_line l = ini_parse_line(&line);
        switch (l.type)
        {
        case INI_SECTION:
        {
            md5_byte_t md5[16];
            if (!parse_hex(l.name, md5, 16))
            {
                DebugMessage(M64MSG_WARNING, "ROM Database: Invalid MD5 on line %i", lineno);
                search = NULL;
                continue;
            }

            *next_search = (romdatabase_search*)malloc(sizeof(romdatabase_search));
            search = *next_search;
            next_search = &search->next_entry;

            search->entry.goodname = NULL;
            memcpy(search->entry.md5, md5, 16);
            search->entry.refmd5 = NULL;
            search->entry.crc1 = 0;
            search->entry.crc2 = 0;
            search->entry.status = 0; /* Set default to 0 stars. */
            search->entry.savetype = DEFAULT;
            search->entry.players = DEFAULT;
            search->entry.rumble = DEFAULT; 

            search->next_entry = NULL;
            search->next_crc = NULL;
            /* Index MD5s by first 8 bits. */
            index = search->entry.md5[0];
            search->next_md5 = g_romdatabase.md5_lists[index];
            g_romdatabase.md5_lists[index] = search;

            break;
        }
        case INI_PROPERTY:
            // This happens if there's stray properties before any section,
            // or if some error happened on INI_SECTION (e.g. parsing).
            if (search == NULL)
            {
                DebugMessage(M64MSG_WARNING, "ROM Database: Ignoring property on line %i", lineno);
                continue;
            }
            if(!strcmp(l.name, "GoodName"))
            {
                search->entry.goodname = strdup(l.value);
            }
            else if(!strcmp(l.name, "CRC"))
            {
                char garbage_sweeper;
                if (sscanf(l.value, "%X %X%c", &search->entry.crc1,
                    &search->entry.crc2, &garbage_sweeper) == 2)
                {
                    /* Index CRCs by first 8 bits. */
                    index = search->entry.crc1 >> 24;
                    search->next_crc = g_romdatabase.crc_lists[index];
                    g_romdatabase.crc_lists[index] = search;
                }
                else
                {
                    search->entry.crc1 = search->entry.crc2 = 0;
                    DebugMessage(M64MSG_WARNING, "ROM Database: Invalid CRC on line %i", lineno);
                }
            }
            else if(!strcmp(l.name, "RefMD5"))
            {
                md5_byte_t md5[16];
                if (parse_hex(l.value, md5, 16))
                {
                    search->entry.refmd5 = (md5_byte_t*)malloc(16*sizeof(md5_byte_t));
                    memcpy(search->entry.refmd5, md5, 16);
                }
                else
                    DebugMessage(M64MSG_WARNING, "ROM Database: Invalid RefMD5 on line %i", lineno);
            }
            else if(!strcmp(l.name, "SaveType"))
            {
                if(!strcmp(l.value, "Eeprom 4KB"))
                    search->entry.savetype = EEPROM_4KB;
                else if(!strcmp(l.value, "Eeprom 16KB"))
                    search->entry.savetype = EEPROM_16KB;
                else if(!strcmp(l.value, "SRAM"))
                    search->entry.savetype = SRAM;
                else if(!strcmp(l.value, "Flash RAM"))
                    search->entry.savetype = FLASH_RAM;
                else if(!strcmp(l.value, "Controller Pack"))
                    search->entry.savetype = CONTROLLER_PACK;
                else if(!strcmp(l.value, "None"))
                    search->entry.savetype = NONE;
                else
                    DebugMessage(M64MSG_WARNING, "ROM Database: Invalid save type on line %i", lineno);
            }
            else if(!strcmp(l.name, "Status"))
            {
                if (string_to_int(l.value, &value) && value >= 0 && value < 6)
                    search->entry.status = value;
                else
                    DebugMessage(M64MSG_WARNING, "ROM Database: Invalid status on line %i", lineno);
            }
            else if(!strcmp(l.name, "Players"))
            {
                if (string_to_int(l.value, &value) && value >= 0 && value < 8)
                    search->entry.players = value;
                else
                    DebugMessage(M64MSG_WARNING, "ROM Database: Invalid player count on line %i", lineno);
            }
            else if(!strcmp(l.name, "Rumble"))
            {
                if(!strcmp(l.value, "Yes"))
                    search->entry.rumble = 1;
                else if(!strcmp(l.value, "No"))
                    search->entry.rumble = 0;
                else
                    DebugMessage(M64MSG_WARNING, "ROM Database: Invalid rumble string on line %i", lineno);
            }
            else
            {
                DebugMessage(M64MSG_WARNING, "ROM Database: Unknown property on line %i", lineno);
            }
            break;
        default:
            break;
        }
    }

    fclose(fPtr);

    /* Resolve RefMD5 references */
    for (search = g_romdatabase.list; search != NULL; search = search->next_entry)
    {
        if (search->entry.refmd5 != NULL)
        {
            romdatabase_entry *ref = ini_search_by_md5(search->entry.refmd5);
            if (ref != NULL)
            {
                if(ref->savetype!=DEFAULT)
                    search->entry.savetype = ref->savetype;
                if(ref->status!=0)
                    search->entry.status = ref->status;
                if(ref->players!=DEFAULT)
                    search->entry.players = ref->players;
                if(ref->rumble!=DEFAULT)
                    search->entry.rumble = ref->rumble;
            }
            else
                DebugMessage(M64MSG_WARNING, "ROM Database: Error solving RefMD5s");
        }
    }
}

void romdatabase_close(void)
{
    if (!g_romdatabase.have_database)
        return;

    while (g_romdatabase.list != NULL)
        {
        romdatabase_search* search = g_romdatabase.list->next_entry;
        if(g_romdatabase.list->entry.goodname)
            free(g_romdatabase.list->entry.goodname);
        if(g_romdatabase.list->entry.refmd5)
            free(g_romdatabase.list->entry.refmd5);
        free(g_romdatabase.list);
        g_romdatabase.list = search;
        }
}

static romdatabase_entry* ini_search_by_md5(md5_byte_t* md5)
{
    romdatabase_search* search;

    if(!g_romdatabase.have_database)
        return NULL;

    search = g_romdatabase.md5_lists[md5[0]];

    while (search != NULL && memcmp(search->entry.md5, md5, 16) != 0)
        search = search->next_md5;

    if(search==NULL)
        return NULL;

    return &(search->entry);
}

romdatabase_entry* ini_search_by_crc(unsigned int crc1, unsigned int crc2)
{
    romdatabase_search* search;

    if(!g_romdatabase.have_database) 
        return NULL;

    search = g_romdatabase.crc_lists[((crc1 >> 24) & 0xff)];

    while (search != NULL && search->entry.crc1 != crc1 && search->entry.crc2 != crc2)
        search = search->next_crc;

    if(search == NULL) 
        return NULL;

    return &(search->entry);
}



#include "stm32f0xx.h"
#include "config.h"
#if defined (USE_BEESIGN)
#include "menu_vtx.h"
#include "menu.h"
#include "beesign_vtx.h"
#include "string.h"

#if defined(USE_VTX_LOCK_FREQ)
#define VTX_FREQ_ISM_MAX                5920
#define VTX_FREQ_ISM_MIN                5650
#endif // USE_VTX_LOCK_FREQ

static uint8_t vtx_MenuBand = 0;
static uint8_t vtx_lastMenuBand = 0xFF;
static uint8_t vtx_MenuChannel = 0;
static uint8_t vtx_lastMenuChannel = 0xFF;
static uint16_t vtx_MenuShowFreq = 0;
static uint16_t vtx_lastMenuShowFreq = 0xFFFF;
static uint8_t vtx_MenuPower = 0;
static uint8_t vtx_lastMenuPower = 0xFF;

#if defined(USE_VTX_LOCK_FREQ)
static uint8_t lastBand;
static uint8_t lastChannel;
#endif // USE_VTX_LOCK_FREQ

static const menuTab_t vtxMenuEntryBand =         {&vtx_MenuBand, &vtx_lastMenuBand, BEESIGN_BAND_COUNT - 1, &vtx58BandNames[1]};
static const menuUint8_t vtxMenuEntryChannel =    {&vtx_MenuChannel, &vtx_lastMenuChannel, 1, VTX_STRING_CHAN_COUNT, 1};
static const menuUint16_t vtxMenuEntryFreq =      {&vtx_MenuShowFreq, &vtx_lastMenuShowFreq, BEESIGN_MIN_FREQUENCY_MHZ, BEESIGN_MAX_FREQUENCY_MHZ, 0};
static const menuTab_t vtxMenuEntryPower =        {&vtx_MenuPower, &vtx_lastMenuPower, BEESIGN_POWER_COUNT - BEESIGN_MIN_POWER, &bsPowerNames[BEESIGN_MIN_POWER]};

static uint8_t vtxMenuConfigChannel(void *key) {
    key = key;
    vtx_MenuShowFreq = vtx58_BandchanFreq(vtx_MenuBand + 1, vtx_MenuChannel);

    return 0;
}

#if defined(USE_VTX_LOCK_FREQ)
static uint8_t vtxMenuCheckLockFreq(uint16_t freq) {
    if (freq < VTX_FREQ_ISM_MIN || freq > VTX_FREQ_ISM_MAX) {
        return 1;
    }
    return 0;
}

static uint8_t vtxMenuBandLockCheck(void *key) {
    if (vtxMenuCheckLockFreq(vtx58_BandchanFreq(vtx_MenuBand + 1, vtx_MenuChannel))) {
        vtx_MenuChannel = 1;
    }
    while (1) {
        if (vtxMenuCheckLockFreq(vtx58_BandchanFreq(vtx_MenuBand + 1, vtx_MenuChannel))) {
            vtx_MenuChannel += 1;
            if (vtx_MenuChannel > 8) {
                if (lastBand - vtx_MenuBand >= 1) {
                    vtx_MenuBand -= 1;
                    vtx_MenuChannel = 1;
                } else {
                    vtx_MenuBand += 1;
                    vtx_MenuChannel = 1;
                }
            }
        } else {
            lastBand = vtx_MenuBand;
            break;
        }
    }
    vtxMenuConfigChannel(key);
    return 0;
}

static uint8_t vtxMenuChannelLockCheck(void *key) {
    while (1) {
        if (vtxMenuCheckLockFreq(vtx58_BandchanFreq(vtx_MenuBand + 1, vtx_MenuChannel))) {
            if (lastChannel - vtx_MenuChannel >= 1) {
                vtx_MenuChannel -= 1;
            } else {
                vtx_MenuChannel += 1;
            }
            if (vtx_MenuChannel == 0 || vtx_MenuChannel == 9) {
                vtx_MenuChannel = lastChannel;
                break;
            }
        } else {
            lastChannel = vtx_MenuChannel;
            break;
        }
    }
    vtxMenuConfigChannel(key);
    return 0;
}
#endif // USE_VTX_LOCK_FREQ


static void vtxMenuExitFun(void) {
    vtx_lastMenuBand = 0xFF;
    vtx_lastMenuChannel = 0xFF;
    vtx_lastMenuShowFreq = 0xFFFF;
    vtx_lastMenuPower = 0xFF;
}

static void vtxMenuEnterFun(void) {
    vtx_MenuBand = bsDevice.vtx.channel / VTX_STRING_CHAN_COUNT;
    vtx_MenuChannel = bsDevice.vtx.channel % VTX_STRING_CHAN_COUNT + 1;
    vtx_MenuShowFreq = vtx58_BandchanFreq(vtx_MenuBand + 1, vtx_MenuChannel);
    vtx_MenuPower = bsDevice.vtx.power;
    #if defined(USE_VTX_LOCK_FREQ)
    lastBand = vtx_MenuBand;
    lastChannel = vtx_MenuChannel;
    #endif // USE_VTX_LOCK_FREQ
}

void vtxMenuInit(void) {
    vtx_MenuBand = bsDevice.vtx.channel / VTX_STRING_CHAN_COUNT;
    vtx_MenuChannel = bsDevice.vtx.channel % VTX_STRING_CHAN_COUNT + 1;
    vtx_MenuShowFreq = vtx58_BandchanFreq(vtx_MenuBand + 1, vtx_MenuChannel);
    vtx_MenuPower = bsDevice.vtx.power;
}

void vtxMenuSaveFun(void) {
    bsVtxSetBandAndChannel(vtx_MenuBand + 1, vtx_MenuChannel);
    bsVtxSetPowerIndex(vtx_MenuPower);
}

static menuEntry_t vtxMenuEntries[] = {
    {"- VTX -",         menu_Label,         NULL,                       NULL,                   0},
#if defined(USE_VTX_LOCK_FREQ)
    {"BAND",            menu_Tab,           vtxMenuBandLockCheck,       &vtxMenuEntryBand,      0},
    {"CHAN",            menu_Uint8,         vtxMenuChannelLockCheck,    &vtxMenuEntryChannel,   0},
#else
    {"BAND",            menu_Tab,           vtxMenuConfigChannel,       &vtxMenuEntryBand,      0},
    {"CHAN",            menu_Uint8,         vtxMenuConfigChannel,       &vtxMenuEntryChannel,   0},
#endif // USE_VTX_LOCK_FREQ
    {"(FREQ)",          menu_Uint16,        NULL,                       &vtxMenuEntryFreq,      DYNAMIC},
    {"POWER",           menu_Tab,           NULL,                       &vtxMenuEntryPower,     0},
    {"EXIT",            menu_Back,          NULL,                       NULL,                   0},
    { NULL,             menu_End,           NULL,                       NULL,                   0}
};

const menu_t vtxMenu = {
    .uperMenu = &mainMenu,
    .onEnter = vtxMenuEnterFun,
    .onExit = vtxMenuExitFun,
    .entries = vtxMenuEntries
};

#endif

/*
 * Copyright (C) 2016 Nickolay Semendyaev <agent00791@gmail.com>
 *
 * Sample console app for Broadcom FM Radio.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <system/audio.h>
#include <tinyalsa/asoundlib.h>
#include "include/fmradio.h"
#include "include/v4l2_ioctl.h"

#define INFO(fmt, ...) fprintf(stderr, "[INFO] " fmt, ##__VA_ARGS__)
#define ERR(fmt, ...) fprintf(stderr, "[ERR]  " fmt, ##__VA_ARGS__)
static int fd = -1;

//Sony play-fm switches.
#define PRI_MI2S_LOOPBACK_Volume "PRI MI2S LOOPBACK Volume"
#define SLIMBUS_0_RX_Port_Mixer_PRI_MI2S_TX "SLIMBUS_0_RX Port Mixer PRI_MI2S_TX"
#define SLIMBUS_DL_HL_Switch "SLIMBUS_DL_HL Switch"
#define SLIM_0_RX_Format "SLIM_0_RX Format"
#define SLIM_0_RX_SampleRate "SLIM_0_RX SampleRate"
//Sony speaker
/*
    <path name="speaker">
        <ctl name="AIF4_VI Mixer SPKR_VI_1" value="1" />
        <ctl name="AIF4_VI Mixer SPKR_VI_2" value="1" />
        <ctl name="SLIM RX1 MUX" value="AIF1_PB" />
        <ctl name="SLIM RX2 MUX" value="AIF1_PB" />
        <ctl name="SLIM_0_RX Channels" value="Two" />
        <ctl name="RX7 MIX1 INP1" value="RX1" />
        <ctl name="RX8 MIX1 INP1" value="RX2" />
        <ctl name="RX7 Digital Volume" value="84" />
        <ctl name="RX8 Digital Volume" value="84" />
        <ctl name="COMP0 Switch" value="1" />
        <ctl name="VI_FEED_TX Channels" value="Two" />
        <ctl name="SLIM0_RX_VI_FB_LCH_MUX"  value="SLIM4_TX" />
        <ctl name="SLIM0_RX_VI_FB_RCH_MUX"  value="SLIM4_TX" />
    </path>
*/
#define AIF4_VI_Mixer_SPKR_VI_1 "AIF4_VI Mixer SPKR_VI_1"
#define AIF4_VI_Mixer_SPKR_VI_2 "AIF4_VI Mixer SPKR_VI_2"
#define SLIM_RX1_MUX "SLIM RX1 MUX"
#define SLIM_RX2_MUX "SLIM RX2 MUX"
#define SLIM_0_RX_Channels "SLIM_0_RX Channels"
#define RX7_MIX1_INP1 "RX7 MIX1 INP1"
#define RX8_MIX1_INP1 "RX8 MIX1 INP1"
#define RX7_Digital_Volume "RX7 Digital Volume"
#define RX8_Digital_Volume "RX8 Digital Volume"
#define COMP0_Switch "COMP0 Switch"
#define VI_FEED_TX_Channels "VI_FEED_TX Channels"
#define SLIM0_RX_VI_FB_LCH_MUX "SLIM0_RX_VI_FB_LCH_MUX"
#define SLIM0_RX_VI_FB_RCH_MUX "SLIM0_RX_VI_FB_RCH_MUX"

int setMixerBoolCtl(char* name, bool val) {
    int ret = 0;
    enum mixer_ctl_type type;
    struct mixer_ctl *ctl;
    struct mixer *mixer = mixer_open(0);
    ctl = mixer_get_ctl_by_name(mixer, name);
    if (ctl == NULL) {
        mixer_close(mixer);
        ERR("%s: Could not find %s\n", __func__, name);
        return -ENODEV;
    }

    type = mixer_ctl_get_type(ctl);
    if (type != MIXER_CTL_TYPE_BOOL) {
        ERR("%s: %s is not supported\n", __func__, name);
        mixer_close(mixer);
        return -ENOTTY;
    }
    ret = mixer_ctl_set_value(ctl, 0, val);
    if (ret < 0)
        return -ENOTTY;

    mixer_close(mixer);
    return 0;
}

int setMixerIntCtl(char* name, bool val) {
    int ret = 0;
    enum mixer_ctl_type type;
    struct mixer_ctl *ctl;
    struct mixer *mixer = mixer_open(0);
    ctl = mixer_get_ctl_by_name(mixer, name);
    if (ctl == NULL) {
        mixer_close(mixer);
        ERR("%s: Could not find %s\n", __func__, name);
        return -ENODEV;
    }

    type = mixer_ctl_get_type(ctl);
    if (type != MIXER_CTL_TYPE_INT) {
        ERR("%s: %s is not supported\n", __func__, name);
        mixer_close(mixer);
        return -ENOTTY;
    }
    ret = mixer_ctl_set_value(ctl, 0, val);
    if (ret < 0)
        return -ENOTTY;

    mixer_close(mixer);
    return 0;
}


int setMixerEnumNameCtl(char* name, char* val) {
    int ret = 0;
    enum mixer_ctl_type type;
    struct mixer_ctl *ctl;
    struct mixer *mixer = mixer_open(0);
    ctl = mixer_get_ctl_by_name(mixer, name);
    if (ctl == NULL) {
        mixer_close(mixer);
        ERR("%s: Could not find %s\n", __func__, name);
        return -ENODEV;
    }

    type = mixer_ctl_get_type(ctl);
    if (type != MIXER_CTL_TYPE_ENUM) {
        ERR("%s: %s is not supported, %d\n", __func__, name, type);
        mixer_close(mixer);
        return -ENOTTY;
    }
    ret = mixer_ctl_set_enum_by_string(ctl, val);
    if (ret < 0)
        return -ENOTTY;

    mixer_close(mixer);
    return 0;
}

static int initMixer() {
    int ret = 0;
    enum mixer_ctl_type type;
    struct mixer_ctl *ctl;
    struct mixer *mixer = mixer_open(0);

    if (mixer == NULL) {
        ERR("Error opening mixer 0");
        return -1;
    }
#if 0
//speaker
//AIF4_VI Mixer SPKR_VI_1
    ret = setMixerBoolCtl(AIF4_VI_Mixer_SPKR_VI_1, true);
    if (ret < 0)
        return -1;
//AIF4_VI Mixer SPKR_VI_2
    ret = setMixerBoolCtl(AIF4_VI_Mixer_SPKR_VI_2, true);
    if (ret < 0)
        return -1;
//SLIM RX1 MUX
    ret = setMixerEnumNameCtl(SLIM_RX1_MUX, "AIF1_PB");
    if (ret < 0)
        return -1;
//SLIM RX2 MUX
    ret = setMixerEnumNameCtl(SLIM_RX2_MUX, "AIF1_PB");
    if (ret < 0)
        return -1;
//SLIM_0_RX Channels
    ret = setMixerEnumNameCtl(SLIM_0_RX_Channels, "Two");
    if (ret < 0)
        return -1;
//RX7_MIX1_INP1
    ret = setMixerEnumNameCtl(RX7_MIX1_INP1, "RX1");
    if (ret < 0)
        return -1;
//RX8_MIX1_INP1
    ret = setMixerEnumNameCtl(RX8_MIX1_INP1, "RX2");
    if (ret < 0)
        return -1;
//RX7 Digital Volume
    ret = setMixerIntCtl(RX7_Digital_Volume, 84);
    if (ret < 0)
        return -1;
//RX8 Digital Volume
    ret = setMixerIntCtl(RX8_Digital_Volume, 84);
    if (ret < 0)
        return -1;
//COMP0 Switch
    ret = setMixerBoolCtl(COMP0_Switch, true);
    if (ret < 0)
        return -1;
//VI_FEED_TX Channels
    ret = setMixerEnumNameCtl(VI_FEED_TX_Channels, "Two");
    if (ret < 0)
        return -1;
//SLIM0_RX_VI_FB_LCH_MUX
    ret = setMixerEnumNameCtl(SLIM0_RX_VI_FB_LCH_MUX, "SLIM4_TX");
    if (ret < 0)
        return -1;
//SLIM0_RX_VI_FB_RCH_MUX
    ret = setMixerEnumNameCtl(SLIM0_RX_VI_FB_RCH_MUX, "SLIM4_TX");
    if (ret < 0)
        return -1;
#endif

//headphones-regulation
/*
<ctl name="SLIM RX1 MUX" value="AIF1_PB" />
        <ctl name="SLIM RX2 MUX" value="AIF1_PB" />
        <ctl name="SLIM_0_RX Channels" value="Two" />
        <ctl name="RX1 MIX1 INP1" value="RX1" />
        <ctl name="RX2 MIX1 INP1" value="RX2" />
        <ctl name="CLASS_H_DSM MUX" value="DSM_HPHL_RX1" />
        <ctl name="HPHL DAC Switch" value="1" />
        <ctl name="COMP1 Switch" value="1" />
        <!--100 % of 20 register: 0x1AE-->
        <ctl name="HPHL Volume" value="20" />
        <!--100 % of 20 register: 0x1B4-->
        <ctl name="HPHR Volume" value="20" />
        <!--60 % of 124 (rounded) register: 0x2B7-->
        <ctl name="RX1 Digital Volume" value="76" />
        <!--60 % of 124 (rounded) register: 0x2BF-->
        <ctl name="RX2 Digital Volume" value="76" />
*/
#if 1
  ret = setMixerEnumNameCtl("SLIM RX1 MUX", "AIF1_PB");
    if (ret < 0)
        return -1;
  ret = setMixerEnumNameCtl("SLIM RX2 MUX", "AIF1_PB");
    if (ret < 0)
        return -1;
  ret = setMixerEnumNameCtl("SLIM_0_RX Channels", "Two");
    if (ret < 0)
        return -1;
  ret = setMixerEnumNameCtl("RX1 MIX1 INP1", "RX1");
    if (ret < 0)
        return -1;
  ret = setMixerEnumNameCtl("RX2 MIX1 INP1", "RX2");
    if (ret < 0)
        return -1;
  ret = setMixerEnumNameCtl("CLASS_H_DSM MUX", "DSM_HPHL_RX1");
    if (ret < 0)
        return -1;
  ret = setMixerBoolCtl("HPHL DAC Switch", true);
    if (ret < 0)
        return -1;
  ret = setMixerBoolCtl("COMP0 Switch", true);
    if (ret < 0)
        return -1;
  ret = setMixerIntCtl("HPHL Volume", 20);
    if (ret < 0)
        return -1;
  ret = setMixerIntCtl("HPHR Volume", 20);
    if (ret < 0)
        return -1;
  ret = setMixerIntCtl("RX1 Digital Volume", 76);
    if (ret < 0)
        return -1;
  ret = setMixerIntCtl("RX2 Digital Volume", 76);
    if (ret < 0)
        return -1;
#endif
    ret = setMixerBoolCtl("SLIMBUS_0_RX Audio Mixer MultiMedia4", true);
    if (ret < 0)
        return -1;
 //play-fm
#if 1
//SLIM_0_RX_Format
    ret = setMixerEnumNameCtl(SLIM_0_RX_Format, "S16_LE");
    if (ret < 0)
        return -1;
//SLIM_0_RX_SampleRate
    ret = setMixerEnumNameCtl(SLIM_0_RX_SampleRate, "KHZ_48");
    if (ret < 0)
        return -1;
#endif
//SLIMBUS_0_RX_Port_Mixer_PRI_MI2S_TX
    ret = setMixerBoolCtl(SLIMBUS_0_RX_Port_Mixer_PRI_MI2S_TX, true);
    if (ret < 0)
        return -1;
//SLIMBUS_DL_HL_Switch
    ret = setMixerBoolCtl(SLIMBUS_DL_HL_Switch, true);
    if (ret < 0)
        return -1;
//MultiMedia1 Mixer PRI_MI2S_TX
    ret = setMixerBoolCtl("MultiMedia1 Mixer PRI_MI2S_TX", true);
    if (ret < 0)
        return -1;
//MultiMedia1 Mixer PRI_MI2S_TX
    ret = setMixerBoolCtl("MultiMedia2 Mixer PRI_MI2S_TX", true);
    if (ret < 0)
        return -1;
//PRI_MI2S_LOOPBACK_Volume
    ret = setMixerIntCtl("PRI MI2S LOOPBACK Volume", 421);
    if (ret < 0)
        return -1;

//Open pcm
     struct pcm *pcm_out;
     struct pcm *pcm_in;
     struct pcm_config pcm_config_fm = {
        .channels = 2,
        .rate = 48000,
        .period_size = 256,
        .period_count = 4,
        .format = PCM_FORMAT_S16_LE,
        .start_threshold = 0,
        .stop_threshold = INT_MAX,
        .avail_min = 0,
     };

     pcm_out = pcm_open(0, 5, PCM_OUT, &pcm_config_fm);
     if (pcm_out && !pcm_is_ready(pcm_out)) {
        ERR("pcm_out_open failed: %s", pcm_get_error(pcm_out));
        return -1;
     }

     pcm_in = pcm_open(0, 6, PCM_IN, &pcm_config_fm);
     if (pcm_in && !pcm_is_ready(pcm_in)) {
        ERR("pcm_in_open failed: %s", pcm_get_error(pcm_in));
        return -1;
     }

     ret = pcm_start(pcm_out);
     if (ret < 0) {
         ERR("pcm_out start fail, %d\n", ret);
         return -1;
     }

     ret = pcm_start(pcm_in);
     if (ret < 0) {
         ERR("pcm_int start fail, %d\n", ret);
         return -1;
     }

     return 0;
}

static int open_device()
{
    int ret = 0;

    INFO("Try open Fm device\n");
    fd = open_dev(DEFAULT_DEVICE);
    if (fd < 0) 
        return -1;
    INFO("Fm device opened\n");

    INFO("Start check tunner radio capability\n");
    ret = get_tun_radio_cap(fd);
    if (ret < 0)
        return -1;
    INFO("End check tunner radio capability\n");;

    INFO("Start get V4L tuner\n");
    struct v4l2_tuner vt;
    vt.index = 0;
    vt.audmode = V4L2_TUNER_MODE_STEREO;
    ret = get_v4l2_tuner(fd, &vt);
    if (ret < 0)
        return -1;
    INFO("End get V4L tuner\n");

    INFO("Start get fact\n");
    float fact = get_fact(fd, &vt);
    if (fact < 0)
        return -1;
    INFO("End get fact %.6f\n", fact);;

    INFO("Start set audmode stereo\n");
    ret = set_force_mono(fd, &vt, false);
    if (ret < 0)
        return -1;
    INFO("End set audmode stereo\n");

    INFO("Start set freq: %.6f\n", 104400*fact);
    ret = set_freq(fd, 104400*fact);
    if (ret < 0)
        return -1;
    INFO("End set freq\n");

    INFO("Start set DEFAULT_VOLUME\n");
    ret = set_volume(fd, 255);
    if (ret < 0)
        return -1;
    INFO("End set DEFAULT_VOLUME\n");

    INFO("Start set MUTE OFF\n");
    ret = set_mute(fd, 0);
    if (ret < 0)
        return -1;
    INFO("End set MUTE_OFF\n");
#if 0
    INFO("Init mixers\n");
    ret = initMixer();
    if (ret < 0)
        return -1;
    INFO("Mixers inited\n");
#endif
    while(1) {
        get_signal_strength(fd, &vt);
        sleep(5);
    }

    close_dev(fd);
    INFO("Fm device closed\n");
    return 0;
}

int main(int argc, char **argv)
{
    INFO("Start fm radio app\n");
    if (open_device() < 1 && fd > 0)
        close_dev(fd);

    return 0;
}


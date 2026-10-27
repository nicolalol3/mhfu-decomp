/* Wrappers of the system sound libraries: sceSas (the synthesizer, core at 0x08A8B200) and
 * sceAudio (the output channels), with the error codes of the SDK samples. Plus small
 * helpers: placement new, FPU control, vector add. (Guessed names.) */
#include "common.h"

#define SAS_CORE ((void *)0x08A8B200)
#define SAS_NOT_INIT 0x80420100

extern u32 D_08A8B1EC;            /* sceSas ready (1) */
extern u32 D_08A8C040;            /* audio channels ready */
extern u32 D_08A8C4C8;            /* main channel */
extern s32 D_08A8C4D8[3];         /* other channels */
extern u32 D_08A8C4D0, D_08A8C4D4;
extern u8 D_08A8C0C4[0x200], D_08A8C2C4[0x200];

s32 __sceSasInit(void *core, u32 grain, u32 voices, u32 mode, u32 rate);
s32 __sceSasCore(void *core, void *buf);
s32 __sceSasSetPitch(void *core, s32 v, s32 p);
s32 __sceSasSetSimpleADSR(void *core, s32 v, u32 a, u32 b);
s32 __sceSasSetVolume(void *core, s32 v, s32 l, s32 r, s32 el, s32 er);
s32 __sceSasSetKeyOn(void *core, s32 v);
s32 __sceSasSetKeyOff(void *core, s32 v);
s32 __sceSasSetVoice(void *core, s32 v, void *data, s32 size, s32 loop);
s32 __sceSasGetEndFlag(void *core);
s32 __sceSasGetOutputmode(void *core);
s32 __sceSasRevVON(void *core, s32 dry, s32 wet);
s32 __sceSasRevEVOL(void *core, s32 l, s32 r);
s32 __sceSasRevType(void *core, s32 t);
s32 __sceSasRevParam(void *core, s32 d, s32 f);
s32 sceAudioChReserve(s32 ch, s32 samples, s32 format);
s32 sceAudioChRelease(s32 ch);
s32 sceAudioSetChannelDataLen(s32 ch, s32 samples);
s32 sceAudioChangeChannelVolume(s32 ch, s32 l, s32 r);
s32 sceAudioOutputPannedBlocking(s32 ch, s32 l, s32 r, void *buf);
s32 sceAudioChangeChannelConfig(s32 ch, s32 format);
s32 sceAudioGetChannelRestLength(s32 ch);

void *placement_new(u32 size, void *p)
{
    return p;
}

void stub_nop(void *p)
{
}

s32 sas_core(void *buf)
{
    if (D_08A8B1EC == 0)
        return SAS_NOT_INIT;
    if (buf == NULL || ((u32)buf & 0x3F) != 0)
        return 0x80420005;
    return __sceSasCore(SAS_CORE, buf);
}

s32 sas_init(u32 grain)
{
    s32 r, v;

    if (D_08A8B1EC != 0)
        return 0x80420101;
    r = __sceSasInit(SAS_CORE, grain, 0x20, 0, 44100);
    if (r != 0)
        return r;
    for (v = 0; v < 0x20; v++) {
        if (D_08A8B1EC != 0) {
            __sceSasSetPitch(SAS_CORE, v, 0x1000);
            if (D_08A8B1EC != 0) {
                __sceSasSetSimpleADSR(SAS_CORE, v, 0xF, 0x5FC0);
                if (D_08A8B1EC != 0)
                    __sceSasSetVolume(SAS_CORE, v, 0, 0, 0, 0);
            }
        }
    }
    D_08A8B1EC = 1;
    return 0;
}

s32 sas_key_on(s32 v)
{
    if (D_08A8B1EC == 0)
        return SAS_NOT_INIT;
    return __sceSasSetKeyOn(SAS_CORE, v);
}

s32 sas_key_off(s32 v)
{
    if (D_08A8B1EC == 0)
        return SAS_NOT_INIT;
    return __sceSasSetKeyOff(SAS_CORE, v);
}

s32 sas_set_volume(s32 v, s32 l, s32 r, s32 el, s32 er)
{
    if (D_08A8B1EC == 0)
        return SAS_NOT_INIT;
    return __sceSasSetVolume(SAS_CORE, v, l, r, el, er);
}

s32 sas_set_pitch(s32 v, s32 p)
{
    if (D_08A8B1EC == 0)
        return SAS_NOT_INIT;
    if (p == 0)
        return 0x80420012;
    return __sceSasSetPitch(SAS_CORE, v, p);
}

s32 sas_set_voice(s32 v, void *data, s32 size, s32 loop)
{
    if (D_08A8B1EC == 0)
        return SAS_NOT_INIT;
    return __sceSasSetVoice(SAS_CORE, v, data, size, loop);
}

s32 sas_set_adsr(s32 v, u32 a, u32 b)
{
    if (D_08A8B1EC == 0)
        return SAS_NOT_INIT;
    return __sceSasSetSimpleADSR(SAS_CORE, v, a, b);
}

s32 sas_end_flags(void)
{
    if (D_08A8B1EC == 0)
        return 0;
    return __sceSasGetEndFlag(SAS_CORE);
}

/* the reverb needs the stereo output */
static s32 sas_output_mode(void)
{
    s32 m = SAS_NOT_INIT;

    if (D_08A8B1EC == 1)
        m = __sceSasGetOutputmode(SAS_CORE);
    return m;
}

s32 sas_rev_von(s32 dry, s32 wet)
{
    if (D_08A8B1EC == 0)
        return SAS_NOT_INIT;
    if (sas_output_mode() == 1)
        return 0x80420024;
    return __sceSasRevVON(SAS_CORE, dry, wet);
}

s32 sas_rev_evol(s32 l, s32 r)
{
    if (D_08A8B1EC == 0)
        return SAS_NOT_INIT;
    if (sas_output_mode() == 1)
        return 0x80420024;
    return __sceSasRevEVOL(SAS_CORE, l, r);
}

s32 sas_rev_type(s32 t)
{
    if (D_08A8B1EC == 0)
        return SAS_NOT_INIT;
    if (sas_output_mode() == 1)
        return 0x80420024;
    return __sceSasRevType(SAS_CORE, t);
}

s32 sas_rev_param(s32 d, s32 f)
{
    if (D_08A8B1EC == 0)
        return SAS_NOT_INIT;
    if (sas_output_mode() == 1)
        return 0x80420024;
    return __sceSasRevParam(SAS_CORE, d, f);
}

void sas_init_default(void)
{
    sas_init(0x100);
}

/* reserves the audio channels: the main one and 3 more */
s32 audio_init(void)
{
    s32 i, c;

    if (D_08A8C040 != 0)
        return 0x80440001;
    D_08A8C4D4 = 0x08A8C044;
    D_08A8C4D0 = 0x08A8C0B4;
    for (i = 0; i < 0x200; i++)
        D_08A8C0C4[i] = 0;
    for (i = 0; i < 0x200; i++)
        D_08A8C2C4[i] = 0;
    D_08A8C4C8 = sceAudioChReserve(-1, 0x1C0, 0);
    if ((s32)D_08A8C4C8 < 0)
        return 0x80440001;
    for (i = 0; i < 3; i++) {
        c = sceAudioChReserve(-1, 0x100, 0);
        D_08A8C4D8[i] = c;
        if (c < 0) {
            while (--i >= 0)
                sceAudioChRelease(i);
            sceAudioChRelease(D_08A8C4C8);
            return 0x80440001;
        }
    }
    D_08A8C040 = 1;
    return 0;
}

s32 audio_set_voice_volume(u32 v, u32 l, u32 r)
{
    if (v >= 8)
        return 0x80440005;
    if (l > 0x1000 || r > 0x1000)
        return 0x8044000A;
    *(u32 *)(v * 0x40 + 0x08A8C0D0) = r;
    *(u32 *)(v * 0x40 + 0x08A8C0CC) = l;
    return 0;
}

s32 audio_set_len(u32 ch, u32 n)
{
    if (ch >= 4)
        return 0x80440010;
    if (n - 0x40 >= 0xFF81 || (n & 0x3F) != 0)
        return 0x80440011;
    return sceAudioSetChannelDataLen(D_08A8C4D8[ch], n);
}

s32 audio_set_volume(u32 ch, u32 l, u32 r)
{
    if (ch >= 4)
        return 0x80440010;
    if (l > 0x8000 || r > 0x8000)
        return 0x8044000A;
    return sceAudioChangeChannelVolume(D_08A8C4D8[ch], l, r);
}

s32 audio_output(u32 ch, u32 l, u32 r, void *buf)
{
    if (ch >= 4)
        return 0x80440010;
    if (l > 0x8000 || r > 0x8000)
        return 0x8044000A;
    return sceAudioOutputPannedBlocking(D_08A8C4D8[ch], l, r, buf);
}

s32 audio_set_format(u32 ch, s32 f)
{
    if (ch >= 4)
        return 0x80440010;
    return sceAudioChangeChannelConfig(D_08A8C4D8[ch], f);
}

s32 audio_rest(u32 ch)
{
    if (ch >= 4)
        return 0x80440010;
    return sceAudioGetChannelRestLength(D_08A8C4D8[ch]);
}

/* sets the FPU control register, returns the old value */
u32 fpu_set_fcsr(u32 v)
{
    u32 old;

    __asm__ volatile("cfc1 %0, $31\n"
                     "ctc1 $0, $31\n"
                     "nop\n"
                     "ctc1 %1, $31" : "=&r"(old) : "r"(v));
    return old;
}

f32 *vec4_add(f32 *d, const f32 *a, const f32 *b)
{
    d[0] = a[0] + b[0];
    d[1] = a[1] + b[1];
    d[2] = a[2] + b[2];
    d[3] = a[3] + b[3];
    return d;
}

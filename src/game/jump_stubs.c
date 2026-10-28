/* Jump stubs of the sound library: each one only jumps to the real function (they are
 * the entries used by the rest of the game). (Names from the target address.) */
#include "common.h"

u32 func_0889EC14(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EC78(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EC80(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EC88(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EC90(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EC98(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889ECA0(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889ECA8(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889ECB0(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EE48(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EE50(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EE58(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EE60(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EE68(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EE70(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EE78(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EE88(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EE90(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_0889EE98(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A1200(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A49D0(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A4B5C(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A4BC4(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A4BE8(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A4BF0(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A4CB0(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A59C4(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A6014(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A6024(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A65D4(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A6628(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A6798(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A67D8(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A6818(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A6868(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A68B8(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A68FC(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A6940(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A69DC(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A6BA0(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A6C48(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A9CE8(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A9E28(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088A9E60(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088AA054(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088AA0BC(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088AD0D4(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088AD0F8(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088AE598(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);
u32 func_088B7350(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h);

u32 stub_0889e9d4(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EC14(a, b, c, d, e, f, g, h);
}

u32 stub_0889ea4c(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EC78(a, b, c, d, e, f, g, h);
}

u32 stub_0889ea54(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EC80(a, b, c, d, e, f, g, h);
}

u32 stub_0889ea5c(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EC88(a, b, c, d, e, f, g, h);
}

u32 stub_0889ea64(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EC90(a, b, c, d, e, f, g, h);
}

u32 stub_0889ea6c(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889ECA0(a, b, c, d, e, f, g, h);
}

u32 stub_0889ea74(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889ECA8(a, b, c, d, e, f, g, h);
}

u32 stub_0889ea7c(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889ECB0(a, b, c, d, e, f, g, h);
}

u32 stub_0889ea84(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EE48(a, b, c, d, e, f, g, h);
}

u32 stub_0889ea8c(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EE98(a, b, c, d, e, f, g, h);
}

u32 stub_0889ea94(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EE50(a, b, c, d, e, f, g, h);
}

u32 stub_0889ea9c(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EE60(a, b, c, d, e, f, g, h);
}

u32 stub_0889eaa4(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EE70(a, b, c, d, e, f, g, h);
}

u32 stub_0889eaac(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EE58(a, b, c, d, e, f, g, h);
}

u32 stub_0889eab4(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EE68(a, b, c, d, e, f, g, h);
}

u32 stub_0889eabc(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EE78(a, b, c, d, e, f, g, h);
}

u32 stub_0889eac4(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EE88(a, b, c, d, e, f, g, h);
}

u32 stub_0889eacc(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EE90(a, b, c, d, e, f, g, h);
}

u32 stub_0889ead4(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_0889EC98(a, b, c, d, e, f, g, h);
}

u32 stub_0889ec78(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A4B5C(a, b, c, d, e, f, g, h);
}

u32 stub_0889ec80(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A4BC4(a, b, c, d, e, f, g, h);
}

u32 stub_0889ec88(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A4BE8(a, b, c, d, e, f, g, h);
}

u32 stub_0889ec90(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A4BF0(a, b, c, d, e, f, g, h);
}

u32 stub_0889ec98(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A6014(a, b, c, d, e, f, g, h);
}

u32 stub_0889eca0(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A6024(a, b, c, d, e, f, g, h);
}

u32 stub_0889eca8(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A59C4(a, b, c, d, e, f, g, h);
}

u32 stub_0889ecb0(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A4CB0(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee40(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A65D4(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee48(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A6628(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee50(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A6798(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee58(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A67D8(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee60(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A6818(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee68(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A6868(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee70(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A68B8(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee78(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A68FC(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee80(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A6940(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee88(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A69DC(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee90(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A6BA0(a, b, c, d, e, f, g, h);
}

u32 stub_0889ee98(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A6C48(a, b, c, d, e, f, g, h);
}

u32 stub_088a0bec(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A1200(a, b, c, d, e, f, g, h);
}

u32 stub_088a0bf4(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A1200(a, b, c, d, e, f, g, h);
}

u32 stub_088a4be8(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A49D0(a, b, c, d, e, f, g, h);
}

u32 stub_088a72f0(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A9E28(a, b, c, d, e, f, g, h);
}

u32 stub_088a72f8(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A9E60(a, b, c, d, e, f, g, h);
}

u32 stub_088a7300(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088A9CE8(a, b, c, d, e, f, g, h);
}

u32 stub_088a7368(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088AA054(a, b, c, d, e, f, g, h);
}

u32 stub_088a7370(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088AA0BC(a, b, c, d, e, f, g, h);
}

u32 stub_088ad0f0(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088AD0D4(a, b, c, d, e, f, g, h);
}

u32 stub_088ad12c(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088AD0F8(a, b, c, d, e, f, g, h);
}

u32 stub_088b099c(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088AE598(a, b, c, d, e, f, g, h);
}

u32 stub_088b1584(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h)
{
    return func_088B7350(a, b, c, d, e, f, g, h);
}

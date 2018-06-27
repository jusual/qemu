#include "qemu/osdep.h"
#include "libqtest.h"

static void test_reserved_reg(void)
{
    QTestState *s = qtest_startf("-M microbit -nographic -kernel start.hex");

    int32_t i, scr;
    uint64_t reserved_reg[] = { 0xe000ed10, /* SCR */
                                0xe000ed18, /* SHPR1 */
                                0xe000ed28, /* CFSR */
                                0xe000ed2C, /* HFSR */
                                0xe000ed34, /* MMAR */
                                0xe000ed38, /* BFAR */
                                0xe000ed3C, /* AFSR */
                                0xe000ed40, /* CPUID */
                                0xe000ed88, /* CAR */
                                0xe000ef00,  /* STIR */
                                0x40002000,
                                0x4000251c};

    for (i = 0; i < ARRAY_SIZE(reserved_reg); i++) {
        qtest_writel(s, reserved_reg[i], 1);
        scr = qtest_readl(s, reserved_reg[i]);
        printf("%d ", scr);
    }
    printf("\n");

    qtest_quit(s);
}

int main(int argc, char **argv)
{
    int ret;

    g_test_init(&argc, &argv, NULL);

    qtest_add_func("tcg/arm/test-reserved-reg", test_reserved_reg);
    ret = g_test_run();

    return ret;
}

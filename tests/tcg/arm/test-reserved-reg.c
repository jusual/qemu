#include "qemu/osdep.h"
#include "libqtest.h"

static void test_reserved_reg(void)
{
    QTestState *s;
    int i;
    static const uint64_t reserved_reg[] = { 0xe000ed10, /* SCR */
                                             0xe000ed18, /* SHPR1 */
                                             0xe000ed28, /* CFSR */
                                             0xe000ed2c, /* HFSR */
                                             0xe000ed34, /* MMFAR */
                                             0xe000ed38, /* BFAR */
                                             0xe000ed3c, /* AFSR */
                                             0xe000ed40, /* CPUID */
                                             0xe000ed88, /* CPACR */
                                             0xe000ef00  /* STIR */ };
    static const uint8_t mini_kernel[] = { 0x00, 0x00, 0x00, 0x00,
                                           0x09, 0x00, 0x00, 0x00 };
    ssize_t wlen, kernel_size = sizeof(mini_kernel);
    int code_fd;
    char codetmp[] = "/tmp/reserved-reg-test-XXXXXX";

    code_fd = mkstemp(codetmp);
    wlen = write(code_fd, mini_kernel, sizeof(mini_kernel));
    g_assert(wlen == kernel_size);
    close(code_fd);

    s = qtest_startf("-kernel %s -M microbit -nographic", codetmp);

    for (i = 0; i < ARRAY_SIZE(reserved_reg); i++) {
        int res;
        qtest_writel(s, reserved_reg[i], 1);
        res = qtest_readl(s, reserved_reg[i]);
        g_assert(!res);
    }

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

#include <nautilus/nautilus.h>
#include <nautilus/shell.h>

static int test_mpl() {
	nk_vc_printf("hello from mpl\n");
	mpl_main();
	return 0;
}

static int
handle_mpl(char *buf, void *priv)
{
	test_mpl();
	return 0;
}

static struct shell_cmd_impl mpl_impl = {
    .cmd      = "testmpl",
    .help_str = "testmpl",
    .handler  = handle_mpl,
};
nk_register_shell_cmd(mpl_impl);

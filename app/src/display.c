//#ifdef CONFIG_ZEPHYR_HDL
#include "hdl/hdl.h"
#include <device.h>
#include <init.h>
#include <logging/log.h>

static const struct device *display;

static struct HDL_Interface interface;

LOG_MODULE_REGISTER(hdldisp, CONFIG_DISPLAY_LOG_LEVEL);


static void display_thread(void *arg, void *unused2, void *unused3) {


    while(1) {
        k_msleep(1000);

    }
}


static int display_init () {

    display = DEVICE_DT_GET_ANY(il0323n);

    if (display == NULL) {
        LOG_ERR("Failed to get il0323n device");
        return -EINVAL;
    }

    // Initialize HDL
    interface = HDL_CreateInterface(80, 128, HDL_COLORS_MONO, HDL_FEAT_TEXT | HDL_FEAT_LINE_HV);

    // Set interface functions

    return 0;
}

SYS_INIT(display_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
K_THREAD_DEFINE(display_thr, 1024, display_thread, NULL, NULL, NULL, K_PRIO_COOP(10), 0, 0);
//#endif
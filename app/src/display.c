//#ifdef CONFIG_ZEPHYR_HDL
#include "hdl/hdl.h"
#include <device.h>
#include <init.h>
#include <logging/log.h>
#include "../drivers/epd/il0323n.h"

static const struct device *display;

static struct HDL_Interface interface;

unsigned char HDL_PAGE_OUTPUT[141] = {
0x00, 0x01, 0x02, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x16, 0x00, 0x0E, 0x00, 0x0D, 0x00, 0x01, 0x00, 0x00, 0x1E, 0x00, 0xCC, 0x06, 0x18, 0x37, 0xB1, 
0xB3, 0x6D, 0x86, 0xCD, 0xEC, 0x0C, 0xC0, 0x61, 0x80, 0x30, 0x01, 0xE0, 0x03, 0x14, 0x00, 0x0A, 
0x00, 0x10, 0x00, 0x01, 0x1E, 0x3F, 0xFF, 0xFF, 0x03, 0xC0, 0xF0, 0x3C, 0x0F, 0x03, 0xC0, 0xF0, 
0x3C, 0x0F, 0x7B, 0xDE, 0xF0, 0x3F, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x05, 0x04, 0x01, 0x02, 0x02, 
0x00, 0x00, 0x02, 0x04, 0x04, 0x01, 0x01, 0x05, 0x04, 0x01, 0x01, 0x02, 0x00, 0x00, 0x02, 0x07, 
0x07, 0x01, 0x00, 0x0A, 0x04, 0x01, 0x02, 0x00, 0x00, 0x00, 0x02, 0x07, 0x07, 0x01, 0x01, 0x0A, 
0x04, 0x01, 0x02, 0x00, 0x00, 0x00, 0x02, 0x04, 0x04, 0x01, 0x02, 0x05, 0x04, 0x01, 0x02, 0x01, 
0x00, 0x31, 0x32, 0x0A, 0x33, 0x32, 0x00, 0x01, 0x0A, 0x04, 0x01, 0x05, 0x00
};

LOG_MODULE_REGISTER(hdldisp, CONFIG_DISPLAY_LOG_LEVEL);

void dsp_clear (int16_t x, int16_t y, uint16_t w, uint16_t h) {
    il0323_clear_area(display, x, y, w, h);
}

void dsp_render (int16_t x, int16_t y, uint16_t w, uint16_t h) {
    il0323_refresh(display, x, y, w, h);
}

void dsp_pixel (int16_t x, int16_t y) {
    il0323_set_pixel(display, x, y);
}

void dsp_hline (int16_t x, int16_t y, int16_t len) {
    il0323_h_line(display, x, y, len);
}

void dsp_vline (int16_t x, int16_t y, int16_t len) {
    il0323_v_line(display, x, y, len);
}

extern const char HDL_FONT[2048];

void dsp_text (int16_t x, int16_t y, const char *text, uint8_t fontSize) {

    int len = strlen(text);
    int line = 0;
    int acol = 0;

    for (int g = 0; g < len; g++) {
		// Starting character in single quotes

        if (text[g] == '\n') {
			line++;
			acol = 0;
			continue;
		}
		else if (text[g] == ' ') {
			acol++;
		}
		
		for (int py = 0; py < 8; py++) {
			for (int px = 0; px < 5; px++) {
				if ((HDL_FONT[text[g] * 8 + py] >> (7 - px)) & 1) {
                    int rx = x + (px + acol * 5) * fontSize;
                    int ry = y + (py + line * 6) * fontSize;

                    for(int sy = 0; sy < fontSize; sy++) {
                        for(int sx = 0; sx < fontSize; sx++) {
                            dsp_pixel(rx + sx, ry + sy);
                        }
                    }
				}
			}
		}
		acol++;

    }
}

int err = 0;

static void display_thread(void *arg, void *unused2, void *unused3) {
    k_msleep(2000);
    // Initialize HDL
    interface = HDL_CreateInterface(80, 128, HDL_COLORS_MONO, HDL_FEAT_TEXT | HDL_FEAT_LINE_HV);

    // Set interface functions
    interface.f_clear = dsp_clear;
    interface.f_renderPart = dsp_render;
    interface.f_pixel = dsp_pixel;
    interface.f_hline = dsp_hline;
    interface.f_vline = dsp_vline;
    interface.f_text = dsp_text;

    interface.textHeight = 6;
    interface.textWidth = 4;

    err |= HDL_Build(&interface, HDL_PAGE_OUTPUT, sizeof(HDL_PAGE_OUTPUT));

    err |= HDL_Update(&interface);

    while(1) {
        k_msleep(60000);
        err |= HDL_Update(&interface);

        il0323_hibernate(display);
    }
}


static int display_init () {

    display = DEVICE_DT_GET_ANY(gooddisplay_il0323n);

    if (display == NULL) {
        LOG_ERR("Failed to get il0323n device");
        err |= -EINVAL;
        return -EINVAL;
    }

    return 0;
}

SYS_INIT(display_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
K_THREAD_DEFINE(display_thr, 1024, display_thread, NULL, NULL, NULL, K_PRIO_COOP(10), 0, 0);
//#endif
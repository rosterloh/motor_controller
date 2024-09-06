#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <drivers/seesaw.h>
#include <app_version.h>
#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#define NEOSLIDER_NEOPIN   14
#define NEOSLIDER_ANALOGIN 18

static const struct device *const neoslider = DEVICE_DT_GET(DT_NODELABEL(neoslider));
static uint16_t last_slider = 0;
struct k_work_delayable inputs_scan_work;
char slide_str[11] = {0};
lv_obj_t *slide_label;

static void inputs_scan_fn(struct k_work *work)
{
	int err;
	uint16_t slide_val;

	err = seesaw_read_analog(neoslider, NEOSLIDER_ANALOGIN, &slide_val);
	if (err == 0) {
		if (slide_val != last_slider) {
			sprintf(slide_str, "%d", slide_val);
			lv_label_set_text(slide_label, slide_str);
			last_slider = slide_val;
		}
	}

	k_work_reschedule(&inputs_scan_work, K_MSEC(300));
}

int main(void)
{
	int ret;
	const struct device *display_dev;
	struct display_capabilities capabilities;
	lv_obj_t *hello_world_label;

	LOG_INF("Motor Controller Application %s", APP_VERSION_STRING);

	if (!device_is_ready(neoslider)) {
		LOG_ERR("Error: neoslider not ready");
		return 0;
	}

	ret = seesaw_neopixel_setup(neoslider, NEO_GRB + NEO_KHZ800, 4, NEOSLIDER_NEOPIN);
	ret |= seesaw_neopixel_set_brightness(neoslider, 40);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to setup neoslider leds", ret);
		return 0;
	}

	k_work_init_delayable(&inputs_scan_work, inputs_scan_fn);
	k_work_reschedule(&inputs_scan_work, K_NO_WAIT);

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device %s not found", display_dev->name);
	}

	display_get_capabilities(display_dev, &capabilities);

	// lv_disp_set_rotation(NULL, LV_DISP_ROT_90);

	hello_world_label = lv_label_create(lv_scr_act());
	lv_label_set_text(hello_world_label, "Hello world!");
	lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);

	slide_label = lv_label_create(lv_scr_act());
	lv_obj_align(slide_label, LV_ALIGN_BOTTOM_MID, 0, 0);

	lv_task_handler();
	display_blanking_off(display_dev);

	while (1) {
		lv_task_handler();
		k_sleep(K_MSEC(10));
	}

	return 0;
}
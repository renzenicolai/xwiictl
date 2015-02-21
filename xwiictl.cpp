/* Wiimote ctl                                            */
/* Renze Nicolai 2015                                     */
/* Version 1.0                      20-02-2015            */

/* Based on xwiishow, written 2010-2013 by David Herrmann */
/* Dedicated to the Public Domain                         */

#include <errno.h>
#include <poll.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <unistd.h>
#include "xwiimote.h"

static struct xwii_iface *iface;
Display *display;
Screen*  scrn;
int screen_height;
int screen_width;
unsigned int keycode;
static int mode;

/* error messages */

static void print_info(const char *format, ...)
{
	va_list list;
	char str[58 + 1];

	va_start(list, format);
	vsnprintf(str, sizeof(str), format, list);
	str[sizeof(str) - 1] = 0;
	va_end(list);

	printf("%s\n", str);
}

static void print_error(const char *format, ...)
{
	va_list list;
	char str[58 + 80 + 1];

	va_start(list, format);
	vsnprintf(str, sizeof(str), format, list);
	str[sizeof(str) - 1] = 0;
	va_end(list);

	printf( "%s", str);
}

/* Mode select */

static void nextmode(void)
{
	mode++;
	//if (mode>4) {
	if (mode>3) {
		mode = 1;
	}
	xwii_iface_set_led(iface, XWII_LED(1), (mode==1));
	xwii_iface_set_led(iface, XWII_LED(2), (mode==2));
	xwii_iface_set_led(iface, XWII_LED(3), (mode==3));
	xwii_iface_set_led(iface, XWII_LED(4), (mode==4));
}

/* key events */

static void key_handle(const struct xwii_event *event)
{
	unsigned int code = event->v.key.code;
	bool pressed = event->v.key.state;

	keycode = 0;
	if (code == XWII_KEY_LEFT) {
		keycode = XKeysymToKeycode(display, XK_Left);
	} else if (code == XWII_KEY_RIGHT) {
		keycode = XKeysymToKeycode(display, XK_Right);
	} else if (code == XWII_KEY_UP) {
		keycode = XKeysymToKeycode(display, XK_Up);
	} else if (code == XWII_KEY_DOWN) {
		keycode = XKeysymToKeycode(display, XK_Down);
	} else if (code == XWII_KEY_A) {
		if (mode == 1) keycode = XKeysymToKeycode(display, XK_Return);
		if (mode == 2) XTestFakeButtonEvent( display, 1, pressed, CurrentTime );
	} else if (code == XWII_KEY_B) {
		if (mode == 1) keycode = XKeysymToKeycode(display, XK_BackSpace);
		if (mode == 2) XTestFakeButtonEvent( display, 3, pressed, CurrentTime );
	} else if (code == XWII_KEY_HOME) {
		keycode = XKeysymToKeycode(display, XK_Escape);
	} else if (code == XWII_KEY_MINUS) {
		keycode = XKeysymToKeycode(display, XK_minus);
	} else if (code == XWII_KEY_PLUS) {
		keycode = XKeysymToKeycode(display, XK_equal);
	} else if (code == XWII_KEY_ONE) {
		keycode = XKeysymToKeycode(display, XK_P);
	} else if (code == XWII_KEY_TWO) {
		//keycode = XKeysymToKeycode(display, XK_C);
		if (pressed) nextmode();
		xwii_iface_rumble(iface, pressed);
	} else {
		printf("Unknown keycode! '%d'\n",code);
	}
	if (keycode) {
		printf("[KeyEvent (Main): %d:%d]\n",keycode, pressed);
		XTestFakeKeyEvent(display, keycode, pressed, 0);
	}
	XFlush(display);
}

/* Accelerometer */

static void accel_handle(const struct xwii_event *event)
{
	//printf("[Accel: %4d,%4d,%4d]\n", event->v.abs[0].x, event->v.abs[0].y, event->v.abs[0].z);
}

/* IR events */

static void ir_handle(const struct xwii_event *event)
{
	int amount_of_valid_points = 0;
	int x[2] = {0};
	int y[2] = {0};
	int mouse_x = 0;
	int mouse_y = 0;
	if (xwii_event_ir_is_valid(&event->v.abs[0])) {
		printf("IR1: (%i,%i) ", event->v.abs[0].x, event->v.abs[0].y);
		x[amount_of_valid_points] = event->v.abs[0].x;
		y[amount_of_valid_points] = event->v.abs[0].y;
		amount_of_valid_points++;
	}

	if ((xwii_event_ir_is_valid(&event->v.abs[1]))&&(event->v.abs[1].x!=0)&&(event->v.abs[1].y!=0)) {
		printf("IR2: (%i,%i) ", event->v.abs[1].x, event->v.abs[1].y);
		x[amount_of_valid_points] = event->v.abs[0].x;
		y[amount_of_valid_points] = event->v.abs[0].y;
		amount_of_valid_points++;
	}

	if ((xwii_event_ir_is_valid(&event->v.abs[2]))&&(event->v.abs[2].x!=0)&&(event->v.abs[2].y!=0)) {
		printf("IR3: (%i,%i) ", event->v.abs[2].x, event->v.abs[2].y);
		if (amount_of_valid_points<2) {
			x[amount_of_valid_points] = event->v.abs[0].x;
			y[amount_of_valid_points] = event->v.abs[0].y;
			amount_of_valid_points++;
		}
	}

	if ((xwii_event_ir_is_valid(&event->v.abs[3]))&&(event->v.abs[3].x!=0)&&(event->v.abs[3].y!=0)) {
		printf("IR4: (%i,%i) ", event->v.abs[3].x, event->v.abs[3].y);
		if (amount_of_valid_points<2) {
			x[amount_of_valid_points] = event->v.abs[0].x;
			y[amount_of_valid_points] = event->v.abs[0].y;
			amount_of_valid_points++;
		}
	}
	printf("\n");

	//Fallback wanneer er maar 1 punt gevonden is
	//Oftewel: renze's prutsmethode :P
	//if (amount_of_valid_points == 1) {
		mouse_x = screen_width+5-x[0]*((screen_width+5)/1000.0);
		mouse_y = y[0]*(screen_height/750.0);
		//printf("Mouse_y = %d*(%d/%d)\n",y[0],screen_height,1024);
	//}

	//De waarden 1000 en 750 heb ik experimenteel voor mijn situatie bepaald.
	//De sensor geeft een waarde van 0-1023
	//(De +5 is een offset waardoor ik bij de rand van mijn scherm kan XD )

	//Met meerdere punten werkt het beter
	//if (amount_of_valid_points == 2) {
		//TO-DO, spieken in de "echte" code hoe het moet
	//}

	if ((mode == 2)&&(amount_of_valid_points>0)) { //Simulate mouse movement in mode 2.
		XTestFakeMotionEvent( display, -1, mouse_x, mouse_y, CurrentTime );
		XFlush(display);
	}
}

/* motion plus */

static bool mp_do_refresh;

static void mp_handle(const struct xwii_event *event)
{
	static int32_t mp_x, mp_y;
	int32_t x, y, z, factor, i;

	if (mp_do_refresh) {
		xwii_iface_get_mp_normalization(iface, &x, &y, &z, &factor);
		x = event->v.abs[0].x + x;
		y = event->v.abs[0].y + y;
		z = event->v.abs[0].z + z;
		xwii_iface_set_mp_normalization(iface, x, y, z, factor);
	}

	x = event->v.abs[0].x;
	y = event->v.abs[0].y;
	z = event->v.abs[0].z;

	if (mp_do_refresh) {
		/* try to stabilize calibration as MP tends to report huge
		 * values during initialization for 1-2s. */
		if (x < 5000 && y < 5000 && z < 5000)
			mp_do_refresh = false;
	}

	printf("[MP: %6d, %6d, %6d]\n", (int16_t)x,(int16_t)y,(int16_t)z);
}

static void mp_normalization_toggle(void)
{
	int32_t x, y, z, factor;

	xwii_iface_get_mp_normalization(iface, &x, &y, &z, &factor);
	if (!factor) {
		xwii_iface_set_mp_normalization(iface, x, y, z, 50);
		print_info("Info: Enable MP Norm: (%i:%i:%i)",
			    (int)x, (int)y, (int)z);
	} else {
		xwii_iface_set_mp_normalization(iface, x, y, z, 0);
		print_info("Info: Disable MP Norm: (%i:%i:%i)",
			    (int)x, (int)y, (int)z);
	}
}

static void mp_refresh(void)
{
	mp_do_refresh = true;
}

/* nunchuk */

bool nunchuk_c = false;
bool nunchuk_z = false;

static void nunchuk_handle(const struct xwii_event *event)
{
	double val;
	const char *str = " ";
	int32_t v;

	if (event->type == XWII_EVENT_NUNCHUK_MOVE) {
		printf("[Nunchuk: %4d,%4d  %4d,%4d,%4d, %1d, %1d]\n", event->v.abs[0].x, event->v.abs[0].y, event->v.abs[1].x, event->v.abs[1].y, event->v.abs[1].z, nunchuk_c, nunchuk_z);
	}

	if (event->type == XWII_EVENT_NUNCHUK_KEY) {
		if (event->v.key.code == XWII_KEY_C) {
			nunchuk_c = event->v.key.state;
		} else if (event->v.key.code == XWII_KEY_Z) {
			nunchuk_z = event->v.key.state;
		}
	}
}

/* balance board */

static void bboard_handle(const struct xwii_event *event)
{
	uint16_t w, x, y, z;

	w = event->v.abs[0].x;
	x = event->v.abs[1].x;
	y = event->v.abs[2].x;
	z = event->v.abs[3].x;
	printf("[bboard: %4d, %4d, %4d, %4d]\n", w,x,y,z);
}

/* pro controller */

static void pro_handle(const struct xwii_event *event)
{
	uint16_t code = event->v.key.code;
	int32_t v;
	bool pressed = event->v.key.state;

	if (event->type == XWII_EVENT_PRO_CONTROLLER_MOVE) {
		v = event->v.abs[0].x;
		printf("[Pro: (%5d", v);
		v = -event->v.abs[0].y;
		printf(", %5d) ", v);
		v = event->v.abs[1].x;
		printf("(%5d", v);
		v = -event->v.abs[1].y;
		printf(", %5d)]\n", v);
	} else if (event->type == XWII_EVENT_PRO_CONTROLLER_KEY) {
		//if (pressed)
		//	str = "X";
		//else
		//	str = " ";
		keycode = 0;
		if (code == XWII_KEY_A) {
			keycode = XKeysymToKeycode(display, XK_Right);
		} else if (code == XWII_KEY_B) {
			keycode = XKeysymToKeycode(display, XK_Down);
		} else if (code == XWII_KEY_X) {
			keycode = XKeysymToKeycode(display, XK_Up);
		} else if (code == XWII_KEY_Y) {
			keycode = XKeysymToKeycode(display, XK_Left);
		} else if (code == XWII_KEY_PLUS) {
			printf("[Pro: PLUS]\n");
		} else if (code == XWII_KEY_MINUS) {
			printf("[Pro: MINUS]\n");
		} else if (code == XWII_KEY_HOME) {
			printf("[Pro: HOME]\n");
		} else if (code == XWII_KEY_LEFT) {
			keycode = XKeysymToKeycode(display, XK_A);
		} else if (code == XWII_KEY_RIGHT) {
			keycode = XKeysymToKeycode(display, XK_D);
		} else if (code == XWII_KEY_UP) {
			keycode = XKeysymToKeycode(display, XK_W);
		} else if (code == XWII_KEY_DOWN) {
			keycode = XKeysymToKeycode(display, XK_S);
		} else if (code == XWII_KEY_TL) {
			keycode = XKeysymToKeycode(display, XK_Q);
		} else if (code == XWII_KEY_TR) {
			keycode = XKeysymToKeycode(display, XK_E);
		} else if (code == XWII_KEY_ZL) {
			keycode = XKeysymToKeycode(display, XK_space);
		} else if (code == XWII_KEY_ZR) {
			printf("[Pro: ZR]\n");
		} else if (code == XWII_KEY_THUMBL) {
			printf("[Pro: THUMB L]\n");
		} else if (code == XWII_KEY_THUMBR) {
			printf("[Pro: THUMB R]\n");
		}
		if (keycode) {
			printf("[KeyEvent (Classic/Pro): %d:%d]\n",keycode, pressed);
			XTestFakeKeyEvent(display, keycode, pressed, 0);
		}
		XFlush(display);
	}
}

/* classic controller */

static void classic_handle(const struct xwii_event *event)
{
	struct xwii_event ev;
	int32_t v;
	const char *str;
	/* forward key events to pro handler */
	if (event->type == XWII_EVENT_CLASSIC_CONTROLLER_KEY) {
		ev = *event;
		ev.type = XWII_EVENT_PRO_CONTROLLER_KEY;
		return pro_handle(&ev);
	}

	/* forward axis events to pro handler... */
	if (event->type == XWII_EVENT_CLASSIC_CONTROLLER_MOVE) {
		ev = *event;
		ev.type = XWII_EVENT_PRO_CONTROLLER_MOVE;
		ev.v.abs[0].x *= 45;
		ev.v.abs[0].y *= 45;
		ev.v.abs[1].x *= 45;
		ev.v.abs[1].y *= 45;
		pro_handle(&ev);

		/* ...but handle RT/LT triggers which are not reported by pro
		 * controllers. Note that if they report MAX (31) a key event is
		 * sent, too. */
		printf("[Classic (RT&LT): %3d, %3d]\n", event->v.abs[2].x, event->v.abs[2].y);
	}
}

/* guitar */
static void guitar_handle(const struct xwii_event *event)
{
	uint16_t code = event->v.key.code;
	bool pressed = event->v.key.state;
	int32_t v;

	if (event->type == XWII_EVENT_GUITAR_MOVE) {
		printf("[Guitar: %4d,%4d   %4d]\n", event->v.abs[0].x, event->v.abs[0].y, event->v.abs[1].x);
	} else if (event->type == XWII_EVENT_GUITAR_KEY) {
		switch (code) {
		case XWII_KEY_FRET_FAR_UP:
			if (pressed) {
				printf("Guitar key: 'fret far up'\n");
			}
			break;
		case XWII_KEY_FRET_UP:
			if (pressed) {
				printf("Guitar key: 'fret up'\n");
			}
			break;
		case XWII_KEY_FRET_MID:
			if (pressed) {
				printf("Guitar key: 'fret mid'\n");
			}
			break;
		case XWII_KEY_FRET_LOW:
			if (pressed) {
				printf("Guitar key: 'fret low'\n");
			}
			break;
		case XWII_KEY_FRET_FAR_LOW:
			if (pressed) {
				printf("Guitar key: 'fret far low'\n");
			}
			break;
		case XWII_KEY_STRUM_BAR_UP:
			if (pressed) {
				printf("Guitar key: 'strum bar up'\n");
			}
			break;
		case XWII_KEY_STRUM_BAR_DOWN:
			if (pressed) {
				printf("Guitar key: 'strum bar down'\n");
			}
			break;
		case XWII_KEY_HOME:
			if (pressed) {
				printf("Guitar key: 'home'\n");
			}
			break;
		case XWII_KEY_PLUS:
			if (pressed) {
				printf("Guitar key: 'plus'\n");
			}
			break;
		}
	}
}

/* guitar hero drums */
static void drums_handle(const struct xwii_event *event)
{
	uint16_t code = event->v.key.code;
	bool pressed = event->v.key.state;
	int32_t v;
	int n;

	if (event->type == XWII_EVENT_DRUMS_KEY) {
		switch (code) {
		case XWII_KEY_MINUS:
			if (pressed) {
				printf("Drums key: 'minus'\n");
			}
			break;
		case XWII_KEY_PLUS:
			if (pressed) {
				printf("Drums key: 'plus'\n");
			}
			break;
		default:
			break;
		}
	}

	if (event->type != XWII_EVENT_DRUMS_MOVE)
		return;

	v = event->v.abs[XWII_DRUMS_ABS_PAD].x;
	//mvprintw(38, 145, "%3d", v);
	//-25 to 25

	v = event->v.abs[XWII_DRUMS_ABS_PAD].y;
	//mvprintw(38, 154, "%3d", v);
	//-20 to 20

	for (n = 0; n < XWII_DRUMS_ABS_NUM; ++n) {
		if (n == XWII_DRUMS_ABS_BASS) {
			v = event->v.abs[n].x;
			printf("Drums: Bass = %4d\n",v);
		} else {
			switch (n) {
			case XWII_DRUMS_ABS_CYMBAL_RIGHT:
				printf("Drums: cymbal right\n");
				break;
			case XWII_DRUMS_ABS_TOM_LEFT:
				printf("Drums: tom left\n");
				break;
			case XWII_DRUMS_ABS_CYMBAL_LEFT:
				printf("Drums: cymbal right\n");
				break;
			case XWII_DRUMS_ABS_TOM_FAR_RIGHT:
				printf("Drums: tom far right\n");
				break;
			case XWII_DRUMS_ABS_TOM_RIGHT:
				printf("Drums: tom right\n");
				break;
			}
		}
	}
}

/* LEDs */

static bool led_state[4];

static void led_handle(int n, bool on)
{
	//mvprintw(5, 59 + n*5, on ? "(#%i)" : " -%i ", n+1);
}

static void led_toggle(int n)
{
	int ret;

	led_state[n] = !led_state[n];
	ret = xwii_iface_set_led(iface, XWII_LED(n+1), led_state[n]);
	if (ret) {
		print_error("Error: Cannot toggle LED %i: %d", n+1, ret);
		led_state[n] = !led_state[n];
	}
	led_handle(n, led_state[n]);
}

static void led_refresh(int n)
{
	int ret;

	ret = xwii_iface_get_led(iface, XWII_LED(n+1), &led_state[n]);
	if (ret)
		print_error("Error: Cannot read LED state");
	else
		led_handle(n, led_state[n]);
}

/* battery status */

static void battery_handle(uint8_t capacity)
{
	printf("Battery: %3u%%\n", capacity);
}

static void battery_refresh(void)
{
	int ret;
	uint8_t capacity;

	ret = xwii_iface_get_battery(iface, &capacity);
	if (ret)
		print_error("Error: Cannot read battery capacity");
	else
		battery_handle(capacity);
}

/* device type */

static void devtype_refresh(void)
{
	int ret;
	char *name;

	ret = xwii_iface_get_devtype(iface, &name);
	if (ret) {
		print_error("Error: Cannot read device type");
	} else {
		//mvprintw(9, 28, "                                                   ");
		//mvprintw(9, 28, "%s", name);
		free(name);
	}
}

/* extension type */

static void extension_refresh(void)
{
	int ret;
	char *name;

	ret = xwii_iface_get_extension(iface, &name);
	if (ret) {
		print_error("Error: Cannot read extension type");
	} else {
		//mvprintw(7, 54, "                      ");
		printf("Extension: %s\n",name);
		free(name);
	}
}

/* basic setup */

static void refresh_all(void)
{
	battery_refresh();
	led_refresh(0);
	led_refresh(1);
	led_refresh(2);
	led_refresh(3);
	devtype_refresh();
	extension_refresh();
	mp_refresh();
}

/* device watch events */

static void handle_watch(void)
{
	static unsigned int num;
	int ret;

	print_info("Info: Watch Event #%u", ++num);

	ret = xwii_iface_open(iface, xwii_iface_available(iface) |
				     XWII_IFACE_WRITABLE);
	if (ret)
		print_error("Error: Cannot open interface: %d", ret);

	refresh_all();
}

static int run_iface(struct xwii_iface *iface)
{
	struct xwii_event event;
	int ret = 0, fds_num;
	struct pollfd fds[2];

	memset(fds, 0, sizeof(fds));
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[1].fd = xwii_iface_get_fd(iface);
	fds[1].events = POLLIN;
	fds_num = 2;

	ret = xwii_iface_watch(iface, true);
	if (ret)
		print_error("Error: Cannot initialize hotplug watch descriptor");

	while (true) {
		ret = poll(fds, fds_num, -1);
		if (ret < 0) {
			if (errno != EINTR) {
				ret = -errno;
				print_error("Error: Cannot poll fds: %d", ret);
				break;
			}
		}

		ret = xwii_iface_dispatch(iface, &event, sizeof(event));
		if (ret) {
			if (ret != -EAGAIN) {
				print_error("Error: Read failed with err:%d",
					    ret);
				break;
			}
		} else {
			//printf("EVENT %d\n",event.type);
			switch (event.type) {
			case XWII_EVENT_GONE:
				print_info("Info: Device gone");
				fds[1].fd = -1;
				fds[1].events = 0;
				fds_num = 1;
				break;
			case XWII_EVENT_WATCH:
				handle_watch();
				break;
			case XWII_EVENT_KEY:
				key_handle(&event);
				break;
			case XWII_EVENT_ACCEL:
				accel_handle(&event);
				break;
			case XWII_EVENT_IR:
				ir_handle(&event);
				break;
			case XWII_EVENT_MOTION_PLUS:
				mp_handle(&event);
				break;
			case XWII_EVENT_NUNCHUK_KEY:
			case XWII_EVENT_NUNCHUK_MOVE:
				nunchuk_handle(&event);
				break;
			case XWII_EVENT_CLASSIC_CONTROLLER_KEY:
			case XWII_EVENT_CLASSIC_CONTROLLER_MOVE:
				classic_handle(&event);
				break;
			case XWII_EVENT_BALANCE_BOARD:
				bboard_handle(&event);
				break;
			case XWII_EVENT_PRO_CONTROLLER_KEY:
			case XWII_EVENT_PRO_CONTROLLER_MOVE:
				pro_handle(&event);
				break;
			case XWII_EVENT_GUITAR_KEY:
			case XWII_EVENT_GUITAR_MOVE:
				guitar_handle(&event);
				break;
			case XWII_EVENT_DRUMS_KEY:
			case XWII_EVENT_DRUMS_MOVE:
				drums_handle(&event);
				break;
			}
		}
	}

	return ret;
}

static int enumerate()
{
	struct xwii_monitor *mon;
	char *ent;
	int num = 0;

	mon = xwii_monitor_new(false, false);
	if (!mon) {
		printf("Cannot create monitor\n");
		return -EINVAL;
	}

	while ((ent = xwii_monitor_poll(mon))) {
		printf("  Found device #%d: %s\n", ++num, ent);
		free(ent);
	}

	xwii_monitor_unref(mon);
	return 0;
}

static char *get_dev(int num)
{
	struct xwii_monitor *mon;
	char *ent;
	int i = 0;

	mon = xwii_monitor_new(false, false);
	if (!mon) {
		printf("Cannot create monitor\n");
		return NULL;
	}

	while ((ent = xwii_monitor_poll(mon))) {
		if (++i == num)
			break;
		free(ent);
	}

	xwii_monitor_unref(mon);

	if (!ent)
		printf("Cannot find device with number #%d\n", num);

	return ent;
}

int main(int argc, char **argv)
{
	mode = 1;
	int ret = 0;
	char *path = NULL;
	if (geteuid() != 0)
		printf("\n\n --- Warning: Please run as root! (sysfs+evdev access needed) ---\n\n");

	if (argc < 2 || !strcmp(argv[1], "-h")) {
		printf("Usage:\n");
		printf("\txwiictl [-h]: Show help\n");
		printf("\txwiictl list: List connected devices\n");
		printf("\txwiictl <num> <display>: Use device with number #num\n");
		printf("\txwiictl /sys/path/to/device <display>: Use given device\n");
		ret = -1;
	} else if (!strcmp(argv[1], "list")) {
		printf("Listing connected Wii Remote devices:\n");
		ret = enumerate();
		printf("End of device list\n");
	} else {
		if (argv[1][0] != '/')
			path = get_dev(atoi(argv[1]));

		ret = xwii_iface_new(&iface, path ? path : argv[1]);
		free(path);
		if (ret) {
			printf("Cannot create xwii_iface '%s' err:%d\n",
								argv[1], ret);
		} else {
			//Start?

			refresh_all();

			ret = xwii_iface_open(iface,
					      xwii_iface_available(iface) |
					      XWII_IFACE_WRITABLE);
			if (ret)
				print_error("Error: Cannot open interface: %d",
					    ret);
			char* displayname = NULL;
			if (argc > 2) {
				displayname = argv[2];
				printf("Using display '%s'\n",displayname);
			}
			display = XOpenDisplay(NULL);
			if (!display) {
				print_error("Error: Cannot open display.", 1);
			}
			scrn = DefaultScreenOfDisplay(display);
			screen_height = scrn->height;
			screen_width  = scrn->width;

			xwii_iface_set_led(iface, XWII_LED(1), 1);
			xwii_iface_set_led(iface, XWII_LED(2), 0);
			xwii_iface_set_led(iface, XWII_LED(3), 0);
			xwii_iface_set_led(iface, XWII_LED(4), 0);

			
			ret = run_iface(iface);
			xwii_iface_unref(iface);
			if (ret) {
				print_error("Program failed.");
			}
		}
	}

	return abs(ret);
}

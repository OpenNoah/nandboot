#include "io.h"
#include "lcd.h"
#include "gpio.h"
#include "config.h"

typedef struct hw_lcd_t {
	_IO uint32_t LCDCFG;
	_IO uint32_t LCDVSYNC;
	_IO uint32_t LCDHSYNC;
	_IO uint32_t LCDVAT;
	_IO uint32_t LCDDAH;
	_IO uint32_t LCDDAV;

	_IO uint32_t LCDPS;
	_IO uint32_t LCDCLS;
	_IO uint32_t LCDSPL;
	_IO uint32_t LCDREV;
	_IO uint32_t _RESERVED0[2];

	_IO uint32_t LCDCTRL;
	_IO uint32_t LCDSTATE;
	_I  uint32_t LCDIID;
	_IO uint32_t _RESERVED1[1];

	_IO uint32_t LCDDA0;
	_I  uint32_t LCDSA0;
	_I  uint32_t LCDFID0;
	_I  uint32_t LCDCMD0;

	_IO uint32_t LCDDA1;
	_I  uint32_t LCDSA1;
	_I  uint32_t LCDFID1;
	_I  uint32_t LCDCMD1;
} hw_lcd_t;

static hw_lcd_t *lcd = LCD_BASE;

struct lcd_desc_t {
	uint32_t da;
	uint32_t sa;
	uint32_t fid;
	uint32_t cmd;
};

static _IO struct lcd_desc_t *desc[2];
static _IO uint32_t *buf[2];

static int desc_idx = 0;

void lcd_init(void)
{
	lcd->LCDCTRL = 0;
	lcd->LCDCFG = (0 << 31) | (1 << 23) | (1 << 22) | (1 << 21) |
		      (1 << 20) | (0 << 19) | (0 << 18) | (0 << 17) |
		      (0 << 16) | (0 << 15) | (0 << 14) | (0 << 13) |
		      (0 << 12) |
		      (!!(config.lcd.flags & DRM_MODE_FLAG_NHSYNC) << 11) |
		      (!!(config.lcd.bus_flags & DRM_BUS_FLAG_PIXDATA_DRIVE_NEGEDGE) << 10) |
		      (!!(config.lcd.bus_flags & DRM_BUS_FLAG_DE_LOW) << 9) |
		      (!!(config.lcd.flags & DRM_MODE_FLAG_NVSYNC) << 8) |
		      ((config.lcd.bus_format == MEDIA_BUS_FMT_RGB666_1X18) << 7) |
		      (config.lcd.bus_format == MEDIA_BUS_FMT_RGB888_3X8 ? 0b1100 : 0);
	lcd->LCDVSYNC = config.lcd.vsync_end - config.lcd.vsync_start;
	lcd->LCDHSYNC = config.lcd.hsync_end - config.lcd.hsync_start;
	lcd->LCDVAT = (config.lcd.htotal << 16) | config.lcd.vtotal;
	lcd->LCDDAH = ((config.lcd.htotal - config.lcd.hsync_start) << 16) |
		      (config.lcd.htotal - (config.lcd.hsync_start - config.lcd.hdisplay));
	lcd->LCDDAV = ((config.lcd.vtotal - config.lcd.vsync_start) << 16) |
		      (config.lcd.vtotal - (config.lcd.vsync_start - config.lcd.vdisplay));
	lcd->LCDCTRL = (0b10 << 28) | (0 << 27) | (1 << 26) | (0b101);
	while (lcd->LCDCTRL & (1 << 3));

	// LCD buffers
	static const unsigned align = 16 * 4;
	static const unsigned bpp = 4;
	uint32_t bufs = kseg0_to_kseg1((void *)(((uint32_t)alloc(align +
		2 * config.lcd.vdisplay * config.lcd.hdisplay * bpp +
		sizeof(struct lcd_desc_t) * 2) + align) & ~(align - 1)));
	buf[0] = (void *)bufs;
	bufs += config.lcd.vdisplay * config.lcd.hdisplay * bpp;
	buf[1] = (void *)bufs;
	bufs += config.lcd.vdisplay * config.lcd.hdisplay * bpp;
	desc[0] = (void *)bufs;
	bufs += sizeof(struct lcd_desc_t);
	desc[1] = (void *)bufs;
	bufs += sizeof(struct lcd_desc_t);

	// LCD buffer descriptor
	desc[0]->da = kseg1_to_pa((void *)desc[0]);
	desc[0]->sa = kseg1_to_pa((void *)buf[0]);
	desc[0]->fid = 0;
	desc[0]->cmd = (config.lcd.vdisplay * config.lcd.hdisplay * bpp) / 4;
	lcd->LCDDA0 = kseg1_to_pa((void *)desc[0]);
	lcd->LCDDA1 = kseg1_to_pa((void *)desc[1]);

	// Initial image
	for (unsigned y = 0; y < config.lcd.vdisplay; y++)
		for (unsigned x = 0; x < config.lcd.hdisplay; x++)
			buf[0][y * config.lcd.hdisplay + x] =
				((~x & 0xff) << 16) |
				((y & 0xff) << 8) |
				((x & 0xff) << 0);

	// Enable LCD controller
	lcd->LCDCTRL |= 1 << 3;
	gpio_lcd_enable(1);
}

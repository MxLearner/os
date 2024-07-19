#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <amdev.h>
#include "output_rgb.h"

#define KEYNAME(key) \
    [AM_KEY_##key] = #key,
static const char *key_names[] = {
    AM_KEYS(KEYNAME)};

int img_width = 300;
int img_height = 300;

uint32_t buffer[1024 * 1024];

void draw_image(uint32_t *buffer, int width, int height);
void check_for_exit();

int main()
{
    ioe_init();
    int w = io_read(AM_GPU_CONFIG).width;
    int h = io_read(AM_GPU_CONFIG).height;
    draw_image(buffer, w, h);
    io_write(AM_GPU_FBDRAW, .x = 0, .y = 0, .pixels = buffer, .w = w, .h = h, .sync = true);
    printf("Image displayed\n");
    while (1)
    {
        check_for_exit();
    }
}

void draw_image(uint32_t *buffer, int width, int height)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int img_x = x * img_width / width;
            int img_y = y * img_height / height;
            int pixel_index = (img_y * img_width + img_x) * 3;
            uint32_t color = 0;
            color |= (uint32_t)output_rgb[pixel_index] << 16;
            color |= (uint32_t)output_rgb[pixel_index + 1] << 8;
            color |= (uint32_t)output_rgb[pixel_index + 2];

            buffer[y * width + x] = color;
        }
    }
}

void check_for_exit()
{
    AM_INPUT_KEYBRD_T event = {.keycode = AM_KEY_NONE};
    ioe_read(AM_INPUT_KEYBRD, &event);
    if (event.keycode != AM_KEY_NONE && event.keydown)
    {
        if (event.keycode == AM_KEY_ESCAPE)
            halt(0);
        printf("Key pressed: ");
        printf("%s\n", key_names[event.keycode]);
    }
}

int readkey()
{
    AM_INPUT_KEYBRD_T event = {.keycode = AM_KEY_NONE};
    ioe_read(AM_INPUT_KEYBRD, &event);
    if (event.keycode != AM_KEY_NONE && event.keydown)
    {
        return event.keycode;
    }
    else
    {
        return AM_KEY_NONE;
    }
}

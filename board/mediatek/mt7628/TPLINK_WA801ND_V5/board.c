#include <asm/gpio.h>
#include <linux/delay.h>
#include <env.h>
#include <init.h>

/* All LEDs are on Bank PB (gpio1), offsets within the bank */
#define LED_POWER_GPIO   (32 + 4)   /* PB4  active-low  */
#define LED_LAN_GPIO     (32 + 5)   /* PB5  active-low  */
#define LED_WPS_GRN_GPIO (32 + 10)  /* PB10 active-high */
#define LED_WPS_RED_GPIO     (32 + 11)  /* PB11 active-high */
#define LED_WLAN_GPIO    (32 + 12)  /* PB12 active-low  */

#define RESET_GPIO       (32 + 6)   /* PB6  active-low */
#define WPS_GPIO    (32 + 14)  /* PB14 active-low */

static void led_al(unsigned int gpio, int on)
{
    gpio_request(gpio, "led");
    gpio_direction_output(gpio, on ? 0 : 1);
}

static void led_ah(unsigned int gpio, int on)
{
    gpio_request(gpio, "led");
    gpio_direction_output(gpio, on ? 1 : 0);
}

int board_late_init(void)
{
    int i;

    /* startup blink - all on */
    led_al(LED_POWER_GPIO,   1);
    led_al(LED_LAN_GPIO,     1);
    led_al(LED_WLAN_GPIO,    1);
    led_ah(LED_WPS_GRN_GPIO, 1);
    led_ah(LED_WPS_RED_GPIO, 1);
    mdelay(300);

    /* all but power off */
    led_al(LED_LAN_GPIO,     0);
    led_al(LED_WLAN_GPIO,    0);
    led_ah(LED_WPS_GRN_GPIO, 0);
    led_ah(LED_WPS_RED_GPIO, 0);
    mdelay(100);

    /* check reset button */
    gpio_request(RESET_GPIO, "reset");
    gpio_direction_input(RESET_GPIO);
    gpio_request(WPS_GPIO, "wps");
    gpio_direction_input(WPS_GPIO);

    if (gpio_get_value(RESET_GPIO) == 0) {
        printf("Reset held - starting recovery\n");

        led_ah(LED_WPS_RED_GPIO, 1);

        env_set("bootcmd", "run recovery");
    } else if (gpio_get_value(WPS_GPIO) == 0) {
        printf("WPS held - starting WPS boot\n");
        led_ah(LED_WPS_GRN_GPIO, 1);
        env_set("bootcmd", "run wpsboot");
    }

    return 0;
}
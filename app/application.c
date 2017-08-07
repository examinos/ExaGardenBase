#include <application.h>
#include <usb_talk.h>

// LED instance
bc_led_t led;

uint64_t my_device_address;
uint64_t peer_device_address;

// Button instance
bc_button_t button;
bc_button_t button_5s;

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

        if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_pulse(&led, 100);

        static uint16_t event_count = 0;

        bc_radio_pub_push_button(&event_count);

        event_count++;
    }
    else if (event == BC_BUTTON_EVENT_HOLD)
    {
        static bool base = true;
        if (base)
            bc_radio_enrollment_start();
        else
            bc_radio_enroll_to_gateway();
        base = !base;

        bc_led_set_mode(&led, BC_LED_MODE_BLINK_FAST);
    }
}

static void button_5s_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
	(void) self;
	(void) event_param;

	if (event == BC_BUTTON_EVENT_HOLD)
	{
		bc_radio_enrollment_stop();

		uint64_t devices_address[BC_RADIO_MAX_DEVICES];

		// Read all remote address
		bc_radio_get_peer_devices_address(devices_address, BC_RADIO_MAX_DEVICES);

		for (int i = 0; i < BC_RADIO_MAX_DEVICES; i++)
		{
			if (devices_address[i] != 0)
			{
				// Remove device
				bc_radio_peer_device_remove(devices_address[i]);
			}
		}

		bc_led_pulse(&led, 2000);
	}
}

void radio_event_handler(bc_radio_event_t event, void *event_param)
{
    (void) event_param;

    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    peer_device_address = bc_radio_get_event_device_address();

    if (event == BC_RADIO_EVENT_ATTACH)
    {
        bc_led_pulse(&led, 1000);
    }
    else if (event == BC_RADIO_EVENT_DETACH)
	{
		bc_led_pulse(&led, 3000);
	}
    else if (event == BC_RADIO_EVENT_ATTACH_FAILURE)
    {
        bc_led_pulse(&led, 10000);
    }

    else if (event == BC_RADIO_EVENT_INIT_DONE)
    {
        my_device_address = bc_radio_get_device_address();
    }
}

void bc_radio_on_push_button(uint64_t *peer_device_address, uint16_t *event_count)
{
	(void) peer_device_address;
	(void) event_count;

    bc_led_pulse(&led, 100);
}



void bc_radio_on_thermometer(uint64_t *peer_device_address, uint8_t *i2c, float *temperature)
{
    char prefix[20];
    sprintf(prefix, "%llu", *peer_device_address);
    usb_talk_publish_thermometer(prefix, i2c, temperature);

    bc_led_pulse(&led, 10);
}

void bc_radio_on_humidity(uint64_t *peer_device_address, uint8_t *i2c, float *humidity)
{
    char prefix[20];
    sprintf(prefix, "%llu", *peer_device_address);
    usb_talk_publish_humidity_sensor(prefix, i2c, humidity);

    bc_led_pulse(&led, 10);}

void bc_radio_on_lux_meter(uint64_t *peer_device_address, uint8_t *i2c, float *lux)
{
    char prefix[20];
    sprintf(prefix, "%llu", *peer_device_address);
    usb_talk_publish_lux_meter(prefix, i2c, lux);

    bc_led_pulse(&led, 10);}

void bc_radio_on_barometer(uint64_t *peer_device_address, uint8_t *i2c, float *pressure, float *altitude)
{
    char prefix[20];
    sprintf(prefix, "%llu", *peer_device_address);
    usb_talk_publish_barometer(prefix, i2c, pressure, altitude);

    bc_led_pulse(&led, 10);}

static void relay_state_set(usb_talk_payload_t *payload, void *param)
{
    (void) param;
    int state;
    uint16_t onoff;
    /*bool bit;*/


    if (usb_talk_payload_get_int(payload, &state))
    {
        bc_led_pulse(&led, 100);
        onoff = state;
        bc_radio_pub_relay(&onoff);
    }

/*
    onoff = state;
    bc_radio_pub_relay(&onoff);

    bit = onoff & (1 << 0);
    usb_talk_publish_relay(PREFIX_BASE, &bit);*/
}

void application_init(void)
{
    usb_talk_init();

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    bc_button_init(&button_5s, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button_5s, button_5s_event_handler, NULL);
    bc_button_set_hold_time(&button_5s, 5000);

    // Initialize radio
    bc_radio_init();
    bc_radio_set_event_handler(radio_event_handler, NULL);
    bc_radio_listen();

    // MQTT sub
    usb_talk_sub("base/relay/-/state/set", relay_state_set, NULL);
}


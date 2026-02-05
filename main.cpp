#include <iostream>
#include <sys/time.h>
#include <cstdint>

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define CLK_DIV 125
#define WRAP_VALUE 999
#define PWM_MAX 1000

#define SW0 9 // left button
#define SW1 8 // middle button
#define SW2 7 // right button

#define LED1 22 // right led
#define LED2 21 // middle led
#define LED3 20 // left led

static constexpr uint32_t POLL_MS = 10; // poll every 10 ms
static constexpr uint32_t RELEASE = 1000; // 1 second after release

class GPIOPin {
public:
    GPIOPin(int pin, bool input = true, bool pullup = true, bool invert = false)
        : pin_(pin), is_input_(input), valid_(false) {
        if (pin_ < 0 || pin_ > 29) return;

        const uint32_t bit = (1u << pin_);
        if (pins_in_use & bit) return;
        pins_in_use |= bit;
        valid_ = true;

        gpio_init(pin_);

        if (is_input_) {
            gpio_set_dir(pin_, GPIO_IN);
            if (pullup) {
                gpio_pull_up(pin_);
            } else {
                gpio_disable_pulls(pin_);
            }
            gpio_set_inover(pin_, invert ? GPIO_OVERRIDE_INVERT : GPIO_OVERRIDE_NORMAL);
        } else {
            gpio_set_dir(pin_, GPIO_OUT);
            gpio_disable_pulls(pin_);
            gpio_set_outover(pin_, invert ? GPIO_OVERRIDE_INVERT: GPIO_OVERRIDE_NORMAL);
        }
    }
    GPIOPin(const GPIOPin &) = delete; // non-copyable
    GPIOPin& operator = (const GPIOPin&) = delete; // non-assignable

    ~GPIOPin() {
        if (!valid_) return;
        gpio_set_inover(pin_, GPIO_OVERRIDE_NORMAL);
        gpio_set_outover(pin_, GPIO_OVERRIDE_NORMAL);
        gpio_disable_pulls(pin_);
        gpio_set_dir(pin_, GPIO_IN);
        pins_in_use &= ~(1u << pin_);
    }

    bool read() const {
        if (!valid_) return false;
        return gpio_get(pin_);
    }

    void write(bool value) {
        if (!valid_) return;
        gpio_put(pin_, value);
    }

    explicit operator bool() const {return valid_;}
    int number() const {return pin_;}

private:
    static uint32_t pins_in_use;
    int pin_;
    bool is_input_;
    bool valid_;
};

uint32_t GPIOPin :: pins_in_use = 0u;

// add construct for pwm channel mapping
class ButtonAndLed {
public:
    ButtonAndLed (int button_pin, int led_pin)
        : button_(button_pin, true, true, false),
          led_(led_pin, false, false, false) {}

    // checking for channel conflicts.
    void init_pwm() {
        const int pin = led_.number();

        // Find the PWM slice and channel this GPIO maps to
        const uint slice = pwm_gpio_to_slice_num(pin);
        const uint ch = pwm_gpio_to_channel(pin);
        const int ch_idx = (ch == PWM_CHAN_A) ? 0 : 1;


        if (pwm_used_[slice][ch_idx]) {
            printf("PWM conflict: GPIO %d wants slice %u channel %c, but it's already in use.\n",
                   pin, slice, ch_idx == 0 ? 'A' : 'B');
            pwm_ok_ = false;
            slice_ = slice;
            channel_ = ch;
            brightness_ = 0;
            led_on_ = false;
            off_deadline_ = nil_time;
            return;
        }
        // Mark channel as taken so later users can't grab it
        pwm_used_[slice][ch_idx] = true;
        gpio_set_function(pin, GPIO_FUNC_PWM);

        if (!slice_inited_[slice]) {
            pwm_config cfg = pwm_get_default_config();
            pwm_config_set_clkdiv(&cfg, CLK_DIV);
            pwm_config_set_wrap(&cfg, WRAP_VALUE);
            pwm_init(slice, &cfg, true);
            slice_inited_[slice] = true;
        } else {
            pwm_set_enabled(slice, true);
        }
        pwm_set_chan_level(slice, ch, 0);

        slice_ = slice;
        channel_ = ch;
        brightness_ = 0;
        led_on_ = false;
        off_deadline_ = nil_time;

        pwm_ok_ = true;
    }

    void update (absolute_time_t now) {
        bool pressed = (button_.read() == 0);

        if (pressed) {
            set_brightness(PWM_MAX);
            led_on_ = true;
            off_deadline_ = nil_time;
        } else if (led_on_) {
            if (is_nil_time(off_deadline_)) {
                off_deadline_ = make_timeout_time_ms(RELEASE);
            } else if (absolute_time_diff_us(now, off_deadline_) <= 0){

                set_brightness(0);
                led_on_ = false;
                off_deadline_ = nil_time;
            } else {
                uint32_t total_us = RELEASE * 1000u;
                int64_t  rem_us   = absolute_time_diff_us(now, off_deadline_);
                uint16_t b = (uint16_t)((uint64_t) PWM_MAX * rem_us / total_us);
                set_brightness(b);
            }
        }
    }

private:
    void set_brightness (uint16_t level) {
        if (level > PWM_MAX) level = PWM_MAX;
        brightness_ = level;

        if (pwm_ok_) {
            pwm_set_chan_level(slice_, channel_, brightness_);
        } else {
            led_.write(brightness_ > (PWM_MAX / 2));
        }
    }

    GPIOPin button_;
    GPIOPin led_;

    uint slice_ = 0;
    uint channel_ = PWM_CHAN_A;
    uint16_t brightness_ = 0;

    bool led_on_ = false;
    absolute_time_t off_deadline_ = nil_time;
    bool pwm_ok_ = false;
    static inline bool pwm_used_[NUM_PWM_SLICES][2] = {{false}};
    static inline bool slice_inited_[NUM_PWM_SLICES] = {false};
};

int main() {
    stdio_init_all();
    sleep_ms(100);

    ButtonAndLed pair1 (SW2, LED1);
    ButtonAndLed pair2 (SW1, LED2);
    ButtonAndLed pair3 (SW0, LED3);

    pair1.init_pwm();
    pair2.init_pwm();
    pair3.init_pwm();

    while (true) {
        absolute_time_t now = get_absolute_time();
        pair1.update(now);
        pair2.update(now);
        pair3.update(now);
        sleep_ms(POLL_MS);
    }
}

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "driver/timer.h"


const float radius = 0.6;
const float pi = 3.141593;


static void example_tg_timer_init(int group, int timer, int timer_interval)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = 16,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .auto_reload = false,
    };
    
    timer_init(group, timer, &config);

    /* Timer's counter will initially start from value below. */
    timer_set_counter_value(group, timer, 0);

    timer_start(group, timer);
}


void calc_speed(){

	double time = 0;
    timer_get_counter_time_sec(0, 0, &time);
    
    if (time < 0.0001){
        return;
    }
    else{
        printf("time passed: %f s\n", time);
        float rps = 1/(time);
        printf("RPS: %f\n", rps);
        float speed = 2*pi*radius*rps;
        printf("Speed: %f m/s\n", speed);
    }
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);

}

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_interrupt(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            calc_speed();
        }
    }
}


void read_data(void) {

    /* GPIO SETUP*/
    gpio_config_t io_conf;
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pin
    io_conf.pin_bit_mask = GPIO_SEL_17;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

		gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_interrupt, "gpio_interrupt", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(17, gpio_isr_handler, (void*) 17);

    example_tg_timer_init(TIMER_GROUP_0, TIMER_0, 100);
	while (true){
		vTaskDelay(400 / portTICK_RATE_MS);
	}
}
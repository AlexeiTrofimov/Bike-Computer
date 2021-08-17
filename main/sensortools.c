#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "driver/timer.h"
#include "cyclocomp.h"


const float pi = 3.141593;
const int interrupt_time = 15;

static xQueueHandle evt_queue = NULL;


static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;
    uint8_t itemPtr;
    xQueueSendFromISR(evt_queue, &itemPtr, &high_task_awoken);
    return high_task_awoken == pdTRUE;
}


static void interval_timer_init(int group, int timer, bool interrupt, int timer_interval)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = 16,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = interrupt ? TIMER_ALARM_EN : TIMER_ALARM_DIS,
        .auto_reload = interrupt,
    };
    
    timer_init(group, timer, &config);

    /* Timer's counter will initially start from value below. */
    timer_set_counter_value(group, timer, 0);
    
    if (interrupt){
        timer_set_alarm_value(group, timer, timer_interval*(TIMER_BASE_CLK/16));
    
        timer_enable_intr(group, timer);

        timer_isr_callback_add(group, timer, timer_group_isr_callback, NULL, 0);
    }
    
    timer_start(group, timer);
}


void calc_speed(){
	if (wheel_diameter != 0){
        double time = 0;
        timer_get_counter_time_sec(0, 0, &time);

        if (time < 0.0001F){  //prevents double readings
            return;
        }
        if (time >= interrupt_time){
            speedValue = 0;
            notify_change();
        }
        else{
            printf("circumference: %f m\n", wheel_diameter);
            printf("time passed: %f s\n", time);
            float rps = 1/(time);
            printf("RPS: %f\n", rps);
            speedValue = pi*wheel_diameter*rps*3.6F;
            printf("Speed: %f km/h\n", speedValue);
            notify_change();
        }
    }
    
    
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);

    timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
    timer_start(TIMER_GROUP_0, TIMER_1);

}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(evt_queue, &gpio_num, NULL);
}

static void gpio_interrupt(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(evt_queue, &io_num, portMAX_DELAY)) {
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

    evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_interrupt, "gpio_interrupt", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(17, gpio_isr_handler, (void*) 17);

    interval_timer_init(TIMER_GROUP_0, TIMER_0,false, 100);

    interval_timer_init(TIMER_GROUP_0, TIMER_1,true, interrupt_time);
}
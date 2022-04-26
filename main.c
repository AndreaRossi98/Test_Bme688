#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_drv_timer.h"
#include "bsp.h"
#include "app_error.h"
#include "nrf_drv_saadc.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_drv_twi.h"
#include "nrf_delay.h"

//librerie per i sensori
#include "reading_sps30.h"
#include "reading_scd41.h"
#include "sps30.h"
#include "scd4x_i2c.h"
#include "bme68x.h"
#include "common.h"

#define LED1 07
uint32_t count = 0;
#define SAMPLE_COUNT  UINT16_C(300)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //////TWI PART/////
                        ///////////////////
//TWI instance ID.
#if TWI0_ENABLED
#define TWI_INSTANCE_ID     0
#elif TWI1_ENABLED
#define TWI_INSTANCE_ID     1
#endif

//TWI instance
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

//TWI initialization
void twi_init (void)
{
    ret_code_t err_code;
    const nrf_drv_twi_config_t twi_config = {
       .scl                = 19,
       .sda                = 18,
       .frequency          = NRF_DRV_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

//Number of possible TWI addresses.
#define TWI_ADDRESSES      127
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //////SAADC PART/////
                        /////////////////////

void saadc_callback_handler(nrf_drv_saadc_evt_t const * p_event)
{
}

void saadc_init(void)
{
    ret_code_t err_code;
    // Create a config struct and assign it default values along with the Pin number for ADC Input.
    //nrf_saadc_channel_config_t channel0_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN1);
    nrf_saadc_channel_config_t channel1_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN2);
    nrf_saadc_channel_config_t channel2_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN3);
    channel2_config.gain = 1;
    channel1_config.gain = 1;
    //channel0_config.gain = 1;

    // Initialize the saadc 
    err_code = nrf_drv_saadc_init(NULL, saadc_callback_handler);
    APP_ERROR_CHECK(err_code);

    // Initialize the Channel which will be connected to that specific pin.
    //err_code = nrfx_saadc_channel_init(0, &channel0_config);
    //APP_ERROR_CHECK(err_code);
    err_code = nrfx_saadc_channel_init(1, &channel1_config);
    APP_ERROR_CHECK(err_code);
    err_code = nrfx_saadc_channel_init(2, &channel2_config);
    APP_ERROR_CHECK(err_code);
}

nrf_saadc_value_t adc_val;      //variable to hold the value read by the ADC

float ADC_TO_VOLTS (int adc)
{
    float volts;
    //volts = (0.00363385)*adc + 0.14411395;
    volts = (0.000884)*adc + 0.14776;
    return volts;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //////LOG PART/////
                        ///////////////////
void log_init(void)
{
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));  // check if any error occurred during its initialization
    NRF_LOG_DEFAULT_BACKENDS_INIT();  // Initialize the log backends module
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //////TIMER PART/////
                        /////////////////////

const nrfx_timer_t TIMER_LED = NRFX_TIMER_INSTANCE(0); // Timer 0 Enabled

void timer0_handler(nrf_timer_event_t event_type, void* p_context)
{
    switch(event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
            count++;
            nrf_gpio_pin_toggle(LED1);
            //printf("%d\n\r", count);
            //nrfx_saadc_sample_convert(1, &adc_val);
            //Vref = ADC_TO_VOLTS(adc_val);
            //printf("%d\n", adc_val);
            //NRF_LOG_INFO("%d) Vref [V]: " NRF_LOG_FLOAT_MARKER ";\r",i, NRF_LOG_FLOAT(Vref));
            break;

        default:
            // Nothing
            break;
    
    }
}

void timer_init(void)
{
    uint32_t err_code = NRF_SUCCESS;
    uint32_t time_ms = 500;        //DEFINISCE OGNI QUANTO SCATTA INTERRUPT DEL TIMER
    uint32_t time_ticks;  
    nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG; // Configure the timer instance to default settings

    err_code = nrfx_timer_init(&TIMER_LED, &timer_cfg, timer0_handler); // Initialize the timer0 with default settings
    APP_ERROR_CHECK(err_code); // check if any error occured

    time_ticks = nrfx_timer_ms_to_ticks(&TIMER_LED, time_ms); // convert ms to ticks

    // Assign a channel, pass the number of ticks & enable interrupt
    nrfx_timer_extended_compare(&TIMER_LED, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



int main(void)
{
    ret_code_t err_code;
    uint8_t address;
    uint8_t sample_data;
    bool detected_device = false;
    nrf_gpio_cfg_output(LED1); // Initialize the pin
    nrf_gpio_pin_set(LED1); // Turn off the LED
    log_init();
    //saadc_init(); 
    
    twi_init();

    NRF_LOG_INFO("Starting the program");
    //nrf_delay_ms(3000);

    

//////////////////////////////////////
///SCANNING OF CONNECTED SENSORS//////
//////////////////////////////////////   
/*    for (address = 1; address <= TWI_ADDRESSES; address++)
    {
        err_code = nrf_drv_twi_rx(&m_twi, address, &sample_data, sizeof(sample_data));
        if (err_code == NRF_SUCCESS)
        {
            detected_device = true;
            NRF_LOG_INFO("TWI device detected at address 0x%x.", address);
        }
        NRF_LOG_FLUSH();
    }

    if (!detected_device)
    {
        NRF_LOG_INFO("No device was found.");
        NRF_LOG_FLUSH();
    }
*/    
    timer_init();       //INIZIALIZZAZIONE DEL TIMER
    nrfx_timer_enable(&TIMER_LED);

//////////////////////////////
///SENSORS' INITIALIZATION////
//////////////////////////////
    //lettura_scd41(3);
    //lettura_sps30(1);

    struct bme68x_dev bme;
    int8_t rslt;
    struct bme68x_conf conf;
    struct bme68x_heatr_conf heatr_conf;
    struct bme68x_data data;
    uint32_t del_period;
    uint32_t time_ms = 0;
    uint8_t n_fields;
    uint16_t sample_count = 1;

    rslt = bme68x_interface_init(&bme, BME68X_I2C_INTF);
    bme68x_check_rslt("bme68x_interface_init", rslt);

    rslt = bme68x_init(&bme);
    bme68x_check_rslt("bme68x_init", rslt);

    //Check if rslt == BME68X_OK, report or handle if otherwise
    conf.filter = BME68X_FILTER_OFF;
    conf.odr = BME68X_ODR_NONE;
    conf.os_hum = BME68X_OS_16X;
    conf.os_pres = BME68X_OS_1X;
    conf.os_temp = BME68X_OS_2X;
    rslt = bme68x_set_conf(&conf, &bme);
    bme68x_check_rslt("bme68x_set_conf", rslt);
printf("chip id 0x%x\n\n", bme.chip_id);
    // Check if rslt == BME68X_OK, report or handle if otherwise 
    heatr_conf.enable = BME68X_ENABLE;
    heatr_conf.heatr_temp = 300;
    heatr_conf.heatr_dur = 100;
    rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, &bme);
    bme68x_check_rslt("bme68x_set_heatr_conf", rslt);

    printf("Sample, TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%%), Gas resistance(ohm), Status\n");

    while (sample_count <= SAMPLE_COUNT)
    {
        rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);
        bme68x_check_rslt("bme68x_set_op_mode", rslt);

        // Calculate delay period in microseconds 
        del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme) + (heatr_conf.heatr_dur * 1000);
        bme.delay_us(del_period, bme.intf_ptr);

        time_ms = count*50;

        // Check if rslt == BME68X_OK, report or handle if otherwise
        rslt = bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &bme);
        bme68x_check_rslt("bme68x_get_data", rslt);

        if (n_fields)
        {
#ifdef BME68X_USE_FPU
            printf("%u, %lu, %.2f, %.2f, %.2f, %.2f, 0x%x\n",
                   sample_count,
                   (long unsigned int)time_ms,
                   data.temperature,
                   data.pressure,
                   data.humidity,
                   data.gas_resistance,
                   data.status);
#else
            printf("%u, %lu, %d, %lu, %lu, %lu, 0x%x\n",
                   sample_count,
                   (long unsigned int)time_ms,
                   (data.temperature / 100),
                   (long unsigned int)data.pressure,
                   (long unsigned int)(data.humidity / 1000),
                   (long unsigned int)data.gas_resistance,
                   data.status);
#endif
            sample_count++;
        }
    }


    while (1)
    {
         __WFI();//GO INTO LOW POWER MODE
    }
}

/** @} */

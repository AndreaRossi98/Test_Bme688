/**
 * Copyright (C) 2021 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bme68x.h"
#include "bme68x_defs.h"
#include "common.h"
#include "nrf_drv_twi.h"
#include "nrf_delay.h"


/******************************************************************************/
/*!                 Macro definitions                                         */
/*! BME68X shuttle board ID */
#define BME68X_SHUTTLE_ID  0x93
#define SDA_PIN 18
#define SCL_PIN 19
/******************************************************************************/
/*!                Static variable definition                                 */
static uint8_t dev_addr;
static const nrf_drv_twi_t i2c_instance = NRF_DRV_TWI_INSTANCE(0);


/******************************************************************************/
/*!                User interface functions                                   */

/*!
 * I2C read function map to COINES platform
 */

uint8_t GTXBuffer[512], GRXBuffer[2048];

BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{

    uint8_t dev_addr = *(uint8_t*)intf_ptr; 
    GTXBuffer[0] = reg_addr | 0x80;
    ret_code_t ret_code;
    ret_code = nrf_drv_twi_tx(&i2c_instance, dev_addr, GTXBuffer,1,false);
    if(ret_code != NRF_SUCCESS)
    {
        return ret_code;
    }

    ret_code = nrf_drv_twi_rx(&i2c_instance, dev_addr, reg_data, len);
    if (ret_code == NRF_SUCCESS) 
        return BME68X_OK;
    else 
        return 1;
        
/*   //alternative version. Same problem
    uint8_t dev_addr = *(uint8_t*)intf_ptr;
    BME68X_INTF_RET_TYPE ret;
    ret_code_t err_code;

    //Write register address
    err_code = nrf_drv_twi_tx(&i2c_instance, dev_addr, (uint8_t *)&reg_addr, (uint16_t)1, false);

    //Read data
    err_code = nrf_drv_twi_rx(&i2c_instance, dev_addr, reg_data, (uint16_t)len);

    ret = !(err_code == NRF_SUCCESS);

    return ret;
    */
}


/*!
 * I2C write function map to COINES platform
 */
BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{

    //VERSIONE PRINCIPALE
    uint8_t dev_addr = *(uint8_t*)intf_ptr;     //volendo mettere <<1
    ret_code_t ret_code;
    uint8_t send_tmp[512] = {0};
    send_tmp[0] = reg_addr;
    memcpy(send_tmp+1, reg_data, len);
    ret_code_t err_code = nrf_drv_twi_tx(&i2c_instance, dev_addr, send_tmp, len, false);
    if (ret_code == NRF_SUCCESS) return BME68X_OK;
    else return 1;

/*    uint8_t dev_addr =*(uint8_t*)intf_ptr;
    GTXBuffer[0] = reg_addr;
    memcpy(&GTXBuffer[1], reg_data, len);
    //ret_code_t ret_code = nrf_drv_twi_tx(&i2c_instance, dev_addr, GTXBuffer, len, false);
    if (ret_code == NRF_SUCCESS) return BME68X_OK;
    else return 1;
/*
    uint8_t dev_addr = *(uint8_t*)intf_ptr;
    BME68X_INTF_RET_TYPE ret;
    ret_code_t err_code;
    uint8_t m_i2c_wr_buf[10];
    if (len > 200)
    {
    return !BME68X_INTF_RET_SUCCESS;
    }

    m_i2c_wr_buf[0] = reg_addr;
    memcpy(&m_i2c_wr_buf[1], reg_data, len);
    //Write data
    err_code = nrf_drv_twi_tx(&i2c_instance, dev_addr, (uint8_t *)m_i2c_wr_buf, (uint16_t)len, false);
    ret = !(err_code == NRF_SUCCESS);
    return ret;
    */
}


/*!
 * Delay function map to COINES platform
 */
void bme68x_delay_us(uint32_t period, void *intf_ptr)
{
    nrf_delay_us(period);
}


int8_t bme68x_interface_init(struct bme68x_dev *bme, uint8_t intf)
{
    int8_t rslt = BME68X_OK;
    uint8_t reg_addr; 
    const uint8_t *reg_data;
    uint32_t len;
    void *intf_ptr;

    if (bme != NULL)
    {
        /* Bus configuration : I2C */
        if (intf == BME68X_I2C_INTF)
        {
            //printf("\nI2C Interface\n");
            dev_addr = BME68X_I2C_ADDR_LOW;
            bme->read = bme68x_i2c_read;
            bme->write = bme68x_i2c_write;
            bme->intf = BME68X_I2C_INTF;
            //i2C communication initialized in main function (I try to initialize here but nothing changes)
            /*const nrf_drv_twi_config_t i2c_instance_config = {  .scl = SCL_PIN,
                                                                .sda = SDA_PIN,
                                                                .frequency = NRF_TWI_FREQ_100K,
                                                                .interrupt_priority = 0};
            rslt = nrf_drv_twi_init(&i2c_instance, &i2c_instance_config, NULL, NULL);
            //enable TWI instance
            nrf_drv_twi_enable(&i2c_instance);
    */
        }
        //coines_set_shuttleboard_vdd_vddio_config(3300, 3300);
        nrf_delay_ms(100);
        bme->delay_us = bme68x_delay_us;
        bme->intf_ptr = &dev_addr;
        bme->amb_temp = 25; /* The ambient temperature in deg C is used for defining the heater temperature */
    }
    else
    {
        rslt = BME68X_E_NULL_PTR;
    }

    return rslt;
}


void bme68x_check_rslt(const char api_name[], int8_t rslt)
{
    switch (rslt)
    {
        case BME68X_OK:
            printf("API name [%s]: Tutto ok\n", api_name);
            /* Do nothing */
            break;
        case BME68X_E_NULL_PTR:
            printf("API name [%s]  Error [%d] : Null pointer\r\n", api_name, rslt);
            break;
        case BME68X_E_COM_FAIL:
            printf("API name [%s]  Error [%d] : Communication failure\r\n", api_name, rslt);
            break;
        case BME68X_E_INVALID_LENGTH:
            printf("API name [%s]  Error [%d] : Incorrect length parameter\r\n", api_name, rslt);
            break;
        case BME68X_E_DEV_NOT_FOUND:
            printf("API name [%s]  Error [%d] : Device not found\r\n", api_name, rslt);
            break;
        case BME68X_E_SELF_TEST:
            printf("API name [%s]  Error [%d] : Self test error\r\n", api_name, rslt);
            break;
        case BME68X_W_NO_NEW_DATA:
            printf("API name [%s]  Warning [%d] : No new data found\r\n", api_name, rslt);
            break;
        default:
            printf("API name [%s]  Error [%d] : Unknown error code\r\n", api_name, rslt);
            break;
    }
}

/*
void bme68x_coines_deinit(void)
{
    fflush(stdout);

    coines_set_shuttleboard_vdd_vddio_config(0, 0);
    coines_delay_msec(1000);

    // Coines interface reset
    coines_soft_reset();
    coines_delay_msec(1000);
    coines_close_comm_intf(COINES_COMM_INTF_USB);
}
*/
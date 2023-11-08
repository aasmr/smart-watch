#include "spi.h"

void spi_pre_transfer_callback(spi_transaction_t *trans)
{
    int dc = (int)trans->user;
    gpio_set_level(LCD_DC_Pin, dc);
}

void spi_config(spi_device_handle_t* spi)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = ((1ULL << LCD_DC_Pin) | (1ULL << LCD_RST_Pin));
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

	esp_err_t ret;
	    spi_bus_config_t buscfg = {
	        .miso_io_num = LCD_SPI_MISO_Pin,
	        .mosi_io_num = LCD_SPI_MOSI_Pin,
	        .sclk_io_num = LCD_SPI_CLK_Pin,
	        .quadwp_io_num = -1,
	        .quadhd_io_num = -1,
	        .max_transfer_sz = PARALLEL_LINES * 240 * 2 + 8
	    };
	    spi_device_interface_config_t devcfg = {
	        .clock_speed_hz = 40 * 1000 * 1000,     //Clock out at 26 MHz
	        .mode = 0,                              //SPI mode 0
	        .spics_io_num = LCD_SPI_CS_Pin,         //CS pin
	        .queue_size = 7,                        //We want to be able to queue 7 transactions at a time
	        .pre_cb = spi_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
	    };
	    //Initialize the SPI bus
	    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
	    ESP_ERROR_CHECK(ret);
	    //Attach the LCD to the SPI bus
	    ret = spi_bus_add_device(SPI2_HOST, &devcfg, spi);
	    ESP_ERROR_CHECK(ret);
}

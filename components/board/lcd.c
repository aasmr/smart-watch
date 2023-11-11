#include "lcd.h"

static void lcd_send_lines(spi_device_handle_t *spi, uint16_t ypos, uint16_t *linedata)
{
    esp_err_t ret;
    int x;
    //Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x = 0; x < 6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0) {
            //Even transfers are commands
            trans[x].length = 8;
            trans[x].user = (void*)0;
        } else {
            //Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user = (void*)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0] = 0x2A;         //Column Address Set
    trans[1].tx_data[0] = 0;            //Start Col High
    trans[1].tx_data[1] = 0;            //Start Col Low
    trans[1].tx_data[2] = ((uint16_t)(239)) >> 8;   //End Col High
    trans[1].tx_data[3] = ((uint16_t)(239)) & 0xff; //End Col Low
    trans[2].tx_data[0] = 0x2B;         //Page address set
    trans[3].tx_data[0] = ypos >> 8;    //Start page high
    trans[3].tx_data[1] = ypos & 0xff;  //start page low
    trans[3].tx_data[2] = (ypos + PARALLEL_LINES-1) >> 8; //end page high
    trans[3].tx_data[3] = (ypos + PARALLEL_LINES-1) & 0xff; //end page low
    trans[4].tx_data[0] = 0x2C;         //memory write
    trans[5].tx_buffer = linedata;      //finally send the line data
    trans[5].length = 240 * 2 * 8 * PARALLEL_LINES;  //Data length, in bits
    trans[5].flags = 0; //undo SPI_TRANS_USE_TXDATA flag

    //Queue all transactions.
    for (x = 0; x < 6; x++) {
        ret = spi_device_queue_trans(*spi, &trans[x], portMAX_DELAY);
        assert(ret == ESP_OK);
    }

    //When we are here, the SPI driver is busy (in the background) getting the transactions sent. That happens
    //mostly using DMA, so the CPU doesn't have much to do here. We're not going to wait for the transaction to
    //finish because we may as well spend the time calculating the next line. When that is done, we can call
    //send_line_finish, which will wait for the transfers to be done and check their status.
}

static void lcd_send_line_finish(spi_device_handle_t *spi)
{
    spi_transaction_t *rtrans;
    esp_err_t ret;
    //Wait for all 6 transactions to be done and get back the results.
    for (int x = 0; x < 6; x++) {
        ret = spi_device_get_trans_result(*spi, &rtrans, portMAX_DELAY);
        assert(ret == ESP_OK);
        //We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
    }
}

static void parline_fill_color(uint16_t* lines, uint16_t color)
{
    for (int x = 0; x < 240*PARALLEL_LINES; x++)
    {
    	*lines = color;
    	//printf("ADDR: %u\n", (unsigned int) lines);
    	lines++;
    }
}

static void parline_fill_image(uint16_t* lines, uint16_t* image)
{
    for (int x = 0; x < 240*PARALLEL_LINES; x++)
    {
    	*lines = *image;
    	lines++;
    	image++;
    }
}

void lcd_reset()
{
	gpio_set_level(LCD_RST_Pin, 1);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	gpio_set_level(LCD_RST_Pin, 0);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	gpio_set_level(LCD_RST_Pin, 1);
	vTaskDelay(150 / portTICK_PERIOD_MS);
}

void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd, bool keep_cs_active)
{
    esp_err_t ret;
    spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));       //Zero out the transaction
    trans.length = 8;                   //Command is 8 bits
    trans.tx_buffer = &cmd;             //The data is the cmd itself
    trans.user = (void*)0;              //D/C needs to be set to 0
    if (keep_cs_active) {
    	trans.flags = SPI_TRANS_CS_KEEP_ACTIVE;   //Keep CS active after data transfer
    }
    ret = spi_device_polling_transmit(spi, &trans); //Transmit!
    assert(ret == ESP_OK);          //Should have had no issues.
}

void lcd_data(spi_device_handle_t spi, const uint8_t* data, int len)
{
    esp_err_t ret;
    spi_transaction_t trans;
    if (len == 0) {
        return;    //no need to send anything
    }
    memset(&trans, 0, sizeof(trans));       //Zero out the transaction
    trans.length = len * 8;             //Len is in bytes, transaction length is in bits.
    trans.tx_buffer = data;             //Data
    trans.user = (void*)1;              //D/C needs to be set to 1
    ret = spi_device_polling_transmit(spi, &trans); //Transmit!
    assert(ret == ESP_OK);          //Should have had no issues.
}

void lcd_init_seq(spi_device_handle_t* spi, lcd_init_cmd_t* cmd_init)
{
	uint8_t cmd = 0;
	while (cmd_init[cmd].cmd_len != 0xFF)
	{
		lcd_cmd(*spi, cmd_init[cmd].cmd_reg, false);
		if(cmd_init[cmd].cmd_len != 0x00)
		{
			lcd_data(*spi, cmd_init[cmd].cmd_data, cmd_init[cmd].cmd_len & 0x1F);

		}
		if (cmd_init[cmd].cmd_reg == 0x11)
		{
			vTaskDelay(120 / portTICK_PERIOD_MS);
		}
		if (cmd_init[cmd].cmd_reg == 0x29)
		{
			vTaskDelay(20 / portTICK_PERIOD_MS);
		}
	    cmd++;
	}
}

void lcd_draw_all(spi_device_handle_t* spi, uint16_t* image)
{
	uint16_t *lines[240/PARALLEL_LINES];
	int sending_line = -1;
	esp_err_t ret;
	int x;
	//Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
	//function is finished because the SPI driver needs access to it even while we're already calculating the next line.
	static spi_transaction_t trans[6];

	//In theory, it's better to initialize trans and data only once and hang on to the initialized
	//variables. We allocate them on the stack, so we need to re-init them each call.
	for (x = 0; x < 6; x++)
	{
		memset(&trans[x], 0, sizeof(spi_transaction_t));
		if ((x & 1) == 0) {
			//Even transfers are commands
			trans[x].length = 8;
			trans[x].user = (void*)0;
		} else {
			//Odd transfers are data
			trans[x].length = 8 * 4;
			trans[x].user = (void*)1;
		}
		trans[x].flags = SPI_TRANS_USE_TXDATA;
	}
	trans[5].flags = 0;

	trans[0].tx_data[0] = 0x2A;         //Column Address Set
	trans[1].tx_data[0] = 0;            //Start Col High
	trans[1].tx_data[1] = 0;            //Start Col Low
	trans[1].tx_data[2] = 0;   //End Col High
	trans[1].tx_data[3] = ((uint16_t)(239)) & 0xff; //End Col Low
	trans[2].tx_data[0] = 0x2B;         //Page address set
	trans[3].tx_data[0] = 0;    //Start page high
	trans[4].tx_data[0] = 0x2C;         //memory write
	trans[5].length = 240 * 2 * 8 * PARALLEL_LINES;  //Data length, in bits


	for(int i = 0; i<240/PARALLEL_LINES; i++)
	{
		lines[i] = heap_caps_malloc(240 * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_DMA);
		assert(lines[i] != NULL);
		parline_fill_image(lines[i], image);
	}
	for (uint16_t y = 0; y < 240; y += PARALLEL_LINES)
	{
		//Finish up the sending process of the previous line, if any
		if (sending_line != -1)
		{
			//vTaskDelay(100 / portTICK_PERIOD_MS);
			lcd_send_line_finish(spi);
		}
		//Swap sending_line and calc_line
		sending_line++;
		//Send the line we currently calculated.

		trans[3].tx_data[1] = y & 0xff;  //start page low
		trans[3].tx_data[2] = (y + PARALLEL_LINES-1) >> 8; //end page high
		trans[3].tx_data[3] = (y + PARALLEL_LINES-1) & 0xff; //end page low
		trans[5].tx_buffer = lines[sending_line];      //finally send the line data
		//Queue all transactions.
		for (x = 0; x < 6; x++) {
			ret = spi_device_queue_trans(*spi, &trans[x], portMAX_DELAY);
			assert(ret == ESP_OK);
		}
		//The line set is queued up for sending now; the actual sending happens in the
		//background. We can go on to calculate the next line set as long as we do not
		//touch line[sending_line]; the SPI sending process is still reading from that.
	}
	lcd_send_line_finish(spi);
	for(int i = 0; i<240/PARALLEL_LINES; i++)
	{
		heap_caps_free(lines[i]);
	}
}

void lcd_draw_part(spi_device_handle_t* spi, uint16_t color, uint8_t start_x, uint8_t start_y, uint8_t w, uint8_t h, uint16_t* image)
{
	uint16_t *lines[h/PARALLEL_LINES];
	int sending_line = -1;
    esp_err_t ret;
    int x;
    //Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x = 0; x < 6; x++)
    {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0) {
            //Even transfers are commands
            trans[x].length = 8;
            trans[x].user = (void*)0;
        } else {
            //Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user = (void*)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[5].flags = 0;

    trans[0].tx_data[0] = 0x2A;         //Column Address Set
    trans[1].tx_data[0] = 0;            //Start Col High
    trans[1].tx_data[1] = 0;            //Start Col Low
    trans[1].tx_data[2] = 0;   //End Col High
    trans[1].tx_data[3] = ((uint16_t)(239)) & 0xff; //End Col Low
    trans[2].tx_data[0] = 0x2B;         //Page address set
    trans[3].tx_data[0] = 0;    //Start page high
    trans[4].tx_data[0] = 0x2C;         //memory write
    trans[5].length = 240 * 2 * 8 * PARALLEL_LINES;  //Data length, in bits


	for(int i = 0; i<240/PARALLEL_LINES; i++)
	{
		lines[i] = heap_caps_malloc(240 * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_DMA);
		assert(lines[i] != NULL);
		parline_fill_image(lines[i], image);
	}
    for (uint16_t y = 0; y < 240; y += PARALLEL_LINES)
    {
        //Finish up the sending process of the previous line, if any
        if (sending_line != -1)
        {
        	//vTaskDelay(100 / portTICK_PERIOD_MS);
            lcd_send_line_finish(spi);
        }
        //Swap sending_line and calc_line
        sending_line++;
        //Send the line we currently calculated.

        trans[3].tx_data[1] = y & 0xff;  //start page low
        trans[3].tx_data[2] = (y + PARALLEL_LINES-1) >> 8; //end page high
        trans[3].tx_data[3] = (y + PARALLEL_LINES-1) & 0xff; //end page low
        trans[5].tx_buffer = lines[sending_line];      //finally send the line data
        //Queue all transactions.
        for (x = 0; x < 6; x++) {
            ret = spi_device_queue_trans(*spi, &trans[x], portMAX_DELAY);
            assert(ret == ESP_OK);
        }
        //The line set is queued up for sending now; the actual sending happens in the
        //background. We can go on to calculate the next line set as long as we do not
        //touch line[sending_line]; the SPI sending process is still reading from that.
    }
    lcd_send_line_finish(spi);
    for(int i = 0; i<240/PARALLEL_LINES; i++)
    {
    	heap_caps_free(lines[i]);
    }
}
void lcd_clear(spi_device_handle_t* spi, uint16_t color)
{
	uint16_t *lines[240/PARALLEL_LINES];
	int sending_line = -1;
    esp_err_t ret;
    int x;
    //Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x = 0; x < 6; x++)
    {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0) {
            //Even transfers are commands
            trans[x].length = 8;
            trans[x].user = (void*)0;
        } else {
            //Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user = (void*)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[5].flags = 0;

    trans[0].tx_data[0] = 0x2A;         //Column Address Set
    trans[1].tx_data[0] = 0;            //Start Col High
    trans[1].tx_data[1] = 0;            //Start Col Low
    trans[1].tx_data[2] = 0;   //End Col High
    trans[1].tx_data[3] = ((uint16_t)(239)) & 0xff; //End Col Low
    trans[2].tx_data[0] = 0x2B;         //Page address set
    trans[3].tx_data[0] = 0;    //Start page high
    trans[4].tx_data[0] = 0x2C;         //memory write
    trans[5].length = 240 * 2 * 8 * PARALLEL_LINES;  //Data length, in bits


	for(int i = 0; i<240/PARALLEL_LINES; i++)
	{
		lines[i] = heap_caps_malloc(240 * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_DMA);
		assert(lines[i] != NULL);
		parline_fill_color(lines[i], color);
	}
    for (uint16_t y = 0; y < 240; y += PARALLEL_LINES)
    {
        //Finish up the sending process of the previous line, if any
        if (sending_line != -1)
        {
        	//vTaskDelay(100 / portTICK_PERIOD_MS);
            lcd_send_line_finish(spi);
        }
        //Swap sending_line and calc_line
        sending_line++;
        //Send the line we currently calculated.

        trans[3].tx_data[1] = y & 0xff;  //start page low
        trans[3].tx_data[2] = (y + PARALLEL_LINES-1) >> 8; //end page high
        trans[3].tx_data[3] = (y + PARALLEL_LINES-1) & 0xff; //end page low
        trans[5].tx_buffer = lines[sending_line];      //finally send the line data
        //Queue all transactions.
        for (x = 0; x < 6; x++) {
            ret = spi_device_queue_trans(*spi, &trans[x], portMAX_DELAY);
            assert(ret == ESP_OK);
        }
        //lcd_send_lines(spi, y, lines[sending_line]);
        //lcd_send_line_finish(spi);
        //The line set is queued up for sending now; the actual sending happens in the
        //background. We can go on to calculate the next line set as long as we do not
        //touch line[sending_line]; the SPI sending process is still reading from that.
    }
    lcd_send_line_finish(spi);
    for(int i = 0; i<240/PARALLEL_LINES; i++)
    {
    	heap_caps_free(lines[i]);
    }
}

/*
void lcd_set_bgnd(spi_device_handle_t* spi)
{
	uint16_t *lines[240/PARALLEL_LINES];
	int sending_line = -1;
    esp_err_t ret;
    int x;
    //Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x = 0; x < 6; x++)
    {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0) {
            //Even transfers are commands
            trans[x].length = 8;
            trans[x].user = (void*)0;
        } else {
            //Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user = (void*)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[5].flags = 0;

    trans[0].tx_data[0] = 0x2A;         //Column Address Set
    trans[1].tx_data[0] = 0;            //Start Col High
    trans[1].tx_data[1] = 0;            //Start Col Low
    trans[1].tx_data[2] = 0;   //End Col High
    trans[1].tx_data[3] = ((uint16_t)(239)) & 0xff; //End Col Low
    trans[2].tx_data[0] = 0x2B;         //Page address set
    trans[3].tx_data[0] = 0;    //Start page high
    trans[4].tx_data[0] = 0x2C;         //memory write
    trans[5].length = 240 * 2 * 8 * PARALLEL_LINES;  //Data length, in bits

    uint8_t y_start = 0;
	for(int i = 0; i<240/PARALLEL_LINES; i++)
	{
		y_start = i*PARALLEL_LINES;
		lines[i] = heap_caps_malloc(240 * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_DMA);
		assert(lines[i] != NULL);
		parline_fill_bgnd(lines[i], y_start);
	}
    for (uint16_t y = 0; y < 240; y += PARALLEL_LINES)
    {
        //Finish up the sending process of the previous line, if any
        if (sending_line != -1)
        {
        	//vTaskDelay(100 / portTICK_PERIOD_MS);
            lcd_send_line_finish(spi);
        }
        //Swap sending_line and calc_line
        sending_line++;
        //Send the line we currently calculated.

        trans[3].tx_data[1] = y & 0xff;  //start page low
        trans[3].tx_data[2] = (y + PARALLEL_LINES-1) >> 8; //end page high
        trans[3].tx_data[3] = (y + PARALLEL_LINES-1) & 0xff; //end page low
        trans[5].tx_buffer = lines[sending_line];      //finally send the line data
        //Queue all transactions.
        for (x = 0; x < 6; x++) {
            ret = spi_device_queue_trans(*spi, &trans[x], portMAX_DELAY);
            assert(ret == ESP_OK);
        }
        //lcd_send_lines(spi, y, lines[sending_line]);
        //lcd_send_line_finish(spi);
        //The line set is queued up for sending now; the actual sending happens in the
        //background. We can go on to calculate the next line set as long as we do not
        //touch line[sending_line]; the SPI sending process is still reading from that.
    }
    lcd_send_line_finish(spi);
    for(int i = 0; i<240/PARALLEL_LINES; i++)
    {
    	heap_caps_free(lines[i]);
    }
}
*/
void lcd_init(spi_device_handle_t* spi)
{

	DRAM_ATTR static lcd_init_cmd_t lcd_init_cmds[] = {
			{0xEF, {0x00},  0},
			{0xEB, {0x14},  1},
			{0xFE, {0x00},  0},
			{0xEF, {0x00},  0}, //Enable seq

			{0xEB, {0x14},  1},
			{0x84, {0x40},  1},
			{0x85, {0xFF},  1},
			{0x86, {0xFF},  1},
			{0x87, {0xFF},  1},
			{0x88, {0x0A},  1},
			{0x89, {0x21},  1},
			{0x8A, {0x00},  1},
			{0x8B, {0x80},  1},
			{0x8C, {0x01},  1},
			{0x8D, {0x01},  1},
			{0x8E, {0xFF},  1},
			{0x8F, {0xFF},  1},

			//Display Function Control
			//SS=1 GS = 0
			{0xB6, {0x00, 0x20},  2},

			//Memory Access Control
			//MH(2) = 0 BGR(3) = 0 ML(4) = 0 MV(5) = 0 MX(6) = 0 MY(7) = 0
			//MY - Miror Y; MX - Mirror X; MV - exchange row and col;
			//ML - 0:upper line refresh first, 1:upper line refresh last
			//BGR - 0:RGB, 1:BGR
			//MH - 0:first col refresh first, 1:first col refresh last
			{0x36, {0x08},  1},

			//Pixel Format Set
			//DBI(0,1,2) = 5 DPI(4,5,6) = 0
			//
			{0x3A, {0x05},  1},

			{0x90, {0x08, 0x08, 0x08, 0x08},  4},

			{0xBD, {0x06},  1},
			{0xBC, {0x00},  1},

			{0xFF, {0x60, 0x01, 0x04},  3},

			{0xC3, {0x13},  1},
			{0xC4, {0x13},  1},
			{0xC9, {0x22},  1},
			{0xBE, {0x11},  1},

			{0xE1, {0x10, 0x0E},  2},

			{0xDF, {0x21, 0x0C, 0x02},  3},

			//SET GAMMA
			{0xF0, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A},  6},
			{0xF1, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F},  6},
			{0xF2, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A},  6},
			{0xF3, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F},  6},

			{0xED, {0x1B, 0x0B},  2},

			{0xAE, {0x77},  1},
			{0xCD, {0x63},  1},

			{0x70, {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03},  9},

			{0xE8, {0x34},  1},

			{0x62, {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70},  12},
			{0x63, {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70},  12},

			{0x64, {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07},  7},

			{0x66, {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00},  10},
			{0x67, {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98},  10},

			{0x74, {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00},  7},

			{0x98, {0x3E, 0x07},  2},

			{0x35, {0x00},  0},
			{0x21, {0x00},  0},

			{0x11, {0x00}, 0x80},
			{0x29, {0x00}, 0x80},

			{0x00, {0x00}, 0xFF},

	};

	lcd_reset();
	vTaskDelay(100 / portTICK_PERIOD_MS);
	lcd_init_seq(spi, lcd_init_cmds);
	lcd_clear(spi, BLACK);
}

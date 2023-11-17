/*
 * watch.c
 *
 *  Created on: 9 нояб. 2023 г.
 *      Author: aasmr
 */

#include "watch.h"

static const char *TAG = "watch";

uint16_t *bradis_sin;
uint16_t *bradis_tan;
watch_t watch;

static float get_min(float n1, float n2, float n3, float n4)
{
	float min = 0;

	if(n1 < n2)
		min = n1;
	else
		min = n2;

	if(min > n3)
		min = n3;

	if(min > n4)
		min = n4;

	return min;
}

static float get_max(float n1, float n2, float n3, float n4)
{
	float max = 0;

	if(n1 > n2)
		max = n1;
	else
		max = n2;

	if(max < n3)
		max = n3;

	if(max < n4)
		max = n4;

	return max;
}

static void write_to_buf()
{

}
static void merging_layers(uint16_t* sum_img, uint16_t* img1, uint32_t* img2, uint8_t w, uint8_t h)
{

	int y, x;
	float a, r, g, b;
	uint16_t old_color;
	uint16_t new_color;

	for(y = 0; y < h; y++)
	{
		for(x = 0; x < w; x++)
		{
			a = (float)((img2[w*y+x] >> 8) & 0xFF);
			old_color = (img2[w*y+x] >> 16) & 0xFF;
			old_color = ((old_color >> 8) & 0xFF) + ((old_color & 0xFF) << 8);
			b = (float)((old_color >> 11) & 0x1F);
			g = (float)((old_color >> 5) & 0x3F);
			r = (float)((old_color >> 0) & 0x1F);

			if( a!= 0 )
			{
				new_color = (((uint16_t)(r*(a/255))&0x1F) + (((uint16_t)(g*(a/255))&0x3F) << 5) + (((uint16_t)(b*(a/255))&0x1F) << 11));
				new_color = ((new_color >> 8) & 0xFF) + ((new_color & 0xFF) << 8);
				sum_img[w*y+x] = new_color;
			}
			else
			{
				sum_img[w*y+x] = img1[w*y+x];
			}
		}
	}

}

static void coord_transf(float *new_x, float *new_y, uint8_t x, uint8_t y, uint8_t xc, uint8_t yc, int16_t angle)
{
    float coef;
    coef = 1;

    float ft_x, ft_y, st_x, st_y, tt_x, tt_y;
    ft_x = 0;
    ft_y = 0;
    st_x = 0;
    st_y = 0;
    tt_x = 0;
    tt_y = 0;
    if (angle < 0)
    {
        coef = -1;
        angle = -1 * angle;
    }

    ft_x = (float)(x - xc) - (coef*((float)bradis_tan[angle]/10000)*(float)(y - yc));
    ft_y = (float)(y - yc);

    st_x = ft_x;
    st_y = ft_x * (coef*(float)bradis_sin[angle]/10000) + ft_y;

    tt_x = st_x - (coef*((float)bradis_tan[angle]/10000) * st_y);
    tt_y = st_y;

    *new_x = tt_x + (float)xc;
    *new_y = tt_y + (float)yc;

    //printf("(x, y) (new_x, new_y): (%hhu, %hhu) (%f, %f)\n", x, y, *new_x , *new_y);
}

static img32_t rotate(uint16_t angle, img32_t image)
{
    uint32_t *buf_image;
    img32_t rot_img;

    buf_image = (uint32_t*) heap_caps_malloc(image.width * image.height * sizeof(uint32_t), MALLOC_CAP_SPIRAM);
    int x, y;

    if(angle > 90 && angle <= 180)
    {
    	angle = angle - 90;
        for(y = 0; y < image.height; y++)
        {
        	for(x = 0; x < image.width; x++)
        	{
                buf_image[image.height * x + (image.height - 1 - y)] = image.img_arr[image.width * y + x];
        	}
        }
        uint8_t temp_w, temp_xc;
        temp_xc = image.xc;
        image.xc = image.height - image.yc;
        image.yc = temp_xc;
        temp_w = image.width;
        image.width = image.height;
        image.height = temp_w;
    }

    else if(angle > 180 && angle <= 270)
    {
        angle = angle - 180;
        for(y = 0; y < image.height; y++)
        {
        	for(x = 0; x < image.width; x++)
            {
        		buf_image[image.width * (image.height - 1 - y) + x] = image.img_arr[image.width * y + x];
            }
        }
        image.xc = image.width - image.xc;
        image.yc = image.height - image.yc;
    }

    else if(angle > 270 && angle <= 360)
	{
        angle = angle - 270;
        for(y = 0; y < image.height; y++)
        {
        	for(x = 0; x < image.width; x++)
        	{
        		buf_image[image.height * (image.width - 1 - x) + y] = image.img_arr[image.width * y + x];
        	}
        }
        uint8_t temp_w, temp_xc;
        temp_xc = image.xc;
        image.xc = image.yc;
        image.yc = image.width - temp_xc;
        temp_w = image.width;
        image.width = image.height;
        image.height = temp_w;
	}

    else
    {
        memcpy(buf_image, image.img_arr, image.width * image.height * 4);
    }

    if (angle == 0)
	{
    	rot_img.width 	= image.width;
    	rot_img.height 	= image.height;
    	rot_img.type 	= image.type;
    	rot_img.xc 		= image.xc;
    	rot_img.yc 		= image.yc;
    	rot_img.img_arr = buf_image;
        return rot_img;
	}

    else if (angle == 90)
    {
        uint32_t *temp_buf_image;
        temp_buf_image = (uint32_t*) heap_caps_malloc(image.width * image.height * sizeof(uint32_t), MALLOC_CAP_SPIRAM);
        for(y = 0; y < image.height; y++)
        {
        	for(x = 0; x < image.width; x++)
        	{
        		temp_buf_image[image.height * x + (image.height - 1 - y)] = buf_image[image.width * y + x];
        	}
        }
        heap_caps_free(buf_image);
    	rot_img.width 	= image.height;
    	rot_img.height 	= image.width;
    	rot_img.type 	= image.type;
    	rot_img.xc 		= image.height - image.yc;
    	rot_img.yc 		= image.xc;
    	rot_img.img_arr = temp_buf_image;
        return rot_img;
    }

    float x1, y1, x2, y2, x3, y3, x4, y4;
    int16_t x_offset, y_offset, w_buf, h_buf;
    coord_transf(&x1, &y1, 0, 0, image.xc, image.yc, angle);
    coord_transf(&x2, &y2, image.width - 1, 0, image.xc, image.yc, angle);
    coord_transf(&x3, &y3, 0, image.height - 1, image.xc, image.yc, angle);
    coord_transf(&x4, &y4, image.width - 1, image.height - 1, image.xc, image.yc, angle);

    x_offset = (int16_t)(get_min(x1, x2, x3, x4));
    y_offset = (int16_t)(get_min(y1, y2, y3, y4));

    w_buf = (int16_t)(get_max(x1, x2, x3, x4)) - x_offset + 1;
    h_buf = (int16_t)(get_max(y1, y2, y3, y4)) - y_offset + 1;

    uint32_t *temp_buf_image;
    temp_buf_image = (uint32_t*) heap_caps_malloc(w_buf * h_buf * sizeof(uint32_t), MALLOC_CAP_SPIRAM);
    memset(temp_buf_image, 0, w_buf * h_buf * sizeof(uint32_t));
    float new_x = 0;
    float new_y = 0;
    uint8_t i_new_x = 0;
    uint8_t i_new_y = 0;

    for(y = 0; y < image.height; y++)
    {
    	for(x = 0; x < image.width; x++)
    	{
    	    new_x = 0;
    	    new_y = 0;
    	    i_new_x = 0;
    	    i_new_y = 0;
    		coord_transf(&new_x, &new_y, x, y, image.xc, image.yc, angle);
    		i_new_x = (int16_t)(new_x) - x_offset;
    		i_new_y = (int16_t)(new_y) - y_offset;
			temp_buf_image[w_buf * i_new_y + i_new_x] = buf_image[y*image.width+x];

			//printf("(x, y): %hhu, %hhu\n", i_new_x, i_new_y);
    	}
    }
/*
	printf("i_new_x: %hhu\n", i_new_x);
	printf("i_new_y: %hhu\n", i_new_y);
	printf("new_x: %f\n", new_x);
	printf("new_y: %f\n", new_y);
	printf("x_offset: %hd\n", x_offset);
	printf("y_offset: %hd\n", y_offset);
*/
	rot_img.width 	= (uint8_t)w_buf;
	rot_img.height 	= (uint8_t)h_buf;
	rot_img.type 	= image.type;
	rot_img.xc 		= (uint8_t)((int16_t)image.xc - x_offset);
	rot_img.yc 		= (uint8_t)((int16_t)image.yc - y_offset);
	rot_img.img_arr = temp_buf_image;
	heap_caps_free(buf_image);
	return rot_img;
}

static void rotate_hrs(uint16_t min)
{
	img32_t rot_hrs;

	ESP_LOGI(TAG, "step in hrs draw");
	printf("%hu\n", min/2);
	rot_hrs = rotate(min/2, watch.hrs_img);

	uint16_t *mrg_img;
	uint16_t *part_bg_img;

	img16_t mrg;
	img16_t part_bg;

	part_bg_img = (uint16_t*) heap_caps_malloc(rot_hrs.width * rot_hrs.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	mrg_img = (uint16_t*) heap_caps_malloc(rot_hrs.width * rot_hrs.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);

	memset(part_bg_img, 0, rot_hrs.width * rot_hrs.height * sizeof(uint16_t));
	memset(mrg_img, 0, rot_hrs.width * rot_hrs.height * sizeof(uint16_t));

	uint8_t x, y, start_x, start_y;
	uint32_t i;
	i = 0;
	start_x = 120 - rot_hrs.xc;
	start_y = 120 - rot_hrs.yc;

	for (y = start_y; y < start_y + rot_hrs.height; y++)
	{
		for (x = start_x; x < start_x + rot_hrs.width; x++)
		{
			part_bg_img[i] = watch.cyfer_img.img_arr[240*y+x];
			i++;
		}
	}

	heap_caps_free(watch.buf_bgnd.img_arr);
	uint16_t *bufer_cyfer;
	bufer_cyfer = (uint16_t*) heap_caps_malloc(watch.buf_bgnd.width * watch.buf_bgnd.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	memset(bufer_cyfer, 0, watch.buf_bgnd.width * watch.buf_bgnd.height * sizeof(uint16_t));
	memcpy(bufer_cyfer, watch.cyfer_img.img_arr, watch.buf_bgnd.width * watch.buf_bgnd.height * sizeof(uint16_t));
	watch.buf_bgnd.img_arr = bufer_cyfer;

	part_bg.width = rot_hrs.width;
	part_bg.height = rot_hrs.height;
	part_bg.type = PART_BCKGND;
	part_bg.xc = rot_hrs.xc;
	part_bg.yc = rot_hrs.yc;
	part_bg.img_arr = part_bg_img;

	watch.prev_bgnd_hrs = watch.next_bgnd_hrs;
	uint16_t *prev_bg_img;
	prev_bg_img = (uint16_t*) heap_caps_malloc(watch.prev_bgnd_hrs.width * watch.prev_bgnd_hrs.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	memset(prev_bg_img, 0, watch.prev_bgnd_hrs.width * watch.prev_bgnd_hrs.height * sizeof(uint16_t));
	memcpy(prev_bg_img, watch.next_bgnd_hrs.img_arr, watch.prev_bgnd_hrs.width * watch.prev_bgnd_hrs.height * sizeof(uint16_t));
	watch.prev_bgnd_hrs.img_arr = prev_bg_img;

	heap_caps_free(watch.next_bgnd_hrs.img_arr);
	watch.next_bgnd_hrs = part_bg;

	merging_layers(mrg_img, part_bg_img, rot_hrs.img_arr, rot_hrs.width, rot_hrs.height);

	for (y = 0; y < rot_hrs.height; y++)
	{
		for (x = 0; x < rot_hrs.width; x++)
		{
			watch.buf_bgnd.img_arr[watch.buf_bgnd.width * (start_y + y) + (start_x + x)] = mrg_img[rot_hrs.width*y+x];
		}
	}

	mrg.width = rot_hrs.width;
	mrg.height = rot_hrs.height;
	mrg.type = PART_BCKGND;
	mrg.xc = rot_hrs.xc;
	mrg.yc = rot_hrs.yc;
	mrg.img_arr = mrg_img;

	uint16_t *buf_part_bg_img, *buf_mrg_img;

	buf_part_bg_img = (uint16_t*) heap_caps_malloc(part_bg.width * part_bg.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	memset(buf_part_bg_img, 0, part_bg.width * part_bg.height * sizeof(uint16_t));
	memcpy(buf_part_bg_img, part_bg.img_arr, part_bg.width * part_bg.height * sizeof(uint16_t));

	buf_mrg_img = (uint16_t*) heap_caps_malloc(mrg.width * mrg.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	memset(buf_mrg_img, 0, mrg.width * mrg.height * sizeof(uint16_t));
	memcpy(buf_mrg_img, mrg.img_arr, mrg.width * mrg.height * sizeof(uint16_t));

	watch.next_part_hrs = mrg;
	watch.next_part_hrs.img_arr = buf_mrg_img;

	watch.next_bgnd_hrs.img_arr = buf_part_bg_img;

	heap_caps_free(part_bg.img_arr);
	heap_caps_free(mrg.img_arr);
	heap_caps_free(rot_hrs.img_arr);

}

static void rotate_min(uint16_t sec)
{
	img32_t rot_min;

	rot_min = rotate(sec/10, watch.min_img);

	uint16_t *mrg_img;
	uint16_t *part_bg_img;

	img16_t mrg;
	img16_t part_bg;

	part_bg_img = (uint16_t*) heap_caps_malloc(rot_min.width * rot_min.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	mrg_img = (uint16_t*) heap_caps_malloc(rot_min.width * rot_min.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);

	memset(part_bg_img, 0, rot_min.width * rot_min.height * sizeof(uint16_t));
	memset(mrg_img, 0, rot_min.width * rot_min.height * sizeof(uint16_t));

	uint8_t x, y, start_x, start_y, _start_x, _start_y;
	uint32_t i;


	_start_x = 120 - watch.next_bgnd_min.xc;
	_start_y = 120 - watch.next_bgnd_min.yc;
	for (y = 0; y < watch.next_bgnd_min.height; y++)
	{
		for (x = 0; x < watch.next_bgnd_min.width; x++)
		{
			watch.buf_bgnd.img_arr[watch.buf_bgnd.width * (_start_y + y) + (_start_x + x)] = watch.next_bgnd_min.img_arr[watch.next_bgnd_min.width*y+x];
		}
	}

	_start_x = 120 - watch.next_part_hrs.xc;
	_start_y = 120 - watch.next_part_hrs.yc;
	for (y = 0; y < watch.next_part_hrs.height; y++)
	{
		for (x = 0; x < watch.next_part_hrs.width; x++)
		{
			watch.buf_bgnd.img_arr[watch.buf_bgnd.width * (_start_y + y) + (_start_x + x)] = watch.next_part_hrs.img_arr[watch.next_part_hrs.width*y+x];
		}
	}


	i = 0;
	start_x = 120 - rot_min.xc;
	start_y = 120 - rot_min.yc;

	for (y = start_y; y < start_y + rot_min.height; y++)
	{
		for (x = start_x; x < start_x + rot_min.width; x++)
		{
			part_bg_img[i] = watch.buf_bgnd.img_arr[240*y+x];
			i++;
		}
	}

	part_bg.width = rot_min.width;
	part_bg.height = rot_min.height;
	part_bg.type = PART_BCKGND;
	part_bg.xc = rot_min.xc;
	part_bg.yc = rot_min.yc;
	part_bg.img_arr = part_bg_img;

	watch.prev_bgnd_min = watch.next_bgnd_min;

	uint16_t *prev_bg_img;
	prev_bg_img = (uint16_t*) heap_caps_malloc(watch.prev_bgnd_min.width * watch.prev_bgnd_min.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	memset(prev_bg_img, 0, watch.prev_bgnd_min.width * watch.prev_bgnd_min.height * sizeof(uint16_t));
	memcpy(prev_bg_img, watch.next_bgnd_min.img_arr, watch.next_bgnd_min.width * watch.next_bgnd_min.height * sizeof(uint16_t));
	watch.prev_bgnd_min.img_arr = prev_bg_img;

	heap_caps_free(watch.next_bgnd_min.img_arr);
	watch.next_bgnd_min = part_bg;

	merging_layers(mrg_img, part_bg_img, rot_min.img_arr, rot_min.width, rot_min.height);

	mrg.width = rot_min.width;
	mrg.height = rot_min.height;
	mrg.type = PART_BCKGND;
	mrg.xc = rot_min.xc;
	mrg.yc = rot_min.yc;
	mrg.img_arr = mrg_img;

	for (y = 0; y < rot_min.height; y++)
	{
		for (x = 0; x < rot_min.width; x++)
		{
			watch.buf_bgnd.img_arr[watch.buf_bgnd.width * (start_y + y) + (start_x + x)] = mrg_img[rot_min.width*y+x];
		}
	}

	uint16_t *buf_part_bg_img, *buf_mrg_img;

	buf_part_bg_img = (uint16_t*) heap_caps_malloc(part_bg.width * part_bg.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	memset(buf_part_bg_img, 0, part_bg.width * part_bg.height * sizeof(uint16_t));
	memcpy(buf_part_bg_img, part_bg.img_arr, part_bg.width * part_bg.height * sizeof(uint16_t));

	buf_mrg_img = (uint16_t*) heap_caps_malloc(mrg.width * mrg.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	memset(buf_mrg_img, 0, mrg.width * mrg.height * sizeof(uint16_t));
	memcpy(buf_mrg_img, mrg.img_arr, mrg.width * mrg.height * sizeof(uint16_t));

	watch.next_part_min = mrg;
	watch.next_part_min.img_arr = buf_mrg_img;

	watch.next_bgnd_min.img_arr = buf_part_bg_img;

	heap_caps_free(part_bg.img_arr);
	heap_caps_free(mrg.img_arr);
	heap_caps_free(rot_min.img_arr);

}

static void rotate_sec(uint16_t sec)
{
	img32_t rot_sec;

	rot_sec = rotate(sec*6, watch.sec_img);

	uint16_t *mrg_img;
	uint16_t *part_bg_img;

	img16_t mrg;
	img16_t part_bg;

	part_bg_img = (uint16_t*) heap_caps_malloc(rot_sec.width * rot_sec.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	mrg_img = (uint16_t*) heap_caps_malloc(rot_sec.width * rot_sec.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);

	memset(part_bg_img, 0, rot_sec.width * rot_sec.height * sizeof(uint16_t));
	memset(mrg_img, 0, rot_sec.width * rot_sec.height * sizeof(uint16_t));

	uint8_t x, y, start_x, start_y, _start_x, _start_y;
	uint32_t i;
	i = 0;
	start_x = 120 - rot_sec.xc;
	start_y = 120 - rot_sec.yc;

	for (y = start_y; y < start_y + rot_sec.height; y++)
	{
		for (x = start_x; x < start_x + rot_sec.width; x++)
		{
			part_bg_img[i] = watch.buf_bgnd.img_arr[240*y+x];
			i++;
		}
	}

	part_bg.width = rot_sec.width;
	part_bg.height = rot_sec.height;
	part_bg.type = PART_BCKGND;
	part_bg.xc = rot_sec.xc;
	part_bg.yc = rot_sec.yc;
	part_bg.img_arr = part_bg_img;

	watch.prev_bgnd_sec = watch.next_bgnd_sec;
	/*
	_start_x = 120 - watch.prev_bgnd_sec.xc;
	_start_y = 120 - watch.prev_bgnd_sec.yc;
	for (y = 0; y < watch.prev_bgnd_sec.height; y++)
	{
		for (x = 0; x < watch.prev_bgnd_sec.width; x++)
		{
			watch.buf_bgnd.img_arr[watch.buf_bgnd.width * (_start_y + y) + (_start_x + x)] = watch.prev_bgnd_sec.img_arr[watch.prev_bgnd_min.width*y+x];
		}
	}
	*/

	uint16_t *prev_bg_img;
	prev_bg_img = (uint16_t*) heap_caps_malloc(watch.prev_bgnd_sec.width * watch.prev_bgnd_sec.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	memset(prev_bg_img, 0, watch.prev_bgnd_sec.width * watch.prev_bgnd_sec.height * sizeof(uint16_t));
	memcpy(prev_bg_img, watch.next_bgnd_sec.img_arr, watch.prev_bgnd_sec.width * watch.prev_bgnd_sec.height * sizeof(uint16_t));
	watch.prev_bgnd_sec.img_arr = prev_bg_img;

	heap_caps_free(watch.next_bgnd_sec.img_arr);
	watch.next_bgnd_sec = part_bg;

	merging_layers(mrg_img, part_bg_img, rot_sec.img_arr, rot_sec.width, rot_sec.height);

	mrg.width = rot_sec.width;
	mrg.height = rot_sec.height;
	mrg.type = PART_BCKGND;
	mrg.xc = rot_sec.xc;
	mrg.yc = rot_sec.yc;
	mrg.img_arr = mrg_img;

	uint16_t *buf_part_bg_img, *buf_mrg_img;

	buf_part_bg_img = (uint16_t*) heap_caps_malloc(part_bg.width * part_bg.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	memset(buf_part_bg_img, 0, part_bg.width * part_bg.height * sizeof(uint16_t));
	memcpy(buf_part_bg_img, part_bg.img_arr, part_bg.width * part_bg.height * sizeof(uint16_t));

	buf_mrg_img = (uint16_t*) heap_caps_malloc(mrg.width * mrg.height * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	memset(buf_mrg_img, 0, mrg.width * mrg.height * sizeof(uint16_t));
	memcpy(buf_mrg_img, mrg.img_arr, mrg.width * mrg.height * sizeof(uint16_t));

	watch.next_part_sec = mrg;
	watch.next_part_sec.img_arr = buf_mrg_img;

	watch.next_bgnd_sec.img_arr = buf_part_bg_img;

	heap_caps_free(part_bg.img_arr);
	heap_caps_free(mrg.img_arr);
	heap_caps_free(rot_sec.img_arr);

}

void watch_app_worker(spi_device_handle_t* spi)
{
	uint8_t start_x = 0;
	uint8_t start_y = 0;
	uint16_t sum_sec;
	sum_sec = watch.hrs*60*60 + watch.min * 60 + watch.sec;
	if(sum_sec % 120 == 0)
	{
		start_x = 120 - watch.prev_bgnd_hrs.xc;
		start_y = 120 - watch.prev_bgnd_hrs.yc;
		if(watch.prev_bgnd_hrs.width * watch.prev_bgnd_hrs.height <= 14400)
			lcd_draw_part_wo_lines(spi, start_x, start_y, watch.prev_bgnd_hrs.width, watch.prev_bgnd_hrs.height, watch.prev_bgnd_hrs.img_arr);
		else
			ESP_LOGI(TAG, "hrs need send with lines");

		start_x = 120 - watch.next_part_hrs.xc;
		start_y = 120 - watch.next_part_hrs.yc;
		if(watch.next_part_hrs.width * watch.next_part_hrs.height <= 14400)
			lcd_draw_part_wo_lines(spi, start_x, start_y, watch.next_part_hrs.width, watch.next_part_hrs.height, watch.next_part_hrs.img_arr);
		else
			ESP_LOGI(TAG, "hrs need send with lines");
	}

	if(sum_sec % 10 == 0)
	{
		start_x = 120 - watch.prev_bgnd_min.xc;
		start_y = 120 - watch.prev_bgnd_min.yc;
		if(watch.prev_bgnd_min.width * watch.prev_bgnd_min.height <= 14400)
			lcd_draw_part_wo_lines(spi, start_x, start_y, watch.prev_bgnd_min.width, watch.prev_bgnd_min.height, watch.prev_bgnd_min.img_arr);
		else
			ESP_LOGI(TAG, "min need send with lines");

		start_x = 120 - watch.next_part_min.xc;
		start_y = 120 - watch.next_part_min.yc;
		if(watch.next_part_min.width * watch.next_part_min.height <= 14400)
			lcd_draw_part_wo_lines(spi, start_x, start_y, watch.next_part_min.width, watch.next_part_min.height, watch.next_part_min.img_arr);
		else
			ESP_LOGI(TAG, "min need send with lines");
	}

	start_x = 120 - watch.prev_bgnd_sec.xc;
	start_y = 120 - watch.prev_bgnd_sec.yc;
	if(watch.prev_bgnd_sec.width * watch.prev_bgnd_sec.height <= 14400)
		lcd_draw_part_wo_lines(spi, start_x, start_y, watch.prev_bgnd_sec.width, watch.prev_bgnd_sec.height, watch.prev_bgnd_sec.img_arr);
	else
		ESP_LOGI(TAG, "sec need send with lines");

	start_x = 120 - watch.next_part_sec.xc;
	start_y = 120 - watch.next_part_sec.yc;
	if(watch.next_part_sec.width * watch.next_part_sec.height <= 14400)
		lcd_draw_part_wo_lines(spi, start_x, start_y, watch.next_part_sec.width, watch.next_part_sec.height, watch.next_part_sec.img_arr);
	else
		ESP_LOGI(TAG, "sec need send with lines");

	watch.sec = watch.sec + 1;
	if(watch.sec >= 60)
	{
		watch.sec = 0;
		watch.min = watch.min + 1;
		if(watch.min >= 60)
		{
			watch.min = 0;
			watch.hrs = watch.hrs + 1;
			if(watch.min >= 12)
			{
				watch.hrs = 0;
			}
		}
	}

	sum_sec = watch.hrs*60*60 + watch.min * 60 + watch.sec;

	if(sum_sec % 120 == 0)
	{
		heap_caps_free(watch.prev_bgnd_hrs.img_arr);
		heap_caps_free(watch.next_part_hrs.img_arr);
		rotate_hrs(watch.min + watch.hrs * 60);
	}

	if(sum_sec % 10 == 0)
	{
		heap_caps_free(watch.prev_bgnd_min.img_arr);
		heap_caps_free(watch.next_part_min.img_arr);
		rotate_min(watch.sec + watch.min * 60);
	}

	heap_caps_free(watch.prev_bgnd_sec.img_arr);
	heap_caps_free(watch.next_part_sec.img_arr);
	rotate_sec(watch.sec);

	multi_heap_info_t info;
	heap_caps_get_info(&info, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);
	printf("MF: %d, TFB: %d LFB: %d\n", info.minimum_free_bytes, info.total_free_bytes, info.largest_free_block);
}

void watch_app_init(spi_device_handle_t* spi)
{
	watch.sec = 0;
	watch.min = 0;
	watch.hrs = 0;

	FILE* f;

	bradis_sin = (uint16_t*) heap_caps_malloc(91*sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	bradis_tan = (uint16_t*) heap_caps_malloc(91*sizeof(uint16_t), MALLOC_CAP_SPIRAM);

	ESP_LOGI(TAG, "Opening file: bradis_sin");
	f = fopen("/spiffs/bradis_sin", "rb");
	if (f == NULL)
	{
		ESP_LOGE(TAG, "Failed to open cyfer file for reading");
		return;
	}

	fread(bradis_sin, 91*2, 1, f);
	fclose(f);

	ESP_LOGI(TAG, "Opening file: bradis_tan");
	f = fopen("/spiffs/bradis_tan", "rb");
	if (f == NULL)
	{
		ESP_LOGE(TAG, "Failed to open cyfer file for reading");
		return;
	}

	fread(bradis_tan, 91*2, 1, f);
	fclose(f);

	ESP_LOGI(TAG, "Opening file: cyfer");
	f = fopen("/spiffs/cyfer", "rb");
	if (f == NULL)
	{
		ESP_LOGE(TAG, "Failed to open cyfer file for reading");
	    return;
	}

	uint8_t cyfer_header[5];
	fread(cyfer_header, sizeof(cyfer_header), 1, f);

	uint16_t *cyfer_img, *buffer_cyfer_img;
	cyfer_img = (uint16_t*) heap_caps_malloc(240*240*sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	buffer_cyfer_img = (uint16_t*) heap_caps_malloc(240*240*sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	assert(cyfer_img != NULL);
	fread(cyfer_img, 2*240*240, 1, f);
	fclose(f);

	img16_t cyfer, buf_cyfer;

	cyfer.width 	= cyfer_header[0];
	cyfer.height 	= cyfer_header[1];
	cyfer.type 		= (img_type_t)cyfer_header[2];
	cyfer.xc 		= cyfer_header[3];
	cyfer.yc 		= cyfer_header[4];
	cyfer.img_arr   = cyfer_img;

	buf_cyfer.width 	= cyfer_header[0];
	buf_cyfer.height 	= cyfer_header[1];
	buf_cyfer.type 		= (img_type_t)cyfer_header[2];
	buf_cyfer.xc 		= cyfer_header[3];
	buf_cyfer.yc 		= cyfer_header[4];
	buf_cyfer.img_arr   = buffer_cyfer_img;

	watch.cyfer_img = cyfer;
	watch.buf_bgnd = buf_cyfer;

	ESP_LOGI(TAG, "Opening file: sec");
	f = fopen("/spiffs/sec", "rb");
	if (f == NULL)
	{
		ESP_LOGE(TAG, "Failed to open sec file for reading");
	    return;
	}

	uint8_t sec_header[5];
	fread(sec_header, sizeof(sec_header), 1, f);
	uint32_t *sec_img;
	sec_img = (uint32_t*) heap_caps_malloc(sec_header[0]*sec_header[1]*sizeof(uint32_t), MALLOC_CAP_SPIRAM);
	assert(sec_img != NULL);
	fread(sec_img, 4*sec_header[0]*sec_header[1], 1, f);
	fclose(f);

	img32_t sec;

	sec.width 	= sec_header[0];
	sec.height 	= sec_header[1];
	sec.type 	= (img_type_t)sec_header[2];
	sec.xc 		= sec_header[3];
	sec.yc 		= sec_header[4];
	sec.img_arr   = sec_img;

	watch.sec_img = sec;

	ESP_LOGI(TAG, "Opening file: min");
	f = fopen("/spiffs/min", "rb");
	if (f == NULL)
	{
		ESP_LOGE(TAG, "Failed to open min file for reading");
		return;
	}

	uint8_t min_header[5];
	fread(min_header, sizeof(min_header), 1, f);
	uint32_t *min_img;
	min_img = (uint32_t*) heap_caps_malloc(min_header[0]*min_header[1]*sizeof(uint32_t), MALLOC_CAP_SPIRAM);
	assert(min_img != NULL);
	fread(min_img, 4*min_header[0]*min_header[1], 1, f);
	fclose(f);

	img32_t min;

	min.width 	= min_header[0];
	min.height 	= min_header[1];
	min.type 	= (img_type_t)min_header[2];
	min.xc 		= min_header[3];
	min.yc 		= min_header[4];
	min.img_arr   = min_img;

	watch.min_img = min;

	ESP_LOGI(TAG, "Opening file: hrs");
	f = fopen("/spiffs/hrs", "rb");
	if (f == NULL)
	{
		ESP_LOGE(TAG, "Failed to open hrs file for reading");
		return;
	}

	uint8_t hrs_header[5];
	fread(hrs_header, sizeof(hrs_header), 1, f);
	uint32_t *hrs_img;
	hrs_img = (uint32_t*) heap_caps_malloc(hrs_header[0]*hrs_header[1]*sizeof(uint32_t), MALLOC_CAP_SPIRAM);
	assert(hrs_img != NULL);
	fread(hrs_img, 4*hrs_header[0]*hrs_header[1], 1, f);
	fclose(f);

	img32_t hrs;

	hrs.width 	= hrs_header[0];
	hrs.height 	= hrs_header[1];
	hrs.type 	= (img_type_t)hrs_header[2];
	hrs.xc 		= hrs_header[3];
	hrs.yc 		= hrs_header[4];
	hrs.img_arr   = hrs_img;

	watch.hrs_img = hrs;

	uint16_t *cyfer_hrs, *cyfer_min, *cyfer_sec;
	cyfer_hrs = (uint16_t*) heap_caps_malloc(240*240*sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	cyfer_min = (uint16_t*) heap_caps_malloc(240*240*sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	cyfer_sec = (uint16_t*) heap_caps_malloc(240*240*sizeof(uint16_t), MALLOC_CAP_SPIRAM);

	memcpy(cyfer_hrs, cyfer.img_arr, 240*240*sizeof(uint16_t));
	memcpy(cyfer_min, cyfer.img_arr, 240*240*sizeof(uint16_t));
	memcpy(cyfer_sec, cyfer.img_arr, 240*240*sizeof(uint16_t));

	watch.next_bgnd_sec = cyfer;
	watch.next_bgnd_min = cyfer;
	watch.next_bgnd_hrs = cyfer;

	watch.next_bgnd_sec.img_arr = cyfer_sec;
	watch.next_bgnd_min.img_arr = cyfer_min;
	watch.next_bgnd_hrs.img_arr = cyfer_hrs;

	watch.next_bgnd_sec.width = 0;
	watch.next_bgnd_min.width = 0;

	watch.next_bgnd_sec.height = 0;
	watch.next_bgnd_min.height = 0;

	watch.prev_min = watch.min_img;

	lcd_draw_all(spi, watch.cyfer_img.img_arr);
	rotate_hrs(watch.hrs*60 + watch.min);
	rotate_min(watch.min*60 + watch.sec);
	rotate_sec(watch.sec);

	watch_app_worker(spi);
}

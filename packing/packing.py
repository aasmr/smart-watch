from PIL import Image

bradis_tan = [
0,87,175,262,349,437,524,612,699,787,
875,963,1051,1139,1228,1317,1405,1495,1584,1673,
1763,1853,1944,2035,2126,2217,2309,2401,2493,2586,
2679,2773,2867,2962,3057,3153,3249,3346,3443,3541,
3640,3739,3939,3938,4040,4142,4245,4348,4452,4557,
4663,4770,4877,4986,5095,5205,5317,5430,5543,5658,
5774,5890,6009,6128,6249,6371,6494,6619,6745,6873,
7002,7133,7265,7400,7536,7673,7813,7954,8098,8243,
8391,8541,8693,8847,9004,9163,9325,9490,9657,9827,
10000,
]

bradis_sin = [
0,175,349,523,698,872,1045,1219,1392,1564,
1736,1908,2079,2250,2419,2588,2756,2924,3090,3256,
3420,3584,3746,3907,4067,4226,4384,4540,4695,4848,
5000,5150,5299,5446,5592,5736,5878,6018,6157,6293,
6428,6561,6691,6820,6947,7071,7193,7314,7431,7547,
7660,7771,7880,7986,8090,8192,8290,8387,8480,8572,
8660,8746,8829,8910,8988,9063,9135,9205,9272,9336,
9397,9455,9511,9563,9613,9659,9703,9744,9781,9816,
9848,9877,9903,9925,9945,9962,9976,9986,9994,9998,
10000,
]

def to_argb8565_esp32(color_tuple):
    color = (int(color_tuple[0]/255*31) << 11)+(int(color_tuple[1]/255*63) << 5)+int(color_tuple[2]/255*31)+(color_tuple[3] << 16)
    #color_a = (color & 0xFF0000) >> 16
    #color_m = (color & 0x00FF00) >> 8
    #color_l = (color & 0x0000FF) >> 0
    #color = color_m + (color_l << 8) + (color_a<<16)
    return color

def to_rgb565_esp32(color_tuple):
    color = (int(color_tuple[0]/255*31) << 11)+(int(color_tuple[1]/255*63) << 5)+int(color_tuple[2]/255*31)
    #color_m = (color & 0xFF00) >> 8
    #color_l = (color & 0x00FF) >> 0
    #color = color_m + (color_l << 8)
    return color

if __name__ == '__main__':
    with open('bradis_tan', 'wb') as f:
        for i in bradis_tan:
            f.write(i.to_bytes(2, byteorder='little'))
    with open('bradis_sin', 'wb') as f:
        for i in bradis_sin:
            f.write(i.to_bytes(2, byteorder='little'))

    bg_name = "cyfer.png"
    strelka_name = "sec_6_96.png"
    _bg = Image.open(bg_name)
    bg = _bg.load()

    _strelka = Image.open(strelka_name)
    strelka = _strelka.load()
    
    strelka_arr = []
    strelka_arr.append(12)
    strelka_arr.append(111)
    strelka_arr.append(1)
    strelka_arr.append(6)
    strelka_arr.append(96)
    for y in range(_strelka.size[1]):
        for x in range(_strelka.size[0]):
            if strelka[x, y][3] == 0:
                strelka_arr.append(to_argb8565_esp32([0, 0, 0, 0]))
            else:
                strelka_arr.append(to_argb8565_esp32(strelka[x, y]))
    

    bg_arr = []
    bg_arr.append(240)
    bg_arr.append(240)
    bg_arr.append(0)
    bg_arr.append(120)
    bg_arr.append(120)

    for y in range(_bg.size[1]):
        for x in range(_bg.size[0]):
            #if to_rgb565_esp32(bg[x, y]) != 0:
                #print(x, y, to_rgb565_esp32(bg[x, y]))
            bg_arr.append(to_rgb565_esp32(bg[x, y]))
            

    with open('cyfer', 'wb') as f:
        f.write(bg_arr[0].to_bytes(1, byteorder='big'))
        f.write(bg_arr[1].to_bytes(1, byteorder='big'))
        f.write(bg_arr[2].to_bytes(1, byteorder='big'))
        f.write(bg_arr[3].to_bytes(1, byteorder='big'))
        f.write(bg_arr[4].to_bytes(1, byteorder='big'))
        for i in bg_arr[5:]:
                f.write(i.to_bytes(2, byteorder='big'))

    with open('sec', 'wb') as f:
        f.write(strelka_arr[0].to_bytes(1, byteorder='big'))
        f.write(strelka_arr[1].to_bytes(1, byteorder='big'))
        f.write(strelka_arr[2].to_bytes(1, byteorder='big'))
        f.write(strelka_arr[3].to_bytes(1, byteorder='big'))
        f.write(strelka_arr[4].to_bytes(1, byteorder='big'))
        for i in strelka_arr[5:]:
                f.write(i.to_bytes(4, byteorder='big'))
        print(strelka_arr[21])

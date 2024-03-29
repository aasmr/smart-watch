import math, time
from PIL import Image
import sys
from PySide6 import QtCore, QtGui, QtWidgets
from PySide6.QtCore import Qt, QObject, QTimer, SIGNAL
from copy import deepcopy

        
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
    return color

def to_rgb565_esp32(color_tuple):
    color = (int(color_tuple[0]/255*31) << 11)+(int(color_tuple[1]/255*63) << 5)+int(color_tuple[2]/255*31)
    return color

def from_rgb565_esp32(color):
    color_list = [0, 0, 0, 255]
    color_list[0] = int(((color >> 11)&31)/31*255)
    color_list[1] = int(((color >> 5)&63)/63*255)
    color_list[2] = int((color & 31)/31*255)
    return color_list

def from_argb8565_esp32(color):
    color_list = [0, 0, 0, 0]
    color_list[0] = int(((color >> 11) & 31)/31*255)
    color_list[1] = int(((color >> 5) & 63)/63*255)
    color_list[2] = int((color & 31)/31*255)
    color_list[3] = int(((color >> 16)&255))
    #print(color_list)
    #print(color)
    return color_list 
def merging_layers(image1, image2, w, h):
    new_img = image1.copy()
    for y in range(h):
        for x in range(w):
            color = from_argb8565_esp32(image2[w*y+x])
            new_color = [0, 0, 0]
            if color[3] != 0:
                new_color[0] = color[0]*rounding(color[3]/255)
                new_color[1] = color[1]*rounding(color[3]/255)
                new_color[2] = color[2]*rounding(color[3]/255)
                new_img[w*y+x] = to_rgb565_esp32([new_color[0], new_color[1], new_color[2]])
    return new_img
def rounding(num):
    if num > 0:
        _num = int(num)
        if num - _num >= 0.5:
            return _num+1
        else:
            return _num
    elif num < 0:
        _num = int(num)
        if num - _num <= -0.5:
            return _num-1
        else:
            return _num
    else:
        _num = int(num)
        return _num
    
def new_coord(x, y, xc, yc, angle):
    coef = 1
    if angle < 0:
        coef=-1
        angle = -1 * angle
    ft_x = (x - xc) - (coef*(bradis_tan[angle]/10000)*(y - yc))
    ft_y = (y - yc)

    st_x = ft_x
    st_y = ft_x * (coef*bradis_sin[angle]/10000) + ft_y

    tt_x = st_x - (coef*(bradis_tan[angle]/10000) * st_y)
    tt_y = st_y

    new_x = tt_x + xc
    new_y = tt_y + yc
    return new_x, new_y

def interpol(image, w, h):
    interp_img = copy(image)
    
    for y in range(1, h-1):
        for x in range(1, w-1):
            color_1 = from_argb8565_esp32(image[w*(y-1) + (x+1)])
            color_2 = from_argb8565_esp32(image[w*(y+1) + (x-1)])
            color_3 = from_argb8565_esp32(image[w*(y-1) + (x-1)])
            color_4 = from_argb8565_esp32(image[w*(y+1) + (x+1)])

            interp_img[w*y + x] = to_argb8565_esp32([rounding((color_1[0]+color_2[0]+color_3[0]+color_4[0])/4),
                                                     rounding((color_1[1]+color_2[1]+color_3[1]+color_4[1])/4),
                                                     rounding((color_1[2]+color_2[2]+color_3[2]+color_4[2])/4),
                                                     rounding((color_1[3]+color_2[3]+color_3[3]+color_4[3])/4)])
    '''
    for y in range(1, h-1):
        for x in range(1, w-1):
            color_1 = from_argb8565_esp32(image[w*(y-1) + (x+1)])
            color_2 = from_argb8565_esp32(image[w*(y+1) + (x-1)])
            color_3 = from_argb8565_esp32(image[w*(y-1) + (x-1)])
            color_4 = from_argb8565_esp32(image[w*(y+1) + (x+1)])
            
            R1 = []
            R1.append((1/2)*(color_1[0]+color_3[0]))
            R1.append((1/2)*(color_1[1]+color_3[1]))
            R1.append((1/2)*(color_1[2]+color_3[2]))
            R1.append((1/2)*(color_1[3]+color_3[3]))
            R2 = []
            R2.append((1/2)*(color_2[0]+color_4[0]))
            R2.append((1/2)*(color_2[1]+color_4[1]))
            R2.append((1/2)*(color_2[2]+color_4[2]))
            R2.append((1/2)*(color_2[3]+color_4[3]))

            P = []
            P.append(rounding((1/2)*(R1[0]+R2[0])))
            P.append(rounding((1/2)*(R1[1]+R2[1])))
            P.append(rounding((1/2)*(R1[2]+R2[2])))
            P.append(rounding((1/2)*(R1[3]+R2[3])))
            interp_img[w*y + x] = to_argb8565_esp32([P[0], P[1], P[2], P[3]])
    '''
    return interp_img   
def rotate(image, w, h, axis, angle):
    buf_image=[]
    if angle > 90 and angle <= 180:
        angle = angle-90
        for x in range(w):
            for y in range(h):
                buf_image.append(image[(h-1-y)*w+x])
        axis = [h-axis[1], axis[0]]
        temp_w = w
        w = h
        h = temp_w
    elif angle > 180 and angle <= 270:
        angle = angle-180
        for y in range(h):
            for x in range(w):
                buf_image.append(image[(h-1-y)*w+x])
        axis = [w-axis[0], h-axis[1]]
    elif angle > 270 and angle <= 360:
        angle = angle-270
        for x in range(w):
            for y in range(h):
                buf_image.append(image[y*w+(w-1-x)])
        axis = [axis[1], w-axis[0]]
        temp_w = w
        w = h
        h = temp_w
    else:
        buf_image = image.copy()
    if angle == 0:
        return buf_image, w, h, axis[0], axis[1]
    elif angle == 90:
        temp_buf_image = []
        for x in range(w):
            for y in range(h):
                temp_buf_image.append(buf_image[(h-1-y)*w+x])
        return temp_buf_image, h, w, h-axis[1], axis[0]
    
    x1, y1 = new_coord(0, 0, axis[0], axis[1], angle)
    x2, y2 = new_coord(w-1, 0, axis[0], axis[1], angle)
    x3, y3 = new_coord(0, h-1, axis[0], axis[1], angle)
    x4, y4 = new_coord(w-1, h-1, axis[0], axis[1], angle)
    x_offset = int(min(x1, x2, x3, x4))
    y_offset = int(min(y1, y2, y3, y4))
    
    w_buf = int(max(x1, x2, x3, x4)) - x_offset+1
    h_buf = int(max(y1, y2, y3, y4)) - y_offset+1

    #print(x_offset, y_offset)

    new_img = []
    for y in range(h_buf):
        for x in range(w_buf):
            new_img.append(0)
    for y in range(h):
        for x in range(w):
            new_x, new_y = new_coord(x, y, axis[0], axis[1], angle)
            new_x = int(new_x) - x_offset
            new_y = int(new_y) - y_offset
            new_img[w_buf*new_y + new_x] = buf_image[y*w+x]
    #print(new_x, new_y)
    '''
    interp_img = []
    for y in range(h_buf):
        for x in range(w_buf):
            interp_img.append(0)
    for y in range(h_buf):
        for x in range(w_buf):
            pro_x, pro_y = new_coord(x+x_offset, y+y_offset, axis[0], axis[1], -angle)
            pro_x = int(pro_x)
            pro_y = int(pro_y)
            if pro_x < 0 or pro_x >= w:
                continue
            elif pro_y < 0 or pro_y >= h:
                continue
            else:
                interp_img[w_buf*y + x] = buf_image[pro_y*w+pro_x]
    #interp_img = interpol(new_img, w_buf, h_buf)
    '''
    return new_img, w_buf, h_buf, axis[0]-x_offset, axis[1]-y_offset
        
class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.sec = 1
        self.prev_area = []
        self.prev_coord = []
        self.buf_area = []
        self.buf_coord = []
        self.next_area = []
        self.next_coord = []
        self.label = QtWidgets.QLabel()
        canvas = QtGui.QPixmap(240, 240)
        canvas.fill(Qt.white)
        self.label.setPixmap(canvas)
        self.setCentralWidget(self.label)
        self.draw_bg()
        self.draw_sec(self.sec*6)
        self.prev_area = deepcopy(self.buf_area)
        self.prev_coord = deepcopy(self.buf_area)
        self.draw_sec_external()
        self.timer = QTimer(self)
        self.connect(self.timer, SIGNAL('timeout()'), self.draw_sec_external)
        self.timer.start(1000)

    def draw_sec_external(self):
        self.draw(self.prev_coord[0], self.prev_coord[1], self.prev_coord[2], self.prev_coord[3], self.prev_area)
        self.draw(self.next_coord[0], self.next_coord[1], self.next_coord[2], self.next_coord[3], self.next_area)
        self.sec +=1
        if self.sec > 59:
            self.sec = 0
        self.prev_area = deepcopy(self.buf_area)
        self.prev_coord = deepcopy(self.buf_coord)
        self.draw_sec(self.sec*6)
        
 
    def draw(self, x_start, y_start, x_end, y_end, draw_array):
        canvas = self.label.pixmap()
        painter = QtGui.QPainter(canvas)
        for i in range(len(draw_array)):
            #print(draw_array[i])
            x = (x_start + i%(x_end-x_start+1))
            y = (y_start + i//(x_end-x_start+1))
            color = from_rgb565_esp32(draw_array[i])
            painter.setPen(QtGui.QColor(color[0], color[1], color[2], color[3]))
            painter.drawPoint(x, y)
        painter.end()
        self.label.setPixmap(canvas)
        
    def draw_bg(self):
        self.draw(0, 0, 239, 239, bg_arr[5:])

    def draw_sec(self, angle):
        image = strelka_arr[5:]
        w = strelka_arr[0]
        h = strelka_arr[1]
        axis = [strelka_arr[3], strelka_arr[4]]
        #self.draw(120-axis[0], 120-axis[1], 120-axis[0]+w-1, 120-axis[1]-h-1, image)
        buf_img, w_buf, h_buf, xc, yc = rotate(image, w, h, axis, angle)
        #print(w_buf, h_buf, xc, yc)
        part_bg = []
        for y in range(h_buf):
            for x in range(w_buf):
                part_bg.append(bg_arr[5:][240*(120-yc+y)+(120-xc+x)])
        self.buf_area = deepcopy(part_bg)
        self.buf_coord = [120-xc, 120-yc, 120-xc+w_buf-1, 120-yc-h_buf-1]
        buf_img = merging_layers(part_bg, buf_img, w_buf, h_buf)
        self.next_area = deepcopy(buf_img)
        self.next_coord = [120-xc, 120-yc, 120-xc+w_buf-1, 120-yc-h_buf-1]    

if __name__ == '__main__':
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
            bg_arr.append(to_rgb565_esp32(bg[x, y]))

    app = QtWidgets.QApplication(sys.argv)
    window = MainWindow()
    window.show()            
    app.exec_()
    window.timer.stop()
    sys.exit()

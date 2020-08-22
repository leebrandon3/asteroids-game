
#include "xscugic.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include <XGpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <MyDisp.h>
#include <math.h>

//Defines for interrupt IDs
#define INTC_DEVICE_ID XPAR_PS7_SCUGIC_0_DEVICE_ID
#define GPIO_INT_ID XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define TIMER_INT_ID XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR

//Global variables accessible from any function
XTmrCtr timer;
int led_val = 0;
XGpio input;
MYDISP display;
int h = 10, vx, vy, vel, switch_data = 0, color, i, count = 0;
int move, left, right, shoot;
double angle = M_PI;
int start, game_start = 0;

//Global Interrupt Controller
static XScuGic GIC;

//This function initalizes the GIC, including vector table setup and CPSR IRQ enable
void initIntrSystem(XScuGic * IntcInstancePtr) {

	XScuGic_Config *IntcConfig;
	IntcConfig = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
	XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, (void *)IntcInstancePtr);
	Xil_ExceptionEnable();

}

void drawShoot(int x, int y, int h, double angle, double r, u32 color){
	display.setForeground(color);
	double x1 = x;
	double y1 = y - h;

	double x1p = (x1 - x) * cos(angle) -  (y1 - y) * sin(angle) + x;
	double y1p = (y1 - y) * cos(angle) +  (x1 - x) * sin(angle) + y;

	display.drawEllipse(1, x1p-r, y1p-r, x1p+r, y1p+r);
}

void buttonInterruptHandler(void *instancePointer) {
	switch_data = XGpio_DiscreteRead(&input, 1);
	if(switch_data == 1){
		move = 1; game_start += 1;
	}
	else if(switch_data == 0) move = 0;

	if(switch_data == 4){
		right = 1; game_start += 1;
	}
	else if(switch_data == 0) right = 0;

	if(switch_data == 2){
		left = 1; game_start += 1;
	}
	else if(switch_data == 0) left = 0;

	if(switch_data == 8){
		count += 1; start = 0; game_start += 1;
		if(count == 6) count = 1;
	}
	else if(switch_data == 0) shoot = 0;

	XGpio_InterruptClear(&input, GPIO_INT_ID);
}

void timerInterruptHandler(void *userParam, u8 TmrCtrNumber) {

}

void drawTriangle(int x, int y, int h, double angle, u32 color){
	display.setForeground(color);
	double x1 = x;
	double y1 = y - h;
	double x2 = x - h;
	double y2 = y + h;
	double x3 = x + h;
	double y3 = y + h;

	double x1p = (x1 - x) * cos(angle) -  (y1 - y) * sin(angle) + x;
	double y1p = (y1 - y) * cos(angle) +  (x1 - x) * sin(angle) + y;
	double x2p = (x2 - x) * cos(angle) -  (y2 - y) * sin(angle) + x;
	double y2p = (y2 - y) * cos(angle) +  (x2 - x) * sin(angle) + y;
	double x3p = (x3 - x) * cos(angle) -  (y3 - y) * sin(angle) + x;
	double y3p = (y3 - y) * cos(angle) +  (x3 - x) * sin(angle) + y;

	display.drawLine(x1p, y1p, x2p, y2p);
	display.drawLine(x1p, y1p, x3p, y3p);
	display.drawLine(x2p, y2p, x3p, y3p);
}

int main() {

	initIntrSystem(&GIC);

	//Configure GPIO input and set direction as usual
	XGpio_Initialize(&input, XPAR_AXI_GPIO_0_DEVICE_ID);
	XGpio_SetDataDirection(&input, 1, 0xF);

	//Configure Timer and timer interrupt as done in class
    XTmrCtr_Initialize(&timer, XPAR_AXI_TIMER_0_DEVICE_ID);
    XTmrCtr_SetHandler(&timer, (XTmrCtr_Handler)timerInterruptHandler, (void*) 0x12345678);
	XScuGic_Connect(&GIC, TIMER_INT_ID,(Xil_InterruptHandler)XTmrCtr_InterruptHandler, &timer);
    XScuGic_Enable(&GIC, TIMER_INT_ID);
    XScuGic_SetPriorityTriggerType(&GIC, TIMER_INT_ID, 0x1, 0x3);
    XTmrCtr_SetOptions(&timer, 0,   XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
    XTmrCtr_SetResetValue(&timer, 0, 0xFFFFFFFF-100000); // 1 Hz 33000000
    XTmrCtr_Start(&timer, 0);

    //Configure GPIO interrupt as done in class
    XScuGic_Connect(&GIC, GPIO_INT_ID, (Xil_ExceptionHandler) buttonInterruptHandler, &input);
    XGpio_InterruptEnable(&input, XGPIO_IR_CH1_MASK);
    XGpio_InterruptGlobalEnable(&input);
    XScuGic_Enable(&GIC, GPIO_INT_ID);
    XScuGic_SetPriorityTriggerType(&GIC, GPIO_INT_ID, 0x2, 0x3);

	double x = 120, y = 160, h = 8;
	double vx, vy, vel = 5;
	int color;
	double angle = 0;
	double bx1, by1, bvx1, bvy1, b1, bspeed = 5;
	double bx2, by2, bvx2, bvy2, b2;
	double bx3, by3, bvx3, bvy3, b3;
	double bx4, by4, bvx4, bvy4, b4;
	double bx5, by5, bvx5, bvy5, b5;
	double bangle1, bangle2, bangle3, bangle4, bangle5;
	double r = 1;

	int ast1 = 0, ast2 = 0, ast3 = 0, ast4 = 0; 	//determines when to create new asteroid
	int ax1, ax2, ax3, ax4;							//asteroid x-coordinate
	int ay1, ay2, ay3, ay4;							//asteroid y-coordinate
	double aangle1, aangle2, aangle3, aangle4;		//random angle for each asteroid
	double aspeed1, aspeed2, aspeed3, aspeed4;		//random speed for each asteroid
	double avx1, avy1, avx2, avy2, avx3, avy3, avx4, avy4;
	int ra = 20;
	int life = 3;
	char life_title[20] = "Lives Remaining";
	int lx = 20, ly = 20, lc = clrWhite, ll = clrBlack;
	double langle = 0;
	int start_count = 0;
	char title[20] = "Press BTN to start";

	display.begin();
	display.clearDisplay(clrBlack);


	while (true) {
		if(game_start > 0){
			if(start_count < 3) ++start_count;
			if(start_count == 1){
				display.setForeground(clrBlack);
				display.drawText(title,50,160);
				color = clrWhite;
				drawTriangle(x,y,h,angle,color);
			}
			if(life > 0){
				if(move == 1){
					color = clrBlack;
					drawTriangle(x,y,h,angle,color);
					vx = vel*sin(angle);
					vy = vel*cos(angle);
					x += vx;
					y -= vy;
					if(x < h) x = 240 - h;
					if(x > 240 - h) x = h;
					if(y < h) y = 320 - h;
					if(y > 320 - h) y = h;
					color = clrWhite;
					drawTriangle(x,y,h,angle,color);
				}
				if(right == 1){
					color = clrBlack;
					drawTriangle(x,y,h,angle,color);
					angle += 0.3;
					color = clrWhite;
					drawTriangle(x,y,h,angle,color);
				}
				if(left == 1){
					color = clrBlack;
					drawTriangle(x,y,h,angle,color);
					angle -= 0.3;
					color = clrWhite;
					drawTriangle(x,y,h,angle,color);
				}
				if(ast1 == 0){
					ax1 = rand() % 240; ay1 = 0;
					aangle1 = 2.1;
					aspeed1 = 1.5;
					ast1 = 1;
				}
				if(ast2 == 0){
					ax2 = 0; ay2 = rand() % 320;
					aangle2 = 1;
					aspeed2 = 2;
					ast2 = 1;
				}
				if(ast3 == 0){
					ax3 = rand() % 240; ay3 = 320;
					aangle3 = 5;
					aspeed3 = 2.3;
					ast3 = 1;
				}
				if(ast4 == 0){
					ax4 = 240; ay4 = rand() % 320;
					aangle4 = 2;
					aspeed4 = 3;
					ast4 = 1;
				}

				if(ast1 == 1){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax1-ra, ay1-ra, ax1+ra, ay1+ra);
					avx1 = aspeed1*sin(aangle1);
					avy1 = aspeed1*cos(aangle1);
					ax1 += avx1; ay1 += avy1;
					if(ax1 < ra) ax1 = 240 - ra;
					if(ax1 > 240) ax1 = ra;
					if(ay1 < ra) ay1 = 320 - ra;
					if(ay1 > 320) ay1 = ra;
					display.setForeground(clrWhite);
					display.drawEllipse(0, ax1-ra, ay1-ra, ax1+ra, ay1+ra);
				}
				if(ast2 == 1){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax2-ra, ay2-ra, ax2+ra, ay2+ra);
					avx2 = aspeed2*sin(aangle2);
					avy2 = aspeed2*cos(aangle2);
					ax2 -= avx2; ay2 -= avy2;
					if(ax2 < ra) ax2 = 240 - ra;
					if(ax2 > 240 - ra) ax2 = ra;
					if(ay2 < ra) ay2 = 320 - ra;
					if(ay2 > 320 - ra) ay2 = ra;
					display.setForeground(clrWhite);
					display.drawEllipse(0, ax2-ra, ay2-ra, ax2+ra, ay2+ra);
				}
				if(ast3 == 1){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax3-ra, ay3-ra, ax3+ra, ay3+ra);
					avx3 = aspeed3*sin(aangle3);
					avy3 = aspeed3*cos(aangle3);
					ax3 += avx3; ay3 -= avy3;
					if(ax3 < ra) ax3 = 240 - ra;
					if(ax3 > 240 - ra) ax3 = ra;
					if(ay3 < ra) ay3 = 320 - ra;
					if(ay3 > 320 - ra) ay3 = ra;
					display.setForeground(clrWhite);
					display.drawEllipse(0, ax3-ra, ay3-ra, ax3+ra, ay3+ra);
				}
				if(ast4 == 1){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax4-ra, ay4-ra, ax4+ra, ay4+ra);
					avx4 = aspeed4*sin(aangle4);
					avy4 = aspeed4*cos(aangle4);
					ax4 += avx4; ay4 -= avy4;
					if(ax4 < ra) ax4 = 240 - ra;
					if(ax4 > 240 - ra) ax4 = ra;
					if(ay4 < ra) ay4 = 320 - ra;
					if(ay4 > 320 - ra) ay4 = ra;
					display.setForeground(clrWhite);
					display.drawEllipse(0, ax4-ra, ay4-ra, ax4+ra, ay4+ra);
				}
				if(count == 1){
					++start;
					if(start == 1){
						bangle1 = angle; bx1 = x; by1 = y;
						color = clrWhite;
						drawShoot(bx1,by1,h,bangle1,r,color);
						b1 = 1;
					}
				}
				if(b1 == 1){
					color = clrBlack;
					drawShoot(bx1,by1,h,bangle1,r,color);
					bvx1 = bspeed*sin(bangle1);
					bvy1 = bspeed*cos(bangle1);
					bx1 += bvx1;
					by1 -= bvy1;
					color = clrWhite;
					drawShoot(bx1,by1,h,bangle1,r,color);
				}
				if(count == 2){
					++start;
					if(start == 1){
						bangle2 = angle; bx2 = x; by2 = y;
						color = clrWhite;
						drawShoot(bx2,by2,h,bangle2,r,color);
						b2 = 1;
					}
				}
				if(b2 == 1){
					color = clrBlack;
					drawShoot(bx2,by2,h,bangle2,r,color);
					bvx2 = bspeed*sin(bangle2);
					bvy2 = bspeed*cos(bangle2);
					bx2 += bvx2;
					by2 -= bvy2;
					color = clrWhite;
					drawShoot(bx2,by2,h,bangle2,r,color);
				}
				if(count == 3){
					++start;
					if(start == 1){
						bangle3 = angle; bx3 = x; by3 = y;
						color = clrWhite;
						drawShoot(bx3,by3,h,bangle3,r,color);
						b3 = 1;
					}
				}
				if(b3 == 1){
					color = clrBlack;
					drawShoot(bx3,by3,h,bangle3,r,color);
					bvx3 = bspeed*sin(bangle3);
					bvy3 = bspeed*cos(bangle3);
					bx3 += bvx3;
					by3 -= bvy3;
					color = clrWhite;
					drawShoot(bx3,by3,h,bangle3,r,color);
				}
				if(count == 4){
					++start;
					if(start == 1){
						bangle4 = angle; bx4 = x; by4 = y;
						color = clrWhite;
						drawShoot(bx4,by4,h,bangle4,r,color);
						b4 = 1;
					}
				}
				if(b4 == 1){
					color = clrBlack;
					drawShoot(bx4,by4,h,bangle4,r,color);
					bvx4 = bspeed*sin(bangle4);
					bvy4 = bspeed*cos(bangle4);
					bx4 += bvx4;
					by4 -= bvy4;
					color = clrWhite;
					drawShoot(bx4,by4,h,bangle4,r,color);
				}
				if(count == 5){
					++start;
					if(start == 1){
						bangle5 = angle; bx5 = x; by5 = y;
						color = clrWhite;
						drawShoot(bx5,by5,h,bangle5,r,color);
						b5 = 1;
					}
				}
				if(b5 == 1){
					color = clrBlack;
					drawShoot(bx5,by5,h,bangle5,r,color);
					bvx5 = bspeed*sin(bangle5);
					bvy5 = bspeed*cos(bangle5);
					bx5 += bvx5;
					by5 -= bvy5;
					color = clrWhite;
					drawShoot(bx5,by5,h,bangle5,r,color);
				}

				if((x + h) > (ax1 - ra) && (x - h) < (ax1 + ra) && (y + h) > (ay1 - ra) && (y - h) < (ay1 + ra)){
					life -= 1;
					if(life == 2) drawTriangle(lx+30,ly,h,langle,ll);
					if(life == 1) drawTriangle(lx+15,ly,h,langle,ll);
					if(life == 0) drawTriangle(lx,ly,h,langle,ll);
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax1-ra, ay1-ra, ax1+ra, ay1+ra);
					ax1 = 120; ay1 = 0;
				}
				if((x + h) > (ax2 - ra) && (x - h) < (ax2 + ra) && (y + h) > (ay2 - ra) && (y - h) < (ay2 + ra)){
					life -= 1;
					if(life == 2) drawTriangle(lx+30,ly,h,langle,ll);
					if(life == 1) drawTriangle(lx+15,ly,h,langle,ll);
					if(life == 0) drawTriangle(lx,ly,h,langle,ll);
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax2-ra, ay2-ra, ax2+ra, ay2+ra);
					ax2 = 0; ay2 = 160;
				}
				if((x + h) > (ax3 - ra) && (x - h) < (ax3 + ra) && (y + h) > (ay3 - ra) && (y - h) < (ay3 + ra)){
					life -= 1;
					if(life == 2) drawTriangle(lx+30,ly,h,langle,ll);
					if(life == 1) drawTriangle(lx+15,ly,h,langle,ll);
					if(life == 0) drawTriangle(lx,ly,h,langle,ll);
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax3-ra, ay3-ra, ax3+ra, ay3+ra);
					ax3 = 120; ay3 = 320;
				}
				if((x + h) > (ax4 - ra) && (x - h) < (ax4 + ra) && (y + h) > (ay4 - ra) && (y - h) < (ay4 + ra)){
					life -= 1;
					if(life == 2) drawTriangle(lx+30,ly,h,langle,ll);
					if(life == 1) drawTriangle(lx+15,ly,h,langle,ll);
					if(life == 0) drawTriangle(lx,ly,h,langle,ll);
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax4-ra, ay4-ra, ax4+ra, ay4+ra);
					ax4 = 240; ay4 = 160;
				}

				if(life == 3){
					display.setForeground(clrWhite);
					display.drawText(life_title,15,3);
					drawTriangle(lx+30,ly,h,langle,lc);
					drawTriangle(lx+15,ly,h,langle,lc);
					drawTriangle(lx,ly,h,langle,lc);
				}
				if(life == 2){
					display.setForeground(clrWhite);
					display.drawText(life_title,15,3);
					drawTriangle(lx+15,ly,h,langle,lc);
					drawTriangle(lx,ly,h,langle,lc);
				}
				if(life == 1){
					display.setForeground(clrWhite);
					display.drawText(life_title,15,3);
					drawTriangle(lx,ly,h,langle,lc);
				}

				if((bx1 + r) > (ax1 - ra) && (bx1 - r) < (ax1 + ra) && (by1 + r) > (ay1 - ra) && (by1 - r) < (ay1 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax1-ra, ay1-ra, ax1+ra, ay1+ra);
					ast1 = 0;
					color = clrBlack;
					drawShoot(bx1,by1,h,bangle1,r,color);
					b1 = 0;
				}
				if((bx2 + r) > (ax1 - ra) && (bx2 - r) < (ax1 + ra) && (by2 + r) > (ay1 - ra) && (by2 - r) < (ay1 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax1-ra, ay1-ra, ax1+ra, ay1+ra);
					ast1 = 0;
					color = clrBlack;
					drawShoot(bx2,by2,h,bangle2,r,color);
					b2 = 0;
				}
				if((bx3 + r) > (ax1 - ra) && (bx3 - r) < (ax1 + ra) && (by3 + r) > (ay1 - ra) && (by3 - r) < (ay1 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax1-ra, ay1-ra, ax1+ra, ay1+ra);
					ast1 = 0;
					color = clrBlack;
					drawShoot(bx3,by3,h,bangle3,r,color);
					b3 = 0;
				}
				if((bx4 + r) > (ax1 - ra) && (bx4 - r) < (ax1 + ra) && (by4 + r) > (ay1 - ra) && (by4 - r) < (ay1 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax1-ra, ay1-ra, ax1+ra, ay1+ra);
					ast1 = 0;
					color = clrBlack;
					drawShoot(bx4,by4,h,bangle4,r,color);
					b4 = 0;
				}
				if((bx5 + r) > (ax1 - ra) && (bx5 - r) < (ax1 + ra) && (by5 + r) > (ay1 - ra) && (by5 - r) < (ay1 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax1-ra, ay1-ra, ax1+ra, ay1+ra);
					ast1 = 0;
					color = clrBlack;
					drawShoot(bx5,by5,h,bangle5,r,color);
					b5 = 0;
				}

				if((bx1 + r) > (ax2 - ra) && (bx1 - r) < (ax2 + ra) && (by1 + r) > (ay2 - ra) && (by1 - r) < (ay2 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax2-ra, ay2-ra, ax2+ra, ay2+ra);
					ast2 = 0;
					color = clrBlack;
					drawShoot(bx1,by1,h,bangle1,r,color);
					b1 = 0;
				}
				if((bx2 + r) > (ax2 - ra) && (bx2 - r) < (ax2 + ra) && (by2 + r) > (ay2 - ra) && (by2 - r) < (ay2 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax2-ra, ay2-ra, ax2+ra, ay2+ra);
					ast2 = 0;
					color = clrBlack;
					drawShoot(bx2,by2,h,bangle2,r,color);
					b2 = 0;
				}
				if((bx3 + r) > (ax2 - ra) && (bx3 - r) < (ax2 + ra) && (by3 + r) > (ay2 - ra) && (by3 - r) < (ay2 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax2-ra, ay2-ra, ax2+ra, ay2+ra);
					ast2 = 0;
					color = clrBlack;
					drawShoot(bx3,by3,h,bangle3,r,color);
					b3 = 0;
				}
				if((bx4 + r) > (ax2 - ra) && (bx4 - r) < (ax2 + ra) && (by4 + r) > (ay2 - ra) && (by4 - r) < (ay2 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax2-ra, ay2-ra, ax2+ra, ay2+ra);
					ast2 = 0;
					color = clrBlack;
					drawShoot(bx4,by4,h,bangle4,r,color);
					b4 = 0;
				}
				if((bx5 + r) > (ax2 - ra) && (bx5 - r) < (ax2 + ra) && (by5 + r) > (ay2 - ra) && (by5 - r) < (ay2 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax2-ra, ay2-ra, ax2+ra, ay2+ra);
					ast2 = 0;
					color = clrBlack;
					drawShoot(bx5,by5,h,bangle5,r,color);
					b5 = 0;
				}

				if((bx1 + r) > (ax3 - ra) && (bx1 - r) < (ax3 + ra) && (by1 + r) > (ay3 - ra) && (by1 - r) < (ay3 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax3-ra, ay3-ra, ax3+ra, ay3+ra);
					ast3 = 0;
					color = clrBlack;
					drawShoot(bx1,by1,h,bangle1,r,color);
					b1 = 0;
				}
				if((bx2 + r) > (ax3 - ra) && (bx2 - r) < (ax3 + ra) && (by2 + r) > (ay3 - ra) && (by2 - r) < (ay3 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax3-ra, ay3-ra, ax3+ra, ay3+ra);
					ast3 = 0;
					color = clrBlack;
					drawShoot(bx2,by2,h,bangle2,r,color);
					b2 = 0;
				}
				if((bx3 + r) > (ax3 - ra) && (bx3 - r) < (ax3 + ra) && (by3 + r) > (ay3 - ra) && (by3 - r) < (ay3 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax3-ra, ay3-ra, ax3+ra, ay3+ra);
					ast3 = 0;
					color = clrBlack;
					drawShoot(bx3,by3,h,bangle3,r,color);
					b3 = 0;
				}
				if((bx4 + r) > (ax3 - ra) && (bx4 - r) < (ax3 + ra) && (by4 + r) > (ay3 - ra) && (by4 - r) < (ay3 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax3-ra, ay3-ra, ax3+ra, ay3+ra);
					ast3 = 0;
					color = clrBlack;
					drawShoot(bx4,by4,h,bangle4,r,color);
					b4 = 0;
				}
				if((bx5 + r) > (ax3 - ra) && (bx5 - r) < (ax3 + ra) && (by5 + r) > (ay3 - ra) && (by5 - r) < (ay3 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax3-ra, ay3-ra, ax3+ra, ay3+ra);
					ast3 = 0;
					color = clrBlack;
					drawShoot(bx5,by5,h,bangle5,r,color);
					b5 = 0;
				}

				if((bx1 + r) > (ax4 - ra) && (bx1 - r) < (ax4 + ra) && (by1 + r) > (ay4 - ra) && (by1 - r) < (ay4 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax4-ra, ay4-ra, ax4+ra, ay4+ra);
					ast4 = 0;
					color = clrBlack;
					drawShoot(bx1,by1,h,bangle1,r,color);
					b1 = 0;
				}
				if((bx2 + r) > (ax4 - ra) && (bx2 - r) < (ax4 + ra) && (by2 + r) > (ay4 - ra) && (by2 - r) < (ay4 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax4-ra, ay4-ra, ax4+ra, ay4+ra);
					ast4 = 0;
					color = clrBlack;
					drawShoot(bx2,by2,h,bangle2,r,color);
					b2 = 0;
				}
				if((bx3 + r) > (ax4 - ra) && (bx3 - r) < (ax4 + ra) && (by3 + r) > (ay4 - ra) && (by3 - r) < (ay4 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax4-ra, ay4-ra, ax4+ra, ay4+ra);
					ast4 = 0;
					color = clrBlack;
					drawShoot(bx3,by3,h,bangle3,r,color);
					b3 = 0;
				}
				if((bx4 + r) > (ax4 - ra) && (bx4 - r) < (ax4 + ra) && (by4 + r) > (ay4 - ra) && (by4 - r) < (ay4 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax4-ra, ay4-ra, ax4+ra, ay4+ra);
					ast4 = 0;
					color = clrBlack;
					drawShoot(bx4,by4,h,bangle4,r,color);
					b4 = 0;
				}
				if((bx5 + r) > (ax4 - ra) && (bx5 - r) < (ax4 + ra) && (by5 + r) > (ay4 - ra) && (by5 - r) < (ay4 + ra)){
					display.setForeground(clrBlack);
					display.drawEllipse(0, ax4-ra, ay4-ra, ax4+ra, ay4+ra);
					ast4 = 0;
					color = clrBlack;
					drawShoot(bx5,by5,h,bangle5,r,color);
					b5 = 0;
				}

				if(b1 == 0){
					bx1 = x; by1 = y;
				}
				if(b2 == 0){
					bx2 = x; by2 = y;
				}
				if(b3 == 0){
					bx3 = x; by3 = y;
				}
				if(b4 == 0){
					bx4 = x; by4 = y;
				}
				if(b5 == 0){
					bx5 = x; by5 = y;
				}
			}
			if(life == 0){
				color = clrBlack;
				drawTriangle(x,y,h,angle,color);
				display.setForeground(clrBlack);
				display.drawEllipse(0, ax1-ra, ay1-ra, ax1+ra, ay1+ra);
				display.setForeground(clrBlack);
				display.drawEllipse(0, ax2-ra, ay2-ra, ax2+ra, ay2+ra);
				display.setForeground(clrBlack);
				display.drawEllipse(0, ax3-ra, ay3-ra, ax3+ra, ay3+ra);
				display.setForeground(clrBlack);
				display.drawEllipse(0, ax4-ra, ay4-ra, ax4+ra, ay4+ra);

				color = clrBlack;
				drawShoot(bx1,by1,h,bangle1,r,color);
				drawShoot(bx2,by2,h,bangle2,r,color);
				drawShoot(bx3,by3,h,bangle3,r,color);
				drawShoot(bx4,by4,h,bangle4,r,color);
				drawShoot(bx5,by5,h,bangle5,r,color);

				display.setForeground(clrBlack);
				display.drawText(life_title,15,3);

				char end[20] = "GAME OVER";
				display.setForeground(clrWhite);
				display.drawText(end,80,160);
			}
		}
		else{
			display.setForeground(clrWhite);
			display.drawText(title,50,160);
		}
	}

}

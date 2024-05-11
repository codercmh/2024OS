#include "x86.h"
#include "device.h"

void initSerial(void) {
	outByte(SERIAL_PORT + 1, 0x00);
	outByte(SERIAL_PORT + 3, 0x80);
	outByte(SERIAL_PORT + 0, 0x01);
	outByte(SERIAL_PORT + 1, 0x00);
	outByte(SERIAL_PORT + 3, 0x03);
	outByte(SERIAL_PORT + 2, 0xC7);
	outByte(SERIAL_PORT + 4, 0x0B);
}

static inline int serialIdle(void) {
	return (inByte(SERIAL_PORT + 5) & 0x20) != 0;
}

void putChar(char ch) {
	while (serialIdle() != TRUE);
	outByte(SERIAL_PORT, ch);
}

void putNum(int num) {
    if (num == 0) {
        putChar('0');
        putChar('\n');
        return;
    }

    if (num < 0) {
        putChar('-');
        num = -num;
    }

    int stack[10];  // 数字最大位数通常不会超过10位
    int top = 0;

    // 将每位数字放入栈中
    while (num > 0) {
        stack[top++] = num % 10;
        num /= 10;
    }

    // 从栈中取出并打印，实现逆序打印
    while (top > 0) {
        putChar(stack[--top] + '0');
    }

    putChar('\n');
}


#include "x86.h"
#include "device.h"

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

void GProtectFaultHandle(struct StackFrame *sf);

void timerHandle(struct StackFrame *sf);

void syscallHandle(struct StackFrame *sf);

void syscallWrite(struct StackFrame *sf);
void syscallPrint(struct StackFrame *sf);

void syscallFork(struct StackFrame *sf);
void syscallSleep(struct StackFrame *sf);
void syscallExit(struct StackFrame *sf);


void set_new_running_process(void)
{
	//轮转调度
	int i;
	for(i=(current+1)%MAX_PCB_NUM;i!=current;i=(i+1)%MAX_PCB_NUM){
		if(pcb[i].state==STATE_RUNNABLE && i!=0){
			pcb[i].state=STATE_RUNNING;
			current=i;
			break;
		}
	}
	if(i==current){
		current=0;
		pcb[0].state=STATE_RUNNING;
	}
}


void switch_process(void)
{
	uint32_t tmpStackTop1 = pcb[current].stackTop;
	pcb[current].stackTop = pcb[current].prevStackTop;
	tss.esp0 = (uint32_t)&(pcb[current].stackTop);
	asm volatile("movl %0, %%esp"::"m"(tmpStackTop1)); // switch kernel stack
	asm volatile("popl %gs");
	asm volatile("popl %fs");
	asm volatile("popl %es");
	asm volatile("popl %ds");
	asm volatile("popal");
	asm volatile("addl $8, %esp");
	asm volatile("iret");
}


void irqHandle(struct StackFrame *sf)
{ // pointer sf = esp
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds" ::"a"(KSEL(SEG_KDATA)));
	/*XXX Save esp to stackTop */
	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)sf;

	switch (sf->irq)
	{
	case -1:
		break;
	case 0xd:
		GProtectFaultHandle(sf);
		break;
	case 0x20:
		timerHandle(sf);
		break;
	case 0x80:
		syscallHandle(sf);
		break;
	default:
		assert(0);
	}
	/*XXX Recover stackTop */
	pcb[current].stackTop = tmpStackTop;
}

void GProtectFaultHandle(struct StackFrame *sf)
{
	assert(0);
	return;
}

void timerHandle(struct StackFrame *sf)
{
	// TODO

	//遍历pcb，将状态为STATE_BLOCKED的进程的sleepTime减一，如果进程的sleepTime变为0，重新设为STATE_RUNNABLE
	for(int i=0;i<MAX_PCB_NUM;i++){
		if(pcb[i].state==STATE_BLOCKED){
			pcb[i].sleepTime--;
			if(pcb[i].sleepTime==0){
				pcb[i].state=STATE_RUNNABLE;
			}
		}
	}

	//将当前进程的timeCount加一，如果时间片用完（timeCount==MAX_TIME_COUNT）
	//且有其它状态为STATE_RUNNABLE的进程，切换，否则继续执行当前进程
	pcb[current].timeCount++;
	if(pcb[current].timeCount==MAX_TIME_COUNT){
		pcb[current].timeCount=0;
		pcb[current].state=STATE_RUNNABLE;
		set_new_running_process();
		switch_process();
	}else{
		return;
	}
}

void syscallHandle(struct StackFrame *sf)
{
	switch (sf->eax)
	{ // syscall number
	case 0:
		syscallWrite(sf);
		break; // for SYS_WRITE
	/*TODO Add Fork,Sleep... */
	case 1:
		syscallFork(sf);
		break;
	case 3:
		syscallSleep(sf);
		break;
	case 4:
		syscallExit(sf);
		break;
	default:
		break;
	}
}

void syscallWrite(struct StackFrame *sf)
{
	switch (sf->ecx)
	{ // file descriptor
	case 0:
		syscallPrint(sf);
		break; // for STD_OUT
	default:
		break;
	}
}

void syscallPrint(struct StackFrame *sf)
{
	int sel = sf->ds; // segment selector for user data, need further modification
	char *str = (char *)sf->edx;
	int size = sf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es" ::"m"(sel));
	for (i = 0; i < size; i++)
	{
		asm volatile("movb %%es:(%1), %0" : "=r"(character) : "r"(str + i));
		if (character == '\n')
		{
			displayRow++;
			displayCol = 0;
			if (displayRow == 25)
			{
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
		}
		else
		{
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));
			displayCol++;
			if (displayCol == 80)
			{
				displayRow++;
				displayCol = 0;
				if (displayRow == 25)
				{
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
		// asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
		// asm volatile("int $0x20":::"memory"); //XXX Testing irqTimer during syscall
	}

	updateCursor(displayRow, displayCol);
	// take care of return value
	return;
}


// TODO syscallFork ...
void syscallFork(struct StackFrame *sf)
{
	//寻找一个空闲的pcb
	int index;
	int i;
	for(i=0;i<MAX_PCB_NUM;i++){
		if(pcb[i].state==STATE_DEAD){
			index=i;
			break;
		}
	}
	if(i==MAX_PCB_NUM){
		pcb[current].regs.eax=-1;
		return;
	}

	//复制资源并初始化
	pcb[index].pid = index;
	pcb[index].prevStackTop = pcb[current].prevStackTop - (uint32_t)&(pcb[current]) + (uint32_t)&(pcb[index]);
	pcb[index].sleepTime = 0;

	pcb[index].stackTop = pcb[current].stackTop - (uint32_t)&(pcb[current]) + (uint32_t)&(pcb[index]);

	pcb[index].state = STATE_RUNNABLE;
	pcb[index].timeCount = 0;

	pcb[index].regs.edi = pcb[current].regs.edi;
	pcb[index].regs.esi = pcb[current].regs.esi;
	pcb[index].regs.ebp = pcb[current].regs.ebp;
	pcb[index].regs.xxx = pcb[current].regs.xxx;
	pcb[index].regs.ebx = pcb[current].regs.ebx;
	pcb[index].regs.edx = pcb[current].regs.edx;
	pcb[index].regs.ecx = pcb[current].regs.ecx;
	pcb[index].regs.eax = pcb[current].regs.eax;
	pcb[index].regs.irq = pcb[current].regs.irq;
	pcb[index].regs.error = pcb[current].regs.error;
	pcb[index].regs.eip = pcb[current].regs.eip;
	pcb[index].regs.eflags = pcb[current].regs.eflags;
	pcb[index].regs.esp = pcb[current].regs.esp;

	pcb[index].regs.cs = USEL(1 + 2*index);
	pcb[index].regs.ss = USEL(2 * (index+1));
	pcb[index].regs.ds = USEL(2 * (index+1));
	pcb[index].regs.es = USEL(2 * (index+1));
	pcb[index].regs.fs = USEL(2 * (index+1));
	pcb[index].regs.gs = USEL(2 * (index+1));

	pcb[current].regs.eax = index;
	pcb[index].regs.eax = 0;
	return;
}

void syscallSleep(struct StackFrame *sf)
{
	//判断sleepTime的合法性
	if(sf->ecx<0){
		pcb[current].regs.eax = -1;
		return;
	}
	pcb[current].state = STATE_BLOCKED;
	pcb[current].sleepTime = sf->ecx;

	set_new_running_process();
	
	switch_process();
	
	pcb[current].regs.eax = 0;
	return;
}

void syscallExit(struct StackFrame *sf){

	pcb[current].state = STATE_DEAD;

	set_new_running_process();

	switch_process();
}
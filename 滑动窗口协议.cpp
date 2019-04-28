#include "sysinclude.h"
#include <list>


extern void SendFRAMEPacket(unsigned char* pData, unsigned int len);


#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4


typedef enum { data, ack, nak } frame_kind;
struct frame_head
{
	frame_kind kind;
	unsigned int seq;
	unsigned int ack;
	unsigned char data[100];
};
struct frame
{
	frame_head head;
	int size;
};
struct Buffer
{
	unsigned char* pBuffer;
	frame* pFrame;
	unsigned int leng;
};


/*
* 停等协议测试函数
*/
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static Buffer window;
	static int lower = 1, upper = 1;
	static list<Buffer> waitlist;


	switch (messageType)
	{
	case MSG_TYPE_SEND:
	{
		Buffer buffer;
		buffer.pFrame = new frame();
		*(buffer.pFrame) = *((frame*)pBuffer);
		buffer.pBuffer = (unsigned char*)buffer.pFrame;
		buffer.leng = bufferSize;


		waitlist.push_back(buffer);

		while (!waitlist.empty() && upper == lower)
		{
			Buffer buffer = waitlist.front();
			waitlist.pop_front();
			SendFRAMEPacket(buffer.pBuffer, buffer.leng);
			window = buffer;
			upper++;
		}
		break;
	}
	case MSG_TYPE_RECEIVE:
	{
		unsigned int ack = ntohl(((frame*)pBuffer)->head.ack);
		if (ack != lower) break;
		lower++;


		while (!waitlist.empty() && upper == lower)
		{
			Buffer buffer = waitlist.front();
			waitlist.pop_front();
			SendFRAMEPacket(buffer.pBuffer, buffer.leng);
			window = buffer;
			upper++;
		}
		break;
	}
	case MSG_TYPE_TIMEOUT:
	{
		SendFRAMEPacket(window.pBuffer, window.leng);
		break;
	}
	}
	return 0;
}


/*
* 回退n帧测试函数
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static Buffer window[WINDOW_SIZE_BACK_N_FRAME];
	static int lower = 1, upper = 1;
	static list<Buffer> waitlist;


	switch (messageType)
	{
	case MSG_TYPE_SEND:
	{
		Buffer buffer;
		buffer.pFrame = new frame();
		*(buffer.pFrame) = *((frame*)pBuffer);
		buffer.pBuffer = (unsigned char*)buffer.pFrame;
		buffer.leng = bufferSize;


		waitlist.push_back(buffer);

		while (!waitlist.empty() && upper - lower < WINDOW_SIZE_BACK_N_FRAME)
		{
			Buffer buffer = waitlist.front();
			waitlist.pop_front();
			SendFRAMEPacket(buffer.pBuffer, buffer.leng);
			window[upper % WINDOW_SIZE_BACK_N_FRAME] = buffer;
			upper++;
		}
		break;
	}
	case MSG_TYPE_RECEIVE:
	{
		unsigned int ack = ntohl(((frame*)pBuffer)->head.ack);
		if (ack < lower || ack >= upper) break;
		int i;
		for (i = lower; i < upper; ++i)
		{
			if (ntohl(window[i % WINDOW_SIZE_BACK_N_FRAME].pFrame->head.seq) == ack)
			{
				lower = i + 1;
				break;
			}
		}


		while (!waitlist.empty() && upper - lower < WINDOW_SIZE_BACK_N_FRAME)
		{
			Buffer buffer = waitlist.front();
			waitlist.pop_front();
			SendFRAMEPacket(buffer.pBuffer, buffer.leng);
			window[upper % WINDOW_SIZE_BACK_N_FRAME] = buffer;
			upper++;
		}
		break;
	}
	case MSG_TYPE_TIMEOUT:
	{
		int i;
		for (i = lower; i < upper; ++i)
		{
			Buffer buffer = window[i % WINDOW_SIZE_BACK_N_FRAME];
			SendFRAMEPacket(buffer.pBuffer, buffer.leng);
		}
		break;
	}
	}


	return 0;
}


/*
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static Buffer window[WINDOW_SIZE_BACK_N_FRAME];
	static int lower = 1, upper = 1;
	static list<Buffer> waitlist;


	switch (messageType)
	{
	case MSG_TYPE_SEND:
	{
		Buffer buffer;
		buffer.pFrame = new frame();
		*(buffer.pFrame) = *((frame*)pBuffer);
		buffer.pBuffer = (unsigned char*)buffer.pFrame;
		buffer.leng = bufferSize;


		waitlist.push_back(buffer);

		while (!waitlist.empty() && upper - lower < WINDOW_SIZE_BACK_N_FRAME)
		{
			Buffer buffer = waitlist.front();
			waitlist.pop_front();
			SendFRAMEPacket(buffer.pBuffer, buffer.leng);
			window[upper % WINDOW_SIZE_BACK_N_FRAME] = buffer;
			upper++;
		}
		break;
	}
	case MSG_TYPE_RECEIVE:
	{
		unsigned int ack = ntohl(((frame*)pBuffer)->head.ack);
		unsigned int kind = ntohl((u_long)((frame*)pBuffer)->head.kind);
		if (ack < lower || ack >= upper) break;
		if (kind == nak)
		{
			Buffer buffer = window[ack % WINDOW_SIZE_BACK_N_FRAME];
			SendFRAMEPacket(buffer.pBuffer, buffer.leng);
			return 0;
		}


		int i;
		for (i = lower; i < upper; ++i)
		{
			if (ntohl(window[i % WINDOW_SIZE_BACK_N_FRAME].pFrame->head.seq) == ack)
			{
				lower = i + 1;
				break;
			}
		}


		while (!waitlist.empty() && upper - lower < WINDOW_SIZE_BACK_N_FRAME)
		{
			Buffer buffer = waitlist.front();
			waitlist.pop_front();
			SendFRAMEPacket(buffer.pBuffer, buffer.leng);
			window[upper % WINDOW_SIZE_BACK_N_FRAME] = buffer;
			upper++;
		}
		break;
	}
	case MSG_TYPE_TIMEOUT:
	{
		Buffer buffer = window[lower % WINDOW_SIZE_BACK_N_FRAME];
		SendFRAMEPacket(buffer.pBuffer, buffer.leng);
		break;
	}
	}
	return 0;
}

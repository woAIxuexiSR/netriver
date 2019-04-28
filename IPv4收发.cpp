/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"


extern void ip_DiscardPkt(char* pBuffer, int type);


extern void ip_SendtoLower(char*pBuffer, int length);


extern void ip_SendtoUp(char *pBuffer, int length);


extern unsigned int getIpv4Address();


// implemented by students


unsigned int checksum(unsigned short int *pBuffer, int length)
{
	unsigned int sum = 0;
	for (int i = 0; i < length; ++i)
	{
		sum += ntohs(pBuffer[i]);
		sum = (sum >> 16) + (sum & 0xffff);
	}
	return sum;
}


int stud_ip_recv(char *pBuffer, unsigned short length)
{
	int version = ((int)pBuffer[0]) >> 4;
	if (version != 4)
	{
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
		return 1;
	}


	int headlen = ((int)pBuffer[0]) & 0xf;
	if (headlen <= 4)
	{
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
		return 1;
	}


	int time = (int)pBuffer[8];
	if (time <= 0)
	{
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
		return 1;
	}


	unsigned int p = checksum((unsigned short int*)pBuffer, headlen * 2);
	if (p != 0xffff)
	{
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
		return 1;
	}


	unsigned addr = ntohl(((int*)pBuffer)[4]);
	if (addr != getIpv4Address() && addr != 0xffffffff)
	{
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR);
		return 1;
	}


	pBuffer += headlen * 4;
	length -= headlen * 4;
	ip_SendtoUp(pBuffer, length);
	return 0;
}


int stud_ip_Upsend(char *pBuffer, unsigned short len, unsigned int srcAddr,
	unsigned int dstAddr, byte protocol, byte ttl)
{
	char* pkt = (char*)malloc(len + 20);
	memcpy(pkt + 20, pBuffer, len);


	pkt[0] = 0x45;
	pkt[1] = 0x0;

	((unsigned short*)pkt)[1] = htons(len + 20);
	((unsigned*)pkt)[1] = 0x0;

	pkt[8] = ttl;
	pkt[9] = protocol;
	((unsigned*)pkt)[3] = htonl(srcAddr);
	((unsigned*)pkt)[4] = htonl(dstAddr);


	((unsigned short*)pkt)[5] = 0x0;
	unsigned short p = htons((unsigned short)checksum((unsigned short int*)pkt, 10));
	((unsigned short*)pkt)[5] = ~p;


	ip_SendtoLower(pkt, len + 20);
	return 0;
}

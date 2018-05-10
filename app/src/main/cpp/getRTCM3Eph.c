#include "playback.h"
#include "gpssim.h"
#include "log.h"
#include <math.h>
#include <sys/socket.h>
#include <strings.h>
#include <netdb.h>
#include <endian.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <errno.h>

// Need to link with Ws2_32.lib
//#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT "27015"
#define RTCM3PREAMB 0xD3 
static const unsigned int tbl_CRC24Q[] = {
	0x000000, 0x864CFB, 0x8AD50D, 0x0C99F6, 0x93E6E1, 0x15AA1A, 0x1933EC, 0x9F7F17,
	0xA18139, 0x27CDC2, 0x2B5434, 0xAD18CF, 0x3267D8, 0xB42B23, 0xB8B2D5, 0x3EFE2E,
	0xC54E89, 0x430272, 0x4F9B84, 0xC9D77F, 0x56A868, 0xD0E493, 0xDC7D65, 0x5A319E,
	0x64CFB0, 0xE2834B, 0xEE1ABD, 0x685646, 0xF72951, 0x7165AA, 0x7DFC5C, 0xFBB0A7,
	0x0CD1E9, 0x8A9D12, 0x8604E4, 0x00481F, 0x9F3708, 0x197BF3, 0x15E205, 0x93AEFE,
	0xAD50D0, 0x2B1C2B, 0x2785DD, 0xA1C926, 0x3EB631, 0xB8FACA, 0xB4633C, 0x322FC7,
	0xC99F60, 0x4FD39B, 0x434A6D, 0xC50696, 0x5A7981, 0xDC357A, 0xD0AC8C, 0x56E077,
	0x681E59, 0xEE52A2, 0xE2CB54, 0x6487AF, 0xFBF8B8, 0x7DB443, 0x712DB5, 0xF7614E,
	0x19A3D2, 0x9FEF29, 0x9376DF, 0x153A24, 0x8A4533, 0x0C09C8, 0x00903E, 0x86DCC5,
	0xB822EB, 0x3E6E10, 0x32F7E6, 0xB4BB1D, 0x2BC40A, 0xAD88F1, 0xA11107, 0x275DFC,
	0xDCED5B, 0x5AA1A0, 0x563856, 0xD074AD, 0x4F0BBA, 0xC94741, 0xC5DEB7, 0x43924C,
	0x7D6C62, 0xFB2099, 0xF7B96F, 0x71F594, 0xEE8A83, 0x68C678, 0x645F8E, 0xE21375,
	0x15723B, 0x933EC0, 0x9FA736, 0x19EBCD, 0x8694DA, 0x00D821, 0x0C41D7, 0x8A0D2C,
	0xB4F302, 0x32BFF9, 0x3E260F, 0xB86AF4, 0x2715E3, 0xA15918, 0xADC0EE, 0x2B8C15,
	0xD03CB2, 0x567049, 0x5AE9BF, 0xDCA544, 0x43DA53, 0xC596A8, 0xC90F5E, 0x4F43A5,
	0x71BD8B, 0xF7F170, 0xFB6886, 0x7D247D, 0xE25B6A, 0x641791, 0x688E67, 0xEEC29C,
	0x3347A4, 0xB50B5F, 0xB992A9, 0x3FDE52, 0xA0A145, 0x26EDBE, 0x2A7448, 0xAC38B3,
	0x92C69D, 0x148A66, 0x181390, 0x9E5F6B, 0x01207C, 0x876C87, 0x8BF571, 0x0DB98A,
	0xF6092D, 0x7045D6, 0x7CDC20, 0xFA90DB, 0x65EFCC, 0xE3A337, 0xEF3AC1, 0x69763A,
	0x578814, 0xD1C4EF, 0xDD5D19, 0x5B11E2, 0xC46EF5, 0x42220E, 0x4EBBF8, 0xC8F703,
	0x3F964D, 0xB9DAB6, 0xB54340, 0x330FBB, 0xAC70AC, 0x2A3C57, 0x26A5A1, 0xA0E95A,
	0x9E1774, 0x185B8F, 0x14C279, 0x928E82, 0x0DF195, 0x8BBD6E, 0x872498, 0x016863,
	0xFAD8C4, 0x7C943F, 0x700DC9, 0xF64132, 0x693E25, 0xEF72DE, 0xE3EB28, 0x65A7D3,
	0x5B59FD, 0xDD1506, 0xD18CF0, 0x57C00B, 0xC8BF1C, 0x4EF3E7, 0x426A11, 0xC426EA,
	0x2AE476, 0xACA88D, 0xA0317B, 0x267D80, 0xB90297, 0x3F4E6C, 0x33D79A, 0xB59B61,
	0x8B654F, 0x0D29B4, 0x01B042, 0x87FCB9, 0x1883AE, 0x9ECF55, 0x9256A3, 0x141A58,
	0xEFAAFF, 0x69E604, 0x657FF2, 0xE33309, 0x7C4C1E, 0xFA00E5, 0xF69913, 0x70D5E8,
	0x4E2BC6, 0xC8673D, 0xC4FECB, 0x42B230, 0xDDCD27, 0x5B81DC, 0x57182A, 0xD154D1,
	0x26359F, 0xA07964, 0xACE092, 0x2AAC69, 0xB5D37E, 0x339F85, 0x3F0673, 0xB94A88,
	0x87B4A6, 0x01F85D, 0x0D61AB, 0x8B2D50, 0x145247, 0x921EBC, 0x9E874A, 0x18CBB1,
	0xE37B16, 0x6537ED, 0x69AE1B, 0xEFE2E0, 0x709DF7, 0xF6D10C, 0xFA48FA, 0x7C0401,
	0x42FA2F, 0xC4B6D4, 0xC82F22, 0x4E63D9, 0xD11CCE, 0x575035, 0x5BC9C3, 0xDD8538
};




#define P2_5        0.03125             /* 2^-5 */
#define P2_6        0.015625            /* 2^-6 */
#define P2_11       4.882812500000000E-04 /* 2^-11 */
#define P2_15       3.051757812500000E-05 /* 2^-15 */
#define P2_17       7.629394531250000E-06 /* 2^-17 */
#define P2_19       1.907348632812500E-06 /* 2^-19 */
#define P2_20       9.536743164062500E-07 /* 2^-20 */
#define P2_21       4.768371582031250E-07 /* 2^-21 */
#define P2_23       1.192092895507810E-07 /* 2^-23 */
#define P2_24       5.960464477539063E-08 /* 2^-24 */
#define P2_27       7.450580596923828E-09 /* 2^-27 */
#define P2_29       1.862645149230957E-09 /* 2^-29 */
#define P2_30       9.313225746154785E-10 /* 2^-30 */
#define P2_31       4.656612873077393E-10 /* 2^-31 */
#define P2_32       2.328306436538696E-10 /* 2^-32 */
#define P2_33       1.164153218269348E-10 /* 2^-33 */
#define P2_35       2.910383045673370E-11 /* 2^-35 */
#define P2_38       3.637978807091710E-12 /* 2^-38 */
#define P2_39       1.818989403545856E-12 /* 2^-39 */
#define P2_40       9.094947017729280E-13 /* 2^-40 */
#define P2_43       1.136868377216160E-13 /* 2^-43 */
#define P2_48       3.552713678800501E-15 /* 2^-48 */
#define P2_50       8.881784197001252E-16 /* 2^-50 */
#define P2_55       2.775557561562891E-17 /* 2^-55 */
#define SC2RAD      3.1415926535898     /* semi-circle to radian (IS-GPS) */

unsigned int getbitu(const unsigned char *buff, int pos, int len);
unsigned int crc24q(const unsigned char *buff, int len);

int getbits(const unsigned char *buff, int pos, int len)
{
	unsigned int bits = getbitu(buff, pos, len);
	if (len <= 0 || 32 <= len || !(bits&(1u << (len - 1)))) return (int)bits;
	return (int)(bits | (~0u << len)); /* extend sign */
}
unsigned int getbitu(const unsigned char *buff, int pos, int len)
{
	unsigned int bits = 0;
	int i;
	for (i = pos; i<pos + len; i++) bits = (bits << 1) + ((buff[i / 8] >> (7 - i % 8)) & 1u);
	return bits;
}

unsigned int crc24q(const unsigned char *buff, int len)
{
	unsigned int crc = 0;
	int i;

	for (i = 0; i<len; i++) crc = ((crc << 8) & 0xFFFFFF) ^ tbl_CRC24Q[(crc >> 16) ^ buff[i]];
	return crc;
}
static double timeoffset_ = 0.0;        /* time offset (s) */
int nSV = 0;
int decode_rtcm3(ephem_t eph[][MAX_SAT],unsigned char *buffer, int len) {
	int ret = 0, type = getbitu(buffer, 24, 12);
	if (type == 1019)
	{
		ephem_t _eph = { 0 };
		double toc, sqrtA;
		char *msg;
		int i = 24 + 12, prn, sat, week;// sys = SYS_GPS;

		if (i + 476 <= len * 8) {

			int prn = getbitu(buffer, i, 6);              i += 6;
			if (prn >= 0 && prn <= 32)
			{
				int week = getbitu(buffer, i, 10) + 1024;              i += 10; //VALID UNTIL gps week-->2048
				int sva = getbitu(buffer, i, 4);              i += 4;
				int code = getbitu(buffer, i, 2);              i += 2;
				_eph.idot = getbits(buffer, i, 14)*P2_43*SC2RAD; i += 14;
				_eph.iode = getbitu(buffer, i, 8);              i += 8;
				_eph.toc.sec = getbitu(buffer, i, 16)*16.0;         i += 16;
				_eph.toc.week = week;
				_eph.af2 = getbits(buffer, i, 8)*P2_55;        i += 8;
				_eph.af1 = getbits(buffer, i, 16)*P2_43;        i += 16;
				_eph.af0 = getbits(buffer, i, 22)*P2_31;        i += 22;
				_eph.iodc = getbitu(buffer, i, 10);              i += 10;
				_eph.crs = getbits(buffer, i, 16)*P2_5;         i += 16;
				_eph.deltan = getbits(buffer, i, 16)*P2_43*SC2RAD; i += 16;
				_eph.m0 = getbits(buffer, i, 32)*P2_31*SC2RAD; i += 32;
				_eph.cuc = getbits(buffer, i, 16)*P2_29;        i += 16;
				_eph.ecc = getbitu(buffer, i, 32)*P2_33;        i += 32;
				_eph.cus = getbits(buffer, i, 16)*P2_29;        i += 16;
				_eph.sqrta = getbitu(buffer, i, 32)*P2_19;        i += 32;
				_eph.toe.sec = getbitu(buffer, i, 16)*16.0;         i += 16;
				_eph.toe.week = week;
				_eph.cic = getbits(buffer, i, 16)*P2_29;        i += 16;
				_eph.omg0 = getbits(buffer, i, 32)*P2_31*SC2RAD; i += 32;
				_eph.cis = getbits(buffer, i, 16)*P2_29;        i += 16;
				_eph.inc0 = getbits(buffer, i, 32)*P2_31*SC2RAD; i += 32;
				_eph.crc = getbits(buffer, i, 16)*P2_5;         i += 16;
				_eph.aop = getbits(buffer, i, 32)*P2_31*SC2RAD; i += 32;
				_eph.omgdot = getbits(buffer, i, 24)*P2_43*SC2RAD; i += 24;

				_eph.tgd = getbits(buffer, i, 8)*P2_31;        i += 8;
				int svh = getbitu(buffer, i, 6);              i += 6;
				_eph.vflg = getbitu(buffer, i, 1);              i += 1;
				//eph.fit = getbitu(buffer, i, 1) ? 0.0 : 4.0; /* 0:4hr,1:>4hr */
				_eph.vflg = 1;

				_eph.A = _eph.sqrta * _eph.sqrta;
				_eph.n = sqrt(GM_EARTH / (_eph.A*_eph.A*_eph.A)) + _eph.deltan;
				_eph.sq1e2 = sqrt(1.0 - _eph.ecc*_eph.ecc);
				_eph.omgkdot = _eph.omgdot - OMEGA_EARTH;

				eph[0][prn - 1] = _eph;
				nSV++;
			}

		}
		else {

			return -1;
		}
	}
	return nSV;

	//
	//eph.sat = sat;
	//eph.week = adjgpsweek(week);
	//eph.toe = gpst2time(eph.week, eph.toes);
	//eph.toc = gpst2time(eph.week, toc);
	//eph.ttr = rtcm->time;
	//eph.A = sqrtA*sqrtA;
	//if (!strstr(rtcm->opt, "-EPHALL")) {
	//	if (eph.iode == rtcm->nav.eph[sat - 1].iode) return 0; /* unchanged */
	//}
	//rtcm->nav.eph[sat - 1] = eph;
	//rtcm->ephsat = sat;
}

int CreateTxSocket(char* destination, int port)
{
	struct sockaddr_in socketAddressStruct;
	struct hostent *hostAddressStruct;
	int sock;
	int error;
	int opt;

	/*Addresses of socket*/
	socketAddressStruct.sin_family=AF_INET;
	socketAddressStruct.sin_port=htons(port);
	hostAddressStruct=gethostbyname(destination);
	if(hostAddressStruct==0)
	{
		return -1;
	}
	bcopy(hostAddressStruct->h_addr,&socketAddressStruct.sin_addr,hostAddressStruct->h_length);
	/*Create socket*/
	sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock<=0)
	{
		return -1;
	}
	opt=1;
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(int))) {
		LOGE( "Setsockopt TCP_NODELAY error");
		return -1;
	}
	if (setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, &opt, sizeof(int)))
	{
		LOGE( "Setsockopt TCP_QUICKACK error");
		return -1;
	}
	struct timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	if( (setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO, (char *)&timeout,  sizeof(timeout))) < 0){
		LOGD("Setsockopt send timeout error");
		return -1;
	}

	/*Connection*/
	error=connect(sock, (struct sockaddr*) &socketAddressStruct, sizeof(socketAddressStruct));
	if(error==-1)
	{
		LOGD("Connection to IGS is not established");
		return -1;
	}

	LOGD("Connection to IGS established");
	return sock;
}

int getRTCM3Eph(ephem_t eph[][MAX_SAT])
{
	int ConnectSocket = -1;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char *sendbuf = "GET http://products.igs-ip.net/RTCM3EPH HTTP/1.0\r\nUser-Agent: NTRIP RTKLIB\r\nAuthorization: Basic dGluNms0NDp0aW42azQ0\r\n\r\n";
	unsigned char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	ConnectSocket = CreateTxSocket("78.46.41.26", 2101);

	if (ConnectSocket == -1) {
		printf("Unable to connect to server!\n");
		return 1;
	}

	// Send an initial buffer
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);

	if (iResult == -1) {
		LOGE("send failed with error: %d\n", errno);
		close(ConnectSocket);
		return 1;
	}

	LOGD("Bytes Sent: %ld\n", iResult);

	// shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SHUT_WR);
	if (iResult == -1) {
		LOGE("shutdown failed with error: %d\n", errno);
		close(ConnectSocket);
		return 1;
	}

	// Receive until the peer closes the connection
	unsigned char buff[1200];
	int i = 0;
	int nSV = 0;
	do {

		iResult = recv(ConnectSocket, (char*)recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			//int nbyte=0;

			int len, nbyte;
			for (nbyte = 0; nbyte < iResult; nbyte++) {

				if (i == 0) {
					if (recvbuf[nbyte] != RTCM3PREAMB) continue;
					buff[i++] = recvbuf[nbyte];
					continue;
					//return 0;
				}
				buff[i++] = recvbuf[nbyte];

				if (i == 3) {
					len = getbitu(buff, 14, 10) + 3; /* length without parity */
				}
				if (i < 3 || i < len + 3) continue;
				i = 0;

				/* check parity */
				if (crc24q(buff, len) != getbitu(buff, len * 8, 24)) {
					continue;
				}
				/* decode rtcm3 message */
				nSV=decode_rtcm3(eph,buff, len);
				if (nSV >= MAX_SAT)
					break;
			}
			if (nSV >= MAX_SAT)
				break;
		}

	} while (iResult > 0);

	// cleanup
    close(ConnectSocket);

	return 1;
}

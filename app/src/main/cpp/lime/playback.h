#ifndef _BLADEGPS_H
#define _BLADEGPS_H


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <gpssim.h>
#include "getopt.h"
#ifdef _WIN32
// To avoid conflict between time.h and pthread.h on Windows
#define HAVE_STRUCT_TIMESPEC
#endif
#include <pthread.h>
#include <stdbool.h>

#define TX_FREQUENCY	1575420000
#define TX_SAMPLERATE	1000000
#define TX_BANDWIDTH	2000000
#define TX_VGA1			-25
#define TX_VGA2			0

#define NUM_BUFFERS			32
#define SAMPLES_PER_BUFFER	(TX_SAMPLERATE/10)
#define NUM_TRANSFERS		16
#define TIMEOUT_MS			1000

#define NUM_IQ_SAMPLES  (TX_SAMPLERATE / 10)
//#define NUM_IQ_SAMPLES  SAMPLES_PER_BUFFER
#define FIFO_LENGTH     (NUM_IQ_SAMPLES * 2)

typedef struct {
	pthread_t thread;
	pthread_mutex_t lock;
	//int error;

	struct bladerf *dev;
	int16_t *buffer;
	pthread_cond_t frontend_initialization_done;
} tx_t;

typedef struct {
	pthread_t thread;
	pthread_mutex_t lock;
	//int error;

	int ready;
	pthread_cond_t initialization_done;
} gps_t;
typedef struct{
	pthread_t thread;
	pthread_t rcvThread;
	pthread_mutex_t lock;
}tcp_t;
typedef struct {
	char navfile[MAX_CHAR];
	char umfile[MAX_CHAR];
	int staticLocationMode;
	int nmeaGGA;
	int iduration;
	int verb;
	gpstime_t g0;
	double llh[3];
	int interactive;
	int timeoverwrite;
	int iono_enable;
	int format;
} option_t;

typedef struct {

	option_t opt;
	tx_t tx;
	gps_t gps;
	tcp_t tcp;

	int status;
	bool finished;
	int16_t *fifo;
	char strBinFileName[1024];
	char strMetaFileName[1024];
	long head, tail;
	size_t sample_length;

	pthread_cond_t fifo_read_ready;
	pthread_cond_t fifo_write_ready;
	bool frontendReady;

	double time;
} sim_t;
int nConnectedClient;
extern int is_fifo_write_ready(sim_t *s);
#endif
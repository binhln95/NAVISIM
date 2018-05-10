//
// Created by 8460LK on 4/16/2018.
//

#define EXIT_CODE_CONTROL_C (-3)
#define EXIT_CODE_NO_DEVICE (-2)
#define EXIT_CODE_LMS_OPEN  (-1)

#define DEFAULT_ANTENNA 1 // antenna with BW [30MHz .. 2000MHz]

#include "playback.h"
// for _getch used in Windows runtime.
#ifdef WIN32
#include <conio.h>
#include <lime/LimeSuite.h>

#else
#include <unistd.h>
#endif

#include "LimeSuite.h"
#include "log.h"
#include <sys/socket.h>
#include "gpssim.h"
#include <ctype.h>
#include <lime_main.h>
#include <endian.h>
#include <strings.h>

extern int hasFile;
extern int hasMeta;
//int idx = -1;
int idx2 = 0;
extern char strBinFile[];
extern char strMetaFile[];
extern int tuneVel;
/*//LAM*/
extern lms_stream_t tx_stream = { handle: 0, isTx : true,channel : 0, fifoSize : 32 * 1024,
                        throughputVsLatency : 0.5, dataFmt : LMS_FMT_I12 };
extern lms_device_t *device = NULL;

void init_sim(sim_t *s)
{
    s->tx.dev = NULL;
    pthread_mutex_init(&(s->tx.lock), NULL);
    //s->tx.error = 0;

    pthread_mutex_init(&(s->gps.lock), NULL);

    pthread_mutex_init(&(s->tcp.lock), NULL);
    //s->gps.error = 0;
    s->gps.ready = 0;
    pthread_cond_init(&(s->gps.initialization_done), NULL);

    s->status = 0;
    s->head = 0;
    s->tail = 0;
    s->sample_length = 0;

    pthread_cond_init(&(s->fifo_write_ready), NULL);
    pthread_cond_init(&(s->fifo_read_ready), NULL);

    s->time = 0.0;
    nConnectedClient = 0;
    s->finished = 0;
    pthread_cond_init(&(s->tx.frontend_initialization_done), NULL);
    s->frontendReady = 0;
}

size_t get_sample_length(sim_t *s)
{
    long length;

    length = s->head - s->tail;
    if (length < 0)
        length += FIFO_LENGTH;

    return((size_t)length);
}

size_t fifo_read(int16_t *buffer, size_t samples, sim_t *s)
{
    size_t length;
    size_t samples_remaining;
    int16_t *buffer_current = buffer;

    length = get_sample_length(s);

    if (length < samples)
        samples = length;

    length = samples; // return value

    samples_remaining = FIFO_LENGTH - s->tail;

    if (samples > samples_remaining) {
        memcpy(buffer_current, &(s->fifo[s->tail * 2]), samples_remaining * sizeof(int16_t) * 2);
        s->tail = 0;
        buffer_current += samples_remaining * 2;
        samples -= samples_remaining;
    }

    memcpy(buffer_current, &(s->fifo[s->tail * 2]), samples * sizeof(int16_t) * 2);
    s->tail += (long)samples;
    if (s->tail >= FIFO_LENGTH)
        s->tail -= FIFO_LENGTH;

    return(length);
}

bool is_finish(sim_t *s)
{
    return s->finished;
}

int is_fifo_write_ready(sim_t *s)
{
    int status = 0;

    s->sample_length = get_sample_length(s);
    if (s->sample_length < NUM_IQ_SAMPLES)
        status = 1;

    return(status);
}

int tx_task(char *sExternalStoragePath)
{
    LMS_StartStream(&tx_stream);
    short iq_buff[SAMPLES_PER_BUFFER*2 ];
    char sBinFile[1024];
    strcpy(sBinFile, sExternalStoragePath);
    strcat(sBinFile, "/gpssim.bin");
    setCpuPriority (7, -20);

    FILE* fid = fopen(sBinFile, "rb");

    while (!feof(fid)){

        int nRead = fread(iq_buff, sizeof(short), SAMPLES_PER_BUFFER*2, fid);

        int sendStream = LMS_SendStream(&tx_stream, iq_buff, SAMPLES_PER_BUFFER, NULL, 2000);
        LOGD("Generated %d samples",sendStream);
    }
    LMS_StopStream(&tx_stream); //stream is stopped but can be started again with LMS_StartStream()

    fclose(fid);
    if(device!=NULL){
        LOGD("Closing device");
        LMS_Close(device);
        LOGD("Closing device succesful");
    }
    return 1;
}

void *tx_task_realTime(void *arg)
{
    sim_t *s = (sim_t *)arg;
    size_t samples_populated;
    LMS_StartStream(&tx_stream);
    while (1) {
        int16_t *tx_buffer_current = s->tx.buffer;
        unsigned int buffer_samples_remaining = SAMPLES_PER_BUFFER;
        /*	if (is_finish(s))
            {
                goto out;
            }*/
        while (buffer_samples_remaining > 0) {

            pthread_mutex_lock(&(s->gps.lock));
            while (get_sample_length(s) == 0)
            {
                pthread_cond_wait(&(s->fifo_read_ready), &(s->gps.lock));
            }
            //			assert(get_sample_length(s) > 0);

            samples_populated = fifo_read(tx_buffer_current,
                                          buffer_samples_remaining,
                                          s);
            pthread_mutex_unlock(&(s->gps.lock));

            pthread_cond_signal(&(s->fifo_write_ready));
#if 0
            if (is_fifo_write_ready(s)) {
				/*
				printf("\rTime = %4.1f", s->time);
				s->time += 0.1;
				fflush(stdout);
				*/
			}
			else if (is_finished_generation(s))
			{
				goto out;
			}
#endif
            // Advance the buffer pointer.
            buffer_samples_remaining -= (unsigned int)samples_populated;
            tx_buffer_current += (2 * samples_populated);
        }

        int sendStream = LMS_SendStream(&tx_stream, s->tx.buffer, SAMPLES_PER_BUFFER, NULL, 1000);

    }
    out:
    return NULL;
}

void str2upper(char *str) {
    // Convert to upper case
    while (*str) {
        *str = toupper((unsigned char)*str);
        str++;
    }

}

void* tcp_task(void* arg)
{
    /*WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    sim_t *s = (sim_t *)arg;
    SOCKET ClientSocket2 = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        //return 1;
    }

    //while (1)
    {

        printf("Waiting at port %s\r\n\r\n", DEFAULT_PORT);
        // Accept a client socket
        ClientSocket2 = accept(ListenSocket, NULL, NULL);
        if (ClientSocket2 == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            //closesocket(ListenSocket);
            closesocket(ClientSocket2);
            //WSACleanup();

            //return 1;
        }
        pthread_mutex_lock(&(s->tcp.lock));
        ClientSocket = ClientSocket2;
        nConnectedClient = 1;
        pthread_mutex_unlock(&(s->tcp.lock));
    }




    // No longer need server socket
    closesocket(ListenSocket);*/
}

void* tcp_rcv_task(void* arg)
{
    sim_t *s = (sim_t *)arg;
    int iRcvResult;
    char* ReadBuffer = (char*)malloc(1025);
    char* strTmp;
    while (1)
    {
        //pthread_mutex_lock(&(s->tcp.lock));
        if (nConnectedClient > 0)
        {
            /*iRcvResult= recv(ClientSocket, ReadBuffer, 1024, 0);
            ReadBuffer[iRcvResult] = 0;
            str2upper(ReadBuffer);

            if (iRcvResult == SOCKET_ERROR) {
                nConnectedClient = 0;
                printf("rcv failed with error: %d\n", WSAGetLastError());
                //closesocket(ClientSocket);
                //WSACleanup();
                //return 1;
            }
            else if ((strTmp = strstr(ReadBuffer, "$VEL,")) > 0)
            {
                idx2 = 0;
                strTmp += 5;
                char strTuneVel[MAX_CHAR];
                while (*strTmp != '$' && *strTmp != '\0' && idx2 < 127)
                {
                    strTuneVel[idx2++] = *strTmp;
                    strTmp++;
                }
                strTuneVel[idx2] = 0;
                tuneVel = atoi(strTuneVel);
            }
            else if (strstr(ReadBuffer, "$QUIT$")>0)
            {
                s->finished = 1;
            }
            else
            {
                if ((strTmp = strstr(ReadBuffer, "$FILE,")) > 0)
                {
                    idx2 = 0;
                    strTmp += 6;
                    while (*strTmp != '$' && *strTmp != '\0' && idx2 < 127)
                    {
                        strBinFile[idx2++] = *strTmp;
                        strTmp++;
                    }
                    strBinFile[idx2] = 0;
                    hasFile = 1;
                    printf("Binary file %s\r\n", strBinFile);
                }
                if ((strTmp = strstr(ReadBuffer, "$META,")) > 0)
                {
                    idx2 = 0;
                    strTmp += 6;
                    while (*strTmp != '$' && *strTmp != '\0' && idx2 < 127)
                    {
                        strMetaFile[idx2++] = *strTmp;
                        strTmp++;
                    }

                    strMetaFile[idx2] = 0;
                    hasMeta = 1;
                    printf("Metadata file %s\r\n", strMetaFile);
                }
            }*/
        }
        //pthread_mutex_unlock(&(s->tcp.lock));
    }




    // No longer need server socket
}
int start_tcp_task(sim_t *s)
{
    int status;

    status = pthread_create(&(s->tcp.thread), NULL, tcp_task, s);

    return(status);
}
int start_tcp_rcv_task(sim_t *s)
{
    int status;

    status = pthread_create(&(s->tcp.rcvThread), NULL, tcp_rcv_task, s);

    return(status);
}
int start_tx_task(sim_t *s)
{
    int status;

    status = pthread_create(&(s->tx.thread), NULL, tx_task_realTime, s);

    return(status);
}

int start_gps_task(sim_t *s)
{
    int status = 0;

    status = pthread_create(&(s->gps.thread), NULL,&runCore_realTime, s);

    return(status);
}

int initLimeSDR()
{
/*//LAM*/
    int device_count = 1;
    /*int device_count = LMS_GetDeviceList(NULL);
    if (device_count < 1){
        return(EXIT_CODE_NO_DEVICE);
    }*/
    lms_info_str_t *device_list = (lms_info_str_t*)malloc(sizeof(lms_info_str_t) * device_count);
    device_count = LMS_GetDeviceList(device_list);

    int i =device_count;

    double gain = 1.0;
    int32_t antenna = DEFAULT_ANTENNA;
    int32_t channel = 0;
    int32_t index = 0;
    int32_t bits = 16;
    double sampleRate = TX_SAMPLERATE;
    int32_t dynamic = 2047;


    if (LMS_Open(&device, device_list[index], NULL)){
        return(EXIT_CODE_LMS_OPEN);
    }

    int lmsReset = LMS_Reset(device);
    if (lmsReset){
        printf("lmsReset %d(%s)" "\n", lmsReset, LMS_GetLastErrorMessage());
    }
    int lmsInit = LMS_Init(device);
    if (lmsInit){
        printf("lmsInit %d(%s)" "\n", lmsInit, LMS_GetLastErrorMessage());
    }

    int channel_count = LMS_GetNumChannels(device, LMS_CH_TX);
    // printf("Tx channel count %d" "\n", channel_count);

    printf("Using channel %d" "\n", channel);

    int antenna_count = LMS_GetAntennaList(device, LMS_CH_TX, channel, NULL);
    // printf("TX%d Channel has %d antenna(ae)" "\n", channel, antenna_count);
    lms_name_t antenna_name[4];
    if (antenna_count > 0){
        int i = 0;
        lms_range_t antenna_bw[4];
        LMS_GetAntennaList(device, LMS_CH_TX, channel, antenna_name);
        for (i = 0; i < antenna_count; i++){
            LMS_GetAntennaBW(device, LMS_CH_TX, channel, i, antenna_bw + i);
            // printf("Channel %d, antenna [%s] has BW [%lf .. %lf] (step %lf)" "\n", channel, antenna_name[i], antenna_bw[i].min, antenna_bw[i].max, antenna_bw[i].step);
        }
    }
    if (antenna < 0){
        antenna = DEFAULT_ANTENNA;
    }
    if (antenna >= antenna_count){
        antenna = DEFAULT_ANTENNA;
    }
    // LMS_SetAntenna(device, LMS_CH_TX, channel, antenna); // SetLOFrequency should take care of selecting the proper antenna

    LMS_SetNormalizedGain(device, LMS_CH_TX, channel, gain);

    // Disable all other channels
    LMS_EnableChannel(device, LMS_CH_TX, 1 - channel, false);
    LMS_EnableChannel(device, LMS_CH_RX, 0, false);
    LMS_EnableChannel(device, LMS_CH_RX, 1, false);
    // Enable our Tx channel
    LMS_EnableChannel(device, LMS_CH_TX, channel, true);

    int setLOFrequency = LMS_SetLOFrequency(device, LMS_CH_TX, channel, TX_FREQUENCY);
    if (setLOFrequency){
        printf("setLOFrequency(%lf)=%d(%s)" "\n", TX_FREQUENCY, setLOFrequency, LMS_GetLastErrorMessage());
    }

#ifdef __USE_LPF__
    lms_range_t LPFBWRange;
	LMS_GetLPFBWRange(device, LMS_CH_TX, &LPFBWRange);
	// printf("TX%d LPFBW [%lf .. %lf] (step %lf)" "\n", channel, LPFBWRange.min, LPFBWRange.max, LPFBWRange.step);
	double LPFBW = TX_BANDWIDTH;
	if (LPFBW < LPFBWRange.min){
		LPFBW = LPFBWRange.min;
	}
	if (LPFBW > LPFBWRange.max){
		LPFBW = LPFBWRange.min;
	}
	int setLPFBW = LMS_SetLPFBW(device, LMS_CH_TX, channel, LPFBW);
	if (setLPFBW){
		printf("setLPFBW(%lf)=%d(%s)" "\n", LPFBW, setLPFBW, LMS_GetLastErrorMessage());
	}
	int enableLPF = LMS_SetLPF(device, LMS_CH_TX, channel, true);
	if (enableLPF){
		printf("enableLPF=%d(%s)" "\n", enableLPF, LMS_GetLastErrorMessage());
	}
#endif


    lms_range_t sampleRateRange;
    int getSampleRateRange = LMS_GetSampleRateRange(device, LMS_CH_TX, &sampleRateRange);
    if (getSampleRateRange){
        LOGD("getSampleRateRange=%d(%s)" "\n", getSampleRateRange, LMS_GetLastErrorMessage());
    }
    else{
        // printf("sampleRateRange [%lf MHz.. %lf MHz] (step=%lf Hz)" "\n", sampleRateRange.min / 1e6, sampleRateRange.max / 1e6, sampleRateRange.step);
    }

    printf("Set sample rate to %lf ..." "\n", sampleRate);
    int setSampleRate = LMS_SetSampleRate(device, sampleRate, 0);
    if (setSampleRate){
        LOGD("setSampleRate=%d(%s)" "\n", setSampleRate, LMS_GetLastErrorMessage());
    }
    double actualHostSampleRate = 0.0;
    double actualRFSampleRate = 0.0;
    int getSampleRate = LMS_GetSampleRate(device, LMS_CH_TX, channel, &actualHostSampleRate, &actualRFSampleRate);
    if (getSampleRate){
        LOGD("getSampleRate=%d(%s)" "\n", getSampleRate, LMS_GetLastErrorMessage());
    }
    else{
        LOGD("actualRate %lf (Host) / %lf (RF)" "\n", actualHostSampleRate, actualRFSampleRate);
    }

    LOGD("Calibrating ..." "\n");
    int calibrate = LMS_Calibrate(device, LMS_CH_TX, channel, TX_BANDWIDTH, 0);
    if (calibrate){
        LOGD("calibrate=%d(%s)" "\n", calibrate, LMS_GetLastErrorMessage());
    }

    LOGD("Setup TX stream ..." "\n");

    int setupStream = LMS_SetupStream(device, &tx_stream);
    if (setupStream){
        printf("setupStream=%d(%s)" "\n", setupStream, LMS_GetLastErrorMessage());
    }

}

int limeMain()
{
    sim_t s;

    int result;
    double duration;
    datetime_t t0;

    init_sim(&s);

    // Allocate TX buffer to hold each block of samples to transmit.
    s.tx.buffer = (int16_t *)malloc(SAMPLES_PER_BUFFER * sizeof(int16_t) * 2); // for 16-bit I and Q samples

    if (s.tx.buffer == NULL) {
        LOGE( "Failed to allocate TX buffer.\n");
        goto out;
    }

    // Allocate FIFOs to hold 0.1 seconds of I/Q samples each.
    s.fifo = (int16_t *)malloc(FIFO_LENGTH * sizeof(int16_t) * 2); // for 16-bit I and Q samples

    if (s.fifo == NULL) {
        LOGE( "Failed to allocate I/Q sample buffer.\n");
        goto out;
    }

    // Start GPS task.
    s.status = start_gps_task(&s);

    // Wait until GPS task is initialized
    pthread_mutex_lock(&(s.tx.lock));
    while (!s.gps.ready)
        pthread_cond_wait(&(s.gps.initialization_done), &(s.tx.lock));
    pthread_mutex_unlock(&(s.tx.lock));

    // Fillfull the FIFO.
    if (is_fifo_write_ready(&s))
        pthread_cond_signal(&(s.fifo_write_ready));

    // Start TX task
    s.status = start_tx_task(&s);
    if (s.status < 0) {
        LOGE( "Failed to start TX task.\n");
        goto out;
    }
    else
        LOGD("Creating TX task...\n");

    // Running...
    LOGD("Running...\n");

    // Wainting for TX task to complete.
    pthread_join(s.tx.thread, NULL);
    LOGD("\nDone!\n");

    out:
    // Free up resources
    if (s.tx.buffer != NULL)
        free(s.tx.buffer);

    if (s.fifo != NULL)
        free(s.fifo);

    LOGD("Closing device...\n");
    if(s.tx.dev!=NULL){
        LMS_Close(s.tx.dev);
        LOGD("Closing device succesful");
    }
    return 0;
}

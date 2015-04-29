// ChE 4240-101 Team 6 Capstone Lab
// A visual display of a concentric tube heat exchanger
// 3/31/15
// Jeff Glancy

#ifndef __CAPSTONE_H__
#define __CAPSTONE_H__

//maximum socket server pthreads open 
#define MAX_CONN 15

//socket server protocol
#define VAR_LEN 20
#define VAL_LEN 10
#define STR_LEN 29
#define VARVALSTR VAR_LEN+VAL_LEN+STR_LEN+1

//lockfile
#define LOCKFILE "/var/run/capstone.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define PROGRAMNAME "Capstone"

//structure for temperature read pthreads
struct _TempThread {
	pthread_t t_ID;
	int label;
};

//structure for socket connection pthreads
struct _socketConn {
	pthread_t t_ID;
	int socket_fd;
};

//function to close program when SIGTERM caught
void sigterm(int signum);
//function to read conf file when SIGHUP caught
void sighup(int signum);
//function to read/write lock file
int already_running();
//function to lock lock file
int lockfile(int fd);
//function to read configuration file
void readConf();

// Update temperature display as RGB on LED strips
void *updateLed(void *arg);
// Read in DS18B20 temps and store
void *readTemp(void *arg);

//function to read 1-wire sensor 
int DSRead(const char *ThermID, float *temp, int label, float offset);

//Input a temperature to get a color value.
//The colors are a transition r(hot) g(med) b(cold)
uint32_t colorTemp(float temp, float cenTemp, float resRGB);

//socket server thread
void *socketServer (void *arg);
// Read variable/value from the socket and returns associated variable/value/string. 
void *socketRead(void *arg);

#endif
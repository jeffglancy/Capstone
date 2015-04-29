// ChE 4240-101 Team 6 Capstone Lab
// A visual display of a concentric tube heat exchanger
// 3/31/15
// Author: Jeff Glancy
// Team members: Jeremy Douglas, Matthew Nelson, Mohammed Alqarni, Sulaiman Alhumyyed 
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <bcm2835.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "capstone.h"
#include "LPD8806.h"

//socket locations
const char capstone_socket_name[] = "/tmp/capstone";

// Number of RGB LEDs in strand:
int nLEDs = 64; //32 per meter
const float maxRGB = 33.0;	//max integer rgb string accepts
const float minRGB = 1.0;	//min integer rgb string accepts

#define NO_OF_TEMPS 10
//thermometer file locations -DS18B20 IDs  -"/sys/bus/w1/devices/" + ID + "/w1_slave"
const char Therm_ID[NO_OF_TEMPS+1][45] = {"/sys/bus/w1/devices//w1_slave",
						"/sys/bus/w1/devices/28-0000068f1191/w1_slave",		// 1
						"/sys/bus/w1/devices/28-0000068f1463/w1_slave",		// 2
						"/sys/bus/w1/devices/28-00000690e19e/w1_slave", 	// 3
						"/sys/bus/w1/devices/28-0000069169e1/w1_slave",		// 4
						"/sys/bus/w1/devices/28-0000068abeb5/w1_slave", 	// 5
						"/sys/bus/w1/devices/28-00000691429a/w1_slave", 	// 6
						"/sys/bus/w1/devices/28-000006916bb0/w1_slave", 	// 7
						"/sys/bus/w1/devices/28-0000068f3916/w1_slave", 	// 8
						"/sys/bus/w1/devices/28-0000068f865e/w1_slave", 	// 9
						"/sys/bus/w1/devices/28-0000068ff3fa/w1_slave"};	// 10

//refresh timings - milliseconds
const int temps_refresh = 800; 
//nearly 10 sec just to scan 10 sensors in order
//at 0, simultaneous reads are about every 1 sec, and 30% CPU use
//at 1 sec, simultaneous reads are about every 2 sec, and 15% CPU use
const int strip_refresh = 1500;

//sensor data structure
#define MAX_TEMPS_INDEX 5	//number of temps to average
struct {
	pthread_mutex_t data_lock;
	float avg_temp[NO_OF_TEMPS+1];
//	float temp[NO_OF_TEMPS+1][MAX_TEMPS_INDEX];
	} temps_data;
	
const float temp_offset[NO_OF_TEMPS+1] = {0.0,
						0.00,	// 1
						-0.01,	// 2
						-0.12, 	// 3
						-0.02,	// 4
						-0.03, 	// 5
						0.08, 	// 6
						0.16, 	// 7
						-0.19, 	// 8
						-0.15, 	// 9
						0.15};	// 10

	
//temperature weights along led strip
float weight1[] = {1.0,(6.0/7.0),(5.0/7.0),(4.0/7.0),(3.0/7.0),(2.0/7.0),(1.0/7.0),0.0,
				0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
				0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
				0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
float weight2[] = {0.0,(1.0/7.0),(2.0/7.0),(3.0/7.0),(4.0/7.0),(5.0/7.0),(6.0/7.0),1.0,
				1.0,(6.0/7.0),(5.0/7.0),(4.0/7.0),(3.0/7.0),(2.0/7.0),(1.0/7.0),0.0,
				0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
				0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
float weight3[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
				0.0,(1.0/7.0),(2.0/7.0),(3.0/7.0),(4.0/7.0),(5.0/7.0),(6.0/7.0),1.0,
				1.0,(6.0/7.0),(5.0/7.0),(4.0/7.0),(3.0/7.0),(2.0/7.0),(1.0/7.0),0.0,
				0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
float weight4[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
				0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
				0.0,(1.0/7.0),(2.0/7.0),(3.0/7.0),(4.0/7.0),(5.0/7.0),(6.0/7.0),1.0,
				1.0,(6.0/7.0),(5.0/7.0),(4.0/7.0),(3.0/7.0),(2.0/7.0),(1.0/7.0),0.0};
float weight5[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
				0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
				0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
				0.0,(1.0/7.0),(2.0/7.0),(3.0/7.0),(4.0/7.0),(5.0/7.0),(6.0/7.0),1.0};

//Thread IDs
pthread_t t_UpdateLed, t_Socket;

int main(int argc, char ** argv)
{
	//start ourselves as a daemon...
	pid_t pid, sid;
	
	// Clone ourselves to make a child
	pid = fork(); 

	// If the pid is less than zero, something went wrong when forking
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	// If the pid we got back was greater than zero, then the clone was successful and we are the parent.
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	// If execution reaches this point we are the child 
	// Set the umask to zero 
	umask(0);

	// Open a connection to the syslog server 
	openlog(PROGRAMNAME,LOG_PID,LOG_USER); 

	// Sends a message to the syslog daemon 
	syslog(LOG_NOTICE, "Program Started\n"); 

	// Try to create our own process group
	sid = setsid();
	if (sid < 0) {
		syslog(LOG_ERR, "Could not create process group\n");
		exit(EXIT_FAILURE);
	}

	// Change the current working directory
	if ((chdir("/")) < 0) {
		syslog(LOG_ERR, "Could not change working directory to /\n");
		exit(EXIT_FAILURE);
	}

	// Close the standard file descriptors
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);	
	
	//make sure we're only running a single instance
	if (already_running())
	{
		syslog(LOG_ERR, "Daemon already running\n");
		exit(EXIT_FAILURE);
	}

	
	//register functions to catch interrupt signals
	struct sigaction sa;
    sa.sa_handler = sigterm;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGHUP);
	sa.sa_flags = 0;
	if (sigaction(SIGTERM, &sa, NULL) < 0)
	{
		syslog(LOG_ERR, "Error setting SIGTERM handler\n");
		exit(EXIT_FAILURE);
	}
    sa.sa_handler = sighup;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGTERM);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		syslog(LOG_ERR, "Error setting SIGHUP handler\n");


	//initialize LED string
	LPD8806init(nLEDs); // Start up the LED strip
	LPD8806show(); // Update the strip, to start they are all 'off'
	
	//initialize temps data mutex
	int ret;
	ret = pthread_mutex_init(&temps_data.data_lock, NULL);
	if (ret != 0)
		fprintf(stderr, "Error setting temps data mutex. Err: %d\n", ret);

	//fill average temp array
	int i;
	pthread_mutex_lock(&temps_data.data_lock);
		for (i = 1; i <= NO_OF_TEMPS; i++)
			temps_data.avg_temp[i] = 0.0;
	pthread_mutex_unlock(&temps_data.data_lock); 
	
	//start read temp threads
	struct _TempThread temp_thread[NO_OF_TEMPS+1];
	for (i = 1; i <= NO_OF_TEMPS; i++)
	{
		temp_thread[i].label = i;
		ret = pthread_create(&temp_thread[i].t_ID, NULL, &readTemp, &temp_thread[i]);
		if (ret != 0)
			syslog(LOG_ERR, "Error starting read temp thread #%d. Err: %d\n", i, ret);
	}

	delay(2500);	//wait for temperature data to be populated

	//start socket server thread
	ret = pthread_create(&t_Socket,NULL,&socketServer,"data");
	if (ret != 0)
		syslog(LOG_ERR, "Error starting socket server thread. Err: %d\n", ret);
	
	//start Update LED thread 
	ret = pthread_create(&t_UpdateLed,NULL,&updateLed,"data");
	if (ret != 0)
		syslog(LOG_ERR, "Error starting Update LED thread. Err: %d\n", ret);

	

	//wait for a system interrupt to shut down
	
	//wait on threads to close
	pthread_join(t_UpdateLed, NULL);
	pthread_join(t_Socket, NULL);
	
	//close all read temp threads
	for (i = 1; i <= NO_OF_TEMPS; i++)
	{
		ret = pthread_cancel(temp_thread[i].t_ID);
		if (ret != 0)
			syslog(LOG_ERR, "Error canceling read temp thread #%d. Err: %d\n", i, ret);
	}
	//wait on all read temp threads
	for (i = 1; i <= NO_OF_TEMPS; i++)
	{
		pthread_join(temp_thread[i].t_ID, NULL);
	}

	//shut off
	LPD8806quit();
	syslog(LOG_INFO,"Program Closed\n");
	closelog();

	return 0;
}

//function to close program when SIGTERM caught
void sigterm(int signum)
{
	//SIGTERM caught, close down
	if (signum == SIGTERM)
	{
		syslog(LOG_INFO, "SIGTERM received, shutting down");
		//cancel threads
		int ret;

		ret = pthread_cancel(t_UpdateLed);
		if (ret != 0)
			syslog(LOG_ERR, "Error canceling Update LED thread. Err: %d\n", ret);

		ret = pthread_cancel(t_Socket);
		if (ret != 0)
			syslog(LOG_ERR, "Error canceling socket server thread. Err: %d\n", ret);

	}
	return;
}

//function to read conf file when SIGHUP caught
void sighup(int signum)
{
	if (signum == SIGHUP)
	{
		readConf();
	}
	return;
}

//function to read/write lock file
int already_running()
{
    int     fd;
    char    buf[16];

    fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
    if (fd < 0) {
        syslog(LOG_ERR, "can't open %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            return(1);
        }
        syslog(LOG_ERR, "can't lock %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf)+1);
    return(0);
}

//function to lock lock file
int lockfile(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return(fcntl(fd, F_SETLK, &fl));
}

//function to read configuration file
void readConf()
{
//	syslog(LOG_DEBUG, "Read in config file successfully");
	return;
}


// Update temperature display as RGB on LED strips
void *updateLed(void *arg)
{
	float temp[NO_OF_TEMPS+1];
	float hiTemp, midTemp, loTemp;
	int i;
	float resRGB;
	float weightedTemp;

	syslog(LOG_DEBUG,"Started Update LED thread\n");

	//continuously update LED strand with temperature color
	while (1)
	{
		// read in averaged temps
		pthread_mutex_lock(&temps_data.data_lock);
			for(i = 1; i <= NO_OF_TEMPS; i++)
				temp[i] = temps_data.avg_temp[i];
		pthread_mutex_unlock(&temps_data.data_lock);
		
//syslog(LOG_DEBUG, "%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f", temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7], temp[8], temp[9], temp[10]);
		
		//find high and low temps
		hiTemp = temp[1];
		loTemp = temp[1];
		for(i = 2; i <= NO_OF_TEMPS; i++)
		{
			if (temp[i] > hiTemp)
				hiTemp = temp[i];
			if (temp[i] < loTemp)
				loTemp = temp[i];
		}
		
		//calculate average temp and resolution from the max and min temperatures:
		midTemp = (hiTemp + loTemp) / 2.0;
		if ((hiTemp - loTemp) < 5.0)
			resRGB = (2.0 * (maxRGB - minRGB)) / 5.0;
		else
			resRGB = (2.0 * (maxRGB - minRGB)) / (hiTemp - loTemp);


		//Set color temps along inner LED strip
		for (i = 0; i < 32; i++)
		{
			//weight average temperatures along strip
			weightedTemp = (temp[5] * weight1[i]) + (temp[4] * weight2[i]) + (temp[3] * weight3[i]) + (temp[2] * weight4[i]) + (temp[1] * weight5[i]);
			LPD8806setPixelColor2(i, colorTemp(weightedTemp,midTemp,resRGB));
		}

		//Set color temps along outer LED strip
		for (i = 32; i < 64; i++)
		{
			//weight average temperatures along strip
			weightedTemp = (temp[6] * weight1[i-32]) + (temp[7] * weight2[i-32]) + (temp[8] * weight3[i-32]) + (temp[9] * weight4[i-32]) + (temp[10] * weight5[i-32]);
			LPD8806setPixelColor2(i, colorTemp(weightedTemp,midTemp,resRGB));
		}

		//Refresh LED strip
		LPD8806show();

		//check to see if kill command set
		pthread_testcancel;

		delay(strip_refresh);
	} // end while(1)

} //end updateLed()

// Read in DS18B20 temps and store
void *readTemp(void *arg)
{
	struct _TempThread *temp_thread = arg;
	int label = temp_thread->label;

	float avg_temp = 0.0;
	float temp[MAX_TEMPS_INDEX];
	int index =0;
	int i;

	syslog(LOG_DEBUG,"Started Read Temp %d thread\n", label);
	
	//initialize temp variables
	DSRead(Therm_ID[label],&avg_temp,label,temp_offset[label]);
	for (i = 0; i < MAX_TEMPS_INDEX; i++)
		temp[i] = avg_temp;

	//continuously update LED strand with temperature color
	while (1)
	{
		//read temp into array for averaging
		DSRead(Therm_ID[label],&temp[index],label,temp_offset[label]);

		//increment index
		if (++index >= MAX_TEMPS_INDEX)
			index = 0;

		//average temp
		avg_temp = 0;
		for (i = 0; i < MAX_TEMPS_INDEX; i++)
			avg_temp += temp[i];
		avg_temp /= MAX_TEMPS_INDEX;
		
		// store in global var
		pthread_mutex_lock(&temps_data.data_lock);
			temps_data.avg_temp[label] = avg_temp;
		pthread_mutex_unlock(&temps_data.data_lock);
		
		//save to next index for next temperature read
		temp[index] = avg_temp;

//syslog(LOG_DEBUG, "Temp %d: %.2f", label, avg_temp);

		//check to see if kill command set
		pthread_testcancel;

		delay(temps_refresh);

	} // end while(1)

} //end readTemp()


//function to read 1-wire sensor 
int DSRead(const char *ThermID, float *temp, int label, float offset)
{
	//error numbers for temperature sensors
	int TempErr1 = -62;
	int TempErr2 = -125;

	FILE *fp;
	long  lFileLen = 80;          
	char cFile[lFileLen];                  
	char *cThisPtr;               /* Pointer to current position in cFile */
	
	int temperaturedata;
	float temperature;

	fp = fopen(ThermID, "rt");
	if (fp == NULL) {
		syslog(LOG_ERR, "Can't open 1-wire file %d to read.\n", label);
		//don't change temp
		return -1;
	}

	fgets(cFile, lFileLen, fp); /* Read the line into cFile */
//syslog(LOG_DEBUG, "Line one: %s\n", cFile);

	cThisPtr = strstr(cFile, "NO");            
	if (!cThisPtr == '\0')
	{
		syslog(LOG_ERR, "Temp sensor %d -Wrong CRC\n", label);
		//don't change temp
		fclose(fp);
		return -1;
	}
	
	fgets(cFile, lFileLen, fp); /* Read the line into cFile */
//syslog(LOG_DEBUG, "Line two: %s\n", cFile);
	
	cThisPtr = strstr(cFile, "t=");
	//found temperature, move to it
	cThisPtr += 2;
	//capture as int first so that comparisons to error work
	temperaturedata = atoi(cThisPtr);

	//close file
	fclose(fp);	

	if ((temperaturedata != TempErr1) && (temperaturedata != TempErr2))
	{
		temperature = (((float)(temperaturedata)) * 1.8 / 1000.0) + 32.0;
	}
	else
	{
		syslog(LOG_ERR, "Error on Temp sensor %d\n", label);
		//don't change temp
		return -1;
	}

	*temp = temperature + offset;

	return 0;
}

//Input a temperature to get a color value.
//The colors are a transition r(hot) g(med) b(cold)
uint32_t colorTemp(float temp, float cenTemp, float resRGB)
{
	int r, g, b;
  
	// red calc
	r = (int)((temp - cenTemp) * resRGB);
	if (r < minRGB)
		r = minRGB;
	else if (r > maxRGB)
		r = maxRGB;
	
	//green calc
	if (temp >= cenTemp)
		g = (int)(maxRGB - ((temp - cenTemp) * resRGB));
	else
		g = (int)(maxRGB - ((cenTemp - temp) * resRGB));
	if (g < minRGB)
		g = minRGB;
	else if (g > maxRGB)
		g = maxRGB;

	//blue calc
	b = (int)((cenTemp - temp) * resRGB * 0.5);	// 0.5 -blue was too intense, overwhelmed green
	if (b < minRGB)
		b = minRGB;
	else if (b > maxRGB)
		b = maxRGB;

  return(LPD8806Color(r,g,b));
}


//socket server thread
void *socketServer (void *arg)
{
	//set thread cancellation state
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	int socket_fd;
	struct sockaddr_un name;
	int client_sent_quit_message;
	
	//Create the socket
	socket_fd = socket (AF_LOCAL, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		syslog(LOG_ERR, "Socket Create Error\n");
	}

	//remove old socket file
	unlink (capstone_socket_name);
//	remove(capstone_socket_name);

	//bind server socket
	memset(&name, 0, sizeof(struct sockaddr_un));
	name.sun_family = AF_LOCAL;
	snprintf(name.sun_path, 108, "%s", capstone_socket_name); // UNIX_PATH_MAX = 108
	if (bind (socket_fd, (struct sockaddr *)&name, SUN_LEN (&name)) != 0)
	{
		syslog(LOG_ERR, "Socket Bind Error\n");
		//would need to delete socket file and try again
	}
	//change socket permissions
	chmod(capstone_socket_name, S_IROTH | S_IWOTH);
	
	// Listen for MAX connections
	if (listen(socket_fd, MAX_CONN) != 0)
	{
		syslog(LOG_ERR, "Socket Listen Error\n");
	}
	
	syslog(LOG_INFO, "Started socket server thread.\n");
	
	struct _socketConn conns[MAX_CONN];
	int i = 0;
	int ret;
	int firstTime = 1;
	struct sockaddr_un client_name; 
	socklen_t client_name_len = sizeof( (struct sockaddr *) &client_name);
	
	// Repeatedly accept connections, spinning off one pthread to deal with each client.
	while (1)
	{
		if (!firstTime)
		{
			//verify conn isn't busy
			ret = pthread_join(conns[i].t_ID, NULL);
			if (ret != 0)
				syslog(LOG_ERR, "Error joining thread #%d. Err: %d\n", i, ret);
		}
		// Accept a connection.
		conns[i].socket_fd = accept(socket_fd, (struct sockaddr *)&client_name, &client_name_len); 
		if (conns[i].socket_fd < 0)
			syslog(LOG_ERR, "Error accepting conn #%d, Err: %d\n", conns[i].socket_fd, errno);
		// Handle the connection. 
		ret = pthread_create(&conns[i].t_ID, NULL, &socketRead, &conns[i]);
		if (ret != 0)
			syslog(LOG_ERR, "Error starting socket thread #%d. Err: %d\n", i, ret);
//syslog(LOG_DEBUG, "Accepted conn #%d\n", i);
		if (++i >= MAX_CONN)
		{
			i = 0;
			firstTime = 0;
		}
		//check to see if kill command set
		pthread_testcancel;
    }
	
	syslog(LOG_ERR, "Socket thread closed.");
	
	// Remove the socket file
	close (socket_fd);
	unlink (capstone_socket_name);

}

// Read variable/value from the socket and returns associated variable/value/string. 
void *socketRead(void *arg)
{
	struct _socketConn *conn = arg;
	int client_socket = conn->socket_fd;
	
	//set thread cancellation state
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	
	char buffer[VARVALSTR + 1];
	char var[VAR_LEN + 1];
	char val[VAL_LEN + 1];
	char str[STR_LEN + 1];
	int len, ret;
	
	//check socket status
	if (client_socket <= 0)
	{
		syslog(LOG_DEBUG, "Socket connection error, conn %d", client_socket);
		close(client_socket);
		return 0;
	}

	//read in variable/value/string
	len = 0;
	while (len < VARVALSTR)
	{
		//reread until we reach length
		ret = recv(client_socket, buffer+len, sizeof(buffer)-1-len, 0);
		if (ret <= 0)
		{
			syslog(LOG_DEBUG, "Socket receive error: %d", ret);
			close(client_socket);
			return 0;
		}
		len += ret;
	}
	buffer[len] = '\0';
	//trim variable/value returned
	snprintf(var, sizeof(var), "%s", buffer);
	snprintf(val, sizeof(val), "%s", buffer + VAR_LEN);
//syslog(LOG_DEBUG, "Read in %d bytes:\n%s", ret, buffer);
//syslog(LOG_DEBUG, "Var:Val\n%s:%s\n", var, val);


	//compare variable sent (must be 20 char long), return value to client

	//Temperature sensor data
	if (!strcmp(var, "GetTemp1            "))
	{
		pthread_mutex_lock(&temps_data.data_lock);
			snprintf(val, (VAL_LEN+1), "%.02f", temps_data.avg_temp[1]);
			snprintf(str, (STR_LEN+1), "%.02f &deg;F", temps_data.avg_temp[1]);
		pthread_mutex_unlock(&temps_data.data_lock);
	}
	else if (!strcmp(var, "GetTemp2            "))
	{
		pthread_mutex_lock(&temps_data.data_lock);
			snprintf(val, (VAL_LEN+1), "%.02f", temps_data.avg_temp[2]);
			snprintf(str, (STR_LEN+1), "%.02f &deg;F", temps_data.avg_temp[2]);
		pthread_mutex_unlock(&temps_data.data_lock);
	}
	else if (!strcmp(var, "GetTemp3            "))
	{
		pthread_mutex_lock(&temps_data.data_lock);
			snprintf(val, (VAL_LEN+1), "%.02f", temps_data.avg_temp[3]);
			snprintf(str, (STR_LEN+1), "%.02f &deg;F", temps_data.avg_temp[3]);
		pthread_mutex_unlock(&temps_data.data_lock);
	}
	else if (!strcmp(var, "GetTemp4            "))
	{
		pthread_mutex_lock(&temps_data.data_lock);
			snprintf(val, (VAL_LEN+1), "%.02f", temps_data.avg_temp[4]);
			snprintf(str, (STR_LEN+1), "%.02f &deg;F", temps_data.avg_temp[4]);
		pthread_mutex_unlock(&temps_data.data_lock);
	}
	else if (!strcmp(var, "GetTemp5            "))
	{
		pthread_mutex_lock(&temps_data.data_lock);
			snprintf(val, (VAL_LEN+1), "%.02f", temps_data.avg_temp[5]);
			snprintf(str, (STR_LEN+1), "%.02f &deg;F", temps_data.avg_temp[5]);
		pthread_mutex_unlock(&temps_data.data_lock);
	}
	else if (!strcmp(var, "GetTemp6            "))
	{
		pthread_mutex_lock(&temps_data.data_lock);
			snprintf(val, (VAL_LEN+1), "%.02f", temps_data.avg_temp[6]);
			snprintf(str, (STR_LEN+1), "%.02f &deg;F", temps_data.avg_temp[6]);
		pthread_mutex_unlock(&temps_data.data_lock);
	}
	else if (!strcmp(var, "GetTemp7            "))
	{
		pthread_mutex_lock(&temps_data.data_lock);
			snprintf(val, (VAL_LEN+1), "%.02f", temps_data.avg_temp[7]);
			snprintf(str, (STR_LEN+1), "%.02f &deg;F", temps_data.avg_temp[7]);
		pthread_mutex_unlock(&temps_data.data_lock);
	}
	else if (!strcmp(var, "GetTemp8            "))
	{
		pthread_mutex_lock(&temps_data.data_lock);
			snprintf(val, (VAL_LEN+1), "%.02f", temps_data.avg_temp[8]);
			snprintf(str, (STR_LEN+1), "%.02f &deg;F", temps_data.avg_temp[8]);
		pthread_mutex_unlock(&temps_data.data_lock);
	}
	else if (!strcmp(var, "GetTemp9            "))
	{
		pthread_mutex_lock(&temps_data.data_lock);
			snprintf(val, (VAL_LEN+1), "%.02f", temps_data.avg_temp[9]);
			snprintf(str, (STR_LEN+1), "%.02f &deg;F", temps_data.avg_temp[9]);
		pthread_mutex_unlock(&temps_data.data_lock);
	}
	else if (!strcmp(var, "GetTemp10           "))
	{
		pthread_mutex_lock(&temps_data.data_lock);
			snprintf(val, (VAL_LEN+1), "%.02f", temps_data.avg_temp[10]);
			snprintf(str, (STR_LEN+1), "%.02f &deg;F", temps_data.avg_temp[10]);
		pthread_mutex_unlock(&temps_data.data_lock);
	}

	else
	{
		snprintf(val, (VAL_LEN+1), "-1");
		snprintf(str, (STR_LEN+1), "No Such Variable");
	}


	//create return variable/value/string
	len = snprintf(buffer, sizeof(buffer), "%-*s%-*s%-*s\n", VAR_LEN, var, VAL_LEN, val, STR_LEN, str);
//syslog(LOG_DEBUG, "Writing %d bytes:\n%s", len, buffer);
	while (len > 0)
	{
		//write until len has been reached
		ret = send(client_socket, buffer+strlen(buffer)-len, len, MSG_NOSIGNAL);
		if (ret <= 0)
		{
			syslog(LOG_DEBUG, "Socket send error: %d", ret);
			close(client_socket);
			return 0;
		}
		len -= ret;
	}

	close(client_socket);
	
	return 0;
}

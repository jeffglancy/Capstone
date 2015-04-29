#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define VAR_LEN 20
#define VAL_LEN 10
#define STR_LEN 29
#define VARVALSTR VAR_LEN+VAL_LEN+STR_LEN+1

const char* const wx_socket_name = "/tmp/capstone";

int main (int argc, char* const argv[])
{
	char buffer[VARVALSTR+1];
	char var[VAR_LEN+1];
	char val[VAL_LEN+1];
	char str[STR_LEN+1];
//	float numVal;
	int ret, len;
	
	if (argc > 2)
	{
		//trim down inputs
		snprintf(var, sizeof(var), "%s", argv[1]);
		snprintf(val, sizeof(val), "%s", argv[2]);
	}
	else if (argc == 2)
	{
		//trim down inputs
		snprintf(var, sizeof(var), "%s", argv[1]);
		snprintf(val, sizeof(val), "0");
	}
	else
	{
		printf("Enter variable:\n");
		scanf("%s", var);
		printf("Enter value:\n");
		scanf("%s", val);
	}
	
	int socket_fd;
	struct sockaddr_un name;
	// Create the socket.
	socket_fd = socket (AF_LOCAL, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		printf("Socket create failed\n");
		return -1;
	}
	// Store the server’s name in the socket address.
	memset(&name, 0, sizeof(struct sockaddr_un));
	name.sun_family = AF_LOCAL;
	snprintf(name.sun_path, 108, "%s", wx_socket_name); // UNIX_PATH_MAX = 108
	// Connect the socket.
	if (connect(socket_fd, (struct sockaddr *)&name, SUN_LEN (&name)) != 0)
	{
		printf("Socket connect failed\n");
		close(socket_fd);
		return -1;
	}

	// Write the variable/value/string to the socket.
	len = snprintf(buffer, sizeof(buffer), "%-*s%-*s%-*s\n", VAR_LEN, var, VAL_LEN, val, STR_LEN, " ");
//printf("Writing %d bytes:\n%s", len, buffer);
	while (len > 0)
	{
		//write until len has been reached
		ret = send(socket_fd, buffer+strlen(buffer)-len, len, MSG_NOSIGNAL);
		if (ret <= 0)
		{
			printf("Socket send error: %d", ret);
			close(socket_fd);
			return -1;
		}
		len -= ret;
	}
	
	// Read in response to get variable/value/string
	len = 0;
	while (len < VARVALSTR)
	{
		//reread until we reach length
		ret = recv(socket_fd, buffer+len, sizeof(buffer)-1-len, 0);
		if (ret <= 0)
		{
			printf("Socket receive error: %d\n", ret);
			close(socket_fd);
			return -1;
		}
		len += ret;
	}
	buffer[len] = '\0';
//printf("Read %d bytes:\n%s", len, buffer);

	//trim variable/value/string returned
	snprintf(var, sizeof(var), "%s", buffer);
	snprintf(val, sizeof(val), "%s", buffer + VAR_LEN);
	snprintf(str, sizeof(str), "%s", buffer + VAR_LEN+ VAL_LEN);
	
	//convert value to number?
//	numVal = strtof(&buffer[VAR_LEN], NULL);

	printf("Variable:\t%s\n", var);
	printf("Value:   \t%s\n", val);
	printf("String:  \t%s\n", str);

	close (socket_fd);
	
	return 0;
}

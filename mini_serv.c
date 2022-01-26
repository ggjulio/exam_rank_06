#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void exit_fatal()
{
	const char* s = "Fatal error\n";
	write(2, s, strlen(s));
	exit(1);
}


int main(int ac, char **av)
{
	if (ac != 2)
	{
		const char* s = "Wrong number of arguments\n";
		write(2, s, strlen(s));
		exit(1);
	}

	int sockfd;
	struct sockaddr_in servaddr;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		exit_fatal();

	// assign IP, PORT
	bzero(&servaddr, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 
  
	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{
		close(sockfd);
		exit_fatal();
	}
	if (listen(sockfd, 10) != 0) // 10 is the value given in the main
		exit_fatal();

	{
		// int connfd;
		// socklen_t len;
		// struct sockaddr_in client;
		// len = sizeof(client);
		// connfd = accept(sockfd, (struct sockaddr *)&client, &len);
		// if (connfd < 0) {
		// 	// printf("server acccept failed...\n"); 
		// } 
		// else
			// printf("server acccept the client...\n");


		fd_set read_fds;
		FD_ZERO(&read_fds);

		while (1)
		{
			if (select(1024, &read_fds, 0, 0, 0))
			{
				printf("select : ");
				exit_fatal();
			}
			for (int i = 0; i < 10; ++i)
			{
				printf(">>%d\n", i);

			}			
			// printf("server\n");
		}
	}
}

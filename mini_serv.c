#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>

typedef struct  s_client
{
	int id;
	int fd;
	struct s_client *next;
} t_client;

int server_fd, g_id = 0;
t_client* clients = NULL;

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
	{
		strcat(newbuf, buf);
		free(buf); // there was an error here in the given main, must be free only if not null.
	}
	strcat(newbuf, add);
	return (newbuf);
}

void exit_fatal()
{
	const char* s = "Fatal error\n";
	write(2, s, strlen(s));
	exit(1);
}

int ft_send(int fd, char* buffer, int len)
{
	int total = 0;
	int left = len;

	while (total < len)
	{
		int n = send(fd, buffer + total, left, 0);
		if (n == -1)
		{
			printf("error send() in sendall\n");
			return -1;
		}
		total += n;
		left -= n;
	}
	return 0;
}
void send_all_clients(char *mess, fd_set *read_fds, int sender_fd)
{
	t_client* it = clients;
	while(it)
	{
		(void)sender_fd;
		// if (it->fd != sender_fd && FD_ISSET(it->fd, read_fds))
		if (FD_ISSET(it->fd, read_fds))
			ft_send(it->fd, mess, strlen(mess));
		it = it->next;
	}
}

t_client* malloc_client(int fd)
{
	t_client *result;

	result = malloc(sizeof(t_client));
	if (!result)
		exit_fatal();
	result->id = g_id++;
	result->fd = fd;
	result->next = NULL;
	return result;
}

void free_client(t_client *to_free)
{
	free(to_free);
}

t_client* add_client()
{
	int client_fd;
	struct sockaddr_in client;
	
	client_fd = accept(server_fd, (struct sockaddr *)&client, (socklen_t*)sizeof(client));
	if (client_fd < 0)
		exit_fatal();
	t_client* new_client = malloc_client(client_fd);
	if (!new_client)
		exit_fatal();
	t_client* it = clients;
	while(it->next)
		it = it->next;
	it->next = new_client;
	return new_client;
}


void run()
{
	char buffer[100];
	fd_set read_fds;
	FD_ZERO(&read_fds);

	while (1)
	{
		if (select(1024, &read_fds, 0, 0, 0) < 0)
			exit_fatal();
		printf("blabla\n");
		if (FD_ISSET(server_fd, &read_fds))
		{
			t_client *new_client = add_client();
			printf("blabla\n");
			sprintf(buffer, "server: client %d just arrived\n", new_client->id);
			send_all_clients(buffer, &read_fds, new_client->fd);
		}
		t_client *it = clients;
		while (it)
		{
			if (FD_ISSET(it->fd, &read_fds))
			{
				// mess = str_join(mess, )
			}
			it = it->next;
		}
		printf("server\n");
	}
}


int main(int ac, char **av)
{
	if (ac != 2)
	{
		const char* s = "Wrong number of arguments\n";
		write(2, s, strlen(s));
		exit(1);
	}

	struct sockaddr_in servaddr;

	// socket create and verification
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
		exit_fatal();

	// assign IP, PORT
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	// Binding newly created socket to given IP and verification
	if ((bind(server_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{
		close(server_fd);
		exit_fatal();
	}
	if (listen(server_fd, 10) != 0) // 10 is the value given in the main
		exit_fatal();
	run();
}

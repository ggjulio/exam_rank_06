#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
      free(buf); // error here, must only be freed when buf not null.
    }	
  strcat(newbuf, add);
  return (newbuf);
}

typedef struct s_client
{
  int id;
  int fd;
  char* data;
  struct s_client* next;
} t_client;

int server_fd, g_id = 0;
t_client* clients;

void exit_fatal()
{
  const char* err = "Fatal error\n";
  write(2, err, strlen(err));
  exit(1);
}

void broadcast_message(char *mess, fd_set *master_set, int sender_fd)
{
  t_client* it = clients;
  while (it)
    {
      if (it->fd != sender_fd && FD_ISSET(it->fd, master_set))
	{
	  int len = strlen(mess);
	  int n = send(it->fd, mess, len, 0);
	  if (n != len)
	    exit(42);
	}
      it = it->next;
    }
}

t_client* malloc_client(int fd)
{
  t_client* result = malloc(sizeof(t_client));

  if (!result)
    exit_fatal();
  result->id = g_id++;
  result->fd = fd;
  result->data = NULL;
  result->next = NULL;
  return result;
}

void free_client(t_client* to_free)
{
  if (to_free->data)
    free(to_free->data);
  free(to_free);
}

t_client* add_client()
{
  int client_fd = accept(server_fd, 0, 0);
  
  if (client_fd < 0)
    exit_fatal();
  t_client* new_client = malloc_client(client_fd);
  t_client* it = clients;
  if (it)
    {
      while (it->next)
	it = it->next;
      it->next = new_client;
    }
  else
    clients = new_client;
  return new_client;
}

t_client* remove_client(int id)
{
  t_client* tmp = 0;
  t_client* it = clients;
  while (it)
    {
      if (it->id == id)
	{
	  if (tmp)
	    tmp->next = it->next;
	  else
	    clients = it->next;
	  free_client(it);
	  return tmp ? tmp->next : clients;
	}
      tmp = it;
      it = it->next;
    }
  return NULL;
}

void run()
{
  char buffer[1000];
  fd_set master_set, read_set;
  FD_ZERO(&master_set);
  FD_SET(server_fd, &master_set);

  while(1)
    {
      FD_COPY(&master_set, &read_set);
      if (select(1024, &read_set, 0, 0, 0) < 0)
	exit_fatal();
      if (FD_ISSET(server_fd, &read_set))
	{
	  t_client* new_client = add_client();
	  sprintf(buffer, "server: client %d just arrived\n", new_client->id);
	  broadcast_message(buffer, &master_set, new_client->fd);
	  FD_SET(new_client->fd, &master_set);
	}
      t_client *it = clients;
      while (it)
	{
	  if (FD_ISSET(it->fd, &read_set))
	    {
	      int rc = recv(it->fd, buffer, sizeof(buffer)-1, 0);
	      if (rc <= 0)
		{
		  sprintf(buffer, "server: client %d just left\n", it->id);
		  broadcast_message(buffer, &master_set, it->fd);
		  close(it->fd);
		  FD_CLR(it->fd, &master_set);
		  it = remove_client(it->id);
		  continue;
		}
	      else
		{
		  buffer[rc] = 0;
		  it->data = str_join(it->data, buffer);
		  char *extracted = 0;
		  while (extract_message(&(it->data), &extracted))
		    {
		      char *to_send = malloc(100 + strlen(extracted));
		      if (!to_send)
			exit_fatal();
		      sprintf(to_send, "client %d: %s", it->id, extracted);
		      broadcast_message(to_send, &master_set, it->fd);
		      free(to_send);
		      free(extracted);
		      extracted = 0;
		    }
		    
		}
	    }

	  it = it->next;
	}
    }
}

int main(int ac, char **av) {
  if (ac != 2)
    {
      const char* err = "Wrong number of arguments\n";
      write(2,err, strlen(err));
      exit(1);
    }
  
  struct sockaddr_in servaddr; 
  
  // socket create and verification 
  server_fd = socket(AF_INET, SOCK_STREAM, 0); 
  if (server_fd == -1)
    exit_fatal();

  bzero(&servaddr, sizeof(servaddr)); 
  
  // assign IP, PORT 
  servaddr.sin_family = AF_INET; 
  servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
  servaddr.sin_port = htons(atoi(av[1])); 
  
  // Binding newly created socket to given IP and verification 
  if ((bind(server_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
    {
      close(server_fd);
      exit_fatal();
    }
  if (listen(server_fd, 10) != 0)
    {
      close(server_fd);
      exit_fatal();
    }
  run();
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kcharla <kcharla@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/10/28 22:49:53 by kcharla           #+#    #+#             */
/*   Updated: 2020/10/30 07:57:13 by kcharla          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <memory.h>
#include <poll.h>
#include <time.h>

#define RESPONSE_OK     0
#define RESPONSE_EXIT   1
#define RESPONSE_ERROR -1

// copy-paste from "https://jameshfisher.com/2017/04/05/set_socket_nonblocking/"
// but this is the first right thing after hours of googling so ill leave it as is
// sorry

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }

//int		client_disconnect()
//{
//
//}

int			server_process_client_line(char *cline, char **response);
int			msleep(long msec);

int main() {
	int listen_socket_fd = guard(socket(AF_INET, SOCK_STREAM, 0), "could not create TCP listening socket");
	int flags = guard(fcntl(listen_socket_fd, F_GETFL), "could not get flags on TCP listening socket");
	guard(fcntl(listen_socket_fd, F_SETFL, flags | O_NONBLOCK), "could not set TCP listening socket to be non-blocking");
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	guard(bind(listen_socket_fd, (struct sockaddr *) &addr, sizeof(addr)), "could not bind");
	guard(listen(listen_socket_fd, 1), "could not listen");

	int client_socket_fd = -1;
	int has_client = 0;
	char welcome_msg[] = "Server is ready. Type 'exit' to close connection.\n";
	char error_msg[] = "Some error caused server to close connection. Press 'Enter' to continue.\n";
	char busy_msg[] = "Server is currently used by another session/user. Try later.\n";
	char *client_str = NULL;
	char *client_line = NULL;
	char *server_line = NULL;
	char client_buff[256];
	int client_recv = 0;
	unsigned long client_str_size = 0;
	unsigned long client_str_old_size = 0;

	while (1)
	{
		//try to get new client
		//just a quick check, if no pending connections just continue the loop
		if (!has_client)
		{
			client_socket_fd = accept(listen_socket_fd, NULL, NULL);
			if (client_socket_fd == -1)
			{
				if (errno == EWOULDBLOCK)
				{
					printf("No pending connections right now.\n");
				}
				else
				{
					perror("error when accepting connection");
					exit(1);
				}
			}
			else
			{
				has_client = 1;
				send(client_socket_fd, welcome_msg, sizeof(welcome_msg), 0);
				printf("Got a connection; writing welcome message.\n");
			}
		}
		else
		{
			int refuse_fd = accept(listen_socket_fd, NULL, NULL);
			send(refuse_fd, busy_msg, sizeof(busy_msg), 0);
			close (refuse_fd);
		}

		// check if client socket has data and get it
		// non-blocking as well
		if (has_client)
		{
			printf("Getting client info...\n");
			struct pollfd pfd;
			pfd.fd = client_socket_fd;
			pfd.events = POLLIN | POLLHUP | POLLRDNORM;
			pfd.revents = 0;
			if (poll(&pfd, 1, 0) > 0) {
				client_recv = recv(client_socket_fd, client_buff, sizeof(client_buff) - 1, MSG_PEEK | MSG_DONTWAIT);
				if (client_recv == 0) {
					// poll() tells that socket changed
					// recv() tell there is no data on socket
					// that means that connection has been closed by client with no error

					has_client = 0;
					close(client_socket_fd);

					free(client_str);
					client_str = NULL;
					client_str_size = 0;
					client_str_old_size = 0;
				}
				else if (client_recv > 0)
				{
					// read all available data into string 'client_str'
					// mark that new data available
					// client_str_size = client_old_size + n;
					while ((client_recv = read(client_socket_fd, client_buff, sizeof(client_buff) - 1)) > 0)
					{
						client_buff[client_recv] = '\0';
						client_str_size += + client_recv;
//						if (client_str != NULL)
//							client_str_old_size += strlen(client_str);
						char *new_str;
						if ((new_str = (char *)malloc(client_str_size + 1)) == NULL)
						{
							send(client_socket_fd, error_msg, sizeof(error_msg), 0);
							close(client_socket_fd);
							printf("%s\n", "Server malloc() error. Connection is closed, shutting down...");
							exit(1);
						}
						*new_str = '\0';
						if (client_str != NULL)
							new_str = strcpy(new_str, client_str);
						new_str = strcat(new_str, client_buff);
						free(client_str);
						client_str = new_str;
					}
					if (client_recv < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
					{
						//error on read from fd
						//try to send error_msg and close connection
						send(client_socket_fd, error_msg, sizeof(error_msg), 0);
						printf("Errno is %d\n", errno);
						printf("%s\n", "Server read() error. Connection is closed, server is running...");

						has_client = 0;
						close(client_socket_fd);

						free(client_str);
						client_str = NULL;
						client_str_size = 0;
						client_str_old_size = 0;
					}
					printf("New str is '%s', size is %lu\n", client_str, client_str_size);
				}
				else
				{
					//recv() error occured
					if (errno == ECONNRESET)
					{
						//client failed
						//but we should stand
						has_client = 0;
						close(client_socket_fd);

						free(client_str);
						client_str = NULL;
						client_str_size = 0;
						client_str_old_size = 0;
					}
					else
					{
						// we failed
						close(client_socket_fd);
						printf("%s\n", "Server error on recv()");
						exit(0);
					}
				}
			}
		}

		// proceed data that changed
		// *** blocking part ***
		if (has_client && client_str_size > client_str_old_size)
		{
			int count = 0;
			unsigned long i = client_str_old_size;
//			unsigned long i = 0;
			unsigned long line_begin = 0;
			int recode = RESPONSE_OK;
			while (i < client_str_size)
			{
				if (client_str[i] == '\n')
				{
					//TODO deleteme
					count ++;
					printf("Serving newline number %d\n", count);


					client_line = strndup(client_str + line_begin, i - line_begin);
					line_begin = i + 1;

					recode = server_process_client_line(client_line, &server_line);

					free(client_line);
					client_line = NULL;

					if (recode == RESPONSE_ERROR || server_line == NULL)
					{
						// some serious error occured, must close
						send(client_socket_fd, error_msg, sizeof(error_msg), 0);
						close(client_socket_fd);
						printf("%s\n", "Server process line error. Connection is closed, shutting down...");
						exit(1);
					}

					printf("This newline is '%s'\n\n", server_line);
					send(client_socket_fd, server_line, strlen(server_line), 0);

					if (recode == RESPONSE_EXIT)
					{
						break ;
					}
				}
				i++;
			}

			if (recode == RESPONSE_EXIT)
			{
				has_client = 0;
				close(client_socket_fd);

				free(client_str);
				client_str = NULL;
				client_str_size = 0;
				client_str_old_size = 0;
			}
			else
			{
				// update client_str so it has no '\n' anymore
				char *tmp = strdup(client_str + line_begin);
				free(client_str);
				client_str = tmp;
				if (client_str == NULL){
					send(client_socket_fd, error_msg, sizeof(error_msg), 0);
					close(client_socket_fd);
					printf("%s\n", "Server malloc() error. Connection is closed, shutting down...");
					exit(1);
				}
				client_str_old_size = strlen(client_str);
				client_str_size = client_str_old_size;
				printf("Client str left is '%s'\n", client_str);
			}
		}
		msleep(100);
	}
	return EXIT_SUCCESS;
}

int			server_process_client_line(char *cline, char **response)
{
	int		recode;
	char	*res;

	recode = RESPONSE_OK;
	if (response == NULL)
		return (RESPONSE_ERROR);
	if (strcmp(cline, "exit") == 0)
	{
		res = strdup("Closing connection. Have a nice day :)\n");
		recode = RESPONSE_EXIT;
	}
	else
	{
//		printf("str '%s' and '%s' are not the same\n", "exit", cline);
		res = strdup(cline);
//		res = strdup("Unknown command. Try again.\n");
	}
	*response = res;
	return (recode);
}

/*
** msleep(): Sleep for the requested number of milliseconds.
** https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds
*/

int msleep(long msec)
{
	struct timespec ts;
	int res;

	if (msec < 0)
	{
		errno = EINVAL;
		return -1;
	}

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;

	do {
		res = nanosleep(&ts, &ts);
	} while (res && errno == EINTR);

	return res;
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kcharla <kcharla@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/10/28 22:49:53 by kcharla           #+#    #+#             */
/*   Updated: 2020/10/29 05:48:26 by hush             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>

// copy-paste from "https://jameshfisher.com/2017/04/05/set_socket_nonblocking/"
// but this is the first right thing after hours of googling so ill leave it as is
// sorry

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }

int main() {
	int listen_socket_fd = guard(socket(AF_INET, SOCK_STREAM, 0), "could not create TCP listening socket");
	int flags = guard(fcntl(listen_socket_fd, F_GETFL), "could not get flags on TCP listening socket");
	guard(fcntl(listen_socket_fd, F_SETFL, flags | O_NONBLOCK), "could not set TCP listening socket to be non-blocking");
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	guard(bind(listen_socket_fd, (struct sockaddr *) &addr, sizeof(addr)), "could not bind");
	guard(listen(listen_socket_fd, 100), "could not listen");
	for (;;) {
		int client_socket_fd = accept(listen_socket_fd, NULL, NULL);
		if (client_socket_fd == -1) {
			if (errno == EWOULDBLOCK) {
				printf("No pending connections; sleeping for one second.\n");
				sleep(1);
			} else {
				perror("error when accepting connection");
				exit(1);
			}
		} else {
			char msg[] = "hello\n";
			printf("Got a connection; writing 'hello' then closing.\n");
			send(client_socket_fd, msg, sizeof(msg), 0);
			close(client_socket_fd);
		}
	}
	return EXIT_SUCCESS;
}

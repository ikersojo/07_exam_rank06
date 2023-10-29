/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_serv.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: isojo-go <isojo-go@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/10/27 15:51:22 by isojo-go          #+#    #+#             */
/*   Updated: 2023/10/29 19:50:32 by isojo-go         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Includes:
#include <string.h>
#include <unistd.h>
// #include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

/* ************************************************************************** */

// Error Handling:

#define ARG		"Wrong number of arguments\n"
#define FATAL	"Fatal error\n"

void	fatalError(void)
{
	write(2, FATAL, strlen(FATAL));
	exit(1);
}

void	argError(void)
{
	write(2, ARG, strlen(ARG));
	exit(1);
}

/* ************************************************************************** */

// Type Definitions:

typedef struct s_client
{
	int		fd;
	int		nl;
}			t_client;

/* ************************************************************************** */

// Global vars:

#define MAX_CLIENTS	1000
#define BUFFSIZE	1000000

int			serverFd;
int			maxFd;
int			maxId;
t_client	clients[MAX_CLIENTS];
char		buff[BUFFSIZE];
fd_set		recvSet, sendSet, activeSet;

/* ************************************************************************** */

// Functions:

int		getClientIdByFd(int fd)
{
	int i = 0;
	while (i < maxId)
	{
		if (clients[i].fd == fd)
			return (i);
		i++;
	}
	return (-1);
}

void	sendAll(int senderFd, char * msg, int len)
{
	int fd = 3;
	while (fd <= maxFd)
	{
		if (FD_ISSET(fd, &sendSet) && fd != senderFd)
			if (send(fd, msg, len, 0) == -1)
				fatalError();
		fd++;
	}
}

void	acceptClient(void)
{
	struct sockaddr_in	clientAddr;
	socklen_t			clientAddrLen = sizeof(clientAddr);
	int					clientFd;

	if ((clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientAddrLen)) == -1)
		fatalError();

	FD_SET(clientFd, &activeSet);
	if (clientFd > maxFd)
		maxFd = clientFd;
	sprintf(buff, "server: client %d just arrived\n", maxId);

	clients[maxId].fd = clientFd;
	clients[maxId].nl = 1;
	maxId++;

	sendAll(clientFd, buff, strlen(buff));
}

void	recvCom(int clientFd)
{
	char	c;
	int		id = getClientIdByFd(clientFd);

	int bytesRead = recv(clientFd, &c, 1, 0);
	if (bytesRead <= 0)
	{
		sprintf(buff, "server: client %d just left\n", id);
		sendAll(clientFd, buff, strlen(buff));
		FD_CLR(clientFd, &activeSet);
		close(clientFd);
	}
	else
	{
		if (clients[id].nl)
		{
			sprintf(buff, "client %d: ", id);
			sendAll(clientFd, buff, strlen(buff));
			clients[id].nl = 0;
		}
		if (c == '\n')
			clients[id].nl = 1;
		sendAll(clientFd, &c, 1);
	}
}

/* ************************************************************************** */

// Main:

int main(int argc, char **argv)
{
	// Check args:
	if (argc != 2)
		argError();

	// Init global vars:
	maxFd = -1;
	serverFd = -1;
	maxId = 0;
	FD_ZERO(&recvSet);
	FD_ZERO(&sendSet);
	FD_ZERO(&activeSet);

	// Setup Server:
	struct sockaddr_in serverAddr;
	bzero(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	serverAddr.sin_port = htons(atoi(argv[1]));

	if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		fatalError();
	if (bind(serverFd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
		fatalError();
	if (listen(serverFd, 256) == -1)
		fatalError();

	FD_SET(serverFd, &activeSet);
	if (serverFd > maxFd)
		maxFd = serverFd;

	// Main Loop:
	while (1)
	{
		recvSet = sendSet = activeSet;
		if (select(maxFd + 1, &recvSet, &sendSet, NULL, NULL) == -1)
			continue;

		int fd = 3;
		while (fd <= maxFd)
		{
			if (FD_ISSET(fd, &recvSet))
			{
				if (fd == serverFd)
					acceptClient();
				else
					recvCom(fd);
			}
			fd++;
		}
	}
	return (0);
}

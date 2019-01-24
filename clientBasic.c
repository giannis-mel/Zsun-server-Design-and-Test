#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;  // Defines a host computer on the internet

    char buffer[256];
    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }

    portno = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);  // argv[1] contains the name of a host
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    // Set fields in serv_addr
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    // The connect function establishes a connection with the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    // Start counting time
    clock_t start, end;

    bzero(buffer, 256);

    // Reads from server
    n = read(sockfd, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");

    printf("%s\n", buffer);

    bzero(buffer, 256);

    // Reads from terminal and sends the command
    fgets(buffer, 255, stdin);
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");

    // You chose to receive messages
    if (strcmp(buffer, "Receive\n") == 0) {
        bzero(buffer, 256);
        // Tell me your name
        n = read(sockfd, buffer, 255);
        if (n < 0)
            error("ERROR reading from socket");
        printf("%s", buffer);

        bzero(buffer, 256);
        // Sends name
        fgets(buffer, 255, stdin);
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing to socket");

        // header gets the size of the message stream from server
        char header[12];
        int size = 0;

        bzero(header, 12);
        // From
        n = recv(sockfd, header, 12, 0);
        if (n < 0)
            error("ERROR reading from socket");

        // Very useful for debugging
        // printf("header is %s\n",header);
        size = atoi(header);
        printf("Connecting to server . . . \n");

        // If size == 0 then there are no messages for this user
        if (size == 0) {
            printf("No messages for you \n");
            exit(1);
        }

        // Create a buffer for the whole message stream
        char completeMessage[size];
        bzero(completeMessage, size);
        n = read(sockfd, completeMessage, size);
        if (n < 0)
            error("ERROR reading from socket");
        // Print all the messages at once
        printf("%s", completeMessage);

	// You chose to send message
    } else if (strcmp(buffer, "Send\n") == 0) {
        char another[] = "Yes";  // Initialize to Yes
        while (strcmp(another, "Yes") == 0) {
            // From
            bzero(buffer, 256);
            n = read(sockfd, buffer, 255);
            if (n < 0)
                error("ERROR reading from socket");
            printf("%s", buffer);

            // This command executes if server is full
            if (strcmp(buffer, "Server is full try later") == 0) {
                printf("\n");
                exit(1);
            }

            // From
            bzero(buffer, 256);
            fgets(buffer, 255, stdin);
            n = write(sockfd, buffer, strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");
            // To
            bzero(buffer, 256);
            n = read(sockfd, buffer, 255);
            if (n < 0)
                error("ERROR reading from socket");
            printf("%s", buffer);
            bzero(buffer, 256);
            fgets(buffer, 255, stdin);
            n = write(sockfd, buffer, strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");
            // Message
            bzero(buffer, 256);
            n = read(sockfd, buffer, 255);
            if (n < 0)
                error("ERROR reading from socket");
            printf("%s", buffer);
            bzero(buffer, 256);
            // Sends the command
            fgets(buffer, 255, stdin);
            n = write(sockfd, buffer, strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");

            // Send another ? or full
            bzero(buffer, 256);
            n = read(sockfd, buffer, 255);
            if (n < 0)
                error("ERROR reading from socket");

            printf("%s", buffer);
            bzero(buffer, 256);
            // Sends the command
            fgets(buffer, 255, stdin);
            // bzero(buffer,256);
            n = write(sockfd, buffer, strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");
            strcpy(another, buffer);
            strtok(another, "\n");
        }

    } else {
        bzero(buffer, 256);
        // Disconnect from server
        n = read(sockfd, buffer, 255);
        if (n < 0)
            error("ERROR reading from socket");
        printf("%s", buffer);
    }

    return 0;
}

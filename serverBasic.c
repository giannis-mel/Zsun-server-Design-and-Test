#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Variable that has the number of messages
int number = 0;

void error(char *msg) {
    perror(msg);
    exit(0);
}

// Struct that has the information of a single message
struct input {
    char sender[256];
    char recipient[256];
    char message[256];
};

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

// Node from the linked list
struct node {
    struct input data;
    struct node *next;
};

// Linked list
struct node *prev, *head, *p;

/* I made some changes in the original function from geeksforgeeks.com : Given a reference (pointer to pointer) to the head of a list
and a key, deletes the first occurrence of key in linked list */
void deleteNode(struct node **head_ref, char name[256]) {
    // Store head node
    struct node *temp = *head_ref, *prev;

    // If head node itself holds the key to be deleted
    if (temp != NULL && (strcmp(temp->data.recipient, name) == 0)) {
        *head_ref = temp->next;  // Changed head
        free(temp);              // free old head
        return;
    }

    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && (strcmp(temp->data.recipient, name) != 0)) {
        prev = temp;
        temp = temp->next;
    }

    // If key was not present in linked list
    if (temp == NULL) return;

    // Unlink the node from linked list
    prev->next = temp->next;

    free(temp);  // Free memory
}

// Searches the link list for messages for the name received, calculates the size of datastream, sends the complete message from server
int search(char name[256], int newSock) {
    int i;
    int n, times = 0;
    int sizeofMessage = 0;

    // Search the linked list
    struct node *tmp;
    tmp = head;
    while (tmp != NULL) {
        // Calculate size of the datastream that will have all messages
        if (strcmp(tmp->data.recipient, name) == 0) {
            sizeofMessage += 768 + 14;  // 768 = 256*3, 14 is the From,To,Message and newlines
            times++;
        }
        tmp = tmp->next;
    }

    // Creating the string to be sent
    char completeMessage[sizeofMessage];
    bzero(completeMessage, sizeofMessage);

    // This is the header that will be sent first
    char str[12];
    bzero(str, 12);
    sprintf(str, "%d", sizeofMessage);

    // strcpy(completeMessage,str);

    n = write(newSock, str, 12);  // First 12 chars are the size of the whole datastram
    if (n < 0) error("ERROR writing to socket");

    if (sizeofMessage == 0) return 0;  // If no messages return

    tmp = head;
    // Creating the datastrean
    while (tmp != NULL) {
        if (strcmp(tmp->data.recipient, name) == 0) {
            // Construct completeMessage
            strcat(completeMessage, "\nFrom ");
            strcat(completeMessage, tmp->data.sender);
            strcat(completeMessage, " to ");
            strcat(completeMessage, tmp->data.recipient);
            strcat(completeMessage, " :\n");
            strcat(completeMessage, tmp->data.message);
            strcat(completeMessage, "\n\n");
        }

        tmp = tmp->next;
    }

    strcat(completeMessage, "No more messages \n");

    // strcat(completeMessage,"\0");
    n = write(newSock, completeMessage, sizeofMessage);
    if (n < 0) error("ERROR writing to socket");

    return times;
}

// Puts a new node in the linked list
struct node *front(struct node *head, struct input value) {
    struct node *p;
    p = malloc(sizeof(struct node));
    p->data = value;
    p->next = head;
    return (p);
}

// When a connection occurs this function handles it
void *handler(void *threadarg) {
    // printf("Inside server handler!\n");
    int n;
    int newSock = *(int *)threadarg;
    char buffer[256];
    char name[256];
    int read_size;

    // char recipient[256] ;
    // char sender[256] ;

    n = write(newSock, "Welcome to the server : Send or Receive ?", 41);
    if (n < 0) error("ERROR writing to socket");

    bzero(buffer, 256);
    n = read(newSock, buffer, 255);  // read will block until it reads something
    if (n < 0) error("ERROR reading from socket");
    strtok(buffer, "\n");

    // printf("The command is : %s \n",buffer);

    // Send something
    if (strcmp(buffer, "Send") == 0) {
        char another[] = "Yes";
        while (strcmp(another, "Yes") == 0) {
            if (number < 100000000) {  // number < available memory
                struct input myInput;

                n = write(newSock, "From : ", 7);
                if (n < 0) error("ERROR writing to socket");

                bzero(buffer, 256);
                n = read(newSock, buffer, 255);
                if (n < 0) error("ERROR reading from socket");
                strcpy(myInput.sender, buffer);
                strtok(myInput.sender, "\n");  // Deleting the newline

                n = write(newSock, "To : ", 5);
                if (n < 0) error("ERROR writing to socket");

                bzero(buffer, 256);
                n = read(newSock, buffer, 255);
                if (n < 0) error("ERROR reading from socket");
                strcpy(myInput.recipient, buffer);
                strtok(myInput.recipient, "\n");

                n = write(newSock, "Write your message : ", 21);
                if (n < 0) error("ERROR writing to socket");

                bzero(buffer, 256);
                n = read(newSock, buffer, 255);
                if (n < 0) error("ERROR reading from socket");
                strcpy(myInput.message, buffer);
                strtok(myInput.message, "\n");

                // printf("From = %s,To = %s,Message is: %s \n", myInput.sender, myInput.recipient, myInput.message);

                // The message goes into the list
                pthread_mutex_lock(&mutex1);

                head = front(head, myInput);
                number++;

                pthread_mutex_unlock(&mutex1);

                // Send another ?
                n = write(newSock, "Do you want to send another(Yes or No) : ", 41);
                if (n < 0) error("ERROR writing to socket");
                bzero(buffer, 256);
                // Waits for Yes or No
                n = read(newSock, buffer, 255);
                if (n < 0) error("ERROR reading from socket");
                strcpy(another, buffer);
                // printf("The user answered %s \n",another);
                strtok(another, "\n");
                bzero(buffer, 256);
            } else {
                n = write(newSock, "Server is full try later", 24);
                if (n < 0) error("ERROR writing to socket");
                bzero(buffer, 256);
                strcpy(another, "No");
                bzero(buffer, 256);
                // Closes
                close(newSock);
                free(threadarg);
                pthread_exit(NULL);
                return 0;
            }
        }

    }  // Read messages
    else if (strcmp(buffer, "Receive") == 0) {
        n = write(newSock, "What is your name ? : ", 21);
        if (n < 0) error("ERROR writing to socket");

        bzero(name, 256);
        n = read(newSock, name, 255);
        if (n < 0) error("ERROR reading from socket");
        strtok(name, "\n");

        pthread_mutex_lock(&mutex1);
        // search() returns the number of messages for 'name'
        int times = search(name, newSock);

        // Deleting messages

        // printf("Name that i will send is : %s  delete %d times\n",name,times);
        int k;
        for (k = 0; k < times; k++) {
            deleteNode(&head, name);
            number--;
        }

        pthread_mutex_unlock(&mutex1);

        close(newSock);
        free(threadarg);

        pthread_exit(NULL);
    } else {
        n = write(newSock, "Unknown instruction, disconnected from server !", 47);
        if (n < 0) error("ERROR writing to socket");
    }

    n = write(newSock, "Bye bye\n", 47);
    if (n < 0) error("ERROR writing to socket");

    close(newSock);
    free(threadarg);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, *newSock;  // Store the values returned by the socket system call and the accept system call.
    int portno;                       // Stores the port number on which the server accepts connections.
    int clilen;

    char buffer[256];  // The server reads chars from the socket connection into this buffer !

    struct sockaddr_in serv_addr, cli_addr;  // Structs containing internet address(server address and clien address in 2 structs)
    int n;                                   // Return value from read(), write() : number of characters

    // Input from user
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Socket returns a small integer to file descriptor table(the value is used for references to this socket)
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));  // Initialize serv_addr to all 0

    portno = atoi(argv[1]);  // The port number in which the server listens

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);  // First we pass portno from htons

    // Binds a socket to an address
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 128);

    int rc, thread = 0;

    // Accept causes the process to block until a client connects to the surver
    clilen = sizeof(cli_addr);
    pthread_mutex_t mutex1;
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        /* Returns a new file descriptor,The second argument is a reference
        pointer to the address of the client on the other end of the connection */
        if (newsockfd < 0)
            error("ERROR on accept");

        // Passing the value of newsockfd to newSock for using multiple threads
        newSock = malloc(1 * sizeof(int));
        *newSock = newsockfd;
        pthread_t newthread;

        rc = pthread_create(&newthread, NULL, handler, (void *)newSock);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    return 0;
}

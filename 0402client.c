#include <unistd.h>     // / header for unix specic functions declarations : fork(), getpid(), getppid(), sleep()
#include <stdio.h>      // header for input and output from console : printf, perror
#include <sys/socket.h> // header for socket specific functions and macros declarations
#include <stdlib.h>     // header for general functions declarations: exit() and also has exit macros such as EXIT_FAILURE - unsuccessful execution of a program
#include <netinet/in.h> //header for MACROS and structures related to addresses "sockaddr_in", INADDR_ANY
#include <string.h>     // header for string functions declarations: strlen()
#include <arpa/inet.h>  // header for functions related to addresses from text to binary form, inet_pton

#define PORT 5564

#define LIGHT_BLUE "\033[1;34m" // imitate shell
#define LIGHT_GREEN "\033[1;32m"
#define NONE "\033[m"
#define clear() printf("\033[H\033[J")

int main()
{
    int sock = 0, valread;
    struct sockaddr_in serv_addr; // structure for storing addres; remote server ip and port

    // Creating socket file descriptor with communication: domain of internet protocol version 4, type of SOCK_STREAM for reliable/conneciton oriented communication, protocol of internet
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) // checking if socket creation fail
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    // setting the address to connect socket to server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form and set the ip
    // This function converts the character string 127.0.0.1 into a network
    // address structure in the af address family, then copies the
    // network address structure to serv_addr.sin_addr
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) // check if conversion failed
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // connect the socket with the adddress and establish connnection with the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    // clear();

    printf("Please waiting for server...\n");

    while (1)
    {
        // read the current location from server
        char message0[1024] = {0};
        recv(sock, message0, sizeof(message0), 0);

        printf(LIGHT_GREEN "user");
        printf(NONE ":");
        printf(LIGHT_BLUE "%s", message0);
        // // ack
        // send(sock, "ok", strlen("ok"), 0);

        // read an string from user and send to server
        printf(NONE "$ ");
        char message2[1024] = {0};
        char ch;
        int i = 0;
        while (1)
        {
            ch = getchar();
            if (ch == '\n')
            {
                break;
            }
            if (ch == '\'')
                continue; // GET RID OF SPECIAL CHARACTERS
            if (ch == '\"')
                continue;

            message2[i] = ch;
            i++;
        }
        if ((strcmp("exit", message2) == 0))
        {
            message2[i] = '\0';
            send(sock, message2, strlen(message2), 0);
            printf("Thank you for using! Now exit.\n");
            break;
        }
        message2[i] = '\0';
        send(sock, message2, strlen(message2), 0);

        // recieve sum from server
        char message3[4096] = {0};
        recv(sock, message3, sizeof(message3), 0);
        printf("%s\n", message3);

        // ack
        send(sock, "ok\n\0", sizeof("ok\n\0"), 0);
    }
    close(sock); // close the socket/end the conection
}
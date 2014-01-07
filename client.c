#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
int main()
{
   
        int sockfd;
        int len;
        struct sockaddr_in address;
        int result;
        char ch = 'A';
        sleep(1);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
        address.sin_port = htons(1234);
        len = sizeof(address);
        result = connect(sockfd, (struct sockaddr *)&address, len);
        if(result == -1)
        {
            perror("oops: client1");
            exit(1);
        }
        write(sockfd, &ch, 1);
	char a[1024];
        read(sockfd, a, 1024);

        printf("from server = %s\n", a);
	printf("from server = %x\n", *a);
        close(sockfd);
	exit(0);
  /*
wpa_hexdump(MSG_MSGDUMP, "nl80211: MLME event frame",
		    nla_data(frame), nla_len(frame));
*/
    
}

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>


#define BUF_SIZE (2048)		//Max buffer size of the data in a frame

/*A frame packet with unique id, length and data*/
enum class TYPE  {
    command, // Default value: 0
    image
};

/*A flag indicate start/stop frame of message*/
enum class INDICATION {
    start,     // Default value: 0
    inProcess,
    stop
};
struct frame_t {
    TYPE type;
    INDICATION ind;
    long int id;
    long int length;
    char data[BUF_SIZE];
};

/**
-------------------------------------------------------------------------------------------------
print_error
------------------------------------------------------------------------------------------------
* This function prints the error message to console
*
* 	@\param msg	User message to print
*
* 	@\return	None
*
*/
static void print_error(const char *msg, ...)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/*-------------------------------------------Main loop-----------------------------------------*/

int main(int argc, char **argv)
{
    struct sockaddr_in sv_addr, cl_addr;
    struct stat st;
    struct frame_t frame;
    struct timeval t_out = {0, 0};

    char msg_recv[BUF_SIZE];
    //char flname_recv[] ="/home/gabriel_wang/share/coco8-seg/images/val/000000000049.jpg" ;
    char flname_recv[] ="/private/l753wang/ultralytics/examples/YOLOv8-LibTorch-CPP-Inference/coco8-seg/images/val/000000000049.jpg" ;
    char cmd_recv[10];

    ssize_t numRead;
    ssize_t length;
    off_t f_size;
    int sfd;

    FILE *fptr;

    /*Clear the server structure - 'sv_addr' and populate it with port and IP address*/
    memset(&sv_addr, 0, sizeof(sv_addr));
    sv_addr.sin_family = AF_INET;
    sv_addr.sin_port = 5008;
    sv_addr.sin_addr.s_addr = INADDR_ANY;

    sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(sfd, (struct sockaddr *) &sv_addr, sizeof(sv_addr));

//    for(;;) {
        printf("Server: Waiting for client to connect\n");

        memset(msg_recv, 0, sizeof(msg_recv));
        memset(cmd_recv, 0, sizeof(cmd_recv));

        length = sizeof(cl_addr);

        if((numRead = recvfrom(sfd, msg_recv, BUF_SIZE, 0, (struct sockaddr *) &cl_addr, (socklen_t *) &length)) == -1)
            print_error("Server: recieve");

        printf("Server: The recieved message ---> %s\n", msg_recv);

/*----------------------------------------------------------------------"get case"-------------------------------------------------------------------------*/
        int total_frame = 0, resend_frame = 0, drop_frame = 0, t_out_flag = 0;
        long int i = 0;

        stat(flname_recv, &st);
        f_size = st.st_size;			//Size of the file

        t_out.tv_sec = 2;
        t_out.tv_usec = 0;
        setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval));   //Set timeout option for recvfrom

        fptr = fopen(flname_recv, "rb");        //open the file to be sent

        if ((f_size % BUF_SIZE) != 0)
           total_frame = (f_size / BUF_SIZE) + 1;				//Total number of frames to be sent
        else
           total_frame = (f_size / BUF_SIZE);

        printf("Total number of packets ---> %d\n", total_frame);
         /*transmit data frames sequentially*/
        for (int i = 0; i <= total_frame; i++) {
            memset(&frame, 0, sizeof(frame));
            if(i == 0) {
                frame.ind = INDICATION::start;
            }
            else if( i == (total_frame -1) )  // last frame;
            {
                frame.ind = INDICATION::stop;
            }
            else {
                frame.ind = INDICATION::inProcess;
            }
            frame.type = TYPE::image;
            frame.id = i;
            frame.length = fread(frame.data, 1, BUF_SIZE, fptr);
            sendto(sfd, &(frame), sizeof(frame), 0, (struct sockaddr *) &cl_addr,  sizeof(cl_addr));        //send the frame
        }
//    }
    close(sfd);
    exit(EXIT_SUCCESS);
}

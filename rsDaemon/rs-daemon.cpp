#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <fstream>              // File IO
#include <iostream>             // Terminal IO
#include <sstream>              // Stringstreams

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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
#include <thread>
#include <string.h>


#define BUF_SIZE (2048)    //Max buffer size of the data in a frame

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
//enum class MSG_TYPE {
//    frame,
//    depth
//};
struct array_packet {
  int * data;
  size_t len;
};

struct frame_t {
    TYPE type;
    INDICATION ind;
    long int id;
    long int length;
    union data_union{
        char char_data[BUF_SIZE];
        int int_data[BUF_SIZE];
    }data;
};
union received_msg {
    char char_data[BUF_SIZE];
    array_packet int_data;
};

//union buffer {
//    frame_t img_msg;
//    int depth_list[100];
//};

//struct udp_packet {
//    MSG_TYPE type;
//    buffer buffer;
//};

received_msg * msg_recv = new received_msg;
 // Declare RealSense pipeline, encapsulating the actual device and sensors
 rs2::pipeline rs_pipe;

//char msg_recv[BUF_SIZE];
char cmd_recv[10];
int sfd;  //socket file description
struct sockaddr_in cl_addr;
bool quit = false;

void capture_color_frame();
void metadata_to_csv(const rs2::frame& frm, const std::string& filename);
int * get_depth_from_coordinates(array_packet * center_coordinates);

void init_socket();
void send_depth_data(array_packet * center_coordinates);
void send_image_file(int type);
void parse_received_msg(received_msg* msg_recv, int length);

int main(int argc, char * argv[]) 
{
    // Start streaming with default recommended configuration
    rs_pipe.start();

    // Capture 30 frames to give autoexposure, etc. a chance to settle
    for (auto i = 0; i < 30; ++i)
    {
	    rs_pipe.wait_for_frames();
    }

    init_socket();

    ssize_t numRead;
    while(!quit)
    {
        printf("Server: Waiting for client to connect\n");
        memset(msg_recv, 0, sizeof(received_msg));
        ssize_t length = sizeof(cl_addr);

        if((numRead = recvfrom(sfd, msg_recv, BUF_SIZE, 0, (struct sockaddr *) &cl_addr, (socklen_t *) &length)) != -1)
        {
            //printf("Server: The received message ---> %s\n", msg_recv);
            parse_received_msg(msg_recv, length);
        }
        rs_pipe.wait_for_frames();
    }
    return EXIT_SUCCESS;
}

void init_socket()
{
    struct sockaddr_in sv_addr;
    /*Clear the server structure - 'sv_addr' and populate it with port and IP address*/
    memset(&sv_addr, 0, sizeof(sv_addr));
    sv_addr.sin_family = AF_INET;
    sv_addr.sin_port = 5008;   // get from config file.
    sv_addr.sin_addr.s_addr = INADDR_ANY; // get from config file.

    sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct timeval t_out = {0, 0};
    t_out.tv_sec = 2;
    t_out.tv_usec = 0;
    setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval));   //Set timeout option for recvfrom

    bind(sfd, (struct sockaddr *) &sv_addr, sizeof(sv_addr));
}

void parse_received_msg(received_msg* msg_recv, int length)
{
    if (strcmp(msg_recv->char_data, "Register to UDP server- D435i .") == 0){
       memset(cmd_recv, 0, sizeof(cmd_recv));
       capture_color_frame();
       send_image_file(0);
    }
    else {
       array_packet * depth_req = &msg_recv->int_data;
//       memset(depth_req, 0, sizeof(depth_req));
//       memcpy(depth_req, msg_recv->int_data, msg_recv->int_data.len);
       send_depth_data(depth_req);
//       size_t num_depths = std::min(length / sizeof(int), static_cast<size_t>(100));
//       int center_coordinates[num_depths];
//       std::memcpy(center_coordinates, msg_recv->int_data, num_depths * sizeof(int));
//       size_t array_length = sizeof(center_coordinates)/sizeof(center_coordinates[0]);
//       for (size_t i=0; i<array_length;i+=3){
//           if (center_coordinates[i] != -1){
//               get_depth_from_coordinates(i,i+1);
//           }
//       }
    }

}
void send_depth_data(array_packet * center_coordinates)
{
    int * result_array  = get_depth_from_coordinates(center_coordinates);
    struct frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = TYPE::command;
    frame.length = sizeof(center_coordinates);
    memcpy(frame.data.int_data, result_array, center_coordinates->len);
    sendto(sfd, &(frame), sizeof(frame), 0, (struct sockaddr *) &cl_addr,  sizeof(cl_addr));        //send the frame
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

}
void send_image_file(int type)
{
    struct stat st;
    struct frame_t frame;
//    struct udp_packet udp_packet;
//    udp_packet.type = MSG_TYPE::frame;
//    udp_packet.buffer.img_msg = frame;
    char flname_recv[] ="/home/lily/rsDaemon/build/rs-daemon-output-Color.png";

    off_t f_size;
    FILE *fptr;
/*----------------------------------------------------------------------"get case"-------------------------------------------------------------------------*/
    int total_frame = 0, resend_frame = 0, drop_frame = 0, t_out_flag = 0;
    long int i = 0;

    stat(flname_recv, &st);
    f_size = st.st_size;          //Size of the file


    fptr = fopen(flname_recv, "rb");        //open the file to be sent

    if ((f_size % BUF_SIZE) != 0)
        total_frame = (f_size / BUF_SIZE) + 1;            //Total number of frames to be sent
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
        frame.length = fread(frame.data.char_data, 1, BUF_SIZE, fptr);
        sendto(sfd, &(frame), sizeof(frame), 0, (struct sockaddr *) &cl_addr,  sizeof(cl_addr));        //send the frame
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

void capture_color_frame()
{
    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    for (auto&& frame : rs_pipe.wait_for_frames())
    {
        // We can only save video frames as pngs, so we skip the rest
        if (auto vf = frame.as<rs2::video_frame>())
        {
            auto stream = frame.get_profile().stream_type();
            // Use the colorizer to get an rgb image for the depth stream
            if (vf.is<rs2::depth_frame>())
	        {
	            vf = color_map.process(frame);
	        }

            // Write images to disk
            std::stringstream png_file;
	        std::string stream_name= vf.get_profile().stream_name();

	        if( stream_name == "Color" )
	        {
    	         png_file << "rs-damon-output-" << stream_name << ".png";
    	         stbi_write_png(png_file.str().c_str(), vf.get_width(), vf.get_height(), vf.get_bytes_per_pixel(), vf.get_data(), vf.get_stride_in_bytes());

	             std::cout << "Saved " << png_file.str()  << ", frame width: " << vf.get_width() << ", height: " << vf.get_height() << std::endl;

                 // Record per-frame metadata for UVC streams
                 std::stringstream csv_file;
                 csv_file << "rs-daemon-output-" << stream_name << "-metadata.csv";
                 metadata_to_csv(vf, csv_file.str());
            }
	    }
    }
}

void metadata_to_csv(const rs2::frame& frm, const std::string& filename)
{
    std::ofstream csv;

    csv.open(filename);

    //    std::cout << "Writing metadata to " << filename << endl;
    csv << "Stream," << rs2_stream_to_string(frm.get_profile().stream_type()) << "\nMetadata Attribute,Value\n";

    // Record all the available metadata attributes
    for (size_t i = 0; i < RS2_FRAME_METADATA_COUNT; i++)
    {
        if (frm.supports_frame_metadata((rs2_frame_metadata_value)i))
        {
            csv << rs2_frame_metadata_to_string((rs2_frame_metadata_value)i) << ","
                << frm.get_frame_metadata((rs2_frame_metadata_value)i) << "\n";
        }
    }
    csv.close();
}

int * get_depth_from_coordinates(array_packet * center_coordinates){
    int * coordinate_list = center_coordinates->data;
    size_t list_len = center_coordinates->len;
    // std::vector<float> depth_results;
    // Block program until frames arrive
    rs2::frameset frames = rs_pipe.wait_for_frames();

    // Try to get a frame of a depth image
    rs2::depth_frame depth = frames.get_depth_frame();

    // Get the depth frame's dimensions
    auto width = depth.get_width();
    auto height = depth.get_height();
    int x_coor, y_coor;
    for (size_t i=0; i<list_len; i++){
       // Query the distance from the camera to the object in the center of the image
       x_coor = coordinate_list[i];
       y_coor = coordinate_list[i+1];
coordinate_list[i+2] = depth.get_distance(x_coor,y_coor);

       // Print the distance
//       std::cout << "The camera is facing an object " << dist_to_center << " meters away \r";
    }
    return coordinate_list;
}

#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netdb.h>
#include <vector>
#include <errno.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <functional>
#include <pthread.h>
#include <map>

//https://zhuanlan.zhihu.com/p/454515997
//https://blog.csdn.net/sanmaoljh/article/details/52183723
//https://stackoverflow.com/questions/58844110/is-there-a-standardized-method-to-send-jpg-images-over-network-with-tcp-ip
//https://github.com/sorabhgandhi01/Reliable-File-Transfer-using-UDP/blob/master/client/client.c#L89
//https://github.com/sorabhgandhi01/Reliable-File-Transfer-using-UDP/blob/master/server/server.c
//https://answers.opencv.org/question/197414/sendreceive-vector-mat-over-socket-cc/
//https://stackoverflow.com/questions/15445207/sending-image-jpeg-through-socket-in-c-linux
//https://gcjyzdd.github.io/2018/opencv/tcp/socket/imageprocessing/Sending-Images-Using-Sockets-in-Real-Time.html
//https://github.com/Dayof/opencv-socket

#define MAX_PACKET_SIZE 2048

struct client_observer_t {
    std::string wantedIP = "";
    std::function<void(const uint8_t * msg, size_t size)> incomingPacketHandler = nullptr;
    std::function<void(const std::string& imageFileName)> incomingImageHandler = nullptr;
    std::function<void(const std::string & ret)> disconnectionHandler = nullptr;
};

class UdpClient
{
private:
    int _sockfd;
    std::atomic<bool> _isConnected;
    std::atomic<bool> _isClosed;
    struct sockaddr_in _server;
    client_observer_t _subscriber;
    std::thread * _receiveTask = nullptr;
    std::mutex _subscribersMtx;
    pthread_t thread_id;


    std::string imageFileName;

    struct sockaddr_in server_addr;

    void publishServerMsg(const uint8_t * msg, size_t msgSize);
    void publishServerMsg(const std::string& imageFileName);

    void publishServerDisconnected(const std::string& ret);
    void run();
    static void* EntryOfThread(void* argv);

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
        char data[MAX_PACKET_SIZE];
    };
    void dataPackageProcess(const uint8_t * msg, size_t msgSize);

public:
    UdpClient();
    ~UdpClient();
    void Start();

    /**
     * Sends a CAN message.
    *
    * @param[in] messageID the CAN ID to send
    * @param[in] data      the data to send (0-8 bytes)
    * @param[in] dataSize  the size of the data to send (0-8 bytes)
    * @param[out] status    Error status variable. 0 on success.
    */
//    void sendMsg(CANFrameId frameId, const uint8_t* data, uint8_t dataSize, int32_t* status);
    void sendMsg();

    void subscribe(const client_observer_t & observer);
    bool close();
};


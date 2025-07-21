#include <fcntl.h>
#include <sys/epoll.h>
#include "UdpClient.h"
#include <memory>

FILE *fptr;

UdpClient::UdpClient() {
    _isConnected = false;
    _isClosed = true;
    imageFileName = "/home/lily/perception/reception/recevied.jpg";
}

UdpClient::~UdpClient() {
    close();
}

void UdpClient::Start() {
    if(pthread_create(&thread_id, nullptr, EntryOfThread,this) != 0) {
    }
}

/*static*/
void* UdpClient::EntryOfThread(void* argv)
{
    UdpClient* client = static_cast<UdpClient*>(argv);
    client->run();
    return (void*) client;
}

#if 0
void UdpClient::sendMsg(uint32_t messageID, const uint8_t* data, uint8_t dataSize, int32_t* status) {

    //const char * msg, size_t size
    //Need to build message based on the messageId/data.
    const char * msg= nullptr;
    const size_t numBytesSent = send(_sockfd, msg, dataSize, 0);

    if (numBytesSent < 0 ) { // send failed
        std::cout << "client is already closed"<<strerror(errno) << std::endl;
    }
    if (numBytesSent < dataSize) { // not all bytes were sent
        char errorMsg[100];
        sprintf(errorMsg, "Only %lu bytes out of %lu was sent to client", numBytesSent, dataSize);
        std::cout << "client is already closed"<< errorMsg << std::endl;
    }
}
#endif

void UdpClient::subscribe(const client_observer_t & observer) {
    std::lock_guard<std::mutex> lock(_subscribersMtx);
    _subscriber = observer;
}


void UdpClient::dataPackageProcess(const uint8_t * msg, size_t msgSize )
{
    frame_t* frame = (frame_t*)(msg);
    if( frame->type == TYPE::image) {
        if(frame->ind == INDICATION::stop) {
            fwrite(frame->data, 1, frame->length, fptr);   /*Write the recieved data to the file*/
            fclose(fptr);
            publishServerMsg(imageFileName);
            std::cout <<"UdpClient::receiveDataProcess, last frame, file is closed." << std::endl;
        }
        else {
            fwrite(frame->data, 1, frame->length, fptr);   /*Write the recieved data to the file*/
            std::cout <<"frame.ID --->" <<frame->id <<"  frame.length --->"<< frame->length << std::endl;
        }
   }
}
/*
 * Publish incomingPacketHandler client message to observer.
 * Observers get only messages that originated
 * from clients with IP address identical to
 * the specific observer requested IP
 */
void UdpClient::publishServerMsg(const uint8_t * msg, size_t msgSize) {
    std::lock_guard<std::mutex> lock(_subscribersMtx);
    _subscriber.incomingPacketHandler(msg,msgSize);
}

/*
 * Publish incomingPacketHandler client message to observer.
 * Observers get only messages that originated
 * from clients with IP address identical to
 * the specific observer requested IP
 */
void UdpClient::publishServerMsg(const std::string& imageFileName) {
    std::lock_guard<std::mutex> lock(_subscribersMtx);
    _subscriber.incomingImageHandler(imageFileName);
    client->sendDepthReq();
    client->recvDepthResp();
}


/*
 * Publish client disconnection to observer.
 * Observers get only notify about clients
 * with IP address identical to the specific
 * observer requested IP
 */
void UdpClient::publishServerDisconnected(const std::string & ret) {
    std::lock_guard<std::mutex> lock(_subscribersMtx);
    _subscriber.disconnectionHandler(ret);
}

void UdpClient::sendMsg() {
    char cmd_send[]="Register to UDP server- D435i .";
    if( sendto(_sockfd, cmd_send, sizeof(cmd_send), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
    {
        std::cout << " failed to send data " << std::endl;
    }
}

void UdpClient::sendDepthReq() {
    if( sendto(_sockfd, (char*)&depth_coordinates, sizeof(cmd_send), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
    {
        std::cout << " failed to send data " << std::endl;
    }
}

void UdpClient::recvDepthResp() {
    memset(&server_addr, 0, sizeof(server_addr));

    _sockfd = socket(AF_INET, SOCK_DGRAM,  IPPROTO_UDP);

    /*Populate send_addr structure with IP address and Port*/
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = 5008;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* Disable socket blocking */
    fcntl(_sockfd, F_SETFL, O_NONBLOCK);

    /* Initialize variables for epoll */
    struct epoll_event ev;

    int epfd = epoll_create(255);
    ev.events = EPOLLIN;
    ev.data.fd = _sockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, _sockfd , &ev);

    struct epoll_event events[256];
    std::cout << "waiting for depth information " << std::endl;

//    while (_isConnected )
    while (1)
    {
        int ready = epoll_wait(epfd, events, 256, -1);
        if (ready < 0)
        {
            perror("epoll_wait error.");
            return ;
        }
        else if (ready == 0) {
            /* timeout, no data coming */
            continue;
        }
        else {
            for (int i = 0; i < ready; i++)
            {
                if (events[i].data.fd == _sockfd)
                {
                    int buffer[100];
                    const size_t numOfBytesReceived = recv(_sockfd, buffer, sizeof(buffer), 0);
                    if (numOfBytesReceived < 1) {
                        std::string errorMsg;
                        if (numOfBytesReceived == 0) {
                            errorMsg = "Server closed connection";
                        } else {
                            errorMsg = strerror(errno);
                        }
                        _isConnected = false;
                        publishServerDisconnected(errorMsg);
                        return;
                    } else {
                        i = 0
                        while (buffer[i]!=0){
                            std::cout<< "1. "<< buffer[i];
                            i+=1;
                        }
                    }
                }
            }
        }

    }
}
/*
 * Receive server packets, and notify user
 */
void UdpClient::run() {
    fptr = fopen(imageFileName.c_str(), "wb");	//open the file in write mode

    memset(&server_addr, 0, sizeof(server_addr));

    _sockfd = socket(AF_INET, SOCK_DGRAM,  IPPROTO_UDP);

    /*Populate send_addr structure with IP address and Port*/
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = 5008;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* Disable socket blocking */
    fcntl(_sockfd, F_SETFL, O_NONBLOCK);

    /* Initialize variables for epoll */
    struct epoll_event ev;

    int epfd = epoll_create(255);
    ev.events = EPOLLIN;
    ev.data.fd = _sockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, _sockfd , &ev);

    struct epoll_event events[256];
    std::cout << "UdpClient::receiveTask is running. " << std::endl;

//    while (_isConnected )
    while (1)
    {
        int ready = epoll_wait(epfd, events, 256, -1);
        if (ready < 0)
        {
            perror("epoll_wait error.");
            return ;
        }
        else if (ready == 0) {
            /* timeout, no data coming */
            continue;
        }
        else {
            for (int i = 0; i < ready; i++)
            {
                if (events[i].data.fd == _sockfd)
                {
                    frame_t msg;
                    memset(&msg,0,sizeof(frame_t));
                    const size_t numOfBytesReceived = recv(_sockfd, &msg, sizeof(frame_t), 0);
                    if (numOfBytesReceived < 1) {
                        std::string errorMsg;
                        if (numOfBytesReceived == 0) {
                            errorMsg = "Server closed connection";
                        } else {
                            errorMsg = strerror(errno);
                        }
                        _isConnected = false;
                        publishServerDisconnected(errorMsg);
                        return;
                    } else {
                        dataPackageProcess((uint8_t *)&msg, numOfBytesReceived);
                    }
                }
            }
        }

    }
}

bool UdpClient::close(){
    if (_isClosed) {
        std::cout << "client is already closed" << std::endl;
        return false;
    }
    _isConnected = false;
    void *result;
    if (pthread_join(thread_id, &result) != 0) {
        perror("Failed to join thread 1");
        return false;
    }

    const bool closeFailed = (::close(_sockfd) == -1);
    if (closeFailed) {

        std::cout << "failed to close socket, error " << strerror(errno) << std::endl;
        return false;
    }
    _isClosed = true;
    return true;
}


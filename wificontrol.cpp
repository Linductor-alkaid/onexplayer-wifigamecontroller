//#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <iostream>
#include <cstring>
#include <arpa/inet.h> // For socket programming
#include <unistd.h>
#include <opencv2/opencv.hpp> // OpenCV for image processing
#include <thread>
#include <chrono>
#include <algorithm> // For std::min
#include <SDL2/SDL.h> // For SDL game controller

#define DEST_IP "192.168.1.178"
#define DEST_PORT 12347
#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT * 3) // Assuming RGB format
#define WAIT_FOR_FRAMES_TIMEOUT 30000 // Increase timeout to 30000 ms
#define SERVER_IP "192.168.1.178"
#define SERVER_PORT 12346
#define BUFFER_SIZE 1024

void send_frames();
void receive_frames();
void udp_communication();

void receive_frames() {
    while (true) {
        // Create TCP socket
        int listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sockfd < 0) {
            std::cerr << "Error creating socket" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait before retrying
            continue;
        }

        sockaddr_in listen_addr;
        memset(&listen_addr, 0, sizeof(listen_addr));
        listen_addr.sin_family = AF_INET;
        listen_addr.sin_addr.s_addr = INADDR_ANY;
        listen_addr.sin_port = htons(DEST_PORT);

        // Bind the socket
        if (bind(listen_sockfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
            std::cerr << "Error binding socket" << std::endl;
            close(listen_sockfd);
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait before retrying
            continue;
        }

        // Listen for incoming connections
        if (listen(listen_sockfd, 1) < 0) {
            std::cerr << "Error listening on socket" << std::endl;
            close(listen_sockfd);
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait before retrying
            continue;
        }

        std::cout << "Waiting for a connection on port " << DEST_PORT << "..." << std::endl;

        // Accept a connection
        int conn_sockfd = accept(listen_sockfd, (struct sockaddr*)NULL, NULL);
        if (conn_sockfd < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            close(listen_sockfd);
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait before retrying
            continue;
        }

        std::cout << "Connection established with the sender." << std::endl;

        uint8_t frame_data[FRAME_SIZE];

        while (true) {
            // Receive frame size
            int frame_size;
            int recv_bytes = recv(conn_sockfd, &frame_size, sizeof(frame_size), 0);
            if (recv_bytes <= 0) {
                std::cerr << "Error receiving frame size or connection closed" << std::endl;
                break;
            } else if (recv_bytes != sizeof(frame_size)) {
                std::cerr << "Partial frame size received" << std::endl;
                break;
            }

            if (frame_size != FRAME_SIZE) {
                std::cerr << "Unexpected frame size received: " << frame_size << std::endl;
                break;
            }

            // Receive frame data in chunks if necessary
            int total_received = 0;
            while (total_received < frame_size) {
                int remaining_bytes = frame_size - total_received;
                int chunk_size = std::min(remaining_bytes, 4096); // Receive in 4KB chunks
                recv_bytes = recv(conn_sockfd, frame_data + total_received, chunk_size, 0);
                if (recv_bytes <= 0) {
                    std::cerr << "Error receiving frame data or connection closed" << std::endl;
                    break;
                }
                total_received += recv_bytes;
            }

            if (total_received != frame_size) {
                std::cerr << "Failed to receive the entire frame" << std::endl;
                break;
            }

            // Convert buffer to OpenCV Mat
            cv::Mat frame(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3, frame_data);

            // Convert from RGB to BGR
            cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);

            // Display the frame
            cv::imshow("Received Frame", frame);
            std::cout << "Successfully received and displayed a frame" << std::endl;
            if (cv::waitKey(1) == 27) { // Exit on ESC key
                break;
            }
        }

        close(conn_sockfd);
        close(listen_sockfd);
    }
}

void udp_communication() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // 创建UDP套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket creation failed");
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    // 设置服务器地址信息
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(sockfd);
        return;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        close(sockfd);
        return;
    }

    SDL_GameController *controller = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            break;
        }
    }

    if (!controller) {
        printf("No game controller found.\n");
        SDL_Quit();
        close(sockfd);
        return;
    }

    while (true) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                SDL_Quit();
                close(sockfd);
                return;
            } else if (e.type == SDL_CONTROLLERAXISMOTION) {
                sprintf(buffer, "Axis %d value: %d", e.caxis.axis, e.caxis.value);
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                sprintf(buffer, "Button %d pressed", e.cbutton.button);
            } else if (e.type == SDL_CONTROLLERBUTTONUP) {
                sprintf(buffer, "Button %d released", e.cbutton.button);
            }

            if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
                perror("sendto failed");
            }
        }
    }

    SDL_GameControllerClose(controller);
    SDL_Quit();
    close(sockfd);
}

int main() {
    std::thread video_thread(receive_frames);
    std::thread udp_thread(udp_communication);

    video_thread.join();
    udp_thread.join();

    return 0;
}

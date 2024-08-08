#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <iostream>
#include <cstring>
#include <arpa/inet.h> // For socket programming
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <chrono>
#include <algorithm> // For std::min

#define DEST_IP "192.168.1.155"
#define DEST_PORT 12347
#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT * 3) // Assuming RGB format
#define WAIT_FOR_FRAMES_TIMEOUT 30000 // Increase timeout to 30000 ms

void send_frames() {
    while (true) {
        try {
            rs2::pipeline p;
            rs2::config cfg;
            cfg.enable_stream(RS2_STREAM_COLOR, FRAME_WIDTH, FRAME_HEIGHT, RS2_FORMAT_RGB8, 30); // 保持原有分辨率和帧率
            p.start(cfg);

            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                std::cerr << "Error creating socket" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待5秒再试
                continue;
            }

            sockaddr_in dest_addr;
            memset(&dest_addr, 0, sizeof(dest_addr));
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(DEST_PORT);
            if (inet_pton(AF_INET, DEST_IP, &dest_addr.sin_addr) <= 0) {
                std::cerr << "Invalid address/ Address not supported" << std::endl;
                close(sockfd);
                std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待5秒再试
                continue;
            }

            while (connect(sockfd, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
                std::cerr << "Connection failed, retrying in 5 seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待5秒再试
            }
            std::cout << "Connected to the server" << std::endl;

            int frame_counter = 0; // 用于计数帧数

            while (true) {
                try {
                    rs2::frameset frames = p.wait_for_frames(WAIT_FOR_FRAMES_TIMEOUT);
                    rs2::video_frame color_frame = frames.get_color_frame();
                    frame_counter++;

                    // 只发送每3帧中的一帧
                    if (frame_counter % 4 == 0) {
                        const uint8_t* frame_data = reinterpret_cast<const uint8_t*>(color_frame.get_data());
                        int frame_size = color_frame.get_height() * color_frame.get_stride_in_bytes();
                        std::cout << "Sending frame of size: " << frame_size << std::endl;

                        int sent_bytes = send(sockfd, &frame_size, sizeof(frame_size), 0);
                        if (sent_bytes < 0) {
                            std::cerr << "Error sending frame size: " << strerror(errno) << std::endl;
                            break;
                        } else if (sent_bytes != sizeof(frame_size)) {
                            std::cerr << "Partial frame size sent" << std::endl;
                            break;
                        }

                        int total_sent = 0;
                        while (total_sent < frame_size) {
                            int remaining_bytes = frame_size - total_sent;
                            int chunk_size = std::min(remaining_bytes, 4096);
                            sent_bytes = send(sockfd, frame_data + total_sent, chunk_size, 0);
                            if (sent_bytes < 0) {
                                std::cerr << "Error sending frame data: " << strerror(errno) << std::endl;
                                break;
                            }
                            total_sent += sent_bytes;
                        }

                        if (total_sent != frame_size) {
                            std::cerr << "Failed to send the entire frame" << std::endl;
                            break;
                        }
                    }
                } catch (const rs2::error & e) {
                    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
                }
            }

            close(sockfd);
        } catch (const rs2::error & e) {
            std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
        } catch (const std::exception & e) {
            std::cerr << "An exception occurred: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait before retrying
    }
}


int main() {
    std::thread sender_thread(send_frames);
    sender_thread.join();
    return 0;
}

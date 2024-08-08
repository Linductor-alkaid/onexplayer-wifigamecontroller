#include <iostream>
#include <cstring>
#include <arpa/inet.h> // For socket programming
#include <unistd.h>
#include <opencv2/opencv.hpp> // OpenCV for image processing

#define LISTEN_PORT 12347
#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT * 3) // Assuming RGB format
#define CHUNK_SIZE 60000

int main() {
    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(LISTEN_PORT);

    // Bind the socket
    if (bind(sockfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(sockfd);
        return -1;
    }

    uint8_t buffer[CHUNK_SIZE + sizeof(int)];
    uint8_t frame_data[FRAME_SIZE];
    sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    while (true) {
        int total_received = 0;
        int expected_chunks = (FRAME_SIZE + CHUNK_SIZE - 1) / CHUNK_SIZE;
        int received_chunks = 0;
        std::vector<bool> received_chunk_flags(expected_chunks, false); // Track received chunks

        while (received_chunks < expected_chunks) {
            // Receive data
            int recv_len = recvfrom(sockfd, buffer, CHUNK_SIZE + sizeof(int), 0, (struct sockaddr*)&sender_addr, &sender_addr_len);
            if (recv_len < 0) {
                std::cerr << "Error receiving data" << std::endl;
                break;
            }

            // Extract chunk index and data
            int chunk_index;
            memcpy(&chunk_index, buffer, sizeof(int));
            int data_len = recv_len - sizeof(int);

            // Print debug information
            std::cout << "Received chunk " << chunk_index << " with size " << data_len << std::endl;

            // Ensure data is within bounds
            if (chunk_index < 0 || chunk_index >= expected_chunks || chunk_index * CHUNK_SIZE + data_len > FRAME_SIZE) {
                std::cerr << "Received data out of bounds: chunk index " << chunk_index << ", data length " << data_len << std::endl;
                break;
            }

            if (!received_chunk_flags[chunk_index]) {
                memcpy(frame_data + chunk_index * CHUNK_SIZE, buffer + sizeof(int), data_len);
                received_chunks++;
                received_chunk_flags[chunk_index] = true;
                total_received += data_len;
            }
        }

        if (received_chunks == expected_chunks && total_received == FRAME_SIZE) {
            // Convert buffer to OpenCV Mat
            cv::Mat frame(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3, frame_data);

            // Display the frame
            cv::imshow("Received Frame", frame);
            std::cout << "Successfully received and reconstructed a frame" << std::endl;
            if (cv::waitKey(1) == 27) { // Exit on ESC key
                break;
            }
        } else {
            std::cerr << "Failed to receive all chunks. Received " << received_chunks << " of " << expected_chunks << " chunks, total received: " << total_received << " bytes." << std::endl;
        }
    }

    close(sockfd);
    return 0;
}

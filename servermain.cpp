#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <string>
#include <map>
#include <fcntl.h>
#include <unordered_map>
#include <iostream>

#define MAIN_SERVER_PORT 33108                // Define main server port
#define BACKEND_SERVER_A_PORT 30108           // Define backend server A's port for reference
#define BACKEND_SERVER_IP "127.0.0.1"         // Define IP address for backend servers (localhost in this case)
#define MAXBUFLEN 20000                        // Define maximum buffer length for receiving data

std::unordered_map<std::string, int> department_backend_mapping;  // Map to hold the department and corresponding backend server mapping

int main() {
    int numbytes;                             // Variable to store number of bytes received

    std::cout << "Main server is up and running." << std::endl;  // Notify user that main server is running

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Create a UDP socket
    int flags = fcntl(sockfd, F_GETFL, 0);       // Fetch current socket flags
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);  // Set socket to non-blocking mode

    if (sockfd == -1) {  // Check for socket creation error
        perror("Main Server: Error creating socket");
        return 1;
    }

    sockaddr_in mainServerAddress{};                   // Structure to hold the main server's address information
    mainServerAddress.sin_family = AF_INET;            // Using IPv4
    mainServerAddress.sin_port = htons(MAIN_SERVER_PORT);  // Set port number after converting to network byte order
    mainServerAddress.sin_addr.s_addr = INADDR_ANY;    // Allow any incoming address

    if (bind(sockfd, (struct sockaddr*)&mainServerAddress, sizeof(mainServerAddress)) == -1) {  // Bind the socket to the defined address and port
        perror("Main Server: Error binding socket to port 33108");
        close(sockfd);
        return 1;
    }

    std::vector<int> backendPorts = {30108, 31108, 32108};  // List of backend server ports
    std::vector<int> backendMapping = {0, 1, 2};  // Mapping to represent backend servers as A, B, or C

    for (size_t i = 0; i < backendPorts.size(); i++) {  // Iterate over each backend server to request department data

        sockaddr_in backendAddress{};                 // Structure to hold the backend server's address information
        backendAddress.sin_family = AF_INET;          // Using IPv4
        backendAddress.sin_port = htons(backendPorts[i]);  // Set port number after converting to network byte order
        inet_pton(AF_INET, BACKEND_SERVER_IP, &(backendAddress.sin_addr));  // Convert IP address to binary form

        std::string request = "send_departments";     // Message to request departments from backend servers
        sendto(sockfd, request.c_str(), request.size(), 0, (struct sockaddr*)&backendAddress, sizeof(backendAddress));  // Send request to backend server

        char buf[MAXBUFLEN];                          // Buffer to receive the departments from the backend server
        struct sockaddr_storage their_addr;           // Structure to hold the sender's address information
        socklen_t addr_len = sizeof(their_addr);      // Variable to hold the length of the address
        int count = 0;                                // Counter to track the number of departments received

        int total_wait_time = 0;                      // Variable to track total wait time
        int wait_duration = 100000;                   // Duration to wait (0.1 seconds)
        int max_wait_time = 2000000;                  // Maximum time to wait (2 seconds)
        bool receivedFromThisBackend = false;         // Flag to check if data has been received from this backend server

        while (count < 10 && total_wait_time < max_wait_time) {  // Loop until we have received all departments or timed out
            numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr*)&their_addr, &addr_len);  // Receive data from backend server
            if (numbytes > 0) {  // If data is received
                buf[numbytes] = '\0';  // Null-terminate the received data
                department_backend_mapping[buf] = backendMapping[i];  // Map the received department to the corresponding backend server
                //unsigned short port = ntohs(((struct sockaddr_in*)&their_addr)->sin_port);  // Extract the sender's port number

                if (!receivedFromThisBackend) {  // If data is received for the first time from this backend
                    std::cout << "Main server has received the department list from Backend server "
                              << char('A' + backendMapping[i])  // Convert 0, 1, 2 to A, B, C respectively
                              << " using UDP over port " << MAIN_SERVER_PORT << std::endl;
                    receivedFromThisBackend = true;  // Set the flag to true
                }
                count++;  // Increment the counter
            } else if (errno == EWOULDBLOCK || errno == EAGAIN) {  // If no data is available to read
                usleep(wait_duration);  // Wait for the specified duration
                total_wait_time += wait_duration;  // Update the total wait time
            } else {  // If there's an error in recvfrom
                perror("recvfrom");
                break;
            }
        }
    }

    std::map<int, std::vector<std::string>> reverse_mapping;  // Create a map to display results by server
    for (const auto& entry : department_backend_mapping) {  // Iterate over the department-backend mapping
        reverse_mapping[entry.second].push_back(entry.first);  // Reverse the mapping
    }

    for (const auto& entry : reverse_mapping) {  // Iterate over the reversed mapping
        std::cout << "Server " << char('A' + entry.first) << std::endl;  // Display server name
        for (const std::string& dept : entry.second) {  // Iterate over the departments for this server
            std::cout << dept << std::endl;  // Display the department name
        }
        std::cout << std::endl;  // Print a new line for clarity
    }

    while (true) {  // Infinite loop to continuously take queries from the user
        std::string queryDepartment;  // Variable to store user's query
        std::cout << "Enter Department Name: ";  // Prompt user to enter a department name
        std::cin >> queryDepartment;  // Read the user's input

        auto it = department_backend_mapping.find(queryDepartment);  // Check if the entered department exists in our mapping
        if (it == department_backend_mapping.end()) {  // If the department doesn't exist
            std::cout << queryDepartment << " does not show up in Backend servers" << std::endl;  // Notify the user
        } else {  // If the department exists
            int backendServer = it->second;  // Get the backend server corresponding to the department
            sockaddr_in backendAddress{};    // Structure to hold the backend server's address information
            backendAddress.sin_family = AF_INET;  // Using IPv4
            backendAddress.sin_port = htons(30108 + 1000 * backendServer);  // Set the port number after converting to network byte order
            inet_pton(AF_INET, BACKEND_SERVER_IP, &(backendAddress.sin_addr));  // Convert IP address to binary form

            sendto(sockfd, queryDepartment.c_str(), queryDepartment.size(), 0, (struct sockaddr*)&backendAddress, sizeof(backendAddress));  // Send the query to the backend server
            std::cout << queryDepartment << " shows up in server " << char('A' + backendServer) << std::endl;  // Notify the user about the server that has the department
            std::cout << "The Main server has sent request for " << queryDepartment << " to server " << char('A' + backendServer) << " using UDP over port " << MAIN_SERVER_PORT << std::endl;  // Notify the user about the request being sent

            usleep(500000);  // Wait for 0.5 seconds to ensure that the backend server has processed the request

            char buf[MAXBUFLEN];  // Buffer to receive data from the backend server
            struct sockaddr_storage their_addr;  // Structure to hold the sender's address information
            socklen_t addr_len = sizeof(their_addr);  // Variable to hold the length of the address
            numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr*)&their_addr, &addr_len);  // Receive data from backend server
            buf[numbytes] = '\0';  // Null-terminate the received data

            std::cout << "The Main server has received searching result(s) of " << queryDepartment << " from Backend server " << char('A' + backendServer) << std::endl;  // Notify the user about the received data
            std::cout << buf << std::endl;  // Display the received data
            std::cout << std::endl << "-----Start a new query-----" << std::endl;  // Prompt for a new query
            memset(buf, 0, MAXBUFLEN);  // Clear the buffer

        }
    }

    close(sockfd);  // Close the socket
    return 0;  // Exit the program
}

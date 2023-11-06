// BackendServer.cpp

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_A 1
#define SERVER_B 2
#define SERVER_C 3

// Set the backend server's name for this instance
#ifndef BACKEND_SERVER_NAME // if not defined in the make file
#define BACKEND_SERVER_NAME SERVER_A // Change this to SERVER_A, SERVER_B, or SERVER_C
#endif
 
// Set the server port and data file based on server name
#if BACKEND_SERVER_NAME == SERVER_A
    #define BACKEND_SERVER_PORT 30108
    #define DATA_FILE "dataA.txt"
#elif BACKEND_SERVER_NAME == SERVER_B
    #define BACKEND_SERVER_PORT 31108
    #define DATA_FILE "dataB.txt"
#else
    #define BACKEND_SERVER_PORT 32108
    #define DATA_FILE "dataC.txt"
#endif

#define MAXBUFLEN 1000      // Define maximum buffer length for socket communication

using namespace std;       // Use the standard namespace

// Global data structures to hold department data and names
map<string, set<unsigned long long>> departmentData;
vector<string> departmentNames;

// Function to load department data from file
void loadFile() {
    ifstream file(DATA_FILE);  // Open the data file for reading
    if (!file.is_open()) {    // Check if file was opened successfully
        cerr << "Unable to open " << DATA_FILE << endl;  // Print error message to standard error
        exit(1);
    }

    string line;  // Variable to hold each line from the file
    while (getline(file, line)) {  // Loop to read each line from the file
        departmentNames.push_back(line);  // Add department name to vector
        if (getline(file, line)) {  // Read next line for student IDs
            stringstream ss(line);  // Convert the line to a string stream for parsing
            string idStr;  // Variable to hold each student ID
            while (getline(ss, idStr, ',')) {  // Loop to parse each student ID
                departmentData[departmentNames.back()].insert(stoull(idStr));  // Add student ID to the department's data set
            }
        }
    }
    file.close();  // Close the data file
}

int main() {
    // Print initial server status message
    cout << "Server " << char('A' + BACKEND_SERVER_NAME - 1) << " is up and running using UDP on port " << BACKEND_SERVER_PORT << endl;

    loadFile();  // Load department data from file

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {  // Check if socket creation was successful
        perror(("Backend Server " + string(1, char('A' + BACKEND_SERVER_NAME - 1)) + ": Error creating socket").c_str());  // Print error message
        return 1;  // Return with error code 1
    }

    // Define the server's address structure
    sockaddr_in myAddress{};
    myAddress.sin_family = AF_INET;  // Use IPv4
    myAddress.sin_port = htons(BACKEND_SERVER_PORT);  // Convert port number to network byte order
    myAddress.sin_addr.s_addr = INADDR_ANY;  // Allow any incoming address

    // Bind the socket to the server's address
    if (::bind(sockfd, (struct sockaddr *) &myAddress, sizeof(myAddress)) == -1) {
        perror(("Backend Server " + string(1, char('A' + BACKEND_SERVER_NAME - 1)) + ": Error binding socket").c_str());  // Print error message
        close(sockfd);  // Close the socket
        return 1;  // Return with error code 1
    }

    char buf[MAXBUFLEN];  // Buffer to store incoming messages
    sockaddr_storage their_addr;  // Structure to store the client's address
    socklen_t addr_len = sizeof(their_addr);  // Variable to hold the length of the client's address
    int numBytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *) &their_addr, &addr_len);  // Receive a message from a client
    buf[numBytes] = '\0';  // Null-terminate the received message

    // Check if the received message is a request for department names
    if (strcmp(buf, "send_departments") == 0) {
        for (const string &dept: departmentNames) {  // Loop through each department name
            sendto(sockfd, dept.c_str(), dept.size(), 0, (struct sockaddr *) &their_addr,
                   addr_len);  // Send the department name to the client
        }
        // Print status message indicating the department list was sent
        cout << "Server " << char('A' + BACKEND_SERVER_NAME - 1) << " has sent a department list to Main Server"
             << endl;


        while (true) {  // Infinite loop to keep the server running and listen to more requests
            numBytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *) &their_addr,
                                &addr_len);  // Receive a message from a client
            buf[numBytes] = '\0';  // Null-terminate the received message

            // Print status message indicating the server received a request
            cout << "Server " << char('A' + BACKEND_SERVER_NAME - 1) << " has received a request for " << buf << endl;

            auto it = departmentData.find(buf);  // Check if the requested department exists in the data
            if (it != departmentData.end()) {  // If the department exists
                stringstream response, printMessage;  // Create string streams for the response message and print message
                // Build the response message
                response << "Server " << char('A' + BACKEND_SERVER_NAME - 1) << " found " << it->second.size()
                         << " distinct students for " << buf << ": ";
                // Build the print message
                printMessage << "There are " << it->second.size() << " distinct students in " << buf
                             << " . Their IDs are ";
                for (unsigned long long id: it->second) {  // Loop through each student ID
                    response << id << ", ";  // Add the student ID to the response message
                    printMessage << id << ", ";  // Add the student ID to the print message
                }
                string responseStr = response.str();
                responseStr = responseStr.substr(0, responseStr.size() -
                                                    2);  // Remove the trailing comma and space from the response message
                string printMessageStr = printMessage.str();
                printMessageStr = printMessageStr.substr(0, printMessageStr.size() -
                                                            2);  // Remove the trailing comma and space from the print message
                sendto(sockfd, responseStr.c_str(), responseStr.size(), 0, (struct sockaddr *) &their_addr,
                       addr_len);  // Send the response message to the client
                cout << printMessageStr << endl;  // Print the print message
                // Print status message indicating the results were sent
                cout << "Server " << char('A' + BACKEND_SERVER_NAME - 1) << " has sent the results to Main Server"
                     << endl;
            } else {  // If the department does not exist
                string response = "No data found for " + string(buf);  // Create the response message
                sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr *) &their_addr,
                       addr_len);  // Send the response message to the client
                // Print status message indicating the results were sent
                cout << "Server " << char('A' + BACKEND_SERVER_NAME - 1) << " has sent the results to Main Server"
                     << endl;
            }
        }
    }

    close(sockfd);  // Close the socket
    return 0;  // Return success code
}
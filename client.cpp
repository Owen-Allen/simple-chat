#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <thread>
#include <atomic>

using namespace std;

const int bufsize = 1024;
char sendBuffer[bufsize];
char receiveBuffer[bufsize];
std::atomic<bool> shouldTerminate(false);

// recv with a specified timeout
int timedRecv(int socket, char *buffer, int size, int timeout_ms) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socket, &fds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int result = select(socket + 1, &fds, nullptr, nullptr, &timeout);

    if (result > 0) {
        return recv(socket, buffer, size, 0);
    } else if (result == 0) {
        // Timeout
        return 0;
    } else {
        // Error
        return -1;
    }
}

void receiveFromServer(int client) {
    while (!shouldTerminate.load()) {

        // use timedRecv so that the program isn't waiting on recv for too long while shouldTerminate is true
        int bytesReceived = timedRecv(client, receiveBuffer, sizeof(receiveBuffer), 1000);

        if (bytesReceived > 0) {
            cout << "Server: " << receiveBuffer << endl;

            if (receiveBuffer[0] == '#') {
                shouldTerminate.store(true);
                break;
            }
        } else if (bytesReceived == 0) {
            // Handle the case when no data is received within the timeout
            // You can perform other actions or continue waiting
        } else {
            // Handle the case when there's an error
            // You can break or handle it based on your application's logic
            break;
        }
    }
}


void sendToServer(int client) {
    while (!shouldTerminate.load()) {
        cout << "Client: ";
        std::cin.getline(sendBuffer, sizeof(sendBuffer));
        send(client, sendBuffer, bufsize, 0);

        if (sendBuffer[0] == '#') {
            cout << "Setting shouldTerminate to true!" << endl;
            shouldTerminate.store(true);
            break;
        }
    }
}

int main() {
    int client;
    int portNum = 1500;
    const char* ip = "127.0.0.1";

    struct sockaddr_in server_addr;

    client = socket(AF_INET, SOCK_STREAM, 0);

    if (client < 0) {
        cout << "\nError establishing socket..." << endl;
        exit(1);
    }

    cout << "\n=> Socket client has been created..." << endl;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portNum);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    cout << "\n=> time to connect..." << endl;
    int connectionResult = connect(client, (struct sockaddr*)&server_addr, sizeof(server_addr));

    cout << "\n=> connectionResult: " << connectionResult << endl;

    if (connectionResult == 0) {
        cout << "=> Connection to the server port number: " << portNum << endl;

        // Start two threads for sending and receiving
        std::thread receiveThread(receiveFromServer, client);
        std::thread sendThread(sendToServer, client);

        // Wait for both threads to finish
        receiveThread.join();
        sendThread.join();
    }
    cout << "our work here is done" << endl;

    // Close the socket
    close(client);

    cout << "\n=> Connection terminated.\nGoodbye...\n";

    return 0;
}

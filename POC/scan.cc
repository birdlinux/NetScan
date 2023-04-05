#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <chrono>
#include <fstream>

using namespace std;

const int NUM_THREADS = 10;

struct ScanParams {
    string host;
    int start_port;
    int end_port;
};

    string host;
    int port;
    bool is_open;
    string banner;
};

    result.host = host;
    result.port = port;

    int sockfd, ret;
    struct sockaddr_in target;
    char buffer[65536];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        result.is_open = false;
        return result;
    }

    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    target.sin_addr.s_addr = inet_addr(host.c_str());

    ret = connect(sockfd, (struct sockaddr*)&target, sizeof(target));
    if(ret < 0) {
        result.is_open = false;
        return result;
    }

    result.is_open = true;

    ret = recv(sockfd, buffer, sizeof(buffer), MSG_PEEK);
    if(ret > 0) {
        buffer[ret] = '\0';
        result.banner = buffer;
    }

    close(sockfd);

    cout << "[DEBUG] Scanned port " << port << " on host " << host << endl;

    return result;
}

vector<ScanResult> scanHost(ScanParams params) {
    vector<ScanResult> results;

    for(int port = params.start_port; port <= params.end_port; port++) {
        ScanResult result = scanPort(params.host, port);
        results.push_back(result);
    }

    return results;
}

void writeResultsToFile(vector<ScanResult> results, string filename) {
    ofstream outfile(filename);

    if(!outfile) {
        cerr << "Error opening file " << filename << endl;
        return;
    }

    outfile << "Host,Port,Status,Banner" << endl;

    for(auto result : results) {
        string status = result.is_open ? "open" : "closed";
        outfile << result.host << "," << result.port << "," << status << "," << result.banner << endl;
    }

    outfile.close();

    cout << "[DEBUG] Results saved to file " << filename << endl;
}

int main(int argc, char** argv) {
    if(argc != 2) {
        cout << "Usage: " << argv[0] << " <host>" << endl;
        return 1;
    }

    string host = argv[1];
    ScanParams params;
    params.host = host;
    params.start_port = 1;
    params.end_port = 65535;

    vector<thread> threads;
    int step = (params.end_port - params.start_port + 1) / NUM_THREADS;

        for(int i = 0; i < NUM_THREADS; i++) {
        ScanParams thread_params;
        thread_params.host = params.host;
        thread_params.start_port = params.start_port + i * step;
        thread_params.end_port = thread_params.start_port + step - 1;

        if(i == NUM_THREADS - 1) {
            thread_params.end_port = params.end_port;
        }

        threads.push_back(thread(scanHost, thread_params));
    }

    for(int i = 0; i < NUM_THREADS; i++) {
        threads[i].join();
    }

    vector<ScanResult> results;

    for(auto& thread : threads) {
        vector<ScanResult> thread_results = thread.get();
        results.insert(results.end(), thread_results.begin(), thread_results.end());
    }

    string filename = host + ".csv";
    writeResultsToFile(results, filename);

    cout << "[DEBUG] All done!" << endl;

    return 0;
}

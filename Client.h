
#ifndef H_CLIENT
#define H_CLIENT

#include <string>

struct ClientConfig {
    int port;
    std::string server;
    std::string command;

    std::string ServerStr();
};

class Client {
  public:
    const int max_response_len = 1024;

    Client(ClientConfig config);
    void SendCommand();

  private:
    ClientConfig config_;
};

#endif // !H_CLIENT

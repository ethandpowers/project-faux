
#ifndef H_CLIENT
#define H_CLIENT

#include <atomic>
#include <optional>
#include <string>

struct ClientConfig {
    int port;
    std::string server;
    std::string command;
    bool is_repl;

    std::string ServerStr();
};

class Client {
  public:
    const int max_response_len = 1024;

    Client(ClientConfig config, const std::atomic<bool> &stop);
    void Operate();

  private:
    const std::atomic<bool> &stop_;
    ClientConfig config_;

    void SendCommand(const std::string command);
    std::optional<std::string> AwaitCommand();
};

#endif // !H_CLIENT

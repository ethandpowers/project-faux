#include "Server.h"
#include "Bacnet.h"
#include "FieldMapParser.h"
#include "Modbus.h"
#include <csignal>
#include <cstdlib>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

// Double-fork to daemonize
static void Daemonize() {
    pid_t pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        _exit(EXIT_SUCCESS);

    if (setsid() < 0)
        exit(EXIT_FAILURE);

    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    if (chdir("/") < 0)
        exit(EXIT_FAILURE);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int dev_null = open("/dev/null", O_RDWR);
    if (dev_null >= 0) {
        dup2(dev_null, STDIN_FILENO);
        dup2(dev_null, STDOUT_FILENO);
        dup2(dev_null, STDERR_FILENO);
        if (dev_null > 2)
            close(dev_null);
    }
}

Server::Server(ServerConfig config) : config_(std::move(config)) {}

void Server::Operate() {
    FieldMapParser parser;
    FieldMap map = parser.Parse(config_.map_path);

    ModbusServer modbus(map);
    modbus.SetMaxConnections(config_.max_modbus_connections);

    BacnetServer bacnet(map);

    sigset_t shutdown_signals;
    sigemptyset(&shutdown_signals);
    sigaddset(&shutdown_signals, SIGINT);
    sigaddset(&shutdown_signals, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &shutdown_signals, nullptr);

    Daemonize();

    if (config_.is_modbus_enabled) {
        std::cout << "Starting modbus server\n";
        try {
            modbus.Start();
        } catch (std::exception &e) {
            std::cerr << e.what() << "\n";
        }
    }

    if (config_.is_bacnet_enabled) {
        std::cout << "Starting BACnet server\n";
        try {
            bacnet.Start();
        } catch (std::exception &e) {
            std::cerr << e.what() << "\n";
        }
    }

    // TODO: Receive commands
    int signal;
    sigwait(&shutdown_signals, &signal);

    std::cout << "Shutting down\n";
    modbus.Stop();
    bacnet.Stop();
}

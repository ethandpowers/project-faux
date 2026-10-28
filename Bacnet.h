#ifndef H_BACNET
#define H_BACNET

#include "FieldMapParser.h"
#include <atomic>
#include <thread>

class BacnetServer {
  public:
    BacnetServer(FieldMap &map);

    ~BacnetServer();

    void Start();
    void Stop();
    bool IsActive();

  private:
    FieldMap &map_;
    std::thread t_;
    std::atomic<bool> stop_{false};

    void ServeContext();
    void DisableWriteServices();
    void BuildObjects();
    void SetObjectValues();
};

#endif // !H_BACNET

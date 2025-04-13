#pragma once

class IApplication {
public:
    virtual void run() = 0;
    virtual ~IApplication() = default;
};

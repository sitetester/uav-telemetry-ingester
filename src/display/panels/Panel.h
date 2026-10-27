#pragma once
#include <ostream>

class Panel {
public:
    virtual ~Panel() = default;

    // Force every panel to define how it prints data to the stream
    virtual void render(std::ostream& out) = 0;
};

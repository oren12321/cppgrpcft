#include <functional>

class Defer {
public:
    Defer(const std::function<void(void) noexcept>& func) : _func(func) {}

    ~Defer() {
        _func();
    }
    
private:
    std::function<void(void) noexcept> _func;
};


#pragma once


namespace jac {


template<class Next>
class EventLoopTerminal : public Next {
public:
    virtual void runOnEventLoop() override {  // NOLINT
        Next::onEventLoop();
    };
};


} // namespace jac

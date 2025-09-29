#pragma once
#include <stdint.h>

class InputHandler {
  public:
    virtual ~InputHandler() { }

    virtual void handleKey([[maybe_unused]] int keycode, [[maybe_unused]] int bDown) { }

    virtual void handleButton([[maybe_unused]] float x,
                              [[maybe_unused]] float y,
                              [[maybe_unused]] int   button,
                              [[maybe_unused]] int   bDown) { }

    virtual void handleMotion([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] int mask) { }
};

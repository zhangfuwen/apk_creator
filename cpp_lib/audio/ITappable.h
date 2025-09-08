#pragma once
class ITappable {
public:
    virtual ~ITappable()  = default;
    virtual void tap(bool isDown) = 0;
};

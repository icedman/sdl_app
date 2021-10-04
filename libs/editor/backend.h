#ifndef BACKEND_H
#define BACKEND_H

#include <string>

struct backend_t {

    backend_t();

    virtual void setClipboardText(std::string text){};
    virtual std::string getClipboardText(){};

    int now();
    void begin();
    void delay(int millis);
    int elapsed();

    static backend_t* instance();
};

#endif // BACKEND_H
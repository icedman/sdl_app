#ifndef BACKEND_H
#define BACKEND_H

#include <string>

struct backend_t {

    backend_t();

    void setClipboard(std::string text);
    std::string getClipboard();

    int now();
    void begin();
    void delay(int millis);
    int elapsed();

    static backend_t* instance();

    std::string cliptext;
};

#endif // BACKEND_H
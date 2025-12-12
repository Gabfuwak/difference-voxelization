#include "app.hpp"

class OurVisionApp : public WgpuApp {

};

int main() {
    auto app = OurVisionApp();
    app.initialize();
    app.start();
    return 0;
}
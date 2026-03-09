#include "app.h"

int main(int argc, char* argv[]) {
    VedApp app;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            app.open_file(argv[i]);
        }
    }
    app.run();
    return 0;
}

#include <SDL.h>  // let SDL2 do it's main() replacement magic

#include <gips_app.h>


int main(int argc, char* argv[]) {
    static GIPS::App app;
    return app.run(argc, argv);
}

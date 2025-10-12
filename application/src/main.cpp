#include "Application.h"

const unsigned int SCREEN_WIDTH = 1200;
const unsigned int SCREEN_HEIGHT = 900;

int main()
{
    Application app(SCREEN_WIDTH, SCREEN_HEIGHT, "Paper Marbling");
    app.run();

    return 0;
}

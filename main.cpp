#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <iostream>
#include <emscripten/emscripten.h>

const int MAPHEIGHT = 24;
const int MAPWIDTH = 24;
const int TILE = 64;
const double EPS = 0.0001;
const double FOV = 60.0;
const int SCREENHEIGHT = 512;
const int SCREENWIDTH = 512;
const double PROJECTIONDISTANCE = (SCREENWIDTH / 2.0) / std::tan((FOV / 2.0) * 3.141592 / 180.0);

//onscreen controls
bool htmlLeft = false;
bool htmlRight = false;
bool htmlDown = false;
bool htmlUp = false;

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void setLeft(int pressed) {
        htmlLeft = pressed;
    }
    void setRight(int pressed) {
        htmlRight = pressed;
    }
    void setDown(int pressed) {
        htmlDown = pressed;
    }
    void setUp(int pressed) {
        htmlUp = pressed;
    }
}

double rad(double deg)
{
    return deg * M_PI / 180.0;
}

struct Player
{
    double x = 16.f + TILE*10;
    double y = 16.f + TILE;
    double dir = 90;

    sf::CircleShape shape;

    void init()
    {
        shape.setRadius(16.f);
        shape.setOrigin({16.f, 16.f});
        shape.setFillColor(sf::Color::Yellow);
    }
};

int mapHit(double x, double y, int map[MAPHEIGHT][MAPWIDTH])
{
    int col = int(x) >> 6;
    int row = int(y) >> 6;

    if (row < 0 || col < 0 || row >= MAPHEIGHT || col >= MAPWIDTH)
        return 0;

    return map[row][col];
}

double dist(double ax, double ay, double bx, double by)
{
    return std::hypot(ax - bx, ay - by);
}

double castRay(Player &p, int map[MAPHEIGHT][MAPWIDTH], double angle, int &color)
{
    angle = fmod(angle + 360.0, 360.0);
    double sinA = sin(rad(angle));
    double cosA = cos(rad(angle));

    double hDist = 1e9;
    double vDist = 1e9;

    int value = 0;
    int value2 = 0;
    //-------- horizontal -------------------
    if (fabs(sinA) > EPS)
    {
        // length that we need to move to find next line
        double yStep = (sinA > 0) ? TILE : -TILE;
        // where our starting y position is
        double y = (int(p.y) / TILE) * TILE;
        if (sinA > 0) y += TILE;

        double x = p.x + (y - p.y) / tan(rad(angle));
        double xStep = yStep / tan(rad(angle));

        for (int i = 0; i < 32; i++)
        {
            double hx = x + cosA * EPS;
            double hy = y + sinA * EPS;

            if ((value = mapHit(hx, hy, map)))
            {
                hDist = dist(p.x, p.y, hx, hy);
                break;
            }
            x += xStep;
            y += yStep;
        }
    }

    //-------- vertical ---------------------
    if (fabs(cosA) > EPS)
    {
        double xStep = (cosA > 0) ? TILE : -TILE;
        double x = (int(p.x) / TILE) * TILE;
        if (cosA > 0) x += TILE;

        double y = p.y + (x - p.x) * tan(rad(angle));
        double yStep = xStep * tan(rad(angle));

        for (int i = 0; i < 32; i++)
        {
            double vx = x + cosA * EPS;
            double vy = y + sinA * EPS;

            if ((value2 = mapHit(vx, vy, map)))
            {
                vDist = dist(p.x, p.y, vx, vy);
                break;
            }
            x += xStep;
            y += yStep;
        }
    }

    //return the closest color
    if (hDist <= vDist) {
        color = value;
    } else {
        color = value2;
    }

    return std::min(hDist, vDist);
}

void handleCollision(Player &p, int map[MAPHEIGHT][MAPWIDTH], double px, double py)
{
    int cx = int(p.x) >> 6;
    int cy = int(p.y) >> 6;

    if (map[cy][cx])
    {
        p.x = px;
        p.y = py;
    }
}

int main()
{
    //make 1024 for 2d grid
    sf::RenderWindow window(sf::VideoMode({512, 512}),"Raycaster");

    window.setFramerateLimit(60);

    int map[MAPWIDTH][MAPHEIGHT] =
{
  {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,7,7,7,7,7,7,7},
  {4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7},
  {4,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7},
  {4,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7},
  {4,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7},
  {4,0,4,0,0,0,0,5,1,2,3,0,5,6,7,5,7,7,0,7,7,7,7,7},
  {4,0,5,0,0,0,0,5,0,5,0,0,0,5,0,5,7,0,0,0,7,7,7,1},
  {4,0,6,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8},
  {4,0,7,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,7,7,7,1},
  {4,0,8,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8},
  {4,0,0,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,7,7,7,1},
  {4,0,0,0,0,0,0,5,5,5,5,0,5,5,5,5,7,7,7,7,7,7,7,1},
  {6,6,6,6,6,6,6,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6},
  {8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4},
  {6,6,6,6,6,6,0,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6},
  {4,4,4,4,4,4,0,4,4,4,6,0,6,2,2,2,2,2,2,2,3,3,3,3},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,0,0,0,6,2,0,0,5,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2},
  {4,0,6,0,6,0,0,0,0,4,6,0,0,0,0,0,5,0,0,0,0,0,0,2},
  {4,0,0,5,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2},
  {4,0,6,0,6,0,0,0,0,4,6,0,6,2,0,0,5,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2},
  {4,4,4,4,4,4,4,4,4,4,1,1,1,2,2,2,2,2,2,3,3,3,3,3}
};

    Player player;
    player.init();

    while (window.isOpen())
    {

        double px = player.x;
        double py = player.y;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || htmlLeft)  player.dir -= 3;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || htmlRight) player.dir += 3;

        double speed = 2.5;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || htmlDown)
        {
            player.x -= cos(rad(player.dir)) * speed;
            player.y -= sin(rad(player.dir)) * speed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || htmlUp)
        {
            player.x += cos(rad(player.dir)) * speed;
            player.y += sin(rad(player.dir)) * speed;
        }

        handleCollision(player, map, px, py);

        window.clear(sf::Color::Black);


        /* uncommment for 2d grid
        for (int y = 0; y < MAPHEIGHT; y++) {
            for (int x = 0; x < MAPWIDTH; x++) {
                if (map[y][x])
                {
                    sf::RectangleShape r({TILE, TILE});
                    r.setPosition({static_cast<float>(x * TILE), static_cast<float>(y * TILE)});
                    r.setFillColor(sf::Color(80, 80, 80));
                    window.draw(r);
                }
            }
        }
        */

        // rays
        for (int i = 0; i < SCREENHEIGHT; i++)
        {
            int blockColor;

            double rayAngle = player.dir - FOV / 2 + FOV * i / SCREENHEIGHT;
            double d = castRay(player, map, rayAngle, blockColor);
            d *= cos(rad(rayAngle - player.dir));

            double h = (TILE * PROJECTIONDISTANCE) / d;
            sf::RectangleShape wall({1, (float)h});
            wall.setPosition({static_cast<float>(i),static_cast<float>(std::max((SCREENHEIGHT - h) / 2, 0.0))});


            if (blockColor == 0) {
                wall.setFillColor(sf::Color(0, 0, 0));      //empty
            } else if (blockColor == 1) {
                wall.setFillColor(sf::Color(255, 0, 0));    //red
            } else if (blockColor == 2) {
                wall.setFillColor(sf::Color(0, 255, 0));    //green
            } else if (blockColor == 3) {
                wall.setFillColor(sf::Color(0, 0, 255));    //blue
            } else if (blockColor == 4) {
                wall.setFillColor(sf::Color(255, 255, 0));  //yellow
            } else if (blockColor == 5) {
                wall.setFillColor(sf::Color(255, 0, 255));  //magenta
            } else if (blockColor == 6) {
                wall.setFillColor(sf::Color(0, 255, 255));  //cyan
            } else if (blockColor == 7) {
                wall.setFillColor(sf::Color(255, 165, 0));  //orange
            }

            window.draw(wall);
        }

        player.shape.setPosition({static_cast<float>(player.x),static_cast<float>(player.y)});

        //uncomment to draw player
        //window.draw(player.shape);

        window.display();
    }
    return 0;
}

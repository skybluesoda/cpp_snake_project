#include <deque>
#include <iostream>
#include <ncurses.h>

// Position struct
struct Position
{
    int x;
    int y;
};

// Snake 클래스
class Snake
{
  private:
    std::deque<Position> body;

  public:
    Snake()
    {
        body.push_back({5, 5});
    }

    void move(int dx, int dy)
    {
        Position head = body.front();
        head.x += dx;
        head.y += dy;
        body.push_front(head);
        body.pop_back();
    }

    void draw() const
    {
        for (const auto& segment : body)
        {
            mvaddch(segment.y, segment.x, 'O');
        }
    }
};

// Map 클래스 (지금은 아주 간단히)
class Map
{
  public:
    void draw() const
    {
        for (int i = 0; i < 20; i++)
        {
            mvaddch(0, i, '#');
            mvaddch(10, i, '#');
        }
        for (int i = 0; i < 10; i++)
        {
            mvaddch(i, 0, '#');
            mvaddch(i, 20, '#');
        }
    }
};

// 메인 게임 루프
int main()
{
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    Map map;
    Snake snake;

    int dx = 1, dy = 0;

    while (true)
    {
        clear();
        map.draw();
        snake.move(dx, dy);
        snake.draw();
        refresh();
        napms(200);

        int ch = getch();
        if (ch == 'q')
            break;
        else if (ch == KEY_UP)
        {
            dx = 0;
            dy = -1;
        }
        else if (ch == KEY_DOWN)
        {
            dx = 0;
            dy = 1;
        }
        else if (ch == KEY_LEFT)
        {
            dx = -1;
            dy = 0;
        }
        else if (ch == KEY_RIGHT)
        {
            dx = 1;
            dy = 0;
        }
    }

    endwin();
    return 0;
}

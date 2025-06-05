#include <deque>
#include <iostream>
#include <ncurses.h>
#include <vector>
using namespace std;

class Map
{
  private:
    int height, width;
    vector<vector<int>> map; // 동적으로 2차원 배열을 생성해야 합니다. 수동 관리인 new / delete
                             // 보다는 GC가 내장된 벡터를 사용했습니다.

  public:
    Map(int h = 21, int w = 21) : height(h), width(w), map(h, vector<int>(w, 0))
    {
        // 기본 크기는 최소 크기인 21입니다. 이니셜라이저 리스트로 높이, 너비와 맵을 생성합니다.
        initMap();
    }

    void initMap()
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                if (i == 0 || i == height - 1 || j == 0 || j == width - 1)
                    map[i][j] = 1; // Wall
                else
                    map[i][j] = 0; // 빈 공간
            }
        }
        // 이하 Immune Wall
        map[0][0] = 2;
        map[0][width - 1] = 2;
        map[height - 1][0] = 2;
        map[height - 1][width - 1] = 2;
    }

    void draw()
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                char ch;
                if (map[i][j] == 1)
                    ch = '#';
                else if (map[i][j] == 2)
                    ch = '@';
                else
                    ch = ' ';
                mvaddch(i, j, ch);
            }
        }
    }
    bool isWall(int y, int x) const
    {
        return map[y][x] == 1;
    } // 충돌 검사를 위한 함수
};

class Snake
{
  private:
    deque<pair<int, int>> body; // 머리부터 꼬리까지 좌표 저장
    int direction;              // 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT

  public:
    Snake(int startY, int startX, int startLen)
    {
        for (int i = 0; i < startLen; i++)
        {
            body.push_back({startY, startX - i});
        }
        direction = 1; // 초기 방향은 RIGHT로 가정
    }

    bool changeDirection(int newDir)
    {
        // 이하 if는 반대 방향 입력의 경우입니다.
        if ((direction == 0 && newDir == 2) || (direction == 2 && newDir == 0) ||
            (direction == 1 && newDir == 3) || (direction == 3 && newDir == 1))
        {
            return false;
        }
        direction = newDir; // direction을 입력한 방향으로 변경
        return true;
    }

    void move()
    {
        pair<int, int> newHead = getNextHeadPos(); // 다음 머리 위치 좌표
        body.push_front(newHead);                  // 다음 머리 위치에 머리 추가
        body.pop_back();                           // 머리를 추가했으니 꼬리 한칸 삭제
        // 움직이면서 아이템을 먹었을 때, body에 push 되는 순간 몸 길이를 반영할 때 오류 가능성이
        // 있습니다.
    }

    pair<int, int> getNextHeadPos()
    {
        // y, x : 머리의 현재 y좌표, x좌표
        int y = body.front().first;
        int x = body.front().second;
        // 방향에 따라 y, x 조절
        if (direction == 0)
            y--;
        else if (direction == 1)
            x++;
        else if (direction == 2)
            y++;
        else if (direction == 3)
            x--;

        return {y, x};
    }

    bool isBody(int y, int x)
    {
        // 주어진 좌표가 몸통과 충돌하는지 검사합니다.
        for (auto coordi : body)
        {
            if ((coordi.first == y) && (coordi.second == x))
                return true;
        }
        return false;
    }

    deque<pair<int, int>> getBody() const
    {
        return body;
    } // snake body에 대한 getter function입니다
};

int main()
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);   // escape sequence 형태의 입력을 매크로 상수로 변환합니다.
                            // 현재 코드에서는 불필요하지만 확장을 위해 남겨두었습니다.
    nodelay(stdscr, TRUE);  // 입력을 기다리지 않고 시간이 지나면 다음 상황으로 넘어갑니다.
    Map gameMap(25, 25);    // 25 * 25 크기의 맵을 생성합니다. (동적 할당이므로 확장 가능)
    Snake snake(12, 12, 5); // (12, 12) 좌표에 길이 5의 snake를 생성합니다.

    while (true)
    {
        int input = getch();
        if (input != ERR)
        {
            if (!snake.changeDirection(input))
            {
                // 반대방향 입력시 루프를 종료합
                break;
            }

            if (input == KEY_UP)
                snake.changeDirection(0);
            else if (input == KEY_RIGHT)
                snake.changeDirection(1);
            else if (input == KEY_DOWN)
                snake.changeDirection(2);
            else if (input == KEY_LEFT)
                snake.changeDirection(3);
            else
            {
                // 방향키 이외에 입력에 대해 예외 처리 필요할 수 있습니다
            }
        }

        auto nextHeadPos = snake.getNextHeadPos(); // 다음 머리 위치
        if (gameMap.isWall(nextHeadPos.first, nextHeadPos.second))
            break; // 다음 머리 위치가 벽이면 중단
        if (snake.isBody(nextHeadPos.first, nextHeadPos.second))
            break; // 다음 머리 위치가 몸통이면 중단

        snake.move();

        clear();

        gameMap.draw();
        for (int i = 0; i < snake.getBody().size(); i++)
        {
            int y = snake.getBody()[i].first;
            int x = snake.getBody()[i].second;

            if (i == 0)
                mvaddch(y, x, 'O'); // 머리
            else
                mvaddch(y, x, 'o'); // 몸통
        }
        refresh();

        napms(1000); // 1000ms == 1 second
    }

    endwin(); // 종료

    return 0;
}

#include <deque>
#include <iostream>
#include <ncurses.h>
#include <vector>
#include <ctime>
#include <algorithm>
#include <cstdlib>
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
                int code = map[i][j];
                if (code == 1) ch = '#';
                else if (code == 2) ch = '@';
                else if (code == 5) { attron(COLOR_PAIR(1)); ch = '+'; }
                else if (code == 6) { attron(COLOR_PAIR(2)); ch = '-'; }
                else ch = ' ';

                mvaddch(i, j, ch);
                if (code == 5) attroff(COLOR_PAIR(1));
                if (code == 6) attroff(COLOR_PAIR(2));
            }
        }
    }

    bool isWall(int y, int x) const { return map[y][x] == 1; }
    int getCell(int y, int x) const { return map[y][x]; }
    void setCell(int y, int x, int val) { map[y][x] = val; }
};

class Snake 
{
    private:
        deque<pair<int, int>> body; // 머리부터 꼬리까지 좌표 저장
        int direction;  // 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT

    public:
        Snake(int y, int x, int len) 
        {
            for (int i = 0; i < len; i++){

                body.push_back({y, x - i});
            } 
            direction = 1;// 초기 방향은 RIGHT로 가정
        }

        bool changeDirection(int newDir) 
        {
            // 이하 if는 반대 방향 입력의 경우입니다.
            if ((direction == 0 && newDir == 2) || (direction == 2 && newDir == 0) ||
                (direction == 1 && newDir == 3) || (direction == 3 && newDir == 1))
            {
                return false;
            }
            direction = newDir;// direction을 입력한 방향으로 변경
            return true;
        }

    void move() {
        body.push_front(getNextHeadPos());
        body.pop_back();
    }

    void grow() { //몸통 길이를 늘립니다
        body.push_front(getNextHeadPos());
    }

    void shrink() { //몸통 길이를 줄입니다
        if (!body.empty()) body.pop_back();
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
        for (auto& part : body)
        {
            if (part.first == y && part.second == x) 
                return true;
        }
        return false;
    }

    deque<pair<int, int>> getBody() const 
    { 
        return body; 
    } // Snake의 현재 몸 길이(= body의 크기)를 반환합니다
    int getLength() const { return body.size(); }
};

struct Item {
    int y, x, type; // 5=Growth, 6=Poison
    time_t timestamp;
};

int main() {
    initscr(); 
    noecho(); 
    cbreak(); 
    curs_set(0);
    keypad(stdscr, TRUE); // escape sequence 형태의 입력을 매크로 상수로 변환합니다.
                         // 현재 코드에서는 불필요하지만 확장을 위해 남겨두었습니다.
    nodelay(stdscr, TRUE);// 입력을 기다리지 않고 시간이 지나면 다음 상황으로 넘어갑니다.
    
    start_color(); //색상 모드를 활성화합니다.
    init_pair(1, COLOR_BLUE, COLOR_BLACK); 
    init_pair(2, COLOR_RED, COLOR_BLACK);

    Map gameMap(25, 25);// 25 * 25 크기의 맵을 생성합니다. (동적 할당이므로 확장 가능)
    Snake snake(12, 12, 5);// (12, 12) 좌표에 길이 5의 snake를 생성합니다.
    
    vector<Item> items;

    auto placeItem = [&](int type) {
        int tries = 0; // 무한 루프 방지를 위한 시도 횟수 제한
        while (tries++ < 100) {
            int y = rand() % 23 + 1;
            int x = rand() % 23 + 1;
            
            // 해당 위치가 빈 칸이고 Snake 몸이 없는 곳일 경우
            if (gameMap.getCell(y, x) == 0 && !snake.isBody(y, x)) {
                // 맵에 아이템을 배치하고
                gameMap.setCell(y, x, type);
                // 아이템 목록에도 저장 (시간 포함)
                items.push_back({y, x, type, time(NULL)});
                break; // 성공적으로 배치했으면 반복 종료
            }
        }
    };

    while (true) 
    {
        int ch = getch();
        if (ch != ERR) 
        {
            int newDir = -1;
            if (ch == KEY_UP) newDir = 0;
            else if (ch == KEY_RIGHT) newDir = 1;
            else if (ch == KEY_DOWN) newDir = 2;
            else if (ch == KEY_LEFT) newDir = 3;
            // 반대 방향 입력 시 게임 종료
            if (newDir != -1 && !snake.changeDirection(newDir)) break;
        }

        pair<int, int> next = snake.getNextHeadPos();
        // 벽 충돌 시 게임 종료
        if (gameMap.isWall(next.first, next.second)) 
            break;
        // 몸통 충돌 시 게임 종료
        if (snake.isBody(next.first, next.second)) 
            break;

        int cell = gameMap.getCell(next.first, next.second);
        if (cell == 5) { // 성장 아이템을 먹은 경우
            snake.grow();
            gameMap.setCell(next.first, next.second, 0); // 맵에서 아이템 제거
            items.erase(std::remove_if(items.begin(), items.end(), [&](Item& it) {
                return it.y == next.first && it.x == next.second;
            }), items.end());
        } 
        else if (cell == 6) { // 독 아이템을 먹은 경우
            snake.move();
            snake.shrink();
            gameMap.setCell(next.first, next.second, 0); // 맵에서 아이템 제거
            items.erase(std::remove_if(items.begin(), items.end(), [&](Item& it) {
                return it.y == next.first && it.x == next.second;
            }), items.end());
            if ((snake.getLength() - 1) <= 2) break; // 몸통 ≤ 2 이면 종료
        } else {
            snake.move(); // 아무 아이템도 없는 경우 일반 이동
        }

        // 5초 이상 된 아이템 제거
        time_t now = time(NULL);
        for (auto it = items.begin(); it != items.end();) {
            if (difftime(now, it->timestamp) >= 5.0) {
                gameMap.setCell(it->y, it->x, 0);
                it = items.erase(it);
            } else ++it;
        }
        // 아이템이 3개 미만이면 새로운 아이템 배치
        while (items.size() < 3) {
            int type = (rand() % 2 == 0) ? 5 : 6;
            placeItem(type);
        }

        clear();
        gameMap.draw();

        auto body = snake.getBody(); 
        for (size_t i = 0; i < body.size(); i++) {
            int y = body[i].first, x = body[i].second;
            mvaddch(y, x, (i == 0 ? 'O' : 'o'));
        }

        refresh();

        napms(1000); // 1000ms == 1 second
    }

    endwin(); // 종료

    return 0;
}

#include <deque>
#include <iostream>
#include <ncurses.h>
#include <vector>
#include <ctime>
#include <algorithm>
#include <cstdlib>
using namespace std;

// Gate 정보 구조체
struct Gate {
    pair<int, int> pos1; // 첫 번째 게이트 위치
    pair<int, int> pos2; // 두 번째 게이트 위치
    int original_map_val1; // 첫 번째 게이트의 원래 맵 값
    int original_map_val2; // 두 번째 게이트의 원래 맵 값
    bool active; // 게이트 활성화 여부
    time_t timestamp; // 게이트 생성 시간
    bool passed_by_snake; // 뱀 통과 여부
};

class Map
{
  private:
    int height, width;
    vector<vector<int>> map; // 동적으로 2차원 배열을 생성해야 합니다. 수동 관리인 new / delete
                             // 보다는 GC가 내장된 벡터를 사용했습니다.
    Gate currentGate;                             

  public:
    Map(int h = 21, int w = 21) : height(h), width(w), map(h, vector<int>(w, 0))
    {
        // 기본 크기는 최소 크기인 21입니다. 이니셜라이저 리스트로 높이, 너비와 맵을 생성합니다.
        initMap();
        currentGate.active = false; // 게이트 초기 비활성화
        currentGate.passed_by_snake = false; // 뱀 통과 플래그 초기화 
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
                else if (code == 7) { attron(COLOR_PAIR(3)); ch = 'G'; }
                else ch = ' ';

                mvaddch(i, j, ch);
                if (code == 5) attroff(COLOR_PAIR(1));
                if (code == 6) attroff(COLOR_PAIR(2));
                if (code == 7) attroff(COLOR_PAIR(3));
            }
        }
    }

    bool isWall(int y, int x) const { return map[y][x] == 1|| map[y][x] == 2; } //면역 벽도 추가로 설정정
    int getCell(int y, int x) const { return map[y][x]; }
    void setCell(int y, int x, int val) { map[y][x] = val; }
    bool placeGate(time_t now) {
        if (currentGate.active) return false; // 게이트가 이미 활성화된 경우

        vector<pair<int, int>> wall_positions;
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                if (map[i][j] == 1) { // 일반 벽만 게이트로 변환 가능
                    wall_positions.push_back({i, j});
                }
            }
        }
    if (wall_positions.size() < 2) return false; // 게이트 생성에 필요한 벽이 부족한 경우

        random_shuffle(wall_positions.begin(), wall_positions.end());

        // 첫 번째 게이트 위치 설정
        currentGate.pos1 = wall_positions[0];
        currentGate.original_map_val1 = map[currentGate.pos1.first][currentGate.pos1.second];
        map[currentGate.pos1.first][currentGate.pos1.second] = 7; // 맵에 게이트 값 설정

        // 두 번째 게이트 위치 설정
        currentGate.pos2 = wall_positions[1];
        currentGate.original_map_val2 = map[currentGate.pos2.first][currentGate.pos2.second];
        map[currentGate.pos2.first][currentGate.pos2.second] = 7; // 맵에 게이트 값 설정

        currentGate.active = true;
        currentGate.timestamp = now; // 생성 시간 기록
        currentGate.passed_by_snake = false; // 뱀 통과 플래그 초기화
        return true;
    }
     void removeGate() { // 게이트 제거
        if (currentGate.active) {
            map[currentGate.pos1.first][currentGate.pos1.second] = currentGate.original_map_val1;
            map[currentGate.pos2.first][currentGate.pos2.second] = currentGate.original_map_val2;
            currentGate.active = false;
        }
    }
    bool isGate(int y, int x) const { return map[y][x] == 7; } // 특정 좌표가 게이트인지 확인
    Gate getGate() const { return currentGate; } // 현재 게이트 정보 반환
    void setGatePassedBySnake(bool status) { currentGate.passed_by_snake = status; } // 뱀 통과 플래그 설정
    bool getGatePassedBySnake() const { return currentGate.passed_by_snake; } // 뱀 통과 플래그 가져오기
    int getHeight() const { return height; } // 맵 높이 반환
    int getWidth() const { return width; }   // 맵 너비 반환
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
    int getDirection() const { return direction; } // 현재 방향 반환 함수 추가

    // 게이트 통과 후 위치 및 방향 설정
    void teleport(pair<int, int> newPos, int newDir) {
        body.front() = newPos; // 머리 위치 변경
        direction = newDir; // 방향 변경
    }
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
    init_pair(3, COLOR_MAGENTA, COLOR_BLACK);

    Map gameMap(25, 25);// 25 * 25 크기의 맵을 생성합니다. (동적 할당이므로 확장 가능)
    Snake snake(12, 12, 4);// (12, 12) 좌표에 길이 5의 snake를 생성합니다.
    
    vector<Item> items;

    auto placeItem = [&](int type) {
        int tries = 0; // 무한 루프 방지를 위한 시도 횟수 제한
        while (tries++ < 100) {
            int y = rand() % (gameMap.getHeight()-2 ) + 1;
            int x = rand() % (gameMap.getWidth()-2 ) + 1;
            
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

    time_t lastGateEventTime; // 마지막 게이트 이벤트 발생 시간
    bool initialGateSpawned = false; // 초기 게이트 생성 플래그
    const int GATE_INITIAL_DELAY_SEC = 5; // 최초 게이트 생성까지 5초 대기
    const int GATE_REGEN_DELAY_SEC = 5; // 게이트 사라진 후 5초 뒤 재생성
    const int GATE_LIFETIME_SEC = 10; // 게이트 최대 유지 시간

    while (true) 
    {

        time_t now = time(NULL);

        // 게임 시작 후 최초 게이트 생성 타이머 초기화
        if (!initialGateSpawned) {
            lastGateEventTime = now; // 게임 시작 시간 기록
            initialGateSpawned = true;
        }

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
        
        // 몸통 충돌 시 게임 종료
        if (snake.isBody(next.first, next.second)) 
            break;

        if (gameMap.isGate(next.first, next.second)) {
            Gate currentGateInfo = gameMap.getGate();
            pair<int, int> entryGatePos; // 뱀이 진입한 게이트 위치
            pair<int, int> exitGatePos;  // 뱀이 나갈 게이트 위치

            // 뱀이 어느 게이트로 진입했는지 확인
            if (next.first == currentGateInfo.pos1.first && next.second == currentGateInfo.pos1.second) {
                entryGatePos = currentGateInfo.pos1;
                exitGatePos = currentGateInfo.pos2;
            } else {
                entryGatePos = currentGateInfo.pos2;
                exitGatePos = currentGateInfo.pos1;
            }

            // 게이트 진출 방향 결정 (Game Rule #4)
            int currentSnakeDir = snake.getDirection();
            int exitDir = -1; // 진출 방향

            // 가장자리 벽 게이트 (고정 진출)
            if (exitGatePos.first == 0) { // 상단 벽
                exitDir = 2; // DOWN
            } else if (exitGatePos.first == gameMap.getHeight() - 1) { // 하단 벽
                exitDir = 0; // UP
            } else if (exitGatePos.second == 0) { // 좌측 벽
                exitDir = 1; // RIGHT
            } else if (exitGatePos.second == gameMap.getWidth() - 1) { // 우측 벽
                exitDir = 3; // LEFT
            } else { // 맵 가운데 벽 게이트
                // 진입 방향 우선, 시계방향, 반시계방향, 반대방향 순
                int possibleDirs[4] = {currentSnakeDir, (currentSnakeDir + 1) % 4, (currentSnakeDir + 3) % 4, (currentSnakeDir + 2) % 4};

                for(int dir_candidate : possibleDirs) {
                    pair<int, int> potentialExitPos = exitGatePos;
                    // 다음 진출 위치 계산
                    if (dir_candidate == 0) potentialExitPos.first--; // UP
                    else if (dir_candidate == 1) potentialExitPos.second++; // RIGHT
                    else if (dir_candidate == 2) potentialExitPos.first++; // DOWN
                    else if (dir_candidate == 3) potentialExitPos.second--; // LEFT

                    // 다음 위치가 벽이 아니고, 뱀 몸통이 아니며 맵 경계를 벗어나지 않는 경우
                    if (potentialExitPos.first >= 0 && potentialExitPos.first < gameMap.getHeight() &&
                        potentialExitPos.second >= 0 && potentialExitPos.second < gameMap.getWidth() &&
                        gameMap.getCell(potentialExitPos.first, potentialExitPos.second) == 0 &&
                        !snake.isBody(potentialExitPos.first, potentialExitPos.second)) {
                        exitDir = dir_candidate;
                        break;
                    }
                }
                // 나갈 곳이 없는 경우 게임 오버
                if (exitDir == -1) {
                    break;
                }
            }

            // 게이트 통과 처리
            snake.teleport(exitGatePos, exitDir); // 뱀 순간이동 및 방향 변경
            

            // 뱀이 지나가면 게이트 즉시 사라짐
            gameMap.removeGate();
            gameMap.setGatePassedBySnake(true); // 뱀 통과 플래그 설정
            lastGateEventTime = now; // 게이트 사라진 시간 기록 (다음 게이트 생성 타이머 시작)
        }
            // 벽 충돌 시 게임 종료
        else if (gameMap.isWall(next.first, next.second)) 
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
        double time_since_last_event = difftime(now, lastGateEventTime);

        if (!gameMap.getGate().active) { // 게이트가 현재 비활성화 상태일 때
            if (time_since_last_event >= GATE_REGEN_DELAY_SEC) { // 사라진 후 5초 뒤에 다시 생성
                gameMap.placeGate(now);
                lastGateEventTime = now; // 새로운 생성 시간 기록
            }
        } else { // 게이트가 현재 활성화 상태일 때
            // 게이트가 10초 동안 유지되다가 뱀이 지나가지 않으면 사라짐
            if (time_since_last_event >= GATE_LIFETIME_SEC && !gameMap.getGatePassedBySnake()) {
                gameMap.removeGate();
                lastGateEventTime = now; // 게이트 사라진 시간 기록 (다음 게이트 생성 타이머 시작)
            }
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

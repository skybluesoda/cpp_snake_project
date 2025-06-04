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
                    ch = 'O';
                mvaddch(i, j, ch);
            }
        }
    }
};

int main()
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE); // escape sequence 형태의 입력을 매크로 상수로 변환합니다. 
                          // 현재 코드에서는 불필요하지만 확장을 위해 남겨두었습니다.

    Map gameMap(25, 25); // 25 * 25 크기의 맵을 생성합니다. (동적 할당이므로 확장 가능)

    gameMap.draw(); // 맵을 출력합니다
    refresh();

    getch(); // 키 입력받을 준비

    endwin(); // 종료

    return 0;
}

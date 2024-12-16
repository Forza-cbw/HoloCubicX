/***************************************************
  2048 APP

  聚合多种APP，内置天气、时钟、相册、特效动画、视频播放、视频投影、
  浏览器文件修改。（各APP具体使用参考说明书）

  Github repositories：https://github.com/AndyXFuture/HoloCubic-2048-anim

  Last review/edit by AndyXFuture: 2021/11/12
 ****************************************************/

#include "game2048_contorller.h"
#include <Arduino.h>

/*
 * 随机刷新一个2
 * 返回刷新的位置
 */
int GAME2048::addRandom(void)
{
    int rand;
    while (1)
    {
        rand = random(3200) % 16;
        if (this->board[rand / 4][rand % 4] == 0)
        {
            this->board[rand / 4][rand % 4] = 2;
            log_i("rand: %d,%d", rand/4, rand%4);
            break;
        }
    }
    return rand;
}

/*
 *   初始化Location数组，用于计算棋盘变化
 *   有数字则按方向填入ABCD
 *   direction  1.上 2.下 3.左 4.右
 */
void GAME2048::initLocation(int direction)
{
    log_i("direction[%d]", direction);
    for (int i = 0; i < SCALE_SIZE; i++)
    {
        for (int j = 0; j < SCALE_SIZE; j++)
        {
            //有数字根据方向填入ABCD
            if (direction <= 2) { // 上下
                Location[i][j] = (board[i][j] != 0 ? std::string(1, 'A' + i) : "");
            } else { // 左右
                Location[i][j] = (board[i][j] != 0 ? std::string(1, 'A' + j) : "");
            }
        }
        log_i("|%s|%s|%s|%s", Location[i][0].c_str(), Location[i][1].c_str(), Location[i][2].c_str(), Location[i][3].c_str());
    }
}

/*
 *   通过解析Location变量的变化，获取并记录移动距离和合并位置
 *   direction  1.上 2.下 3.左 4.右
 *   >4则有合并,-8则是移动的值
 *   <4则直接就是移动的值
 */
void GAME2048::countMoveRecord(int direction)
{

    //清空
    for (int i = 0; i < SCALE_SIZE; i++)
    {
        for (int j = 0; j < SCALE_SIZE; j++)
        {
            moveRecord[i][j] = 0;
        }
    }
    for (int i = 0; i < SCALE_SIZE; i++)
    {
        for (int j = 0; j < SCALE_SIZE; j++)
        {
            switch (direction)
            {
            case 1:
            case 2:
                //移动检测
                if (Location[i][j].find("A") != -1)
                {
                    moveRecord[0][j] += i;
                }
                if (Location[i][j].find("B") != -1)
                {
                    moveRecord[1][j] += i - 1;
                }
                if (Location[i][j].find("C") != -1)
                {
                    moveRecord[2][j] += i - 2;
                }
                if (Location[i][j].find("D") != -1)
                {
                    moveRecord[3][j] += i - 3;
                }
                break;
            case 3:
            case 4:
                //移动检测
                if (Location[i][j].find("A") != -1)
                {
                    moveRecord[i][0] += j;
                }
                if (Location[i][j].find("B") != -1)
                {
                    moveRecord[i][1] += j - 1;
                }
                if (Location[i][j].find("C") != -1)
                {
                    moveRecord[i][2] += j - 2;
                }
                if (Location[i][j].find("D") != -1)
                {
                    moveRecord[i][3] += j - 3;
                }
                break;
            }
            //合并检测
            if (Location[i][j].length() == 2)
            {
                moveRecord[i][j] += 8;
            }
        }
    }
}

void GAME2048::moveOnce(int i, int j, int di, int dj) {
    if (board[i][j] == 0)
    {
        board[i][j] = board[i + di][j + dj];
        board[i + di][j + dj] = 0;
        //动画移动轨迹记录
        Location[i][j] = Location[i + di][j + dj];
        Location[i + di][j + dj] = "";
    }
}

void GAME2048::mergeOnce(int i, int j, int di, int dj) {
    if (board[i][j] == board[i + di][j + dj])
    {
        board[i][j] *= 2;
        board[i + di][j + dj] = 0;
        //动画合并轨迹记录
        Location[i][j].append(Location[i + di][j + dj]);
        Location[i + di][j + dj] = "";
    }
}

//  direction  1.上 2.下 3.左 4.右
void GAME2048::moveAndMerge(int direction) {
    initLocation(direction);
    recordBoard();     //记录数值

    int di = direction <= 2 ? 3 - 2*direction : 0; // 上1，下-1，其他0
    int dj = direction >= 3 ? 7 - 2*direction : 0; // 左1，右-1，其他0

    int diffMap[4][6] = { // 每一行为{startI,endI,stepI,startJ,endJ,stepJ}
        {0,SCALE_SIZE-1,1,0,SCALE_SIZE,1},  //上
        {SCALE_SIZE-1,0,-1,0,SCALE_SIZE,1}, //下
        {0,SCALE_SIZE,1,0,SCALE_SIZE-1,1},  //左
        {0,SCALE_SIZE,1,SCALE_SIZE-1,0,-1}}; //右

    int startI = diffMap[direction-1][0];
    int endI = diffMap[direction-1][1];
    int stepI = diffMap[direction-1][2];

    int startJ = diffMap[direction-1][3];
    int endJ = diffMap[direction-1][4];
    int stepJ = diffMap[direction-1][5];

    //移动2次
    for (int n = 0; n < 2; n++)
        for (int i = startI; i != endI; i += stepI)
            for (int j = startJ; j != endJ; j += stepJ)
                moveOnce(i, j, di, dj);
    //合并
    for (int i = startI; i != endI; i += stepI)
        for (int j = startJ; j != endJ; j += stepJ)
            mergeOnce(i, j, di, dj);
    //移动1次
    for (int i = startI; i != endI; i += stepI)
        for (int j = startJ; j != endJ; j += stepJ)
            moveOnce(i, j, di, dj);

    countMoveRecord(direction);
}


/*
 * judge()判断当前游戏状态
 * 返回0：游戏可以继续
 * 返回1：游戏获胜
 * 返回2：游戏无法继续，失败
 */
int GAME2048::judge(void)
{
    //判赢
    for (int i = 0; i <= SCALE_SIZE * SCALE_SIZE; i++)
    {
        if (board[i / 4][i % 4] >= WIN_SCORE)
        {
            return 1; // Win
        }
    }
    //判空
    for (int i = 0; i <= SCALE_SIZE * SCALE_SIZE; i++)
    {
        if (board[i / 4][i % 4] == 0)
        {
            return 0;
        }
    }

    //判相邻相同
    for (int i = 0; i < SCALE_SIZE; i++)
    {
        for (int j = 0; j < SCALE_SIZE; j++)
        {
            if (i < 3)
            {
                if (board[i][j] == board[i + 1][j])
                {
                    return 0;
                }
            }
            if (j < 3)
            {
                if (board[i][j] == board[i][j + 1])
                {
                    return 0;
                }
            }
        }
    }

    return 2; // Defeatd
}


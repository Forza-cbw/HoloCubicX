#ifndef GAME2048_H
#define GAME2048_H

#include <iostream>
// #include <stdio.h>
using namespace std;

#include <Arduino.h>

#define SCALE_SIZE 4
#define WIN_SCORE 2048

class GAME2048
{
private:
    int board[4][4];
    int previous[4][4];
    string Location[4][4];
    int moveRecord[4][4];
    bool dstNeedZoom[4][4]; // 方块移动的目的地，是否需要播放zoom动画

public:
    void init()
    {
        for (int i = 0; i < SCALE_SIZE * SCALE_SIZE; i++)
        {
            this->board[i / 4][i % 4] = 0;
            this->previous[i / 4][i % 4] = 0;
            this->moveRecord[i / 4][i % 4] = 0;
        }
        // addRandom();
        // addRandom();
    };
    int addRandom(void);
    int judge(void);
    void initLocation(int direction);
    void countMoveRecord(int direction);

    void mergeOnce(int i, int j, int di, int dj);
    void moveOnce(int i, int j, int di, int dj);
    void moveAndMerge(int direction);

    void recordBoard()
    {
        for (int i = 0; i < SCALE_SIZE * SCALE_SIZE; i++)
        {
            this->previous[i / 4][i % 4] = this->board[i / 4][i % 4];
        }
    };

    int isChanged()
    {
        //判断移动后是否相同
        int x = 0;
        for (int i = 0; i < SCALE_SIZE * SCALE_SIZE; i++)
        {
            if (this->board[i / 4][i % 4] == this->previous[i / 4][i % 4])
                x++;
        }

        if (x >= 16)
        {
            return 0;
        }
        return 1;
    };
    int *getBoard()
    {
        return &this->board[0][0];
    };
    string *getLocation()
    {
        return &this->Location[0][0];
    };
    int (*getMoveRecord())[4]
    {
        return this->moveRecord;
    };
    bool (*getDstNeedZoom())[4]
    {
        return this->dstNeedZoom;
    };
};

#endif

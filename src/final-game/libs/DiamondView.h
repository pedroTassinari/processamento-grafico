#ifndef DiamondView_h
#define DiamondView_h

#include "TilemapView.h"
#include <cmath>

class DiamondView : public TilemapView
{
public:
    int offsetCol = 2;
    int offsetRow = 2;

    void computeDrawPosition(const int col, const int row, const float tw, const float th, float &targetx, float &targety) const override
    {
        targetx = (col - row) * (tw / 2.0f);
        targety = (col + row) * (th / 2.0f);
    }

    void computeMouseMap(int &col, int &row, const float tw, const float th, const float mx, const float my) const override
    {
        float halfW = tw / 2.0f;
        float halfH = th / 2.0f;

        float tempCol = (mx / halfW + my / halfH) / 2.0f;
        float tempRow = (my / halfH - mx / halfW) / 2.0f;

        col = static_cast<int>(std::floor(tempCol)) - offsetCol;
        row = static_cast<int>(std::floor(tempRow)) - offsetRow;
    }

    void computeTileWalking(int &col, int &row, const int direction) const override
    {
        switch (direction)
        {
        case 0: // Sul
            row--;
            col--;
            break;
        case 1: // Nordeste
            row--;
            break;
        case 2: // Leste
            row--; 
            col++;
            break;
        case 3: // Sudeste
            col++;
            break;
        case 4: // Norte
            row++;
            col++;
            break;
        case 5: // Sudoeste
            col--;
            break;
        case 6: // Oeste
            row++;
            col--;
            break;
        case 7: // Noroeste
            row++;
            break;
        }
    }
};

#endif /* DiamondView_h */

#pragma once

#include <math.h>

#include "neuphony_board.h"

#define ADS1299_Vref 4.5
#define ADS1299_gain 24.0

class Neuphony : public NeuphonyBoard
{
    double eeg_scale = (double)(ADS1299_Vref / float ((pow (2, 23) - 1)) / ADS1299_gain * 1000000.);
    double accel_scale = (double)(8.0/ 4095);

protected:
    void read_thread ();

public:
    Neuphony (struct BrainFlowInputParams params)
        : NeuphonyBoard (params, (int)BoardIds::NEUPHONY)
    {
    }
};

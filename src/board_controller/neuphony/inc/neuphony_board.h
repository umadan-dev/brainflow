#pragma once

#include <thread>

#include "board.h"
#include "board_controller.h"

class Neuphony : public Board
{
    private:
        volatile bool keep_alive;
        bool initialized;
        bool is_streaming;
        std::thread streaming_thread;

        void read_thread ();

    public:
        Neuphony (struct BrainFlowInputParams params);
        ~Neuphony ();

        int prepare_session ();
        int start_stream (int buffer_size, const char *streamer_params);
        int stop_stream ();
        int release_session ();
        int config_board (std::string config, std::string &response);
};
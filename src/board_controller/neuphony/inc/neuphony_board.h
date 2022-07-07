#pragma once

#include <thread>

#include "board.h"
#include "board_controller.h"
#include "serial.h"

class NeuphonyBoard : public Board
{
    protected:
    volatile bool keep_alive;
    bool initialized;
    bool is_streaming;
    std::thread streaming_thread;

    Serial *serial;

    virtual int open_port ();
    virtual int status_check ();
    virtual int set_port_settings ();
    virtual void read_thread () = 0;
    virtual int send_to_board (const char *msg);
    virtual int send_to_board (const char *msg, std::string &response);
    virtual std::string read_serial_response ();

    public:
        NeuphonyBoard (struct BrainFlowInputParams params, int board_id);
        ~NeuphonyBoard ();

        int prepare_session ();
        int start_stream (int buffer_size, const char *streamer_params);
        int stop_stream ();
        int release_session ();
        int config_board (std::string config, std::string &response);
};
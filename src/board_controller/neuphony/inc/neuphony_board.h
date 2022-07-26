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
    virtual std::string read_serial_response ();

    public:
        NeuphonyBoard (struct BrainFlowInputParams params, int board_id);
        virtual ~NeuphonyBoard ();

        virtual int prepare_session ();
        virtual int start_stream (int buffer_size, const char *streamer_params);
        virtual int stop_stream ();
        virtual int release_session ();
        virtual int config_board (std::string config, std::string &response);
};
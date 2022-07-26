#include <string.h>

#include "neuphony_board.h"
#include "serial.h"
#ifdef _WIN32
#include "windows_registry.h"
#endif

NeuphonyBoard::NeuphonyBoard (struct BrainFlowInputParams params, int board_id)
    : Board (board_id, params)
{
    serial = NULL;
    is_streaming = false;
    keep_alive = false;
    initialized = false;
}

NeuphonyBoard::~NeuphonyBoard ()
{
    skip_logs = true;
    release_session ();
}

int NeuphonyBoard::open_port ()
{
    if (serial->is_port_open ())
    {
        safe_logger (spdlog::level::err, "port {} already open", serial->get_port_name ());
        return (int)BrainFlowExitCodes::PORT_ALREADY_OPEN_ERROR;
    }

    safe_logger (spdlog::level::info, "opening port {}", serial->get_port_name ());
    int res = serial->open_serial_port ();
    if (res < 0)
    {
        safe_logger (spdlog::level::err,
            "Make sure you provided correct port name and have permissions to open it(run with "
            "sudo/admin). Also, close all other apps using this port.");
        return (int)BrainFlowExitCodes::UNABLE_TO_OPEN_PORT_ERROR;
    }
    safe_logger (spdlog::level::trace, "port {} is open", serial->get_port_name ());
    return (int)BrainFlowExitCodes::STATUS_OK;
}

int NeuphonyBoard::config_board (std::string config, std::string &response)
{
    safe_logger (spdlog::level::err, "Neuphony doesn't support config_board");
    return (int)BrainFlowExitCodes::UNSUPPORTED_BOARD_ERROR;
}


int NeuphonyBoard::send_to_board (const char *msg)
{
    int length = (int)strlen (msg);
    safe_logger (spdlog::level::debug, "sending {} to the board", msg);
    int res = serial->send_to_serial_port ((const void *)msg, length);
    if (res != length)
    {
        return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
    }
    return (int)BrainFlowExitCodes::STATUS_OK;
}

int NeuphonyBoard::set_port_settings ()
{
    int res = serial->set_custom_baudrate (921600);
 
    if (res < 0)
    {
        safe_logger (spdlog::level::err, "Unable to set custom baud rate, res is {}", res);
        return (int)BrainFlowExitCodes::SET_PORT_ERROR;
    }
    safe_logger (spdlog::level::trace, "set port settings");
    return (int)BrainFlowExitCodes::STATUS_OK;
}

std::string NeuphonyBoard::read_serial_response ()
{
    constexpr int max_tmp_size = 500;
    unsigned char tmp_array[max_tmp_size];

    if (serial->read_from_serial_port (tmp_array, 450) > 0){
        serial->flush_buffer ();
    }
    tmp_array[450] = '\0';

    return std::string ((const char *)tmp_array);
}

int NeuphonyBoard::status_check ()
{
    char buf[40];
    int max_empty_seq = 5;
    int num_empty_attempts = 0;
    std::string resp = "";
    
    for (int i = 0; i < 10; i++)
    {
        int res = serial->read_from_serial_port (buf, 40);
        if (res > 0)
        {
            buf[39]='\0';
            const char* status = buf;
            num_empty_attempts = 0;
            if (strstr(status,"Please press \"x\" to enable Serial data") != NULL)
                serial->flush_buffer ();
                return (int)BrainFlowExitCodes::STATUS_OK;
        }
        else
        {
            num_empty_attempts++;
            if (num_empty_attempts > max_empty_seq)
            {
                safe_logger (spdlog::level::err, "board doesnt send welcome characters! Msg: {}",
                    resp.c_str ());
                return (int)BrainFlowExitCodes::BOARD_NOT_READY_ERROR;
            }
        }
    }
    return (int)BrainFlowExitCodes::BOARD_NOT_READY_ERROR;
}


int NeuphonyBoard::prepare_session ()
{
    if (initialized)
    {
        safe_logger (spdlog::level::info, "Session already prepared");
        return (int)BrainFlowExitCodes::STATUS_OK;
    }
    if (params.serial_port.empty ())
    {
        safe_logger (spdlog::level::err, "serial port is empty");
        return (int)BrainFlowExitCodes::INVALID_ARGUMENTS_ERROR;
    }
#ifdef _WIN32
    LONG res = set_ftdi_latency_in_registry (1, params.serial_port);
    if (res != ERROR_SUCCESS)
    {
        safe_logger (spdlog::level::warn,
            "failed to adjust latency param in ftdi driver automatically, reboot or dongle "
            "reconnection may be needed.");
    }
#endif
    serial = Serial::create (params.serial_port.c_str (), this);
    int port_open = open_port ();
    if (port_open != (int)BrainFlowExitCodes::STATUS_OK)
    {
        delete serial;
        serial = NULL;
        return port_open;
    }

    int set_settings = set_port_settings ();
    if (set_settings != (int)BrainFlowExitCodes::STATUS_OK)
    {
        delete serial;
        serial = NULL;
        return set_settings;
    }

    int initted = status_check ();
    if (initted != (int)BrainFlowExitCodes::STATUS_OK)
    {
        delete serial;
        serial = NULL;
        return initted;
    }
    int send_res = send_to_board ("x");
    if (send_res != (int)BrainFlowExitCodes::STATUS_OK)
    {
        return send_res;
    }

    initialized = true;
    return (int)BrainFlowExitCodes::STATUS_OK;
}

int NeuphonyBoard::start_stream (int buffer_size, const char *streamer_params)
{
    if (is_streaming)
    {
        safe_logger (spdlog::level::err, "Streaming thread already running");
        return (int)BrainFlowExitCodes::STREAM_ALREADY_RUN_ERROR;
    }
    int res = prepare_for_acquisition (buffer_size, streamer_params);
    if (res != (int)BrainFlowExitCodes::STATUS_OK)
    {
        return res;
    }

    // neuphony sends response back, clean serial buffer and analyze response
    std::string response = read_serial_response ();
    if (response.substr (0, 7).compare ("Failure") == 0)
    {
        safe_logger (spdlog::level::err,
            "Board config error, probably dongle is inserted but Neuphony is off.");
        safe_logger (spdlog::level::trace, "read {}", response.c_str ());
        delete serial;
        serial = NULL;
        return (int)BrainFlowExitCodes::BOARD_NOT_READY_ERROR;
    }

    int send_res = send_to_board ("c");
    if (send_res != (int)BrainFlowExitCodes::STATUS_OK)
    {
        return send_res;
    }
    keep_alive = true;
    streaming_thread = std::thread ([this] { this->read_thread (); });
    is_streaming = true;
    return (int)BrainFlowExitCodes::STATUS_OK;
}

int NeuphonyBoard::stop_stream ()
{
    if (is_streaming)
    {
        keep_alive = false;
        is_streaming = false;
        if (streaming_thread.joinable ())
        {
            streaming_thread.join ();
        }
        return send_to_board ("d");
    }
    else
    {
        return (int)BrainFlowExitCodes::STREAM_THREAD_IS_NOT_RUNNING;
    }
}

int NeuphonyBoard::release_session ()
{
    if (initialized)
    {
        if (is_streaming)
        {
            stop_stream ();
        }
        int stop_res = send_to_board ("y");
        if (stop_res != (int)BrainFlowExitCodes::STATUS_OK)
        {
            return stop_res;
        }
        free_packages ();
        initialized = false;
    }
    if (serial)
    {
        serial->close_serial_port ();
        delete serial;
        serial = NULL;
    }
    return (int)BrainFlowExitCodes::STATUS_OK;
}
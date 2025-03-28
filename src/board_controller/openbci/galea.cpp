#include "galea.h"

#include <chrono>
#include <stdint.h>
#include <string.h>

#include <numeric>
#include <regex>
#include <sstream>

#include "custom_cast.h"
#include "json.hpp"
#include "timestamp.h"

using json = nlohmann::json;

#ifndef _WIN32
#include <errno.h>
#endif

constexpr int Galea::max_bytes_in_transaction;
constexpr int Galea::exg_package_size;
constexpr int Galea::num_exg_packages;
constexpr int Galea::exg_transaction_size;
constexpr int Galea::aux_package_size;
constexpr int Galea::num_aux_packages;
constexpr int Galea::aux_transaction_size;
constexpr int Galea::socket_timeout;

Galea::Galea (struct BrainFlowInputParams params) : Board ((int)BoardIds::GALEA_BOARD, params)
{
    socket = NULL;
    is_streaming = false;
    keep_alive = false;
    initialized = false;
    state = (int)BrainFlowExitCodes::SYNC_TIMEOUT_ERROR;
    half_rtt = 0.0;
}

Galea::~Galea ()
{
    skip_logs = true;
    release_session ();
}

int Galea::prepare_session ()
{
    if (initialized)
    {
        safe_logger (spdlog::level::info, "Session is already prepared");
        return (int)BrainFlowExitCodes::STATUS_OK;
    }
    if ((params.timeout > 600) || (params.timeout < 1))
    {
        params.timeout = 5;
    }

    if (params.ip_address.empty ())
    {
        params.ip_address = find_device ();
        if (params.ip_address.empty ())
        {
            return (int)BrainFlowExitCodes::INVALID_ARGUMENTS_ERROR;
        }
    }
    socket = new SocketClientUDP (params.ip_address.c_str (), 2390);
    int res = socket->connect ();
    if (res != (int)SocketClientUDPReturnCodes::STATUS_OK)
    {
        safe_logger (spdlog::level::err, "failed to init socket: {}", res);
        delete socket;
        socket = NULL;
        return (int)BrainFlowExitCodes::GENERAL_ERROR;
    }

    safe_logger (spdlog::level::trace, "timeout for socket is {}", socket_timeout);
    socket->set_timeout (socket_timeout);
    // force default settings for device
    std::string tmp;
    std::string default_settings = "o"; // use demo mode with agnd
    res = config_board (default_settings, tmp);
    if (res != (int)BrainFlowExitCodes::STATUS_OK)
    {
        safe_logger (spdlog::level::err, "failed to apply default settings");
        delete socket;
        socket = NULL;
        return (int)BrainFlowExitCodes::BOARD_NOT_READY_ERROR;
    }
    // force default sampling rate - 250
    std::string sampl_rate = "~6";
    res = config_board (sampl_rate, tmp);
    if (res != (int)BrainFlowExitCodes::STATUS_OK)
    {
        safe_logger (spdlog::level::err, "failed to apply defaul sampling rate");
        delete socket;
        socket = NULL;
        return (int)BrainFlowExitCodes::BOARD_NOT_READY_ERROR;
    }
    initialized = true;
    return (int)BrainFlowExitCodes::STATUS_OK;
}

int Galea::config_board (std::string conf, std::string &response)
{
    if (socket == NULL)
    {
        safe_logger (spdlog::level::err, "You need to call prepare_session before config_board");
        return (int)BrainFlowExitCodes::BOARD_NOT_CREATED_ERROR;
    }
    // special handling for calc_time command
    if (conf == "calc_time")
    {
        if (is_streaming)
        {
            safe_logger (spdlog::level::err, "can not calc delay during the streaming.");
            return (int)BrainFlowExitCodes::BOARD_NOT_CREATED_ERROR;
        }
        int res = calc_time (response);
        return res;
    }

    const char *config = conf.c_str ();
    safe_logger (spdlog::level::debug, "Trying to config Galea with {}", config);
    int len = (int)strlen (config);
    int res = socket->send (config, len);
    if (len != res)
    {
        if (res == -1)
        {
#ifdef _WIN32
            safe_logger (spdlog::level::err, "WSAGetLastError is {}", WSAGetLastError ());
#else
            safe_logger (spdlog::level::err, "errno {} message {}", errno, strerror (errno));
#endif
        }
        safe_logger (spdlog::level::err, "Failed to config a board");
        return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
    }
    if (!is_streaming)
    {
        char b[Galea::max_bytes_in_transaction];
        res = Galea::exg_transaction_size;
        int max_attempt = 25; // to dont get to infinite loop
        int current_attempt = 0;
        while ((res == Galea::exg_transaction_size) || (res == Galea::aux_transaction_size))
        {
            res = socket->recv (b, Galea::max_bytes_in_transaction);
            if (res == -1)
            {
#ifdef _WIN32
                safe_logger (spdlog::level::err, "config_board recv ack WSAGetLastError is {}",
                    WSAGetLastError ());
#else
                safe_logger (spdlog::level::err, "config_board recv ack errno {} message {}", errno,
                    strerror (errno));
#endif
                return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
            }
            current_attempt++;
            if (current_attempt == max_attempt)
            {
                safe_logger (spdlog::level::err, "Device is streaming data while it should not!");
                return (int)BrainFlowExitCodes::STREAM_ALREADY_RUN_ERROR;
            }
        }
        // set response string
        for (int i = 0; i < res; i++)
        {
            response = response + b[i];
        }
        switch (b[0])
        {
            case 'A':
                return (int)BrainFlowExitCodes::STATUS_OK;
            case 'I':
                safe_logger (spdlog::level::err, "invalid command");
                return (int)BrainFlowExitCodes::INVALID_ARGUMENTS_ERROR;
            default:
                safe_logger (spdlog::level::warn, "unknown char received: {}", b[0]);
                return (int)BrainFlowExitCodes::STATUS_OK;
        }
    }

    return (int)BrainFlowExitCodes::STATUS_OK;
}

int Galea::start_stream (int buffer_size, const char *streamer_params)
{
    if (!initialized)
    {
        safe_logger (spdlog::level::err, "You need to call prepare_session before config_board");
        return (int)BrainFlowExitCodes::BOARD_NOT_CREATED_ERROR;
    }
    if (is_streaming)
    {
        safe_logger (spdlog::level::err, "Streaming thread already running");
        return (int)BrainFlowExitCodes::STREAM_ALREADY_RUN_ERROR;
    }

    // calc time before start stream
    std::string resp;
    for (int i = 0; i < 3; i++)
    {
        int res = calc_time (resp);
        if (res != (int)BrainFlowExitCodes::STATUS_OK)
        {
            return res;
        }
    }

    int res = prepare_for_acquisition (buffer_size, streamer_params);
    if (res != (int)BrainFlowExitCodes::STATUS_OK)
    {
        return res;
    }

    // start streaming
    res = socket->send ("b", 1);
    if (res != 1)
    {
        if (res == -1)
        {
#ifdef _WIN32
            safe_logger (spdlog::level::err, "WSAGetLastError is {}", WSAGetLastError ());
#else
            safe_logger (spdlog::level::err, "errno {} message {}", errno, strerror (errno));
#endif
        }
        safe_logger (spdlog::level::err, "Failed to send a command to board");
        return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
    }

    keep_alive = true;
    streaming_thread = std::thread ([this] { this->read_thread (); });
    // wait for data to ensure that everything is okay
    std::unique_lock<std::mutex> lk (this->m);
    auto sec = std::chrono::seconds (1);
    if (cv.wait_for (lk, 3 * sec,
            [this] { return this->state != (int)BrainFlowExitCodes::SYNC_TIMEOUT_ERROR; }))
    {
        this->is_streaming = true;
        return this->state;
    }
    else
    {
        safe_logger (spdlog::level::err, "no data received in 5sec, stopping thread");
        this->is_streaming = true;
        this->stop_stream ();
        return (int)BrainFlowExitCodes::SYNC_TIMEOUT_ERROR;
    }
}

int Galea::stop_stream ()
{
    if (is_streaming)
    {
        keep_alive = false;
        is_streaming = false;
        streaming_thread.join ();
        this->state = (int)BrainFlowExitCodes::SYNC_TIMEOUT_ERROR;
        int res = socket->send ("s", 1);
        if (res != 1)
        {
            if (res == -1)
            {
#ifdef _WIN32
                safe_logger (spdlog::level::err, "WSAGetLastError is {}", WSAGetLastError ());
#else
                safe_logger (spdlog::level::err, "errno {} message {}", errno, strerror (errno));
#endif
            }
            safe_logger (spdlog::level::err, "Failed to send a command to board");
            return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
        }

        // free kernel buffer
        unsigned char b[Galea::max_bytes_in_transaction];
        res = 0;
        int max_attempt = 25; // to dont get to infinite loop
        int current_attempt = 0;
        while (res != -1)
        {
            res = socket->recv (b, Galea::max_bytes_in_transaction);
            current_attempt++;
            if (current_attempt == max_attempt)
            {
                safe_logger (
                    spdlog::level::err, "Command 's' was sent but streaming is still running.");
                return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
            }
        }

        std::string resp;
        for (int i = 0; i < 3; i++)
        {
            res = calc_time (resp); // call it in the end once to print time in the end
            if (res != (int)BrainFlowExitCodes::STATUS_OK)
            {
                break; // dont send exit code
            }
        }
        return (int)BrainFlowExitCodes::STATUS_OK;
    }
    else
    {
        return (int)BrainFlowExitCodes::STREAM_THREAD_IS_NOT_RUNNING;
    }
}

int Galea::release_session ()
{
    if (initialized)
    {
        if (is_streaming)
        {
            stop_stream ();
        }
        free_packages ();
        initialized = false;
        if (socket)
        {
            socket->close ();
            delete socket;
            socket = NULL;
        }
    }
    return (int)BrainFlowExitCodes::STATUS_OK;
}

void Galea::read_thread ()
{
    int res;
    unsigned char b[Galea::max_bytes_in_transaction];
    DataBuffer time_buffer (1, 11);
    for (int i = 0; i < Galea::max_bytes_in_transaction; i++)
    {
        b[i] = 0;
    }
    int num_exg_rows = board_descr["default"]["num_rows"];
    int num_aux_rows = board_descr["auxiliary"]["num_rows"];
    double *exg_package = new double[num_exg_rows];
    double *aux_package = new double[num_aux_rows];
    for (int i = 0; i < num_exg_rows; i++)
    {
        exg_package[i] = 0.0;
    }
    for (int i = 0; i < num_aux_rows; i++)
    {
        aux_package[i] = 0.0;
    }

    while (keep_alive)
    {
        res = socket->recv (b, Galea::max_bytes_in_transaction);
        double pc_timestamp = get_timestamp ();
        // inform the main thread that everything is ok and first package was received
        if ((this->state != (int)BrainFlowExitCodes::STATUS_OK) &&
            ((res == Galea::exg_transaction_size) || (res == Galea::aux_transaction_size)))
        {
            safe_logger (spdlog::level::info,
                "received first package with {} bytes streaming is started", res);
            {
                std::lock_guard<std::mutex> lk (this->m);
                this->state = (int)BrainFlowExitCodes::STATUS_OK;
            }
            this->cv.notify_one ();
            safe_logger (spdlog::level::debug, "start streaming");
        }
        if (res == Galea::exg_transaction_size)
        {
            add_exg_package (exg_package, b, pc_timestamp, &time_buffer);
        }
        else if (res == Galea::aux_transaction_size)
        {
            add_aux_package (aux_package, b, pc_timestamp, &time_buffer);
        }
        else
        {
            if (res == -1)
            {
#ifdef _WIN32
                safe_logger (spdlog::level::err, "WSAGetLastError is {}", WSAGetLastError ());
#else
                safe_logger (spdlog::level::err, "errno {} message {}", errno, strerror (errno));
#endif
            }
            if (res > 0)
            {
                // more likely its a string received, try to print it
                b[res] = '\0';
                safe_logger (spdlog::level::warn, "Received: {}, size is: {}", b, res);
            }
        }
    }
    delete[] exg_package;
    delete[] aux_package;
}

void Galea::add_exg_package (
    double *package, unsigned char *b, double pc_timestamp, DataBuffer *time_buffer)
{
    // 20 times:
    // b[0] start byte
    // b[1] package num
    // b[2-49] exg
    // b[50-57] timestamp
    // b[58] end byte
    constexpr int offset_last_package = Galea::exg_package_size * (Galea::num_exg_packages - 1);
    double timestamp_last_package = 0.0;
    memcpy (&timestamp_last_package, b + 50 + offset_last_package, 8);
    timestamp_last_package /= 1000; // from ms to seconds
    double time_delta = pc_timestamp - timestamp_last_package;
    time_buffer->add_data (&time_delta);
    double latest_times[10];
    int num_time_deltas = (int)time_buffer->get_current_data (10, latest_times);
    time_delta = 0.0;
    for (int i = 0; i < num_time_deltas; i++)
    {
        time_delta += latest_times[i];
    }
    time_delta /= num_time_deltas;

    for (int cur_package = 0; cur_package < Galea::num_exg_packages; cur_package++)
    {
        int offset = cur_package * Galea::exg_package_size;
        // package num
        package[board_descr["default"]["package_num_channel"].get<int> ()] = (double)b[1 + offset];
        // eeg and emg
        for (int i = 0; i < 16; i++)
        {
            if (i < 6)
                package[i + 1] = emg_scale * (double)cast_24bit_to_int32 (b + offset + 2 + 3 * i);
            else if ((i == 6) || (i == 7)) // fp1 and fp2
                package[i + 1] =
                    eeg_scale_sister_board * (double)cast_24bit_to_int32 (b + offset + 2 + 3 * i);
            else
                package[i + 1] =
                    eeg_scale_main_board * (double)cast_24bit_to_int32 (b + offset + 2 + 3 * i);
        }

        double timestamp_device = 0.0;
        memcpy (&timestamp_device, b + 50 + offset, 8);
        timestamp_device /= 1000; // from ms to seconds
        package[board_descr["default"]["timestamp_channel"].get<int> ()] =
            timestamp_device + time_delta - half_rtt;
        package[board_descr["default"]["other_channels"][0].get<int> ()] = pc_timestamp;
        package[board_descr["default"]["other_channels"][1].get<int> ()] = timestamp_device;

        push_package (package);
    }
}

void Galea::add_aux_package (
    double *package, unsigned char *b, double pc_timestamp, DataBuffer *time_buffer)
{
    // 4 times:
    // b[0] start byte
    // b[1] package num
    // b[2-5] eda
    // b[6-7] temperature
    // b[8-15] ppg
    // b[16] battery
    // b[17-24] timestamp
    // b[25] end byte
    constexpr int offset_last_package = Galea::aux_package_size * (Galea::num_aux_packages - 1);
    double timestamp_last_package = 0.0;
    memcpy (&timestamp_last_package, b + 17 + offset_last_package, 8);
    timestamp_last_package /= 1000; // from ms to seconds
    double time_delta = pc_timestamp - timestamp_last_package;
    time_buffer->add_data (&time_delta);
    double latest_times[10];
    int num_time_deltas = (int)time_buffer->get_current_data (10, latest_times);
    time_delta = 0.0;
    for (int i = 0; i < num_time_deltas; i++)
    {
        time_delta += latest_times[i];
    }
    time_delta /= num_time_deltas;

    for (int cur_package = 0; cur_package < Galea::num_aux_packages; cur_package++)
    {
        int offset = cur_package * Galea::aux_package_size;
        // package num
        package[board_descr["auxiliary"]["package_num_channel"].get<int> ()] =
            (double)b[1 + offset];
        uint16_t temperature = 0;
        int32_t ppg_ir = 0;
        int32_t ppg_red = 0;
        float eda = 0;
        memcpy (&temperature, b + 6 + offset, 2);
        memcpy (&eda, b + 2 + offset, 4);
        memcpy (&ppg_red, b + 8 + offset, 4);
        memcpy (&ppg_ir, b + 12 + offset, 4);
        package[board_descr["auxiliary"]["ppg_channels"][0].get<int> ()] = (double)ppg_red;
        package[board_descr["auxiliary"]["ppg_channels"][1].get<int> ()] = (double)ppg_ir;
        package[board_descr["auxiliary"]["eda_channels"][0].get<int> ()] = (double)eda;
        package[board_descr["auxiliary"]["temperature_channels"][0].get<int> ()] =
            temperature / 100.0;
        package[board_descr["auxiliary"]["battery_channel"].get<int> ()] = (double)b[16 + offset];

        double timestamp_device = 0.0;
        memcpy (&timestamp_device, b + 17 + offset, 8);
        timestamp_device /= 1000; // from ms to seconds
        package[board_descr["auxiliary"]["timestamp_channel"].get<int> ()] =
            timestamp_device + time_delta - half_rtt;
        package[board_descr["auxiliary"]["other_channels"][0].get<int> ()] = pc_timestamp;
        package[board_descr["auxiliary"]["other_channels"][1].get<int> ()] = timestamp_device;

        push_package (package, (int)BrainFlowPresets::AUXILIARY_PRESET);
    }
}

int Galea::calc_time (std::string &resp)
{
    constexpr int bytes_to_calc_rtt = 8;
    unsigned char b[bytes_to_calc_rtt];

    double start = get_timestamp ();
    int res = socket->send ("F4444444", bytes_to_calc_rtt);
    if (res != bytes_to_calc_rtt)
    {
        safe_logger (spdlog::level::warn, "failed to send time calc command to device");
        return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
    }
    res = socket->recv (b, bytes_to_calc_rtt);
    double done = get_timestamp ();
    if (res != bytes_to_calc_rtt)
    {
        safe_logger (
            spdlog::level::warn, "failed to recv resp from time calc command, resp size {}", res);
        return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
    }

    double duration = done - start;
    double timestamp_device = 0;
    memcpy (&timestamp_device, b, 8);
    timestamp_device /= 1000;
    half_rtt = duration / 2;

    json result;
    result["rtt"] = duration;
    result["timestamp_device"] = timestamp_device;
    result["pc_timestamp"] = start + half_rtt;

    resp = result.dump ();
    safe_logger (spdlog::level::info, "calc_time output: {}", resp);

    return (int)BrainFlowExitCodes::STATUS_OK;
}

std::string Galea::find_device ()
{
#ifdef _WIN32
    std::string ssdp_ip_address = "192.168.137.255";
#else
    std::string ssdp_ip_address = "239.255.255.250";
#endif

    safe_logger (spdlog::level::trace, "trying to autodiscover device via SSDP");
    safe_logger (spdlog::level::trace, "timeout for search is {}", params.timeout);
    std::string ip_address = "";
    SocketClientUDP udp_client (ssdp_ip_address.c_str (),
        1900); // ssdp ip and port

    int res = udp_client.connect ();
    if (res == (int)SocketClientUDPReturnCodes::STATUS_OK)
    {
        std::string msearch = ("M-SEARCH * HTTP/1.1\r\nHost: " + ssdp_ip_address +
            ":1900\r\nMAN: ssdp:discover\r\n"
            "ST: urn:schemas-upnp-org:device:Basic:1\r\n"
            "MX: 3\r\n"
            "\r\n"
            "\r\n");

        safe_logger (spdlog::level::trace, "Use search request {}", msearch.c_str ());

        res = (int)udp_client.send (msearch.c_str (), (int)msearch.size ());
        if (res == msearch.size ())
        {
            unsigned char b[250];
            auto start_time = std::chrono::high_resolution_clock::now ();
            int run_time = 0;
            while (run_time < params.timeout)
            {
                res = udp_client.recv (b, 250);
                if (res > 1)
                {
                    std::string response ((const char *)b);
                    safe_logger (spdlog::level::trace, "Search response: {}", b);
                    std::regex rgx_ip ("LOCATION: http://([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
                    std::smatch matches;
                    if (std::regex_search (response, matches, rgx_ip) == true)
                    {
                        if (matches.size () == 2)
                        {
                            std::regex rgx_sn (
                                "USN: uuid:" + params.serial_number + "::upnp:rootdevice");
                            if ((params.serial_number.empty ()) ||
                                (std::regex_search (response, rgx_sn)))
                            {
                                ip_address = matches.str (1);
                                break;
                            }
                        }
                    }
                }
                auto end_time = std::chrono::high_resolution_clock::now ();
                run_time =
                    (int)std::chrono::duration_cast<std::chrono::seconds> (end_time - start_time)
                        .count ();
            }
        }
        else
        {
            safe_logger (spdlog::level::err, "Sent res {}", res);
        }
    }
    else
    {
        safe_logger (spdlog::level::err, "Failed to connect socket {}", res);
    }

    if (ip_address.empty ())
    {
        safe_logger (spdlog::level::err, "failed to find ip address");
    }
    else
    {
        safe_logger (spdlog::level::info, "use ip address {}", ip_address.c_str ());
    }

    udp_client.close ();
    return ip_address;
}

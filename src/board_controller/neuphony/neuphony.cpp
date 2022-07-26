#include <vector>

#include "custom_cast.h"
#include "neuphony.h"
#include "serial.h"
#include "timestamp.h"

#define START_BYTE 0xA5
#define END_BYTE 0xC3

void Neuphony::read_thread ()
{
    /*
    Byte 1: 0x41
    Byte 2: 0x35
    Byte 3-4: Sample Number
    Bytes 5-10: Data value for EEG channel 1
    Bytes 11-16: Data value for EEG channel 2
    Bytes 17-22: Data value for EEG channel 3
    Bytes 23-28: Data value for EEG channel 4
    Bytes 29-34: Data value for EEG channel 5
    Bytes 35-40: Data value for EEG channel 6
    Bytes 41-46: Data value for EEG channel 6
    Bytes 47-52: Data value for EEG channel 8
    Accel Data Bytes 53-64: 6 bytes of data
    Byte 65: 0x43
    Byte 66: 0x33
    */

    int res;
    unsigned char b[66];
    double accel[3] = {0.};
    int num_rows = board_descr["default"]["num_rows"];
    double *package = new double[num_rows];

    for (int i = 0; i < num_rows; i++)
    {
        package[i] = 0.0;
    }
    std::vector<int> eeg_channels = board_descr["default"]["eeg_channels"];

    while (keep_alive)
    {
        // check start bytes
        res = serial->read_from_serial_port (b, 2);
        if (res != 2)
        {
            safe_logger (spdlog::level::debug, "unable to read start bytes");
            continue;
        }
        if (hex_string_to_int32(b,0,1)!=START_BYTE)
        {
            continue;
        }
        int remaining_bytes = 66;
        int pos = 0;
        while ((remaining_bytes > 0) && (keep_alive))
        {
            res = serial->read_from_serial_port (b + pos, remaining_bytes);
            remaining_bytes -= res;
            pos += res;
        }

        if (!keep_alive)
        {
            break;
        }

        if (hex_string_to_int32(b,62,63) != END_BYTE)
        {
            safe_logger (spdlog::level::warn, "Wrong end byte {}", hex_string_to_int32(b,62,63));
            continue;
        }

        // package num
        package[board_descr["default"]["package_num_channel"].get<int> ()] = (double)hex_string_to_int32(b, 0, 1);

        // eeg
        for (unsigned int i = 0; i < eeg_channels.size (); i++)
        {
            package[eeg_channels[i]] = eeg_scale * hex_string_to_int32(b, 2+6*i, 1+6*(i+1));
        }
        // end byte
        package[board_descr["default"]["other_channels"][0].get<int> ()] = (double)hex_string_to_int32(b, 62, 63);

        // place processed bytes for accel
        if (hex_string_to_int32(b,62,63) != END_BYTE)
        {          
            package[board_descr["default"]["accel_channels"][0].get<int> ()] = accel_scale * hex_string_to_int32(b, 50, 53);
            package[board_descr["default"]["accel_channels"][1].get<int> ()] = accel_scale * hex_string_to_int32(b, 54, 57);
            package[board_descr["default"]["accel_channels"][2].get<int> ()] = accel_scale * hex_string_to_int32(b, 58, 61);
        }

        package[board_descr["default"]["timestamp_channel"].get<int> ()] = get_timestamp ();

        push_package (package);
    }
    delete[] package;
}

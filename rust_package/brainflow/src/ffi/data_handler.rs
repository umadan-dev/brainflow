/* automatically generated by rust-bindgen 0.59.2 */

#![allow(non_camel_case_types)]


extern "C" {
    pub fn perform_lowpass(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        sampling_rate: ::std::os::raw::c_int,
        cutoff: f64,
        order: ::std::os::raw::c_int,
        filter_type: ::std::os::raw::c_int,
        ripple: f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn perform_highpass(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        sampling_rate: ::std::os::raw::c_int,
        cutoff: f64,
        order: ::std::os::raw::c_int,
        filter_type: ::std::os::raw::c_int,
        ripple: f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn perform_bandpass(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        sampling_rate: ::std::os::raw::c_int,
        start_freq: f64,
        stop_freq: f64,
        order: ::std::os::raw::c_int,
        filter_type: ::std::os::raw::c_int,
        ripple: f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn perform_bandstop(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        sampling_rate: ::std::os::raw::c_int,
        start_freq: f64,
        stop_width: f64,
        order: ::std::os::raw::c_int,
        filter_type: ::std::os::raw::c_int,
        ripple: f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn remove_environmental_noise(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        sampling_rate: ::std::os::raw::c_int,
        noise_type: ::std::os::raw::c_int,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn perform_rolling_filter(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        period: ::std::os::raw::c_int,
        agg_operation: ::std::os::raw::c_int,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn perform_downsampling(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        period: ::std::os::raw::c_int,
        agg_operation: ::std::os::raw::c_int,
        output_data: *mut f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn perform_wavelet_transform(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        wavelet: ::std::os::raw::c_int,
        decomposition_level: ::std::os::raw::c_int,
        extension: ::std::os::raw::c_int,
        output_data: *mut f64,
        decomposition_lengths: *mut ::std::os::raw::c_int,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn perform_inverse_wavelet_transform(
        wavelet_coeffs: *mut f64,
        original_data_len: ::std::os::raw::c_int,
        wavelet: ::std::os::raw::c_int,
        decomposition_level: ::std::os::raw::c_int,
        extension: ::std::os::raw::c_int,
        decomposition_lengths: *mut ::std::os::raw::c_int,
        output_data: *mut f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn perform_wavelet_denoising(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        wavelet: ::std::os::raw::c_int,
        decomposition_level: ::std::os::raw::c_int,
        wavelet_denoising: ::std::os::raw::c_int,
        threshold: ::std::os::raw::c_int,
        extenstion_type: ::std::os::raw::c_int,
        noise_level: ::std::os::raw::c_int,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn get_csp(
        data: *const f64,
        labels: *const f64,
        n_epochs: ::std::os::raw::c_int,
        n_channels: ::std::os::raw::c_int,
        n_times: ::std::os::raw::c_int,
        output_w: *mut f64,
        output_d: *mut f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn get_window(
        window_function: ::std::os::raw::c_int,
        window_len: ::std::os::raw::c_int,
        output_window: *mut f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn perform_fft(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        window_function: ::std::os::raw::c_int,
        output_re: *mut f64,
        output_im: *mut f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn perform_ifft(
        input_re: *mut f64,
        input_im: *mut f64,
        data_len: ::std::os::raw::c_int,
        restored_data: *mut f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn get_nearest_power_of_two(
        value: ::std::os::raw::c_int,
        output: *mut ::std::os::raw::c_int,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn get_psd(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        sampling_rate: ::std::os::raw::c_int,
        window_function: ::std::os::raw::c_int,
        output_ampl: *mut f64,
        output_freq: *mut f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn detrend(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        detrend_operation: ::std::os::raw::c_int,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn calc_stddev(
        data: *mut f64,
        start_pos: ::std::os::raw::c_int,
        end_pos: ::std::os::raw::c_int,
        output: *mut f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn get_psd_welch(
        data: *mut f64,
        data_len: ::std::os::raw::c_int,
        nfft: ::std::os::raw::c_int,
        overlap: ::std::os::raw::c_int,
        sampling_rate: ::std::os::raw::c_int,
        window_function: ::std::os::raw::c_int,
        output_ampl: *mut f64,
        output_freq: *mut f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn get_band_power(
        ampl: *mut f64,
        freq: *mut f64,
        data_len: ::std::os::raw::c_int,
        freq_start: f64,
        freq_end: f64,
        band_power: *mut f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn get_custom_band_powers(
        raw_data: *mut f64,
        rows: ::std::os::raw::c_int,
        cols: ::std::os::raw::c_int,
        start_freqs: *mut f64,
        stop_freqs: *mut f64,
        num_bands: ::std::os::raw::c_int,
        sampling_rate: ::std::os::raw::c_int,
        apply_filters: ::std::os::raw::c_int,
        avg_band_powers: *mut f64,
        stddev_band_powers: *mut f64,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn set_log_level_data_handler(log_level: ::std::os::raw::c_int) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn set_log_file_data_handler(
        log_file: *const ::std::os::raw::c_char,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn log_message_data_handler(
        log_level: ::std::os::raw::c_int,
        message: *mut ::std::os::raw::c_char,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn write_file(
        data: *const f64,
        num_rows: ::std::os::raw::c_int,
        num_cols: ::std::os::raw::c_int,
        file_name: *const ::std::os::raw::c_char,
        file_mode: *const ::std::os::raw::c_char,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn read_file(
        data: *mut f64,
        num_rows: *mut ::std::os::raw::c_int,
        num_cols: *mut ::std::os::raw::c_int,
        file_name: *const ::std::os::raw::c_char,
        num_elements: ::std::os::raw::c_int,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn get_num_elements_in_file(
        file_name: *const ::std::os::raw::c_char,
        num_elements: *mut ::std::os::raw::c_int,
    ) -> ::std::os::raw::c_int;
}
extern "C" {
    pub fn get_version_data_handler(
        version: *mut ::std::os::raw::c_char,
        num_chars: *mut ::std::os::raw::c_int,
        max_chars: ::std::os::raw::c_int,
    ) -> ::std::os::raw::c_int;
}

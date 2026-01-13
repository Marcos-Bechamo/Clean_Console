#include "console.h"
#include <iostream>
#include <string>

void ConsoleTablePrinter::PrintStatusLine(const IStatusPrint& status)
{
    // ANSI color codes
    constexpr const char* COLOR_RESET  = "\033[0m";
    constexpr const char* COLOR_RED    = "\033[31m";
    constexpr const char* COLOR_YELLOW = "\033[33m";
    constexpr const char* COLOR_GREEN  = "\033[32m";
    constexpr const char* COLOR_BLUE   = "\033[34m";
    const char* color = COLOR_RESET;
    const char* level_str = "VINFO";
    switch (status.level)
    {
        case VINFO: color = COLOR_BLUE;   level_str = "VINFO"; break;
        case INFO:  color = COLOR_GREEN;  level_str = "INFO";  break;
        case WARN:  color = COLOR_YELLOW; level_str = "WARN";  break;
        case ERROR: color = COLOR_RED;    level_str = "ERROR"; break;
    }
    std::cout << "\033[K"; // clear line
    // Format: [<level>][<location>] <data> - where the level is also color coded
    std::cout << color << "[" << level_str << "]" << COLOR_RESET
              << "[" << status.header << "] "
              << status.data
              << std::endl;
}

size_t ConsoleTablePrinter::newStatus(const IStatusPrint status){
    // 1. Move cursor to end of the status rows
    auto move = (current_displayed_table_rows_ > 0) ? current_displayed_table_rows_+3 : 0;
    MoveCursorUp(move);
    // 2. update and print status on bottom of status rows
    status_rows_.push_back(status);
    PrintStatusLine(status);
    // 3. update and redraw the table
    table_start_++;
    printTelemTable(true);
    return (size_t)status_rows_.size()-1;
}

size_t ConsoleTablePrinter::updateStatus(size_t index, const IStatusPrint status){
    // 1. Move cursor to status row commanded from index
    size_t total_rows = 0;
    if (current_displayed_table_rows_ > 0){
            total_rows = total_rows + current_displayed_table_rows_ + 3;
    }
    total_rows= total_rows+(size_t)status_rows_.size()-index;
    MoveCursorUp(total_rows);
    // 2. update and print new status into that status row
    PrintStatusLine(status);
    status_rows_[index] = status;
    MoveCursorDown(total_rows);
    return index;
}

bool ConsoleTablePrinter::addTelemetry(const ITelemetryPrint telem){
    table_rows_.push_back(telem);
    return table_rows_.size() < 2;
}

void ConsoleTablePrinter::printTelemTable(bool full_redraw)
{
    if (table_rows_.empty())
        return;

    // Latest telemetry for header layout
    const ITelemetryPrint& telem = table_rows_.back();
    std::string h_fmt = BuildFormat(telem.columns);
    auto header_args = BuildArgs(telem.columns);
    std::string header_line = fmt::vformat(h_fmt, header_args);
    size_t total_width = telem.columns.size() * column_width_;

    if (full_redraw)
    {
        // Print header + separators
        PrintRow(std::string(total_width, '='));
        PrintRow(header_line);
        PrintRow(std::string(total_width, '='));
    }
    else
    {
        // Move cursor to the start of the data area
        MoveCursorUp(static_cast<int>(current_displayed_table_rows_));
    }

    // Print newest rows (circular buffer ensures we only have max rows)
    for (const auto& row : table_rows_)  // oldest → newest
    {
        fmt::dynamic_format_arg_store<fmt::format_context> store;
        store.push_back(column_width_);  // {0} = column width

        for (const auto& cell : row.data)
            store.push_back(cell);

        std::string line = fmt::vformat(h_fmt, store);
        PrintRow(line);
    }

    current_displayed_table_rows_ = table_rows_.size();
}

// Starts polling a status asynchronously, returns a unique id
size_t ConsoleTablePrinter::startPolling(const IStatusPrint status) {
    auto id = newStatus(status);
    int interval_ms = 200;

    // Stop flag for this poll
    auto stop_flag = std::make_shared<std::atomic<bool>>(false);

    // Launch thread
    u_int8_t frame=0;
    std::thread t([this, id, frame, status, stop_flag, interval_ms]() {
        auto local_frame = frame;
        while (!stop_flag->load()) {
            auto local_status = status;
            local_status.data += pollingWaveform(local_frame++, 8);
            updateStatus(id, local_status);
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }
    });

    // Store thread and flag
    std::lock_guard<std::mutex> lock(mutex_);
    threads_[id] = std::move(t);
    stop_flags_[id] = stop_flag;

    return id;
}

// Stops a polling thread given its id
void ConsoleTablePrinter::stopPolling(size_t id, const IStatusPrint status) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stop_flags_.count(id)) {
        stop_flags_[id]->store(true); // signal stop
        if (threads_[id].joinable())
            threads_[id].join(); // wait for thread to finish

        threads_.erase(id);
        stop_flags_.erase(id);
        updateStatus(id, status);
    }
}

/// @brief Generates a small waveform string for polling animation
/// @param frame The current frame number (increments every update)
/// @param width How many blocks to display in the waveform
/// @return A string like "▁▂▃▄▅▆"
std::string ConsoleTablePrinter::pollingWaveform(int frame, int width = 8){
    static const std::vector<std::string> blocks = {
        "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█", 
        "▇", "▆", "▅", "▄", "▃", "▁"
    };
    int n = blocks.size();
    std::string waveform;

    for (int i = 0; i < width; ++i){
        // Each block is offset by i from the current frame to create motion
        int index = (frame + i) % n;
        waveform += blocks[index];
    }
    return " [" + waveform + "]";
}
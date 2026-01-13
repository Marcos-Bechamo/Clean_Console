#include <boost/circular_buffer.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fmt/format.h>
#include <fmt/args.h>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include "console_base.h"

template<typename T>
std::vector<std::string> convert_to_strings(const std::vector<T>& data, const std::string& fmt_str = "{:.2f}"){
    std::vector<std::string> out;
    out.reserve(data.size());
    for (const auto& val : data)
    {
        // Use fmt::format if numeric, fallback to std::to_string for generic types
        if constexpr (std::is_arithmetic_v<T>)
            out.push_back(fmt::format(fmt_str, val));
        else
            out.push_back(val); // assume already string-like
    }
    return out;
}

class ConsoleTablePrinter : public IConsole {
public:
    ConsoleTablePrinter(std::shared_ptr<spdlog::logger> logger,
                        int width,
                        size_t max_rows = 12)
        : logger_(std::move(logger)),
          column_width_(width),
          max_table_rows_(max_rows),
          current_displayed_table_rows_(0),
          table_rows_(max_table_rows_)
          {}

    ~ConsoleTablePrinter(){stopAllPolling();}

    //****************************************************//
    size_t newStatus(const IStatusPrint status) override;
    bool addTelemetry(const ITelemetryPrint telem) override;
    void printTelemTable(bool full_redraw) override;
    size_t startPolling(const IStatusPrint status) override;
    void stopPolling(size_t id, const IStatusPrint status) override;
    inline std::vector<std::string> convert_data(const std::vector<double>& data) override
        {return convert_to_strings(data);}
    //****************************************************//

    inline void print_banner(std::string version, std::string date) {
        status_start_ = 10; // start printing status messages after the banner
        std::cout << "=================================================================================================\n";
        std::cout << "=================================================================================================\n";
        std::cout << "    ____            __                             _________       __    __     _____ _          \n";
        std::cout << "   / __ )___  _____/ /_  ____ _____ ___  ____     / ____/ (_)___ _/ /_  / /_   / ___/(_)___ ___  \n";
        std::cout << "  / __  / _ \\/ ___/ __ \\/ __ `/ __ `__ \\/ __ \\   / /_  / / / __ `/ __ \\/ __/   \\__ \\/ / __ `__ \\ \n";
        std::cout << " / /_/ /  __/ /__/ / / / /_/ / / / / / / /_/ /  / __/ / / / /_/ / / / / /_    ___/ / / / / / / / \n";
        std::cout << "/_____/\\___/\\___/_/ /_/\\__,_/_/ /_/ /_/\\____/  /_/   /_/_/\\__, /_/ /_/\\__/   /____/_/_/ /_/ /_/  \n";
        std::cout << "                                                         /____/                                  \n";
        std::cout << "=================================================================================================\n";
        std::cout << "=================================================================================================\n";
        std::cout << "Version: " << version << " created on " << date << "\n";
    // https://patorjk.com/software/taag/#p=display&f=Slant&t=Bechamo+Flight+Sim&x=none&v=4&h=4&w=80&we=false
    }

private:
    void PrintStatusLine(const IStatusPrint& status);
    size_t updateStatus(size_t index, const IStatusPrint status);
    std::string pollingWaveform(int frame, int width);
    inline void stopAllPolling() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, flag] : stop_flags_)
            flag->store(true);

        for (auto& [id, t] : threads_)
            if (t.joinable())
                t.join();

        threads_.clear();
        stop_flags_.clear();
    }
    void PrintRow(const std::string& line)
    {
        std::cout << "\033[K"; // clear line
        std::cout << "[" << line << "]" << std::endl;
    }

    inline void MoveCursorUp(int n){if (n > 0) std::cout << "\033[" << n << "A";}
    inline void MoveCursorDown(int n){if (n > 0) std::cout << "\033[" << n << "B";}

    std::string BuildFormat(const std::vector<Column>& columns) const{
        std::string fmt_str;
        for (size_t i = 0; i < columns.size(); ++i)
        {
            size_t index = i + 1; // {0} = width
            if (columns[i].align == ColumnAlign::Center)
                fmt_str += fmt::format("{{{}:^{{0}}}}", index);
            else
                fmt_str += fmt::format("{{{}:<{{0}}}}", index);
        }
        return fmt_str;
    }

    fmt::dynamic_format_arg_store<fmt::format_context>
    BuildArgs(const std::vector<Column>& columns) const
    {
        fmt::dynamic_format_arg_store<fmt::format_context> store;
        store.push_back(column_width_);
        for (const auto& c : columns)
            store.push_back(c.title);
        return store;
    }

    /// @brief Move down cursor to "n" rows from top
    inline void CursorMove(size_t n){
        size_t total_rows = status_start_;
        total_rows = total_rows + (size_t)status_rows_.size();
        if (current_displayed_table_rows_ > 0){
            total_rows = total_rows + current_displayed_table_rows_ + 3;
        }
        if (n < total_rows){
            MoveCursorUp(total_rows - n);
        }
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
    int column_width_;

    /* Rows from the top to place table */
    size_t table_start_;
    /* Maximum num of telemetry rows to display */
    size_t max_table_rows_;
    /* Current num of telemetry row displayed */
    size_t current_displayed_table_rows_;
    /* Telemetry row data */
    boost::circular_buffer<ITelemetryPrint>table_rows_;
    
    /* Rows from the top to place first status row */
    size_t status_start_;
    /* Status row data */
    std::vector<IStatusPrint> status_rows_{};

    std::unordered_map<size_t, std::thread> threads_;
    std::unordered_map<size_t, std::shared_ptr<std::atomic<bool>>> stop_flags_;
    std::mutex mutex_;
};
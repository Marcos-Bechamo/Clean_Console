#ifndef CLARKESIM_SRC_COMMON_THREAD_MANAGER_H_
#define CLARKESIM_SRC_COMMON_THREAD_MANAGER_H_
#include <iostream>
#include <string>

enum class ColumnAlign {
    Left,
    Center
};

struct Column {
    std::string title;
    ColumnAlign align;
};

enum ConsoleLevels {
    VINFO, /* Only recorded to system_log.csv unless in verbose mode */
    INFO,  /* Classic print() like behavior */
    WARN,  /* For non-breaking issues that require investigation*/
    ERROR  /* used for error handling */
};

/// @brief Used to display status messages to the console
/// Format: [<level>][<header>] <data> - where the level is also color coded
struct IStatusPrint {
    ConsoleLevels level;
    std::string header;
    std::string data;
    size_t id{}; // not always needed
};

/// @brief Used transfer telemetry data to the console
struct ITelemetryPrint {
    std::vector<Column> columns;
    std::vector<std::string> data;
};

/// @brief Interface to ConsoleTablePrinter class 
/// calls to these functions will be placed into async queue
struct IConsole {
    /// @brief Appends a new status message above the Telemetry table
    /// @return unique ID of status message created
    virtual size_t newStatus(const IStatusPrint& status)=0;

    /// @brief Overwrites an existing status message (id)
    /// @return unique ID of status message updated
    virtual size_t updateStatus(size_t id, const IStatusPrint& status)=0;

    virtual bool addTelemetry(const ITelemetryPrint telem)=0;

    /// @brief Handles new line in the Telemetry table
    virtual void printTelemTable(bool full_redraw)=0;

    /// @brief Can be used to generate ITelemetryPrint objects
    virtual std::vector<std::string> convert_data(const std::vector<double>& data)=0;
};
#endif
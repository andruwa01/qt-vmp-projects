#include "logger.h"

// Реализация функции для получения текущего времени
std::string getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::tm *local_time = std::localtime(&current_time);
    std::ostringstream time_stream;
    time_stream << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");

    return time_stream.str();
}

// Реализация метода для получения единственного экземпляра Logger
Logger& Logger::getInstance(const std::string& file_name, OutputMode mode)
{
    static Logger instance(file_name, mode);
    return instance;
}

// Конструктор Logger
Logger::Logger(const std::string& file_name, OutputMode mode) : output_mode(mode)
{
    if (output_mode == FILE_OUTPUT)
    {
        log_file.open(file_name, std::ios::app);
        if (!log_file.is_open()) {
            std::cerr << "Error opening log file, using console instead." << std::endl;
            output_mode = CONSOLE_OUTPUT;
        }
    }
}

// Деструктор Logger
Logger::~Logger()
{
    if (log_file.is_open())
    {
        log_file.close();
    }
}

// Метод для создания LogStream
Logger::LogStream Logger::log(LogLevel level)
{
    return LogStream(*this, level);
}

// Конструктор LogStream
Logger::LogStream::LogStream(Logger& logger, LogLevel level) : logger(logger), level(level) {}

// Деструктор LogStream, который выполняет автоматическую запись
Logger::LogStream::~LogStream()
{
    logger.flush(level, log_stream.str());
}

// Метод Logger для записи накопленных сообщений
void Logger::flush(LogLevel level, const std::string& message)
{
    std::string timestamp = getCurrentTime();
    std::ostringstream log_entry;
    log_entry << "[" << timestamp << "] "
              << levelToString(level) << ": " << message << std::endl;

    if (output_mode == FILE_OUTPUT && log_file.is_open())
    {
        log_file << log_entry.str();
        log_file.flush();
    }
    else
    {
        std::cout << log_entry.str();
    }
}

// Преобразование уровня логирования в строку
std::string Logger::levelToString(LogLevel level)
{
    switch (level)
    {
    case INFO:    return "INFO";
    case WARNING: return "WARNING";
    case ERROR:   return "ERROR";
    case DEBUG:   return "DEBUG";
    case CRITICAL:return "CRITICAL";
    default:      return "UNKNOWN";
    }
}

// USAGE EXAMPLE

// int main()
// {
//     LOG(INFO) << "Program started" << "info_1" << "info_2";
//     LOG(DEBUG) << "Debugging info" << "dbug_1" << "dubug_2";
//     LOG(ERROR) << "An error occured";

//     return 0;

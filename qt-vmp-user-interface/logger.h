#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>

// Определения уровня логирования
enum LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

// Определения режима вывода
enum OutputMode { CONSOLE_OUTPUT, FILE_OUTPUT };

// Функция для получения текущего времени в виде строки
std::string getCurrentTime();

// Класс Logger (использует шаблон Singleton)
class Logger
{
public:
    static Logger& getInstance(const std::string& file_name = "", OutputMode mode = CONSOLE_OUTPUT);

    // Вспомогательный класс для автоматической записи лога
    class LogStream
    {
    public:
        LogStream(Logger& logger, LogLevel level);
        ~LogStream();

        template<typename T>
        LogStream& operator<<(const T& message)
        {
            log_stream << message;
            return *this;
        }

    private:
        Logger& logger;
        LogLevel level;
        std::ostringstream log_stream;

        // Запрещаем копирование и присваивание
        LogStream(const LogStream&) = delete;
        LogStream& operator=(const LogStream&) = delete;
    };

    // Метод для создания LogStream
    LogStream log(LogLevel level);

private:
    Logger(const std::string& file_name = "", OutputMode mode = CONSOLE_OUTPUT);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream log_file;         // Поток для записи в файл
    OutputMode output_mode;         // Текущий режим вывода (консоль или файл)

    void flush(LogLevel level, const std::string& message);
    std::string levelToString(LogLevel level);
};

// Макрос для упрощённого использования логгера
#define LOG(level) Logger::getInstance().log(level)

#endif // LOGGER_H

#pragma once

#include <iostream>
#include <string>
#include <deque>
#include <mutex>
#include <stdarg.h>

#define MAX_LOG_ENTRIES 256
#define TEMP_BUF_SIZE 1024

enum class LogLevel {
	Debug,
	Info,
	Warn,
	Error,
};

struct LogEntry {
	std::string content;
	LogLevel level;
	tm time;

	LogEntry(std::string content, LogLevel level, tm time) :
		content(content),
		level(level),
		time(time) {
	}
};

#define LOGGER_VARARGS(level)											\
	std::string text;													\
	text.resize(TEMP_BUF_SIZE);											\
	va_list argptr;														\
	va_start(argptr, message);											\
	auto len = vsnprintf(text.data(), TEMP_BUF_SIZE, message, argptr);	\
	va_end(argptr);														\
	text.resize(len);													\
	return _internalLog(level, std::move(text));

class Log {
public:
	/**
	 * @brief The store log history entries
	 */
	std::deque<LogEntry> history;

	/**
	 * @brief The mutex used for thread safety
	 */
	std::mutex mutex;

	/**
	 * @brief Output stream for the logger
	 */
	std::ostream* logStream = nullptr;

	bool Info(std::string&& string) {
		return _internalLog(LogLevel::Info, std::move(string));
	}

	bool Warn(std::string&& string) {
		return _internalLog(LogLevel::Warn, std::move(string));
	}

	bool Error(std::string&& string) {
		return _internalLog(LogLevel::Error, std::move(string));
	}

	bool Debug(std::string&& string) {
		return _internalLog(LogLevel::Debug, std::move(string));
	}

	bool Info(const char* message, ...) {
		LOGGER_VARARGS(LogLevel::Info);
	}

	bool Warn(const char* message, ...) {
		LOGGER_VARARGS(LogLevel::Warn);
	}

	bool Error(const char* message, ...) {
		LOGGER_VARARGS(LogLevel::Error);
	}

	bool Debug(const char* message, ...) {
		LOGGER_VARARGS(LogLevel::Debug);
	}

	bool Error(const std::exception* ex) {
		return _internalLog(LogLevel::Error, std::string(ex->what()));
	}

	bool Debug(const std::exception* ex) {
		return _internalLog(LogLevel::Debug, std::string(ex->what()));
	}

	void ClearHistory() {
		this->mutex.lock();
		this->history.clear();
		this->mutex.unlock();
	}

protected:
	// Implementation in src/ai/gui/log-window.cpp
	bool _internalLog(LogLevel level, std::string&& string);
};

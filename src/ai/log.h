#pragma once

#include <iostream>
#include <string>
#include <deque>
#include <mutex>
#include <stdarg.h>

#define INMEMORY_LOG_ENTRY_LIMIT 1024
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
	size_t frame;
	tm time;

	LogEntry(std::string content, LogLevel level, size_t frame, tm time) :
		content(content),
		level(level),
		frame(frame),
		time(time) {
	}
};

#define LOGGER_VARARGS(level, frame)									\
	std::string text;													\
	text.resize(TEMP_BUF_SIZE);											\
	va_list argptr;														\
	va_start(argptr, message);											\
	auto len = vsnprintf(text.data(), TEMP_BUF_SIZE, message, argptr);	\
	va_end(argptr);														\
	text.resize(len);													\
	return _internalLog(level, frame, std::move(text));

class Logger {
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

	/*
	 * @brief Set whenever a log entry is added to the history
	 * This is used to notify the GUI that a new log entry has been added
	 */
	bool logAdded = false;

	bool Info(size_t frame, std::string&& string) {
		return _internalLog(LogLevel::Info, frame, std::move(string));
	}

	bool Warn(size_t frame, std::string&& string) {
		return _internalLog(LogLevel::Warn, frame, std::move(string));
	}

	bool Error(size_t frame, std::string&& string) {
		return _internalLog(LogLevel::Error, frame, std::move(string));
	}

	bool Debug(size_t frame, std::string&& string) {
		return _internalLog(LogLevel::Debug, frame, std::move(string));
	}

	bool Info(size_t frame, const char* message, ...) {
		LOGGER_VARARGS(LogLevel::Info, frame);
	}

	bool Warn(size_t frame, const char* message, ...) {
		LOGGER_VARARGS(LogLevel::Warn, frame);
	}

	bool Error(size_t frame, const char* message, ...) {
		LOGGER_VARARGS(LogLevel::Error, frame);
	}

	bool Debug(size_t frame, const char* message, ...) {
		LOGGER_VARARGS(LogLevel::Debug, frame);
	}

	bool Error(size_t frame, const std::exception* ex) {
		return _internalLog(LogLevel::Error, frame, std::string(ex->what()));
	}

	bool Debug(size_t frame, const std::exception* ex) {
		return _internalLog(LogLevel::Debug, frame, std::string(ex->what()));
	}

	void ClearHistory() {
		this->mutex.lock();
		this->history.clear();
		this->mutex.unlock();
	}

protected:
	// Implementation in src/ai/gui/log-window.cpp
	bool _internalLog(LogLevel level, size_t frame, std::string&& string);
};

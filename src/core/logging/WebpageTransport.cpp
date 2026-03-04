#include "WebpageTransport.hpp"

WebpageTransport::WebpageTransport()
    : nextWriteIndex_(0), count_(0) {
    clear();
}

WebpageTransport::~WebpageTransport() {
}

void WebpageTransport::send(const String& level, const String& message, unsigned long timestamp) {
    LogEntry& entry = entries_[nextWriteIndex_];
    entry.timestampMs = static_cast<uint32_t>(timestamp);
    normalizeLevel(level, entry.level, sizeof(entry.level));
    copyMessage(message, entry.message, sizeof(entry.message));

    nextWriteIndex_ = static_cast<uint8_t>((nextWriteIndex_ + 1) % MAX_LOG_COUNT);
    if (count_ < MAX_LOG_COUNT) {
        ++count_;
    }
}

uint8_t WebpageTransport::count() const {
    return count_;
}

bool WebpageTransport::getNewest(uint8_t indexFromNewest, LogEntry& outEntry) const {
    if (indexFromNewest >= count_) {
        return false;
    }

    int16_t index = static_cast<int16_t>(nextWriteIndex_) - 1 - static_cast<int16_t>(indexFromNewest);
    while (index < 0) {
        index += MAX_LOG_COUNT;
    }

    outEntry = entries_[index];
    return true;
}

void WebpageTransport::clear() {
    nextWriteIndex_ = 0;
    count_ = 0;

    for (uint8_t i = 0; i < MAX_LOG_COUNT; ++i) {
        entries_[i].timestampMs = 0;
        entries_[i].level[0] = '\0';
        entries_[i].message[0] = '\0';
    }
}

void WebpageTransport::normalizeLevel(const String& inputLevel, char* outLevel, size_t outSize) const {
    if (outLevel == nullptr || outSize == 0) {
        return;
    }

    String normalized = inputLevel;
    normalized.trim();
    normalized.toUpperCase();

    const char* levelText = "INFO";
    if (normalized == "DEBUG") {
        levelText = "DEBUG";
    } else if (normalized == "INFO") {
        levelText = "INFO";
    } else if (normalized == "WARNING") {
        levelText = "WARNING";
    } else if (normalized == "ERROR") {
        levelText = "ERROR";
    }

    size_t write = 0;
    for (; write + 1 < outSize && levelText[write] != '\0'; ++write) {
        outLevel[write] = levelText[write];
    }
    outLevel[write] = '\0';
}

void WebpageTransport::copyMessage(const String& inputMessage, char* outMessage, size_t outSize) const {
    if (outMessage == nullptr || outSize == 0) {
        return;
    }

    const char* message = inputMessage.c_str();
    size_t write = 0;
    for (; write + 1 < outSize && message[write] != '\0'; ++write) {
        char c = message[write];
        if (c == '\r' || c == '\n') {
            c = ' ';
        }
        outMessage[write] = c;
    }
    outMessage[write] = '\0';
}

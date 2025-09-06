#pragma once
#include <optional>
#include <string>
#include <system_error>
#ifdef __cpp_lib_source_location
    #include <source_location>
#endif

namespace RainEngine {

// Fallback for std::source_location when not available
#ifndef __cpp_lib_source_location
struct SourceLocationFallback {
    constexpr SourceLocationFallback(const char* file = "unknown", 
                                    const char* function = "unknown", 
                                    unsigned int line = 0) noexcept
        : file_name_(file), function_name_(function), line_(line) {}
    
    [[nodiscard]] constexpr const char* file_name() const noexcept { return file_name_; }
    [[nodiscard]] constexpr const char* function_name() const noexcept { return function_name_; }
    [[nodiscard]] constexpr unsigned int line() const noexcept { return line_; }
    
private:
    const char* file_name_;
    const char* function_name_;
    unsigned int line_;
};
#endif

// Modern error handling with source location support
class Result {
public:
    enum class ErrorCode {
        Success = 0,
        DeviceCreationFailed,
        ResourceAllocationFailed,
        InvalidParameter,
        FileOperationFailed,
        DirectXError
    };

    static Result Success() noexcept {
        #ifdef __cpp_lib_source_location
            return Result{ErrorCode::Success, std::nullopt, std::source_location::current()};
        #else
            return Result{ErrorCode::Success, std::nullopt, SourceLocationFallback{}};
        #endif
    }

    #ifdef __cpp_lib_source_location
    static Result Error(ErrorCode code, std::string message = "", 
                       std::source_location location = std::source_location::current()) noexcept {
        return Result{code, std::move(message), location};
    }
    #else
    static Result Error(ErrorCode code, std::string message = "", 
                       SourceLocationFallback location = {}) noexcept {
        return Result{code, std::move(message), location};
    }
    #endif

    [[nodiscard]] bool IsSuccess() const noexcept { return errorCode == ErrorCode::Success; }
    [[nodiscard]] bool IsError() const noexcept { return errorCode != ErrorCode::Success; }
    [[nodiscard]] ErrorCode GetErrorCode() const noexcept { return errorCode; }
    [[nodiscard]] const std::string& GetMessage() const noexcept { 
        static const std::string empty;
        return message.value_or(empty); 
    }
    
    #ifdef __cpp_lib_source_location
    [[nodiscard]] const std::source_location& GetLocation() const noexcept { return location; }
    #else
    [[nodiscard]] const SourceLocationFallback& GetLocation() const noexcept { return location; }
    #endif

private:
    ErrorCode errorCode;
    std::optional<std::string> message;
    #ifdef __cpp_lib_source_location
    std::source_location location;
    
    Result(ErrorCode code, std::optional<std::string> msg, std::source_location loc) noexcept
        : errorCode(code), message(std::move(msg)), location(loc) {}
    #else
    SourceLocationFallback location;
    
    Result(ErrorCode code, std::optional<std::string> msg, SourceLocationFallback loc) noexcept
        : errorCode(code), message(std::move(msg)), location(loc) {}
    #endif
};

// Template for returning values with error handling
template<typename T>
class Expected {
public:
    Expected(T value) noexcept : value_(std::move(value)), hasValue_(true) {}
    Expected(Result error) noexcept : error_(std::move(error)), hasValue_(false) {}

    [[nodiscard]] bool HasValue() const noexcept { return hasValue_; }
    [[nodiscard]] bool HasError() const noexcept { return !hasValue_; }

    [[nodiscard]] const T& Value() const& noexcept { return value_; }
    [[nodiscard]] T& Value() & noexcept { return value_; }
    [[nodiscard]] T&& Value() && noexcept { return std::move(value_); }

    [[nodiscard]] const Result& Error() const noexcept { return error_; }

    // Convenient operators
    [[nodiscard]] explicit operator bool() const noexcept { return hasValue_; }
    [[nodiscard]] const T& operator*() const& noexcept { return value_; }
    [[nodiscard]] T& operator*() & noexcept { return value_; }
    [[nodiscard]] T&& operator*() && noexcept { return std::move(value_); }

private:
    union {
        T value_;
        Result error_;
    };
    bool hasValue_;
};

// Macro for easy error checking
#define RAIN_TRY(expr) \
    do { \
        auto result = (expr); \
        if (result.IsError()) { \
            return result; \
        } \
    } while(0)

#define RAIN_TRY_ASSIGN(var, expr) \
    do { \
        auto result = (expr); \
        if (result.HasError()) { \
            return result.Error(); \
        } \
        var = std::move(result).Value(); \
    } while(0)

} // namespace RainEngine
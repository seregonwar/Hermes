#pragma once

#include <string>

namespace DyForge {

enum class CommandType {
    START_ANALYSIS,
    STOP_ANALYSIS,
    LOAD_MOD,
    UNLOAD_MOD,
    LIST_MODS,
    LIST_HOOKS
};

struct Command {
    CommandType type;
    std::string parameters;
};

enum class ResponseType {
    SUCCESS,
    ERROR
};

struct Response {
    ResponseType type;
    std::string message;
};

} // namespace DyForge 
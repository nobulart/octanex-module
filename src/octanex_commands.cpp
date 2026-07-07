#include "octanex_commands.h"
#include <cstring>

namespace octanex {

namespace commands {

int process_command(const char* op) {
    for (size_t i = 0; i < COMMAND_TABLE_SIZE; ++i) {
        if (strcmp(COMMAND_TABLE[i].op, op) == 0) {
            return i;
        }
    }
    return -1;
}

const char* command_description(const char* op) {
    for (size_t i = 0; i < COMMAND_TABLE_SIZE; ++i) {
        if (strcmp(COMMAND_TABLE[i].op, op) == 0) {
            return COMMAND_TABLE[i].description;
        }
    }
    return "unknown command";
}

} // namespace commands

} // namespace octanex

#include <NBR14522.h>
#include <algorithm>
#include <hexstr2bytes.h>
#include <string>
#include <vector>

using namespace NBR14522;

inline std::vector<std::string> split(const std::string& str,
                                      const std::string& delim) {
    std::vector<std::string> retval;

    size_t start;
    size_t end;
    for (start = 0, end = str.find(delim); end != std::string::npos;
         start = end + delim.length(), end = str.find(delim, start)) {
        retval.push_back(str.substr(start, end - start));
    }

    // add last token
    retval.push_back(str.substr(start, end));

    return retval;
}

inline std::vector<comando_t> getComandosFromData(const std::string& data) {
    std::vector<comando_t> retval;

    // the data string should be in the format: "comando1 comando2 comando3"
    // where each comando is a string of BCD bytes, for example:
    // 14010203 27010203 28010203

    std::vector<std::string> tokens = split(data, " ");

    for (const auto& token : tokens) {
        auto bytes = hexstr2bytes(token);
        comando_t cmd;
        cmd.fill(0x00);
        std::copy(bytes.begin(), bytes.end(), cmd.begin());
        if (isValidCodeCommand(cmd.at(0)))
            retval.push_back(cmd);
    }

    return retval;
}
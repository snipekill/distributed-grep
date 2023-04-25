#include "utility.hpp"

#include <stdio.h>
#include <string.h>

#include <vector>
#include <string>

std::vector<std::string> split(std::string data, std::string delimiter)
{
    std::vector<std::string> result;
    char *cur_content = strtok(&data[0], delimiter.c_str());
    while (cur_content != NULL)
    {
        result.push_back(cur_content);
        cur_content = strtok(NULL, delimiter.c_str());
    }

    return result;
}

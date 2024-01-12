#pragma once
#include <iostream>
#include "vk_types.h"

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            std::cout << err << std::endl;                              \
            abort();                                                    \
        }                                                               \
    } while (0)                                                         
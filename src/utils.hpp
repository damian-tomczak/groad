#pragma once

#define NOT_COPYABLE(ClassName)                                                                                        \
    ClassName(const ClassName &) = delete;                                                                             \
    ClassName &operator=(const ClassName &) = delete;

#define NOT_MOVEABLE(ClassName)                                                                                        \
    ClassName(ClassName &&) = delete;                                                                                  \
    ClassName &operator=(ClassName &&) = delete;

#define NOT_COPYABLE_AND_MOVEABLE(ClassName)                                                                           \
    NOT_COPYABLE(ClassName);                                                                                           \
    NOT_MOVEABLE(ClassName);
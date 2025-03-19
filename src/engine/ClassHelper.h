#pragma once

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
TypeName(const TypeName& other) = delete;      \
TypeName(const TypeName&& other) = delete;      \
TypeName& operator=(const TypeName& other) = delete;      \
void operator=(const TypeName&& other) = delete
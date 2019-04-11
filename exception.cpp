#include "exception.h"

Utils::Exception::Exception()
{
}

Utils::Exception::~Exception()
{
}

const char* Utils::Exception::what() const noexcept( true )
{
    return getMessage().c_str();
}
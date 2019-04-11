#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>
#include <string>

namespace Utils 
{
    class Exception:
        public std::exception
    {
    public:
        Exception();
        virtual ~Exception();

        virtual std::string getMessage() const = 0;

        const char* what() const noexcept( true );
    };
}

#endif // EXCEPTION_H

/*
 * Squawk messenger. 
 * Copyright (C) 2019  Yury Gubich <blue@macaw.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

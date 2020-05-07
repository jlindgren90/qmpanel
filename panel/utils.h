/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * Copyright: 2020 John Lindgren
 * Authors:
 *   John Lindgren <john@jlindgren.net>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <memory>

template<typename T>
using AutoPtr = std::unique_ptr<T, void (*)(T *)>;
template<typename T>
using AutoPtrV = std::unique_ptr<T, void (*)(void *)>;

class CharPtr : public AutoPtrV<char>
{
public:
    using unique_ptr::unique_ptr;
    operator const char *() const { return get(); }
    operator QString() const { return get(); }
};

#endif

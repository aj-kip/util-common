/****************************************************************************

    File: StorageUnion.hpp
    Author: Andrew Janke
    License: GPLv3

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*****************************************************************************/

#pragma once

#include <type_traits>

template <std::size_t LHS, std::size_t RHS>
struct StaticMax {
    static const std::size_t VALUE = LHS > RHS ? LHS : RHS;
};

// zero arguements
template <typename ... Types>
struct StorageUnion {
    static const std::size_t ALIGNMENT = std::alignment_of<char>::value;

    /** by C++ standards, any class is at least one byte (from Stroustrup's C++
     *  book 3rd edition, no I forgot where...) */
    static const std::size_t SIZE = 1;

    /** A type of space, where any type in Types can live. */
    using Type = typename std::aligned_storage<SIZE, ALIGNMENT>::type;
};

// one arguement
template <typename InType, typename ... Types>
struct StorageUnion<InType, Types...> : public StorageUnion<Types...> {
    static const std::size_t ALIGNMENT = StaticMax
        <std::alignment_of<InType>::value, StorageUnion<Types...>::ALIGNMENT>
        ::VALUE;

    static const std::size_t SIZE = StaticMax
        <sizeof(InType), StorageUnion<Types...>::SIZE>::VALUE;

    /** A type of space, where any type in Types can live. */
    using Type = typename std::aligned_storage<SIZE, ALIGNMENT>::type;
};

// one arguement
template <typename BaseType, typename ... Types>
struct IsBaseOfMulti {
    static const bool VALUE = true;
};

// two arguement
template <typename BaseType, typename OtherType, typename ... Types>
struct IsBaseOfMulti<BaseType, OtherType, Types...> :
    public IsBaseOfMulti<BaseType, Types...>
{
    static const bool VALUE =
           std::is_base_of<BaseType, OtherType>::value
        && IsBaseOfMulti<BaseType, Types...>::VALUE;
};

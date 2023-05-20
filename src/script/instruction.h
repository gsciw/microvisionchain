// Copyright (c) 2021-2023 The MVC developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "script.h"
#include "span.h"

#include <iostream>

namespace mvc
{
    // An instruction is an opcode and an operand (data).
    // Most opcodes have no data, in which case the the operand is an empty
    // range. However, some opcodes do have data e.g.
    //     1-75,
    //     OP_PUSHDATAn (were n=1,2,4).
    // For 1-75 the data follows immediately after the opcode.
    // For OP_PUSHDATAn the data is an extra 1,2,4 bytes after the opcode.
    // Therefore, and additional offset is also stored.
    class instruction
    {
        opcodetype opcode_;
        int8_t offset_{};
        using operand_type = mvc::span<const uint8_t>;
        operand_type operand_{};

    public:
        constexpr instruction(opcodetype opcode) noexcept : opcode_{opcode} {}

        constexpr instruction(opcodetype opcode,
                              int8_t offset,
                              const uint8_t* p,
                              size_t n) noexcept
            : opcode_{opcode}, offset_{offset}, operand_{p, n}
        {
        }

        constexpr opcodetype opcode() const noexcept { return opcode_; }
        constexpr size_t offset() const noexcept { return offset_; }
        constexpr const operand_type& operand() const noexcept
        {
            return operand_;
        }
    };

    inline constexpr bool operator==(const instruction& a,
                                     const instruction& b) noexcept
    {
        return a.opcode() == b.opcode() &&
               a.operand().data() == b.operand().data() &&
               a.operand().size() == b.operand().size();
    }

    inline constexpr bool operator!=(const instruction& a,
                                     const instruction& b) noexcept
    {
        return !(a == b);
    }

    inline std::ostream& operator<<(std::ostream& os, const instruction& inst)
    {
        os << inst.opcode() << ' ' << (void*)inst.operand().data() << ' '
           << inst.operand().size();
        return os;
    }
}


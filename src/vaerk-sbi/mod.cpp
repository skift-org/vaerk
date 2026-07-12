export module Vaerk.Sbi;

import Karm.Core;
import Vaerk.Riscv;

using namespace Karm;

namespace Vaerk::Sbi {

// MARK: Chapter 5. Legacy Extensions (EIDs #0x00 - #0x0F) ------------------------------------------------------------

// 5.2. Extension: Console Putchar (EID #0x01)
export usize consolePutchar(int ch) {
    return Riscv::ecall(ch, 0, 0, 0, 0, 0, 0, 1).a0;
}

export void consolePuts(Str str) {
    for (char const c : str) {
        consolePutchar(c);
    }
}

// MARK: Chapter 6. Timer Extension (EID #0x54494D45 "TIME") ----------------------------------------------------------

// 6.1. Function: Set Timer (FID #0)
export void setTimer(u64 stimeValue) {
    Riscv::ecall(stimeValue, 0, 0, 0, 0, 0, 0, 0x54494D45);
}

} // namespace Vaerk::Sbi

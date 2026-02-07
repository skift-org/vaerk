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

} // namespace Vaerk::Sbi

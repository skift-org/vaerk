export module Vaerk.x86_64:sys;

import Karm.Core;
import :intrinsics;
import :gdt;

using namespace Karm;

namespace x86_64 {

export inline void sysInit(void (*handler)()) {
    wrmsr(Msrs::EFER, rdmsr(Msrs::EFER) | 1);
    wrmsr(Msrs::STAR, ((u64)(Gdt::KCODE * 8) << 32) | ((u64)(((Gdt::UDATA - 1) * 8) | 3) << 48));
    wrmsr(Msrs::LSTAR, (u64)handler);
    wrmsr(Msrs::FMASK, 0xfffffffe);
}

export inline void sysSetGs(usize addr) {
    wrmsr(Msrs::UGSBAS, addr);
    wrmsr(Msrs::KGSBAS, addr);
}

} // namespace x86_64

export module Vaerk.Riscv;

import Karm.Core;

using namespace Karm;

namespace Vaerk::Riscv {

// MARK: CSR -------------------------------------------------------------------

export enum struct Csr : usize {
#define CSR(NUM, _, NAME) NAME = NUM,
#include "defs/csr.inc"

#undef CSR
};

export usize csrr(Csr csr) {
    usize tmp;
    switch (csr) {
#define CSR(NUM, name, NAME)                                 \
    case Csr::NAME:                                          \
        __asm__ __volatile__("csrr %0, " #name : "=r"(tmp)); \
        break;
#include "defs/csr.inc"

#undef CSR
    };
    return tmp;
}

export void csrw(Csr csr, usize val) {
    switch (csr) {
#define CSR(NUM, name, NAME)                                   \
    case Csr::NAME:                                            \
        __asm__ __volatile__("csrw " #name ", %0" ::"r"(val)); \
        break;
#include "defs/csr.inc"

#undef CSR
    };
}

export usize csrrc(Csr csr, usize mask) {
    usize tmp;
    switch (csr) {
#define CSR(NUM, name, NAME)                           \
    case Csr::NAME:                                    \
        __asm__ __volatile__("csrrc %0, " #name ", %1" \
                             : "=r"(tmp)               \
                             : "r"(mask));             \
        break;
#include "defs/csr.inc"

#undef CSR
    };
    return tmp;
}

export usize csrrs(Csr csr, usize mask) {
    usize tmp;
    switch (csr) {
#define CSR(NUM, name, NAME)                           \
    case Csr::NAME:                                    \
        __asm__ __volatile__("csrrs %0, " #name ", %1" \
                             : "=r"(tmp)               \
                             : "r"(mask));             \
        break;
#include "defs/csr.inc"

#undef CSR
    };
    return tmp;
}

// MARK: Instructions ----------------------------------------------------------

export void unimp() { __asm__ __volatile__("unimp"); }

export void wfi() { __asm__ __volatile__("wfi"); }

export void di() { __asm__ __volatile__("csrci mstatus, 8"); }

export void ei() { __asm__ __volatile__("csrsi mstatus, 8"); }

export void sfenceVma() { __asm__ __volatile__("sfence.vma"); }

struct Ecall {
    long a0;
    long a1;
};

export Ecall ecall(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall"
                         : "=r"(a0), "=r"(a1)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                           "r"(a6), "r"(a7)
                         : "memory");
    return {a0, a1};
}

} // namespace Vaerk::Riscv

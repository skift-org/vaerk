module;

#include <hal/vmm.h>

export module Vaerk.Riscv:sv39;

import Karm.Core;

namespace Riscv::Sv39 {

export struct [[gnu::packed]] Entry {
    static constexpr u64 VALID = 1 << 0;
    static constexpr u64 READ = 1 << 1;
    static constexpr u64 WRITE = 1 << 2;
    static constexpr u64 EXEC = 1 << 3;
    static constexpr u64 USER = 1 << 4;
    static constexpr u64 GLOBAL = 1 << 5;
    static constexpr u64 ACCESSED = 1 << 6;
    static constexpr u64 DIRTY = 1 << 7;

    // Sv39 supports up to a 56-bit physical address space
    static constexpr u64 PADDR_MASK = 0x00fffffffffff000ULL;
    // Lower 10 bits contain the flags and RSW field
    static constexpr u64 FLAGS_MASK = 0x3ffULL;
    // Mask for the PPN field within the raw 64-bit entry (bits 10-53)
    static constexpr u64 PPN_MASK = 0x003ffffffffffc00ULL;

    u64 _raw{};

    static u64 makeFlags(Flags<Hal::VmmFlags> flags) {
        u64 res = 0;
        if (flags.has(Hal::VmmFlags::READ)) {
            res |= READ;
        }
        if (flags.has(Hal::VmmFlags::WRITE)) {
            res |= WRITE;
        }
        if (flags.has(Hal::VmmFlags::EXEC)) {
            res |= EXEC;
        }
        if (flags.has(Hal::VmmFlags::USER)) {
            res |= USER;
        }
        if (flags.has(Hal::VmmFlags::GLOBAL)) {
            res |= GLOBAL;
        }

        return res;
    }

    Entry() {}

    Entry(usize paddr, u64 flags) {
        _raw = ((paddr & PADDR_MASK) >> 2) | (flags & FLAGS_MASK);
    }

    template <typename T>
    T* as() {
        return (T*)paddr();
    }

    usize paddr() const { return (_raw & PPN_MASK) << 2; }

    u64 flags() const { return _raw & FLAGS_MASK; }

    void paddr(u64 paddr) { _raw = (((paddr & PADDR_MASK) >> 2)) | flags(); }

    void flags(u64 flags) { _raw = (flags & FLAGS_MASK) | (_raw & PPN_MASK); }

    bool present() const { return _raw & VALID; }

    bool isLeaf() const { return present() and (_raw & (READ | WRITE | EXEC)); }
};

static_assert(sizeof(Entry) == 8);

export template <usize L>
struct [[gnu::packed]] Pml {
    constexpr static usize LEVEL = L; // Level 3 = Root, Level 1 = Leaf
    constexpr static usize LEN = 512;

    using Lower = Pml<L - 1>;

    Entry pages[LEN];

    Entry& operator[](usize i) { return pages[i]; }

    Entry const& operator[](usize i) const { return pages[i]; }

    usize virt2index(usize virt) const {
        return (virt >> (12 + (LEVEL - 1) * 9)) & 0x1ff;
    }

    usize index2virt(usize index) const {
        return index << (12 + (LEVEL - 1) * 9);
    }

    Opt<usize> virt2phys(usize virt) const {
        Entry page = pages[virt2index(virt)];

        if (not page.present()) {
            return NONE;
        }

        if (page.isLeaf() or LEVEL == 1) {
            usize page_offset_mask = (1ULL << (12 + (LEVEL - 1) * 9)) - 1;
            return page.paddr() + (virt & page_offset_mask);
        }

        auto* pml = (Lower*)page.paddr();
        return pml->virt2phys(virt);
    }

    Entry pageAt(usize vaddr) {
        return pages[virt2index(vaddr)];
    }

    void putPage(usize vaddr, Entry page) {
        pages[virt2index(vaddr)] = page;
    }

    bool empty() const {
        for (auto page : pages) {
            if (page.present()) {
                return false;
            }
        }

        return true;
    }
};

static_assert(sizeof(Pml<1>) == 0x1000);

} // namespace Riscv::Sv39
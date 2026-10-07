module;

#include <karm/macros>

export module Vaerk.Base:vmm;

import Karm.Core;
import :mem;
import :pmm;

using namespace Karm;

namespace Vaerk {

export struct Vmm;

export enum struct VmmFlags : u64 {
    READ = (1 << 0),
    WRITE = (1 << 1),
    EXEC = (1 << 2),
    USER = (1 << 3),
    GLOBAL = (1 << 4),
    UNCACHED = (1 << 5),
};

export using VmmRange = Range<usize, struct _VmmRangeTag>;

export inline VmmRange identityMapped(PmmRange range) {
    return {range.start, range.size};
}

export inline VmmRange upperHalfMapped(PmmRange range) {
    return {UPPER_HALF + range.start, range.size};
}

export using VmmMem = Mem<Vmm, VmmRange>;

export struct Vmm {
    using enum VmmFlags;

    virtual ~Vmm() = default;

    virtual Res<VmmRange> mapRange(VmmRange vaddr, PmmRange paddr, Flags<VmmFlags> flags) = 0;

    Res<VmmMem> mapOwned(VmmRange vaddr, PmmRange paddr, Flags<VmmFlags> flags) {
        auto range = try$(mapRange(vaddr, paddr, flags));
        return Ok(VmmMem{*this, range});
    }

    virtual Res<> free(VmmRange vaddr) = 0;

    virtual Res<> update(VmmRange vaddr, Flags<VmmFlags> flags) = 0;

    virtual Res<> flush(VmmRange vaddr) = 0;

    virtual void dump() = 0;

    virtual void activate() = 0;

    virtual usize root() = 0;
};

} // namespace Vaerk

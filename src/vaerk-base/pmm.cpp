module;

#include <karm/macros>

export module Vaerk.Base:pmm;

import Karm.Core;
import :mem;

using namespace Karm;

namespace Vaerk {

export struct Pmm;

export enum struct PmmFlags : u64 {
    LOWER = 1 << 0,
    UPPER = 1 << 1,
    DMA = 1 << 2,
};

export using PmmRange = Range<usize, struct PmmRangeTag>;
export using PmmMem = Mem<Pmm, PmmRange>;

export struct Pmm {
    using enum PmmFlags;

    virtual ~Pmm() = default;

    virtual Res<PmmRange> allocRange(usize size, Flags<PmmFlags> flags) = 0;

    Res<PmmMem> allocOwned(usize size, Flags<PmmFlags> flags) {
        return Ok(PmmMem{*this, try$(allocRange(size, flags))});
    }

    virtual Res<> used(PmmRange range, Flags<PmmFlags> flags = {}) = 0;

    virtual Res<> free(PmmRange range) = 0;
};

} // namespace Vaerk

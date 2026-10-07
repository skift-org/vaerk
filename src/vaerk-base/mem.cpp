export module Vaerk.Base:mem;

import Karm.Core;

using namespace Karm;

namespace Vaerk {

export inline constexpr usize PAGE_SIZE = 0x1000;

#ifdef __ck_paging_sv39__
export inline constexpr usize UPPER_HALF = 0xffffffff00000000;
#elifdef __ck_bits_64__
export inline constexpr usize UPPER_HALF = 0xffff800000000000;
#else
export inline constexpr usize UPPER_HALF = 0xC0000000;
#endif

export inline usize pageAlignDown(usize addr) {
    return alignDown(addr, PAGE_SIZE);
}

export inline usize pageAlignUp(usize addr) {
    return alignUp(addr, PAGE_SIZE);
}

export inline bool isPageAlign(usize addr) {
    return aligned(addr, PAGE_SIZE);
}

export struct IdentityMapper {
    template <typename T>
    T map(T addr) { return addr; }

    template <typename T>
    T unmap(T addr) { return addr; }
};

export struct UpperHalfMapper {
    template <typename T>
    T map(T addr) { return (T)((usize)addr + UPPER_HALF); }

    template <typename T>
    T unmap(T addr) { return (T)((usize)addr - UPPER_HALF); }
};

export template <typename Owner, typename Range>
struct Mem {
    Owner* _owner = nullptr;
    Range _range = {};

    Mem(Owner& owner, Range range)
        : _owner(&owner),
          _range(range) {}

    ~Mem() {
        if (_owner)
            _owner->free(_range).expect("failed to free memory");
    }

    Mem(Mem const&) = delete;

    Mem(Mem&& other)
        : _owner(std::exchange(other._owner, nullptr)),
          _range(std::exchange(other._range, {})) {
    }

    Mem& operator=(Mem const&) = delete;

    Mem& operator=(Mem&& other) {
        _owner = std::exchange(other._owner, nullptr);
        _range = std::exchange(other._range, {});
        return *this;
    }

    Range range() const {
        return _range;
    }
};

} // namespace Vaerk

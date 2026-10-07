module;

#include <karm/macros>

export module Vaerk.Riscv:vmm;

import Karm.Core;
import Karm.Logger;
import Vaerk.Base;
import :intrinsics;
import :sv39;

using namespace Karm;

namespace Riscv {

export template <typename Mapper = Vaerk::IdentityMapper>
struct Vmm : Vaerk::Vmm {
    using Entry = Sv39::Entry;

    Vaerk::Pmm& _pmm;
    Sv39::Pml<3>* _root = nullptr;
    Mapper _mapper;

    Vmm(Vaerk::Pmm& pmm, Sv39::Pml<3>* root, Mapper mapper = {})
        : _pmm(pmm),
          _root(root),
          _mapper(mapper) {}

    template <usize L>
    Res<Sv39::Pml<L - 1>*> pml(Sv39::Pml<L>& upper, usize vaddr) {
        auto page = upper.pageAt(vaddr);

        if (not page.present())
            return Error::invalidInput("page not present");

        return Ok(_mapper.map(page.template as<Sv39::Pml<L - 1>>()));
    }

    template <usize L>
    Res<Sv39::Pml<L - 1>*> pmlOrAlloc(Sv39::Pml<L>& upper, usize vaddr) {
        auto page = upper.pageAt(vaddr);

        if (page.present()) {
            return Ok(_mapper.map(page.template as<Sv39::Pml<L - 1>>()));
        }

        usize lower = try$(_pmm.allocRange(Vaerk::PAGE_SIZE, {})).start;
        std::memset(_mapper.map((void*)lower), 0, Vaerk::PAGE_SIZE);

        upper.putPage(vaddr, {lower, Entry::VALID});
        return Ok(_mapper.map((Sv39::Pml<L - 1>*)lower));
    }

    Res<> allocPage(usize vaddr, usize paddr, Flags<Vaerk::VmmFlags> flags) {
        auto pml2 = try$(pmlOrAlloc(*_root, vaddr));
        auto pml1 = try$(pmlOrAlloc(*pml2, vaddr));

        pml1->putPage(vaddr, {paddr, Entry::makeFlags(flags) | Entry::ACCESSED | Entry::DIRTY | Entry::VALID});

        return Ok();
    }

    Res<> freePage(usize vaddr) {
        auto pml2 = try$(pml(*_root, vaddr));
        auto pml1 = try$(pml(*pml2, vaddr));
        pml1->putPage(vaddr, {});

        if (pml1->empty()) {
            pml2->putPage(vaddr, {});
            try$(_pmm.free({_mapper.unmap((usize)pml1), Vaerk::PAGE_SIZE}));
        }

        if (pml2->empty()) {
            _root->putPage(vaddr, {});
            try$(_pmm.free({_mapper.unmap((usize)pml2), Vaerk::PAGE_SIZE}));
        }

        return Ok();
    }

    Res<Vaerk::VmmRange> mapRange(Vaerk::VmmRange vaddr, Vaerk::PmmRange paddr, Flags<Vaerk::VmmFlags> flags) override {
        if (paddr.size != vaddr.size) {
            return Error::invalidInput();
        }

        for (usize page = 0; page < vaddr.size; page += Vaerk::PAGE_SIZE) {
            try$(allocPage(vaddr.start + page, paddr.start + page, flags));
        }

        return Ok(vaddr);
    }

    Res<> free(Vaerk::VmmRange vaddr) override {
        for (usize page = 0; page < vaddr.size; page += Vaerk::PAGE_SIZE) {
            try$(freePage(vaddr.start + page));
        }

        return Ok();
    }

    Res<> update(Vaerk::VmmRange, Flags<Vaerk::VmmFlags>) override {
        notImplemented();
    }

    Res<> flush(Vaerk::VmmRange vaddr) override {
        for (usize i = 0; i < vaddr.size; i += Vaerk::PAGE_SIZE) {
            Riscv::sfenceVma(vaddr.start + i);
        }

        return Ok();
    }

    void activate() override {
        Riscv::csrw(Riscv::Csr::SATP, (8uz << 60) | (root() >> 12));
        Riscv::sfenceVma();
    }

    struct Context {
        usize vstart = -1;
        usize vend = 0;

        usize pstart = 0;
        usize pend = 0;

        void next(usize vaddr, usize paddr) {
            if (vstart == (usize)-1) {
                vstart = vaddr;
                vend = vaddr;

                pstart = paddr;
                pend = paddr;
                return;
            }

            if ((vend + Vaerk::PAGE_SIZE != vaddr) or
                (pend + Vaerk::PAGE_SIZE != paddr)) {

                logInfo("riscv: vmm: {:x}-{:x} {:x}-{:x}", vstart, vend + Vaerk::PAGE_SIZE, pstart, pend + Vaerk::PAGE_SIZE);
                vstart = vaddr;
                vend = vaddr;

                pstart = paddr;
                pend = paddr;
            } else {
                vend += Vaerk::PAGE_SIZE;
                pend += Vaerk::PAGE_SIZE;
            }
        }
    };

    template <usize L>
    void _dumpPml(Context& ctx, Sv39::Pml<L>& pml, usize vaddr) {
        for (usize i = 0; i < 512; i++) {
            auto page = pml[i];
            usize curr = pml.index2virt(i) | vaddr;
            if (page.present()) {
                if constexpr (L == 1) {
                    ctx.next(curr, page.paddr());
                } else {
                    auto& lower = *_mapper.map(page.template as<Sv39::Pml<L - 1>>());
                    _dumpPml(ctx, lower, curr);
                }
            }
        }
    }

    void dump() override {
        logInfo("riscv: vmm: dump root {:x}", (usize)_root);
        Context ctx{};
        _dumpPml(ctx, *_root, 0);
        ctx.next(0, 0);
    }

    usize root() override {
        return _mapper.unmap((usize)_root);
    }
};

} // namespace Riscv

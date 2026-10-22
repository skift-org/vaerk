export module Vaerk.Acpi:mcfg;

import Karm.Core;
import :sdth;

using namespace Karm;

namespace Vaerk::Acpi {

export struct [[gnu::packed]] Mcfg : Sdth {
    static constexpr Signature SIGNATURE = {'M', 'C', 'F', 'G'};

    struct [[gnu::packed]] Record {
        u64 address;
        u16 segmentGroup;
        u8 busStart;
        u8 busEnd;
        u32 reserved;

        void repr(Io::Emit& e) const {
            e("address: {}\n", address);
            e("segmentGroup: {}\n", segmentGroup);
            e("busStart: {}\n", busStart);
            e("busEnd: {}\n", busEnd);
            e("reserved: {}\n", reserved);
        }
    };

    u64 reserved;
    Record records[];

    usize count() const {
        return (len - sizeof(Sdth) - sizeof(reserved)) / sizeof(Record);
    }

    void repr(Io::Emit& e) const {
        Sdth::repr(e);
        e("reserved: {}\n", reserved);
        e("count: {}\n", count());
        for (usize i = 0; i < count(); i++) {
            e("records[{}]:\n", i);
            records[i].repr(e);
        }
    }
};

} // namespace Vaerk::Acpi

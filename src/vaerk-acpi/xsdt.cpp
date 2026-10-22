export module Vaerk.Acpi:xsdt;

import Karm.Core;
import :sdth;

using namespace Karm;

namespace Vaerk::Acpi {

export struct [[gnu::packed]] Xsdt : Sdth {
    u64 children[];

    usize count() const {
        return (len - sizeof(Sdth)) / sizeof(u64);
    }

    void repr(Io::Emit& e) const {
        Sdth::repr(e);
        e("count: {}\n", count());
        for (usize i = 0; i < count(); i++)
            e("children[{}]: {}\n", i, children[i]);
    }
};

} // namespace Vaerk::Acpi

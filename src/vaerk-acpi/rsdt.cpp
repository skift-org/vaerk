export module Vaerk.Acpi:rsdt;

import Karm.Core;
import :sdth;

using namespace Karm;

namespace Vaerk::Acpi {

export struct [[gnu::packed]] Rsdt : Sdth {
    u32 children[];

    usize count() const {
        return (len - sizeof(Sdth)) / sizeof(u32);
    }

    void repr(Io::Emit& e) const {
        Sdth::repr(e);
        e("count: {}\n", count());
        for (usize i = 0; i < count(); i++)
            e("children[{}]: {}\n", i, children[i]);
    }
};

} // namespace Vaerk::Acpi

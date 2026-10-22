export module Vaerk.Acpi:sdth;

import Karm.Core;

using namespace Karm;

namespace Vaerk::Acpi {

using Signature = Array<char, 4>;

export struct [[gnu::packed]] Sdth {
    Signature signature;
    u32 len;
    u8 revision;
    u8 checksum;
    Array<char, 6> oemId;
    Array<char, 8> oemTableId;
    u32 oemRevision;
    u32 creatorId;
    u32 creatorRevision;

    void repr(Io::Emit& e) const {
        e("signature: {}\n", signature);
        e("len: {}\n", len);
        e("revision: {}\n", revision);
        e("checksum: {}\n", checksum);
        e("oemId: {}\n", oemId);
        e("oemTableId: {}\n", oemTableId);
        e("oemRevision: {}\n", oemRevision);
        e("creatorId: {}\n", creatorId);
        e("creatorRevision: {}\n", creatorRevision);
    }

    template <typename T>
    T const* as() const {
        return reinterpret_cast<T const*>(this);
    }
};

} // namespace Vaerk::Acpi

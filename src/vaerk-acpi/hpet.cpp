export module Vaerk.Acpi:hpet;

import Karm.Core;
import :sdth;

using namespace Karm;

namespace Vaerk::Acpi {

export struct [[gnu::packed]] Hpet : Sdth {
    static constexpr Signature SIGNATURE = {'H', 'P', 'E', 'T'};

    u8 hardwareRevId;
    u8 info;
    u16 pciVendorId;
    u8 addressSpaceId;
    u8 registerBitWidth;
    u8 registerBitOffset;
    u8 reserved1;
    u64 address;
    u8 hpetNumber;
    u16 minimumTick;
    u8 pageProtection;

    void repr(Io::Emit& e) const {
        Sdth::repr(e);
        e("hardwareRevId: {}\n", hardwareRevId);
        e("info: {}\n", info);
        e("pciVendorId: {}\n", pciVendorId);
        e("addressSpaceId: {}\n", addressSpaceId);
        e("registerBitWidth: {}\n", registerBitWidth);
        e("registerBitOffset: {}\n", registerBitOffset);
        e("reserved1: {}\n", reserved1);
        e("address: {}\n", address);
        e("hpetNumber: {}\n", hpetNumber);
        e("minimumTick: {}\n", minimumTick);
        e("pageProtection: {}\n", pageProtection);
    }
};

} // namespace Vaerk::Acpi

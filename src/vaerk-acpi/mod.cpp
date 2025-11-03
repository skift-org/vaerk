export module Vaerk.Acpi;

import Karm.Core;

using namespace Karm;

namespace Vaerk::Acpi {

export struct [[gnu::packed]] Rsdp {
    Array<char, 8> signature;
    u8 checksum;
    Array<char, 6> oemId;
    u8 revision;
    u32 rsdt;
};

export struct [[gnu::packed]] Sdth {
    Array<char, 4> signature;
    u32 len;
    u8 revision;
    u8 checksum;
    Array<char, 6> oemId;
    Array<char, 8> oemTableId;
    u32 oemRevision;
    u32 creatorId;
    u32 creatorRevision;
};

export struct [[gnu::packed]] Rsdt : Sdth {
    u32 children[];
};

export struct [[gnu::packed]] Madt : Sdth {
    enum struct Type {
        LAPIC = 0,
        IOAPIC = 1,
        ISO = 2,
        NMI = 4,
        LAPIC_OVERRIDE = 5
    };

    struct [[gnu::packed]] Record {
        u8 type;
        u8 len;
    };

    struct [[gnu::packed]] LapicRecord : Record {
        u8 processorId;
        u8 id;
        u32 flags;
    };

    struct [[gnu::packed]] IoapicRecord : Record {
        u8 id;
        u8 reserved;
        u32 address;
        u32 interruptBase;
    };

    struct [[gnu::packed]] IsoRecord : Record {
        u8 bus;
        u8 irq;
        u32 gsi;
        u16 flags;
    };

    u32 lapic;
    u32 flags;

    Record records[];
};

export struct [[gnu::packed]] Mcfg : Sdth {
    struct Record {
        u64 address;
        u16 segment_groupe;
        u8 busStart;
        u8 busEnd;
        u32 reserved;
    };

    u64 reserved;
    Record records[];
};

export struct [[gnu::packed]] Hpet : Sdth {
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
};

} // namespace Vaerk::Acpi

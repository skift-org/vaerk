export module Vaerk.Acpi;

import Karm.Core;
import Karm.Logger;

using namespace Karm;

namespace Vaerk::Acpi {

// MARK: RSDP (Root System Description Pointer) --------------------------------

export struct [[gnu::packed]] Rsdp {
    static constexpr Array<char, 8> SIGNATURE = {'R', 'S', 'D', ' ', 'P', 'T', 'R', ' '};

    Array<char, 8> signature;
    u8 checksum;
    Array<char, 6> oemId;
    u8 revision;
    u32 rsdt;

    // ACPI 2.0+ fields
    u32 length;
    u64 xsdt;
    u8 extendedChecksum;
    Array<u8, 3> reserved;

    bool isValid() const {
        return signature == SIGNATURE;
    }

    bool isAcpi2() const {
        return revision >= 2;
    }
};

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

    template <typename T>
    T const* as() const {
        return reinterpret_cast<T const*>(this);
    }
};

export struct [[gnu::packed]] Rsdt : Sdth {
    u32 children[];

    usize count() const {
        return (len - sizeof(Sdth)) / sizeof(u32);
    }
};

// MARK: XSDT (Extended System Description Table - 64-bit) ---------------------

export struct [[gnu::packed]] Xsdt : Sdth {
    u64 children[];

    usize count() const {
        return (len - sizeof(Sdth)) / sizeof(u64);
    }
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
    static constexpr Signature SIGNATURE = {'M', 'C', 'F', 'G'};

    struct [[gnu::packed]] Record {
        u64 address;
        u16 segmentGroup;
        u8 busStart;
        u8 busEnd;
        u32 reserved;
    };

    u64 reserved;
    Record records[];

    usize count() const {
        return (len - sizeof(Sdth) - sizeof(reserved)) / sizeof(Record);
    }
};

// MARK: FADT (Fixed ACPI Description Table) -----------------------------------

export struct [[gnu::packed]] Fadt : Sdth {
    static constexpr Signature SIGNATURE = {'F', 'A', 'C', 'P'};

    enum BootFlags : u16 {
        LEGACY_DEVICES = 1 << 0,     // Legacy devices supported (8259, etc.)
        HAS_8042 = 1 << 1,           // 8042 keyboard controller present
        VGA_NOT_PRESENT = 1 << 2,    // VGA not present
        MSI_NOT_SUPPORTED = 1 << 3,  // MSI not supported
        PCIE_ASPM = 1 << 4,          // PCIe ASPM controls
        CMOS_RTC_NOT_PRESENT = 1 << 5, // CMOS RTC not present
    };

    u32 firmwareCtrl;
    u32 dsdt;
    u8 reserved1;
    u8 preferredPmProfile;
    u16 sciInt;
    u32 smiCmd;
    u8 acpiEnable;
    u8 acpiDisable;
    u8 s4BiosReq;
    u8 pStateCtrl;
    u32 pm1aEvtBlk;
    u32 pm1bEvtBlk;
    u32 pm1aCtrlBlk;
    u32 pm1bCtrlBlk;
    u32 pm2CtrlBlk;
    u32 pmTmrBlk;
    u32 gpe0Blk;
    u32 gpe1Blk;
    u8 pm1EvtLen;
    u8 pm1CtrlLen;
    u8 pm2CtrlLen;
    u8 pmTmrLen;
    u8 gpe0BlkLen;
    u8 gpe1BlkLen;
    u8 gpe1Base;
    u8 cstCtrl;
    u16 pLvl2Lat;
    u16 pLvl3Lat;
    u16 flushSize;
    u16 flushStride;
    u8 dutyOffset;
    u8 dutyWidth;
    u8 dayAlrm;
    u8 monAlrm;
    u8 century;
    u16 bootFlags;
    u8 reserved2;
    u32 flags;

    bool has8042() const {
        return bootFlags & HAS_8042;
    }

    bool hasLegacyDevices() const {
        return bootFlags & LEGACY_DEVICES;
    }

    bool hasCmosRtc() const {
        return not(bootFlags & CMOS_RTC_NOT_PRESENT);
    }
};

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
};

// MARK: Helper functions ------------------------------------------------------

export template <typename Func>
void iterTables(Rsdp const& rsdp, usize kernelBase, Func&& func) {
    if (rsdp.isAcpi2() and rsdp.xsdt != 0) {
        auto* xsdt = reinterpret_cast<Xsdt const*>(rsdp.xsdt + kernelBase);
        for (usize i = 0; i < xsdt->count(); i++) {
            auto* sdth = reinterpret_cast<Sdth const*>(xsdt->children[i] + kernelBase);
            func(sdth);
        }
    } else {
        auto* rsdt = reinterpret_cast<Rsdt const*>(rsdp.rsdt + kernelBase);
        for (usize i = 0; i < rsdt->count(); i++) {
            auto* sdth = reinterpret_cast<Sdth const*>(rsdt->children[i] + kernelBase);
            func(sdth);
        }
    }
}

export template <typename T>
T const* findTable(Rsdp const& rsdp, usize kernelBase) {
    T const* result = nullptr;
    iterTables(rsdp, kernelBase, [&](Sdth const* table) {
        if (table->signature == T::SIGNATURE) {
            result = table->as<T>();
        }
    });
    return result;
}

} // namespace Vaerk::Acpi

export module Vaerk.Pci;

import Karm.Core;
import Vaerk.Base;

using namespace Karm;

namespace Vaerk::Pci {

export struct Addr {
    u16 seg;
    u8 bus;
    u8 slot;
    u8 func;

    bool operator==(Addr const& other) const = default;

    usize ecamOffset() const {
        return ((usize)bus << 20) | ((usize)slot << 15) | ((usize)func << 12);
    }
};

export struct Id {
    u16 vendor;
    u16 device;

    bool operator==(Id const& other) const = default;

    static constexpr u16 INVALID = 0xFFFF;

    bool valid() const {
        return vendor != INVALID and vendor != 0x0000;
    }
};

export struct Bar {
    enum struct Type {
        NONE,
        PIO,
        MMIO32,
        MMIO64
    };
    using enum Type;

    Type type = NONE;
    urange range{};
    bool prefetch = false;

    static Bar parse(u32 barLow, u32 sizeLow, u32 barHigh = 0, u32 sizeHigh = 0) {
        Bar bar;

        if (barLow == 0 and sizeLow == 0)
            return bar;

        if (barLow & 0x1) {
            // I/O BAR
            bar.type = PIO;
            bar.range.start = barLow & ~0x3u;
            bar.range.size = ~(sizeLow & ~0x3u) + 1;
            bar.range.size &= 0xFFFF; // I/O ports are 16-bit
        } else {
            // Memory BAR
            u8 memType = (barLow >> 1) & 0x3;
            bar.prefetch = (barLow & 0x8) != 0;

            if (memType == 0x2) {
                // 64-bit BAR
                bar.type = MMIO64;
                bar.range.start = ((u64)barHigh << 32) | (barLow & ~0xFull);
                u64 sizeMask = ((u64)sizeHigh << 32) | (sizeLow & ~0xFull);
                bar.range.size = ~sizeMask + 1;
            } else {
                // 32-bit BAR
                bar.type = MMIO32;
                bar.range.start = barLow & ~0xFull;
                bar.range.size = ~(sizeLow & ~0xFull) + 1;
                bar.range.size &= 0xFFFFFFFF;
            }
        }

        return bar;
    }
};

enum struct Class : u8 {
    UNCLASSIFIED = 0x00,
    MASS_STORAGE = 0x01,
    NETWORK = 0x02,
    DISPLAY = 0x03,
    MULTIMEDIA = 0x04,
    MEMORY = 0x05,
    BRIDGE = 0x06,
    SIMPLE_COMM = 0x07,
    BASE_PERIPHERAL = 0x08,
    INPUT = 0x09,
    DOCKING = 0x0A,
    PROCESSOR = 0x0B,
    SERIAL_BUS = 0x0C,
    WIRELESS = 0x0D,
    INTELLIGENT_IO = 0x0E,
    SATELLITE = 0x0F,
    ENCRYPTION = 0x10,
    SIGNAL_PROC = 0x11,
};

struct SubClass {
    Class class_;
    u8 subclass;

    constexpr SubClass(Class class_, u8 subClass) : class_(class_), subclass(subClass) {}
};

export constexpr SubClass PCI_TO_PCI_BRIDGE = {Class::BRIDGE, 0x04};

export struct [[gnu::packed]] ConfigSpace {
    u16 vendorId;
    u16 deviceId;
    u16 command;
    u16 status;
    u8 revisionId;
    u8 progIf;
    u8 subclass;
    u8 classCode;
    u8 cacheLineSize;
    u8 latencyTimer;
    u8 headerType;
    u8 bist;

    // Header Type 0x0
    struct [[gnu::packed]] Type0 {
        u32 bar[6];
        u32 cardbusCisPtr;
        u16 subsystemVendorId;
        u16 subsystemId;
        u32 expansionRomBase;
        u8 capabilitiesPtr;
        u8 reserved[7];
        u8 interruptLine;
        u8 interruptPin;
        u8 minGrant;
        u8 maxLatency;
    };

    // Header Type 0x1 (PCI-to-PCI bridge)
    struct [[gnu::packed]] Type1 {
        u32 bar[2];
        u8 primaryBus;
        u8 secondaryBus;
        u8 subordinateBus;
        u8 secondaryLatency;
        u8 ioBase;
        u8 ioLimit;
        u16 secondaryStatus;
        u16 memoryBase;
        u16 memoryLimit;
        u16 prefetchMemoryBase;
        u16 prefetchMemoryLimit;
        u32 prefetchBaseUpper;
        u32 prefetchLimitUpper;
        u16 ioBaseUpper;
        u16 ioLimitUpper;
        u8 capabilitiesPtr;
        u8 reserved[3];
        u32 expansionRomBase;
        u8 interruptLine;
        u8 interruptPin;
        u16 bridgeControl;
    };

    // Header Type 0x2 (PCI-to-CardBus bridge)

    struct [[gnu::packed]] Type2 {
        u32 cardBusSocketBase;
        u8 capabilitiesPtr;
        u8 reserved;
        u16 secondaryStatus;
        u8 pciBus;
        u8 cardBusBus;
        u8 subordinateBus;
        u8 cardBusLatency;
        u32 memoryBase0;
        u32 memoryLimit0;
        u32 memoryBase1;
        u32 memoryLimit1;
        u32 ioBase0;
        u32 ioLimit0;
        u32 ioBase1;
        u32 ioLimit1;
        u8 interruptLine;
        u8 interruptPin;
        u16 bridgeControl;
    };

    union {
        Type0 type0;
        Type1 type1;
        Type2 type2;
    };

    bool isMultiFunction() const {
        return (headerType & 0x80) != 0;
    }

    u8 headerTypeKind() const {
        return headerType & 0x7F;
    }

    bool isBridge() const {
        return headerTypeKind() == 1;
    }

    SubClass subClass() {
        return {static_cast<Class>(classCode), subclass};
    }

    u8 secondaryBus() const {
        if (not isBridge())
            panic("expected bridge");
        return type1.secondaryBus;
    }

    u8 subordinateBus() const {
        if (not isBridge())
            panic("expected bridge");
        return type1.subordinateBus;
    }
};

static_assert(sizeof(ConfigSpace) == 64);

struct EcamDevice {
    void* _base;

    ConfigSpace& config() {
        return *reinterpret_cast<ConfigSpace*>(_base);
    }

    ConfigSpace const& config() const {
        return *reinterpret_cast<ConfigSpace const*>(_base);
    }

    Id id() const {
        return {config().vendorId, config().deviceId};
    }

    bool valid() const { return id().valid(); }

    Array<Bar, 6> probBars() {
        Array<Bar, 6> res = {};

        usize nbar = config().isBridge() ? 2 : 6;
        u32* bars = config().isBridge() ? config().type1.bar : config().type0.bar;

        for (usize i = 0; i < nbar;) {
            u32 origBar = mmioRead<u32>(&bars[i]);
            if (origBar == 0) {
                i++;
                continue;
            }

            mmioWrite<u32>(&bars[i], 0xFFFFFFFF);
            u32 sizeMask = mmioRead<u32>(&bars[i]);
            mmioWrite<u32>(&bars[i], origBar);

            if (sizeMask == 0 or sizeMask == 0xFFFFFFFF) {
                i++;
                continue;
            }

            bool is64Bit = ((origBar & 0x1) == 0) and (((origBar >> 1) & 0x3) == 0x2);

            if (is64Bit and i + 1 < nbar) {
                u32 origBarHigh = mmioRead<u32>(&bars[i + 1]);
                mmioWrite<u32>(&bars[i + 1], 0xFFFFFFFF);
                u32 sizeMaskHigh = mmioRead<u32>(&bars[i + 1]);
                mmioWrite<u32>(&bars[i + 1], origBarHigh);

                res[i] = Bar::parse(origBar, sizeMask, origBarHigh, sizeMaskHigh);
                i += 2;
            } else {
                res[i] = Bar::parse(origBar, sizeMask);
                i++;
            }
        }

        return res;
    }
};

struct Ecam {
    void* _base;

    EcamDevice at(Addr addr) {
        return {static_cast<u8*>(_base) + addr.ecamOffset()};
    }
};

} // namespace Vaerk::Pci

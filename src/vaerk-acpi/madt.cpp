export module Vaerk.Acpi:madt;

import Karm.Core;
import :sdth;

using namespace Karm;

namespace Vaerk::Acpi {

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

        void repr(Io::Emit& e) const {
            e("type: {}\n", type);
            e("len: {}\n", len);
        }
    };

    struct [[gnu::packed]] LapicRecord : Record {
        u8 processorId;
        u8 id;
        u32 flags;

        void repr(Io::Emit& e) const {
            Record::repr(e);
            e("processorId: {}\n", processorId);
            e("id: {}\n", id);
            e("flags: {}\n", flags);
        }
    };

    struct [[gnu::packed]] IoapicRecord : Record {
        u8 id;
        u8 reserved;
        u32 address;
        u32 interruptBase;

        void repr(Io::Emit& e) const {
            Record::repr(e);
            e("id: {}\n", id);
            e("reserved: {}\n", reserved);
            e("address: {}\n", address);
            e("interruptBase: {}\n", interruptBase);
        }
    };

    struct [[gnu::packed]] IsoRecord : Record {
        u8 bus;
        u8 irq;
        u32 gsi;
        u16 flags;

        void repr(Io::Emit& e) const {
            Record::repr(e);
            e("bus: {}\n", bus);
            e("irq: {}\n", irq);
            e("gsi: {}\n", gsi);
            e("flags: {}\n", flags);
        }
    };

    u32 lapic;
    u32 flags;

    Record records[];

    void repr(Io::Emit& e) const {
        Sdth::repr(e);
        e("lapic: {}\n", lapic);
        e("flags: {}\n", flags);

        auto* curr = reinterpret_cast<u8 const*>(records);
        auto* end = reinterpret_cast<u8 const*>(this) + len;
        usize i = 0;

        while (curr + sizeof(Record) <= end) {
            auto* record = reinterpret_cast<Record const*>(curr);
            if (record->len < sizeof(Record))
                break;

            e("records[{}]:\n", i++);
            switch (static_cast<Type>(record->type)) {
            case Type::LAPIC:
                reinterpret_cast<LapicRecord const*>(record)->repr(e);
                break;
            case Type::IOAPIC:
                reinterpret_cast<IoapicRecord const*>(record)->repr(e);
                break;
            case Type::ISO:
                reinterpret_cast<IsoRecord const*>(record)->repr(e);
                break;
            default:
                record->repr(e);
                break;
            }

            curr += record->len;
        }
    }
};

} // namespace Vaerk::Acpi

export module Vaerk.Acpi:rsdp;

import Karm.Core;
import :rsdt;
import :sdth;
import :xsdt;

using namespace Karm;

namespace Vaerk::Acpi {

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

    void repr(Io::Emit& e) const {
        e("signature: {}\n", signature);
        e("checksum: {}\n", checksum);
        e("oemId: {}\n", oemId);
        e("revision: {}\n", revision);
        e("rsdt: {}\n", rsdt);

        if (isAcpi2()) {
            e("length: {}\n", length);
            e("xsdt: {}\n", xsdt);
            e("extendedChecksum: {}\n", extendedChecksum);
        }
    }

    template <typename Func>
    void iterTables(usize kernelBase, Func&& func) const {
        if (isAcpi2() and xsdt != 0) {
            auto* table = reinterpret_cast<Xsdt const*>(xsdt + kernelBase);
            for (usize i = 0; i < table->count(); i++) {
                auto* sdth = reinterpret_cast<Sdth const*>(table->children[i] + kernelBase);
                func(sdth);
            }
        } else {
            auto* table = reinterpret_cast<Rsdt const*>(rsdt + kernelBase);
            for (usize i = 0; i < table->count(); i++) {
                auto* sdth = reinterpret_cast<Sdth const*>(table->children[i] + kernelBase);
                func(sdth);
            }
        }
    }

    template <typename T>
    T const* findTable(usize kernelBase) const {
        T const* result = nullptr;
        iterTables(kernelBase, [&](Sdth const* table) {
            if (table->signature == T::SIGNATURE) {
                result = table->as<T>();
            }
        });
        return result;
    }
};

} // namespace Vaerk::Acpi

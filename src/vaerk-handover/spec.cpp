export module Vaerk.Handover:spec;

import Karm.Core;

using namespace Karm;

namespace Handover {

#ifdef __ck_paging_sv39__
export constexpr usize UPPER_HALF = 0xffffffff00000000;
#elifdef __ck_bits_64__
export constexpr usize KERNEL_BASE = 0xffffffff80000000;
export constexpr usize UPPER_HALF = 0xffff800000000000;
#else
export inline usize KERNEL_BASE = 0xc0000000;
export inline usize UPPER_HALF = 0xc0000000;
#endif

export constexpr u32 COOLBOOT = 0xc001b001;

#define FOREACH_TAG(TAG)      \
    TAG(FREE, 0)              \
    TAG(MAGIC, COOLBOOT)      \
    TAG(SELF, 0xa24f988d)     \
    TAG(STACK, 0xf65b391b)    \
    TAG(KERNEL, 0xbfc71b20)   \
    TAG(LOADER, 0xf1f80c26)   \
    TAG(BLOB, 0xcbc36d3b)     \
    TAG(RSDP, 0x8d3bbb)       \
    TAG(FDT, 0xb628bbc1)      \
    TAG(FB, 0xe2d55685)       \
    TAG(RESERVED, 0xb8841d2d) \
    TAG(END, 0xffffffff)

export enum struct Tag : u32 {
#define ITER(NAME, VALUE) NAME = VALUE,
    FOREACH_TAG(ITER)
#undef ITER
};

export using enum Tag;

export inline char const* tagName(Tag tag) {
    switch (tag) {
#define ITER(NAME, VALUE) \
    case Tag::NAME:       \
        return #NAME;
        FOREACH_TAG(ITER)
#undef ITER
    }
    return "UNKNOWN";
}

export enum struct PixelFormat : u16 {
    RGBX8888 = 0x7451,
    BGRX8888 = 0xd040,
};

export struct Record {
    Tag tag;
    u32 flags = 0;
    u64 start = 0;
    u64 size = 0;

    char const* name() const {
        return tagName(tag);
    }

    union {
        struct
        {
            u16 width;
            u16 height;
            u16 pitch;
            PixelFormat format;
        } fb;

        struct
        {
            u32 name;
            u32 meta;
        } blob;

        u64 more;
    };

    u64 end() const {
        return start + size;
    }

    bool empty() const {
        return size == 0;
    }

    template <typename R>
    R range() const {
        return R{
            static_cast<usize>(start),
            static_cast<usize>(size),
        };
    }
};

export struct Payload {
    u32 magic, agent, size, len;
    Record records[];

    char const* stringAt(u64 offset) const {
        if (offset == 0) {
            return "";
        }
        char const* data = reinterpret_cast<char const*>(this);
        return data + offset;
    }

    char const* agentName() const {
        return stringAt(agent);
    }

    Record const* findTag(Tag tag) const {
        for (auto const& r : *this) {
            if (r.tag == tag) {
                return &r;
            }
        }

        return nullptr;
    }

    Record const* blobByName(char const* name) const {
        for (auto const& r : *this) {
            if (r.tag == BLOB and cstrEq(stringAt(r.blob.name), name)) {
                return &r;
            }
        }

        return nullptr;
    }

    Record* begin() {
        return records;
    }

    Record* end() {
        return records + len;
    }

    Record const* begin() const {
        return records;
    }

    Record const* end() const {
        return records + len;
    }

    usize sum(Tag tag) {
        usize total = 0;
        for (auto const& r : *this) {
            if (r.tag == tag) {
                total += r.size;
            }
        }
        return total;
    }

    Record find(usize size) {
        for (auto& r : *this) {
            if (r.tag == FREE and r.size >= size) {
                return r;
            }
        }

        return {};
    }

    template <typename R>
    R usableRange() const {
        bool first = true;
        usize start = 0, end = 0;

        for (auto const& r : *this) {
            if (r.tag == FREE) {
                if (r.start < start or first) {
                    start = r.start;
                    first = false;
                }
                if (r.end() > end) {
                    end = r.end();
                }
            }
        }

        return R{
            start,
            end - start,
        };
    }
};

export struct Request {
    Tag tag;
    u32 flags;
    u64 more;

    char const* name() const {
        return tagName(tag);
    }
};

export constexpr Request requestSelf() {
    return {SELF, 0, 0};
}

export constexpr Request requestStack(u64 preferedSize = 64 * 1024) {
    return {STACK, 0, preferedSize};
}

export constexpr Request requestKernel() {
    return {KERNEL, 0, 0};
}

export constexpr Request requestBlobs() {
    return {BLOB, 0, 0};
}

export constexpr Request requestRsdp() {
    return {RSDP, 0, 0};
}

export constexpr Request requestFdt() {
    return {FDT, 0, 0};
}

export constexpr Request requestFb(PixelFormat preferredFormat = PixelFormat::BGRX8888) {
    return {FB, 0, static_cast<u64>(preferredFormat)};
}

export bool valid(u32 magic, Payload const& payload) {
    if (magic != COOLBOOT)
        return false;

    if (payload.magic != COOLBOOT)
        return false;

    return true;
}

export constexpr char const* REQUEST_SECTION = ".handover";

export using EntryPoint = void (*)(u64 magic, Payload const* handover);

} // namespace Handover

export template <>
struct Io::Formatter<Handover::Record> {
    Res<> format(TextWriter& writer, Handover::Record record) {
        return Io::format(writer, "Record({}, {:x}-{:x})", record.name(), record.start, record.end());
    }
};
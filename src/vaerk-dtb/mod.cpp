module;

#include <karm/macros>

export module Vaerk.Dtb;

import Karm.Core;

using namespace Karm;

// https://devicetree-specification.readthedocs.io/en/v0.1/flattened-format.html
namespace Vaerk::Dtb {

export using Karm::begin;
export using Karm::end;

export struct Header {
    u32be magic;
    u32be totalSize;
    u32be structureBlockOffset;
    u32be stringsBlockOffset;
    u32be memoryReservationBlockOffset;
    u32be version;
    u32be lastCompatibleVersion;
    u32be bootCpuidPhys;
    u32be stringsBlockSize;
    u32be structureBlockSize;

    Range<u32> structureBlockRange() const {
        return {structureBlockOffset, structureBlockSize};
    }

    Range<u32> stringsRange() const {
        return {stringsBlockOffset, stringsBlockSize};
    }

    Range<u32> dtbRange() const {
        return {0, totalSize};
    }
};

export struct ReserveEntry {
    u64be address;
    u64be size;

    explicit operator bool() const {
        return address or size;
    }
};

// MARK: Token -----------------------------------------------------------------

static constexpr auto RE_SEP = '\0'_re;

static bool _sniffStr(Bytes extra) {
    if (not extra.len())
        return false;

    if (last(extra) != 0)
        return false;

    bool hasPrint = false;
    for (auto b : sub(extra, 0, extra.len() - 1)) {
        if (isAsciiPrint(b)) {
            hasPrint = true;
            continue;
        }
        if (b != '\0')
            return false;
    }
    return hasPrint;
}

// MARK: Tokens ----------------------------------------------------------------

struct Token {
    enum struct Type : u32 {
        BEGIN_NODE = 0x00000001,
        END_NODE = 0x00000002,
        PROP = 0x00000003,
        NOP = 0x00000004,
        END = 0x00000009,
        _LEN,
    };

    using enum Type;

    Type type;
    Str name = "";
    Opt<usize> address = NONE;
    Bytes extra = {};

    void repr(Io::Emit& e) const {
        e("({}", type);
        if (name)
            e(" name:{#}", name);
        if (address)
            e(" address:{:#08x}", address);
        if (extra)
            e(" extra:{:#02x}", extra);
        e(")");
    }
};

// MARK: Iterators -------------------------------------------------------------

Tuple<Str, Opt<usize>> _parseName(Str str) {
    Io::SScan s{str};
    auto name = s.token(Re::until("@"_re));
    Opt<usize> addr = NONE;
    if (s.skip("@")) {
        addr = Io::atou(s.token(Re::xdigit()), {.base = 16});
    }
    return {name, addr};
}

struct TokenIter {
    Bytes strings;
    Io::BScan tokens;

    Opt<Token> next() {
        if (tokens.ended())
            return NONE;
        auto type = static_cast<Token::Type>(tokens.nextU32be());
        if (type == Token::BEGIN_NODE) {
            auto [name, addr] = _parseName(tokens.nextCStr());
            tokens.align(sizeof(u32));
            return Token{.type = type, .name = name, .address = addr};
        } else if (type == Token::END_NODE) {
            return Token{.type = type};
        } else if (type == Token::PROP) {
            auto len = tokens.nextU32be();
            auto nameoff = tokens.nextU32be();
            auto name = Io::BScan{strings}.skip(nameoff).nextCStr();
            auto extra = tokens.nextBytes(len);
            tokens.align(sizeof(u32));
            return Token{.type = type, .name = name, .extra = extra};
        } else if (type == Token::NOP) {
            return Token{.type = type};
        } else if (type == Token::END) {
            return NONE;
        } else {
            panic("invalid token");
        }
    }
};

// MARK: Node ------------------------------------------------------------------

export struct Prop {
    enum struct Type {
        NIL,
        STR,
        U32,
        U64,
        BYTES
    };

    using enum Type;

    Token _token;

    Str name() const {
        return _token.name;
    }

    Type sniff() const {
        if (raw().len() == 0 or (raw().len() == 1 and raw()[0] == '\0'))
            return NIL;
        if (_sniffStr(raw()))
            return STR;
        if ((raw().len() % sizeof(u32)) == 0)
            return U32;
        if ((raw().len() % sizeof(u64)) == 0)
            return U64;
        return BYTES;
    }

    Bytes raw() const {
        return _token.extra;
    }

    Slice<u32be> regs32() const {
        if (sniff() == U32 or sniff() == U64)
            return raw().cast<u32be>();
        return {};
    }

    Slice<u64be> regs64() const {
        if (sniff() == U64)
            return raw().cast<u64be>();
        return {};
    }

    template <typename T>
    Opt<T> as() {
        if (raw().len() != sizeof(T))
            return NONE;
        return raw().cast<T>()[0];
    }

    struct StrIter {
        Io::SScan scan;

        Opt<Str> next() {
            if (scan.ended())
                return NONE;
            scan.skip(RE_SEP);
            return scan.token(Re::until(RE_SEP));
        }
    };

    [[nodiscard]] auto iterStr() const {
        Str str = sub(raw(), 0, raw().len() - 1).cast<char>();
        return StrIter(str);
    }

    void dump(Io::Emit& e) const {
        auto type = sniff();
        if (type == NIL) {
            e("{}", name());
        } else if (type == U32 or type == U64) {
            e("{} = <", name());
            bool first = true;
            for (auto r : regs32()) {
                if (not first)
                    e(" ");
                else
                    first = false;
                e("{:#08x}", r);
            }
            e(">");
        } else if (type == BYTES) {
            e("{} = {:#02x}", name(), raw());
        } else if (type == STR) {
            e("{} = ", name());
            bool first = true;
            for (auto s : iterStr()) {
                if (not first)
                    e(" ");
                else
                    first = false;
                e("{:#}", s);
            }
            e("");
        }
    }
};

export struct Node {
    TokenIter tokens;

    Token token() const {
        auto copy = tokens;
        return copy.next().unwrap();
    }

    Str name() const {
        auto name = token().name;
        if (not name)
            return "/";
        return name;
    }

    struct PropIter {
        TokenIter tokens;

        Opt<Prop> next() {
            auto token = tokens.next();
            if (not token)
                return NONE;
            if (token->type != Token::PROP)
                return NONE;
            return token;
        }
    };

    PropIter iterProp() const {
        auto copy = tokens;
        (void)copy.next(); // skip begin node
        return {copy};
    }

    struct ChildrenIter {
        TokenIter tokens;
        usize depth = 0;

        Opt<Node> next() {
            while (true) {
                auto before = tokens;
                auto token = tokens.next();
                if (not token)
                    return NONE;
                if (token->type == Token::BEGIN_NODE) {
                    depth++;
                    if (depth == 1) {
                        return before;
                    }
                } else if (token->type == Token::END_NODE) {
                    if (depth == 0)
                        return NONE;
                    depth--;
                }
            }
        }
    };

    ChildrenIter iterChildren() const {
        auto copy = tokens;
        (void)copy.next(); // skip begin node
        return {copy};
    }

    Opt<Node> findChildren(Str name) {
        for (auto node : iterChildren()) {
            if (node.name() == name)
                return node;
        }
        return NONE;
    }

    Opt<Prop> getProperty(Str name) {
        for (auto prop : iterProp()) {
            if (prop.name() == name)
                return prop;
        }
        return NONE;
    }

    template <typename T>
    Opt<T> getProperty(Str name) {
        auto prop = try$(getProperty(name));
        return try$(prop.as<T>());
    }

    void dump(Io::Emit& e) const {
        e("{}", name());
        if (token().address) {
            e(" @ {:p}", token().address);
        }
        e(" {");
        e.indentNewline();
        for (auto prop : iterProp()) {
            prop.dump(e);
            e(";\n");
        }
        for (auto child : iterChildren()) {
            child.dump(e);
            e(";\n");
        }
        e.deindent();
        e("}");
    }
};

// MARK: Blob ------------------------------------------------------------------

export u32 MAGIC = 0xD00DFEED;

export struct Blob : Io::BChunk {
    Header _header;

    Header const& header() const { return _header; }

    static Res<Blob> openFromAddr(void const* addr) {
        auto const* hd = static_cast<u32be const*>(addr);
        if (hd[0] != MAGIC)
            return Error::invalidData("invalid magic number");

        return open(Bytes{static_cast<u8 const*>(addr), hd[1]});
    }

    static Res<Blob> open(Bytes dtb) {
        if (dtb.len() < sizeof(Header))
            return Error::invalidData("data too small");

        auto header = Io::BScan{dtb}.next<Header>();

        if (header.magic != MAGIC)
            return Error::invalidData("invalid magic number");

        if (header.totalSize != dtb.len())
            return Error::invalidData("total size mismatch");

        if (header.memoryReservationBlockOffset >= header.totalSize)
            return Error::invalidData("invalid memory reservation block offset");

        if (not header.dtbRange().contains(header.structureBlockRange()))
            return Error::invalidData("invalid structure block range");

        if (not header.dtbRange().contains(header.stringsRange()))
            return Error::invalidData("invalid strings block range");

        return Ok(Blob{dtb, header});
    }

    Bytes stringsBlock() const {
        return sub(bytes(), _header.stringsRange().cast<usize>());
    }

    Bytes structureBlock() const {
        return sub(bytes(), _header.structureBlockRange().cast<usize>());
    }

    Slice<ReserveEntry> memoryReservations() {
        auto reservations = sub(bytes(), _header.memoryReservationBlockOffset, _header.totalSize).cast<ReserveEntry>();
        usize len = 0;
        for (auto& r : reservations) {
            if (r.address == 0 and r.size == 0)
                break;
            len++;
        }
        return sub(reservations, 0, len);
    }

    TokenIter iterTokens() const {
        return {stringsBlock(), structureBlock()};
    }

    Node root() const {
        return Node{iterTokens()};
    }

    Opt<Range<u64>> initrd() const {
        auto chosenNode = try$(root().findChildren("chosen"));
        auto initrdStart = try$(chosenNode.getProperty<u64>("linux,initrd-start"));
        auto initrdEnd = try$(chosenNode.getProperty<u64>("linux,initrd-end"));
        return Range<u64>::fromStartEnd(initrdStart, initrdEnd);
    }

    void dump(Io::Emit& e) {
        e("magic: {:x}\n", header().magic);
        e("total size: {}\n", DataSize{header().totalSize});
        e("version: {}\n", header().version);
        e("last compatible version: {}\n", header().lastCompatibleVersion);
        e("memory reservation:\n");
        for (auto& reserved : memoryReservations()) {
            e("  - {:#08x}-{:#08x}\n", reserved.address, reserved.address + reserved.size);
        }
        e("tree:\n");
        root().dump(e);
        e(";\n");
    }
};

} // namespace Vaerk::Dtb

export module Vaerk.Dtb;

import Karm.Core;

using namespace Karm;

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

    Opt<Str> extraStr() {
        if (not extra.len())
            return NONE;

        if (last(extra) != 0)
            return NONE;

        for (auto b : sub(extra, 0, extra.len() - 1)) {
            if (not isAsciiPrint(b))
                return NONE;
        }
        return Str::fromNullterminated((char const*)extra.buf());
    }
};

struct TokenIter {
    Bytes strings;
    Io::BScan tokens;

    Opt<Token> next() {
        if (tokens.ended())
            return NONE;
        auto type = static_cast<Token::Type>(tokens.nextU32be());
        if (type == Token::BEGIN_NODE) {
            auto name = tokens.nextCStr();
            tokens.align(sizeof(u32));
            return Token{.type = type, .name = name};
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

struct PropIter;
struct ChildrenIter;

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

    PropIter iterProp() const;
    ChildrenIter iterChildren() const;
};

struct PropIter {
    TokenIter tokens;

    Opt<Token> next() {
        auto token = tokens.next();
        if (not token)
            return NONE;
        if (token->type != Token::PROP)
            return NONE;
        return token;
    }
};

PropIter Node::iterProp() const {
    auto copy = tokens;
    (void)copy.next(); // skip begin node
    return PropIter{copy};
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
                    return Node{before};
                }
            } else if (token->type == Token::END_NODE) {
                if (depth == 0)
                    return NONE;
                depth--;
            }
        }
    }
};

ChildrenIter Node::iterChildren() const {
    auto copy = tokens;
    (void)copy.next(); // skip begin node
    return ChildrenIter{copy};
}

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

    TokenIter iterTokens() {
        return {stringsBlock(), structureBlock()};
    }

    Node root() {
        return Node{iterTokens()};
    }
};

} // namespace Vaerk::Dtb

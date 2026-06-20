module;

#include <karm/macros>

export module Vaerk.Dtb;

import Karm.Core;

using namespace Karm;
using namespace Karm::Re::Literals;

// https://devicetree-specification.readthedocs.io/en/v0.1/flattened-format.html
namespace Vaerk::Dtb {

export using Karm::begin;
export using Karm::end;

// MARK: Header ----------------------------------------------------------------

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

// MARK: Cells -----------------------------------------------------------------

// https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html#id2
export struct InheritedProperties {
    usize addressCells = 1;
    usize sizeCells = 1;
    usize interruptCells = 1;
};

export struct AddressSizeCell : Range<u64> {
    using Range::Range;

    static Res<AddressSizeCell> parse(Cursor<u32be>& c, InheritedProperties const& inherited) {
        u64 address = 0;
        for (auto _ : urange::zeroTo(inherited.addressCells)) {
            if (c.ended())
                return Error::invalidData();
            address <<= 32;
            address |= c.next();
        }

        u64 size = 0;
        for (auto _ : urange::zeroTo(inherited.sizeCells)) {
            if (c.ended())
                return Error::invalidData();
            size <<= 32;
            size |= c.next();
        }

        return Ok(AddressSizeCell{address, size});
    }

    void repr(Io::Emit& e) const {
        e("{:#x}-{:#x}", start, end());
    }
};

export template <typename T>
concept Cell = requires(Cursor<u32be>& c, InheritedProperties& inherited) {
    { T::parse(c, inherited) } -> Meta::Same<Res<T>>;
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

Tuple<Str, Opt<usize>> _parseName(Str str) {
    Io::SScan s{str};
    auto name = s.token(Re::until("@"_re));
    Opt<usize> addr = NONE;
    if (s.skip("@"))
        addr = Io::atou(s.token(Re::zeroOrMore(Re::xdigit())), {.base = 16});
    return {name, addr};
}

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
    Str fullname = "";
    Bytes extra = {};

    Str name() const {
        auto [name, _] = _parseName(fullname);
        return name;
    }

    Opt<usize> address() const {
        auto [_, address] = _parseName(fullname);
        return address;
    }

    void repr(Io::Emit& e) const {
        e("({}", type);
        if (auto n = name())
            e(" name:{#}", n);
        if (auto addr = address())
            e(" address:{:#08x}", addr);
        if (extra)
            e(" extra:{:#02x}", extra);
        e(")");
    }
};

// MARK: Iterators -------------------------------------------------------------

struct TokenIter {
    Bytes strings;
    Io::BScan tokens;

    Opt<Token> next() {
        if (tokens.ended())
            return NONE;
        auto type = static_cast<Token::Type>(tokens.nextU32be());
        if (type == Token::BEGIN_NODE) {
            auto fullname = tokens.nextCStr();
            tokens.align(sizeof(u32));
            return Token{.type = type, .fullname = fullname};
        } else if (type == Token::END_NODE) {
            return Token{.type = type};
        } else if (type == Token::PROP) {
            auto len = tokens.nextU32be();
            auto nameoff = tokens.nextU32be();
            auto name = Io::BScan{strings}.skip(nameoff).nextCStr();
            auto extra = tokens.nextBytes(len);
            tokens.align(sizeof(u32));
            return Token{.type = type, .fullname = name, .extra = extra};
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
    InheritedProperties _inherited;

    Prop(Token token, InheritedProperties inherited)
        : _token(token), _inherited(inherited) {}

    Str name() const {
        return _token.name();
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

    template <Cell C>
    Res<C> cell() const {
        Cursor c = regs32();
        return C::parse(c, _inherited);
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
    TokenIter _tokens;
    InheritedProperties _inherited;

    Node(TokenIter tokens, InheritedProperties inherited)
        : _tokens(tokens),
          _inherited(inherited) {
        if (auto [addressCells] = getProperty("#address-cells")) {
            if (auto [cells] = addressCells.as<u32be>()) {
                _inherited.addressCells = cells;
            }
        }

        if (auto [sizeCells] = getProperty("#size-cells")) {
            if (auto [cells] = sizeCells.as<u32be>()) {
                _inherited.sizeCells = cells;
            }
        }

        if (auto [interruptCells] = getProperty("#interrupt-cells")) {
            if (auto [cells] = interruptCells
                                   .as<u32be>()) {
                _inherited.interruptCells = cells;
            }
        }
    }

    Token token() const {
        auto copy = _tokens;
        return copy.next().unwrap();
    }

    Str fullname() const {
        return token().fullname;
    }

    Str name() const {
        auto name = token().name();
        if (not name)
            return "/";
        return name;
    }

    Opt<usize> address() const {
        return token().address();
    }

    struct PropIter {
        TokenIter _tokens;
        InheritedProperties _inherited;

        Opt<Prop> next() {
            auto token = _tokens.next();
            if (not token)
                return NONE;
            if (token->type != Token::PROP)
                return NONE;
            return Prop{token.unwrap(), _inherited};
        }
    };

    PropIter iterProp() const {
        auto copy = _tokens;
        (void)copy.next(); // skip begin node
        return {copy, _inherited};
    }

    struct ChildrenIter {
        TokenIter _tokens;
        InheritedProperties _inherited;
        usize _depth = 0;

        ChildrenIter(TokenIter tokens, InheritedProperties inherited)
            : _tokens(tokens),
              _inherited(inherited) {}

        Opt<Node> next() {
            while (true) {
                auto before = _tokens;
                auto token = _tokens.next();
                if (not token)
                    return NONE;
                if (token->type == Token::BEGIN_NODE) {
                    _depth++;
                    if (_depth == 1) {
                        return Node{before, _inherited};
                    }
                } else if (token->type == Token::END_NODE) {
                    if (_depth == 0)
                        return NONE;
                    _depth--;
                }
            }
        }
    };

    ChildrenIter iterChildren() const {
        auto copy = _tokens;
        (void)copy.next(); // skip begin node
        return {copy, _inherited};
    }

    auto iterChildrenByType(Str type) const {
        return iterChildren() |
               Where([=](Dtb::Node& n) {
                   return n.name() == type;
               });
    }

    Opt<Node> findChildren(Str name) const {
        for (auto node : iterChildren()) {
            if (node.name() == name)
                return node;
        }
        return NONE;
    }

    Opt<Prop> getProperty(Str name) const {
        for (auto prop : iterProp()) {
            if (prop.name() == name)
                return prop;
        }
        return NONE;
    }

    template <Cell C>
    Opt<C> getProperty(Str name) const {
        auto prop = try$(getProperty(name));
        return prop.cell<C>().ok();
    }

    template <typename T>
    Opt<T> getProperty(Str name) const {
        auto prop = try$(getProperty(name));
        return try$(prop.as<T>());
    }

    void dump(Io::Emit& e) const {
        e("{}", name());
        if (auto [addr] = address())
            e(" @ {:p}", addr);
        e(" {#} ", fullname());
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
        return Node{iterTokens(), InheritedProperties{}};
    }

    Opt<Range<u64>> initrd() const {
        auto chosenNode = try$(root().findChildren("chosen"));
        auto initrdStart = try$(chosenNode.getProperty<u64be>("linux,initrd-start"));
        auto initrdEnd = try$(chosenNode.getProperty<u64be>("linux,initrd-end"));
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

export module Vaerk.Console;

import Karm.Core;

using namespace Karm;

namespace Vaerk {

u8 _FONT_DATA[]{
#include "defs/firacode.inc"
};

export struct Console : Io::TextWriter {
    static constexpr usize TAB_WIDTH = 4;

    struct Font {
        static constexpr Rune FIRST = 0x20;
        static constexpr Rune LAST = 0x7e;

        Slice<u8> buf;
        usize advance;
        usize height;

        usize glyphSize() const {
            return advance * height;
        }

        bool has(Rune rune) const {
            return rune >= FIRST and rune <= LAST;
        }

        u8 load(u8 c, usize x, usize y) const {
            return buf[(c - FIRST) * glyphSize() + y * advance + x];
        }
    };

    struct Buffer {
        u8* buf = nullptr;
        usize width = 0;
        usize height = 0;
        usize pitch = 0;

        u32* row(usize y) {
            return reinterpret_cast<u32*>(buf + y * pitch);
        }

        void fill(usize y, usize h) {
            for (usize dy : Iota(0uz, h)) {
                u32* r = row(y + dy);
                for (usize x : Iota(0uz, width))
                    r[x] = 0;
            }
        }

        void clear() {
            fill(0, height);
        }

        void scroll(usize lines) {
            if (lines >= height) {
                clear();
                return;
            }

            for (usize y : Iota(0uz, height - lines)) {
                u32* dst = row(y);
                u32* src = row(y + lines);
                for (usize x : Iota(0uz, width))
                    dst[x] = src[x];
            }

            fill(height - lines, lines);
        }

        void blit(Font const& font, usize x, usize y, u8 c) {
            for (usize gy : Iota(0uz, font.height)) {
                u32* r = row(y + gy);
                for (usize gx : Iota(0uz, font.advance)) {
                    u32 v = font.load(c, gx, gy);
                    r[x + gx] = v | v << 8 | v << 16;
                }
            }
        }
    };

    bool _enabled = false;
    bool _escaped = false;
    Buffer _frontbuffer{};
    Font _font{
        {_FONT_DATA, sizeof(_FONT_DATA)},
        10,
        20,
    };
    usize _cx = 0, _cy = 0;

    usize width() const {
        return _frontbuffer.width / _font.advance;
    }

    usize height() const {
        return _frontbuffer.height / _font.height;
    }

    void enable(bool enabled = true) {
        _enabled = enabled;
    }

    void bind(Buffer fb) {
        _frontbuffer = fb;
    }

    void clear() {
        if (not _frontbuffer.buf)
            return;
        _frontbuffer.clear();
        _cx = 0;
        _cy = 0;
    }

    void _set(usize x, usize y, u8 c) {
        _frontbuffer.blit(_font, x * _font.advance, y * _font.height, c);
    }

    void _newline() {
        _cx = 0;
        if (_cy + 1 < height()) {
            _cy++;
            return;
        }
        _frontbuffer.scroll(_font.height);
    }

    void _put(u8 c) {
        if (_cx >= width())
            _newline();
        _set(_cx, _cy, c);
        _cx++;
    }

    Res<> writeRune(Rune rune) override {
        if (not _enabled or not _frontbuffer.buf or width() == 0 or height() == 0)
            return Ok();

        switch (rune) {
        case '\n':
            _newline();
            break;

        case '\r':
            _cx = 0;
            break;

        case '\t': {
            usize spaces = TAB_WIDTH - (_cx % TAB_WIDTH);
            for (usize i = 0; i < spaces; i++)
                _put(' ');
            break;
        }

        case '\b':
            if (_cx > 0) {
                _cx--;
                _set(_cx, _cy, ' ');
            }
            break;

        case '\e':
            _escaped = true;
            break;

        default:
            if (_escaped and isAsciiAlpha(rune)) {
                _escaped = false;
                break;
            }

            if (rune < 0x20 or rune == 0x7f)
                break;

            if (not _escaped)
                _put(_font.has(rune) ? static_cast<u8>(rune) : static_cast<u8>('?'));
            break;
        }

        return Ok();
    }
};

} // namespace Vaerk

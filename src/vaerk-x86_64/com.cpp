module;

#include <karm/macros>

export module Vaerk.x86_64:com;

import Karm.Core;
import Vaerk.Base;
import :intrinsics;

using namespace Karm;

namespace x86_64 {

export struct Com : Io::TextWriter {
    Vaerk::RawPortIo _io;

    Com(Vaerk::RawPortIo io) : _io(io) {
    }

    static Com com1() {
        return {Vaerk::RawPortIo({0x3F8, 8})};
    }

    static Com com2() {
        return {Vaerk::RawPortIo({0x2F8, 8})};
    }

    static Com com3() {
        return {Vaerk::RawPortIo({0x3E8, 8})};
    }

    static Com com4() {
        return {Vaerk::RawPortIo({0x2E8, 8})};
    }

    static constexpr usize SPIN_LIMIT = 100'000;
    static constexpr u16 BAUD_DIVISOR = 115200 / 9600;

    enum Regs {
        DATA = 0,
        INTERRUPT_ENABLE = 1,
        DIVISOR_LOW = 0,
        DIVISOR_HIGH = 1,
        INTERRUPT_IDENTIFICATION = 2,
        FIFO_CONTROLLER = 2,
        LINE_CONTROL = 3,
        MODEM_CONTROL = 4,
        LINE_STATUS = 5,
        MODEM_STATUS = 6,
        SCRATCH_REGISTER = 7,
    };

    enum LineCtrls {
        DATA_SIZE_5 = 0,
        DATA_SIZE_6 = 1,
        DATA_SIZE_7 = 2,
        DATA_SIZE_8 = 3,
        DLAB_STATUS = 1 << 7,
    };

    enum FifoCtrl {
        FIFO_ENABLE = 1 << 0,
        FIFO_CLEAR_RECEIVE = 1 << 1,
        FIFO_CLEAR_TRANSMIT = 1 << 2,
        FIFO_TRIGGER_14 = 0b11 << 6,
    };

    enum ModemCtrl {
        MODEM_DTR = 1 << 0,
        MODEM_RTS = 1 << 1,
        MODEM_OUT1 = 1 << 2,
        MODEM_OUT2 = 1 << 3,
        MODEM_LOOPBACK = 1 << 4,
    };

    enum Int {
        WHEN_DATA_AVAILABLE = 1 << 0,
        WHEN_TRANSMITTER_EMPTY = 1 << 1,
        WHEN_BREAK_ERROR = 1 << 2,
        WHEN_STATUS_UPDATE = 1 << 3,
    };

    enum LineStatus {
        DATA_READY = 1 << 0,
        OVERRUN_ERROR = 1 << 1,
        PARITY_ERROR = 1 << 2,
        FRAMING_ERROR = 1 << 3,
        BREAK_INDICATOR = 1 << 4,
        TRANSMITTER_BUF_EMPTY = 1 << 5,
        TRANSMITTER_EMPTY = 1 << 6,
        IMPENDING_ERROR = 1 << 7,
    };

    Res<> writeReg(Regs reg, u8 value) {
        return _io.out8(reg, value);
    }

    Res<u8> readReg(Regs reg) {
        return _io.in8(reg);
    }

    Res<bool> probe() {
        try$(writeReg(LINE_CONTROL, DATA_SIZE_8));
        try$(writeReg(MODEM_CONTROL, MODEM_RTS | MODEM_OUT1 | MODEM_OUT2 | MODEM_LOOPBACK));

        usize stale = 0;
        while (stale++ < SPIN_LIMIT and try$(canRead()))
            try$(readReg(DATA));

        // NOTE: Even in loopback mode the byte is shifted out at the
        //       configured baud rate, so it doesn't come back immediately.
        try$(writeReg(DATA, 0xAE));
        bool present = _waitFor(DATA_READY) and try$(readReg(DATA)) == 0xAE;

        try$(writeReg(MODEM_CONTROL, MODEM_DTR | MODEM_RTS | MODEM_OUT2));
        return Ok(present);
    }

    Res<> init() {
        try$(writeReg(INTERRUPT_ENABLE, 0));

        try$(writeReg(LINE_CONTROL, DLAB_STATUS));
        try$(writeReg(DIVISOR_LOW, BAUD_DIVISOR & 0xff));
        try$(writeReg(DIVISOR_HIGH, BAUD_DIVISOR >> 8));
        try$(writeReg(LINE_CONTROL, DATA_SIZE_8));

        try$(writeReg(FIFO_CONTROLLER, FIFO_ENABLE | FIFO_CLEAR_RECEIVE | FIFO_CLEAR_TRANSMIT | FIFO_TRIGGER_14));
        try$(writeReg(MODEM_CONTROL, MODEM_DTR | MODEM_RTS | MODEM_OUT2));
        try$(writeReg(INTERRUPT_ENABLE, WHEN_DATA_AVAILABLE));

        return Ok();
    }

    Res<bool> canRead() {
        return Ok<bool>(try$(readReg(LINE_STATUS)) & DATA_READY);
    }

    Res<> _waitFor(LineStatus status) {
        for (usize i = 0; i < SPIN_LIMIT; i++) {
            if (try$(readReg(LINE_STATUS)) & status)
                return Ok();
        }
        return Error::timedOut("uart not responding");
    }

    Res<> waitWrite() {
        return _waitFor(TRANSMITTER_BUF_EMPTY);
    }

    Res<> waitRead() {
        return _waitFor(DATA_READY);
    }

    Res<> putByte(u8 c) {
        try$(waitWrite());
        return writeReg(DATA, c);
    }

    Res<u8> getByte() {
        try$(waitRead());
        return readReg(DATA);
    }

    Res<usize> write(Bytes bytes) {
        for (auto b : iter(bytes)) {
            try$(putByte(b));
        }

        return Ok(sizeOf(bytes));
    }

    Res<> writeRune(Rune rune) override {
        Utf8::One one;
        if (not Utf8::encodeUnit(rune, one))
            return Error::invalidInput("encoding error");
        try$(write(bytes(one)));
        return Ok();
    }
};

} // namespace x86_64

export module Vaerk.Acpi:fadt;

import Karm.Core;
import :sdth;

using namespace Karm;

namespace Vaerk::Acpi {

export struct [[gnu::packed]] Fadt : Sdth {
    static constexpr Signature SIGNATURE = {'F', 'A', 'C', 'P'};

    enum BootFlags : u16 {
        LEGACY_DEVICES = 1 << 0,       // Legacy devices supported (8259, etc.)
        HAS_8042 = 1 << 1,             // 8042 keyboard controller present
        VGA_NOT_PRESENT = 1 << 2,      // VGA not present
        MSI_NOT_SUPPORTED = 1 << 3,    // MSI not supported
        PCIE_ASPM = 1 << 4,            // PCIe ASPM controls
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

    void repr(Io::Emit& e) const {
        Sdth::repr(e);
        e("firmwareCtrl: {}\n", firmwareCtrl);
        e("dsdt: {}\n", dsdt);
        e("reserved1: {}\n", reserved1);
        e("preferredPmProfile: {}\n", preferredPmProfile);
        e("sciInt: {}\n", sciInt);
        e("smiCmd: {}\n", smiCmd);
        e("acpiEnable: {}\n", acpiEnable);
        e("acpiDisable: {}\n", acpiDisable);
        e("s4BiosReq: {}\n", s4BiosReq);
        e("pStateCtrl: {}\n", pStateCtrl);
        e("pm1aEvtBlk: {}\n", pm1aEvtBlk);
        e("pm1bEvtBlk: {}\n", pm1bEvtBlk);
        e("pm1aCtrlBlk: {}\n", pm1aCtrlBlk);
        e("pm1bCtrlBlk: {}\n", pm1bCtrlBlk);
        e("pm2CtrlBlk: {}\n", pm2CtrlBlk);
        e("pmTmrBlk: {}\n", pmTmrBlk);
        e("gpe0Blk: {}\n", gpe0Blk);
        e("gpe1Blk: {}\n", gpe1Blk);
        e("pm1EvtLen: {}\n", pm1EvtLen);
        e("pm1CtrlLen: {}\n", pm1CtrlLen);
        e("pm2CtrlLen: {}\n", pm2CtrlLen);
        e("pmTmrLen: {}\n", pmTmrLen);
        e("gpe0BlkLen: {}\n", gpe0BlkLen);
        e("gpe1BlkLen: {}\n", gpe1BlkLen);
        e("gpe1Base: {}\n", gpe1Base);
        e("cstCtrl: {}\n", cstCtrl);
        e("pLvl2Lat: {}\n", pLvl2Lat);
        e("pLvl3Lat: {}\n", pLvl3Lat);
        e("flushSize: {}\n", flushSize);
        e("flushStride: {}\n", flushStride);
        e("dutyOffset: {}\n", dutyOffset);
        e("dutyWidth: {}\n", dutyWidth);
        e("dayAlrm: {}\n", dayAlrm);
        e("monAlrm: {}\n", monAlrm);
        e("century: {}\n", century);
        e("bootFlags: {}\n", bootFlags);
        e("reserved2: {}\n", reserved2);
        e("flags: {}\n", flags);
    }
};

} // namespace Vaerk::Acpi

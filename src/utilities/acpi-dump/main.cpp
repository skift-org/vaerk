#include <karm/entry>

import Vaerk.Acpi;
import Karm.Cli;

using namespace Karm;
using namespace Karm::Literals;

using namespace Vaerk;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto inputArg = Cli::operand<Str>("acpi-table"s, "Path to the acpi table"s);

    Cli::Command cmd{
        "acpi-dump"s,
        "Dump a acpi table"s,
        {
            Cli::Section{"Input"s, {inputArg}},
        }
    };

    co_try$(cmd.exec(env));
    if (not cmd)
        co_return Ok();

    if (not inputArg.value())
        co_return Error::invalidInput("no acpi table provided");

    auto url = Ref::parseUrlOrPath(inputArg.value(), env.cwd());
    auto file = co_try$(Sys::File::open(url));
    auto map = co_try$(Sys::mmap(file));
    auto bytes = map.bytes();

    auto table = bytes.cast<Acpi::Sdth>().index(0);
    if (not table or table->len < sizeof(Acpi::Sdth) or table->len > bytes.len())
        co_return Error::invalidData("invalid acpi table");

    Io::Emit e{Sys::out()};
    Str signature = table->signature;
    if (signature == "FACP"s)
        table->as<Acpi::Fadt>()->repr(e);
    else if (signature == "HPET"s)
        table->as<Acpi::Hpet>()->repr(e);
    else if (signature == "APIC"s)
        table->as<Acpi::Madt>()->repr(e);
    else if (signature == "MCFG"s)
        table->as<Acpi::Mcfg>()->repr(e);
    else if (signature == "RSDT"s)
        table->as<Acpi::Rsdt>()->repr(e);
    else if (signature == "XSDT"s)
        table->as<Acpi::Xsdt>()->repr(e);
    else
        table->repr(e);

    co_return Ok();
}

import Karm.Dl.Elf;

#include <karm/entry>
#include <vaerk-handover/spec.h>

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    if (env.argsLen() == 0)
        co_return Error::invalidInput("Usage: handover-dump <elf-file>");

    auto url = Ref::parseUrlOrPath(env[0], env.cwd());
    auto file = co_try$(Sys::File::open(url));
    auto mem = co_try$(Sys::mmap(file, {.options = Sys::MmapOption::READ}));
    auto elf = Elf::ElfObject<Elf::Elf64LeAbi>{mem.bytes()};

    if (not elf.validate())
        co_return Error::invalidData("kernel is not a valid ELF executable");

    auto requests =
        co_try$(elf.section(Handover::REQUEST_SECTION))
            .data.cast<Handover::Request>();

    Sys::println("Requests:");
    for (auto const& request : requests)
        Sys::println(" - {}", request.name());
    Sys::println("Kernel is valid");

    co_return Ok();
}

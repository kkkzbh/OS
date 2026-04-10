module;

export module storage.init;

import ahci;
import ata.pio.init;
import console;

export auto storage_init() -> void;

auto storage_init() -> void
{
    console::println("storage_init start");
    if(ahci_init()) {
        console::println("storage_init: using ahci");
        return;
    }
    console::println("storage_init: fallback to ata_pio");
    ata_pio_init();
}

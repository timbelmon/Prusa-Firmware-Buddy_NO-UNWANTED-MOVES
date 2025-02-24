#include <common/disable_interrupts.h>
#include <common/timing_precise.hpp>
#include <common/timing.h>
#include <freertos/critical_section.hpp>
#include <buddy/mmu_port.hpp>
#include <common/hwio_pindef.h>
#include <hw_configuration.hpp>

namespace mmu_port {

// TODO: Make hw a standalone module -> this would allow this to be standalone module too
using namespace buddy::hw;

void activate_reset() {
    MMUReset.write(Configuration::Instance().has_inverted_mmu_reset() ? Pin::State::low : Pin::State::high);
}

void deactivate_reset() {
    MMUReset.write(Configuration::Instance().has_inverted_mmu_reset() ? Pin::State::high : Pin::State::low);
}

void setup_reset_pin() {
    const auto &config = Configuration::Instance();

    // Newer BOMs need push-pull for the reset pin, older open drain.
    // Setting it like this is a bit hacky, because the MMUReset defined in hwio_pindef is constexpr,
    // so it's not possible to change it right at the source.
    if (config.needs_push_pull_mmu_reset_pin()) {
        OutputPin pin = MMUReset;
        pin.m_mode = OMode::pushPull;
        pin.configure();
    }
}

// The code below pulse-charges the MMU capacitors, as the current inrush
// would due to an inferior HW design cause overcurrent on the xBuddy board.
// In case overcurrent would still be triggered, increase the us_total
// value to pre-charge longer.
static constexpr uint32_t us_high = 5;
static constexpr uint32_t us_low = 70;
static constexpr uint32_t us_total = 15000;

void power_on() {
    const auto &config = Configuration::Instance();

    // Power on the MMU with sreset activated
    activate_reset();

    if (!config.can_power_up_mmu_without_pulses()) {
        freertos::CriticalSection critical_section;

        for (uint32_t i = 0; i < us_total; i += (us_high + us_low)) {
            {
                buddy::DisableInterrupts disable_interrupts;
                MMUEnable.write(Pin::State::high);
                delay_us_precise<us_high>();
                MMUEnable.write(Pin::State::low);
            }
            delay_us(us_low);
        }
    }

    MMUEnable.write(Pin::State::high);

    // Give some time for the MMU to catch up with the reset signal - it takes some time for the voltage to actually start
    delay(200);

    deactivate_reset();
}

void power_off() {
    MMUEnable.write(Pin::State::low);
}

} // namespace mmu_port

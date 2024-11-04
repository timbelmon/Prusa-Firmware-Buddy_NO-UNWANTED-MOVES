#include "emergency_stop.hpp"
#include <config_store/store_c_api.h>
#include <common/power_panic.hpp>
#include <module/stepper.h>

namespace buddy {

namespace {
    // TODO: Tune? 2mm is _probably_ OK (it won't squish a finger too much even
    // if it was a tight fit before), but someone needs to confirm.

    // We allow this distance to be traveled before doing any panicky things -
    // we'll just schedule the stop, wait for the moves to get through Planner
    // and then park. Only if we travel in Z by more than this, we start doing
    // a bit more.
    constexpr float allowed_mm = 2.0;

    // If we travel even more before any of the above measures had a chance to
    // stop it, we do a BSOD as a last resort.
    constexpr float extra_emergency_mm = 4.0;

    int32_t current_z() {
        return stepper.position_from_startup(Z_AXIS);
    }

    // Try to do some desperate measures to stop moving in Z (currently implemented as power panic).
    void invoke_emergency() {
        // Do a "synthetic" power panic. Should stop _right now_ and reboot, then we'll deal with the consequences.
        if (!power_panic::ac_fault_triggered) {
            // this is normally supposed to be called from ISR, but since disables IRQ so it works fine even outside of ISR
            power_panic::ac_fault_isr();
        }
    }

} // namespace

void EmergencyStop::step() {
    const bool emergency = do_stop.load();
    if (emergency) {
        if (start_z.has_value()) {
            const int32_t difference = std::abs(*start_z - current_z());
            const float steps_per_mm = get_steps_per_unit_z();
            const int32_t allowed_steps = allowed_mm * steps_per_mm;
            const int32_t extra_emergency_steps = extra_emergency_mm * steps_per_mm;
            if (difference > allowed_steps) {
                if (difference > extra_emergency_steps) {
                    // Didn't work the first time around? What??
                    bsod("Emergency stop failed, last-resort stop");
                } else {
                    invoke_emergency();
                }
            }
        } else {
            start_z = current_z();
        }
    } else {
        start_z.reset();
    }
}

EmergencyStop &emergency_stop() {
    static EmergencyStop instance;
    return instance;
}

} // namespace buddy

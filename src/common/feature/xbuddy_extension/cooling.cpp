#include "cooling.hpp"

namespace buddy {

uint8_t Cooling::fan_pwm(bool already_spinning, Temperature current_temperature) {
    // Phase 1: Decide what PWM we would like to run at
    if (auto_control && !target_temperature.has_value()) {
        // No temp set -> don't cool.
        target_pwm = 0;
    } else if (auto_control) {
        // Linear mapping from the allowed temp difference over the target to allowed RPM range
        const int temp_diff = current_temperature - *target_temperature;
        const int pwm_diff = max_pwm - min_pwm;
        const int desired = temp_diff * pwm_diff / fans_max_temp_diff + min_pwm;

        if (desired > max_pwm) {
            // Don't go over max.
            target_pwm = max_pwm;
        } else if (desired < min_pwm) {
            const int below_by = *target_temperature - current_temperature;
            const bool below_by_much = below_by > off_temp_below;
            if (already_spinning && !below_by_much) {
                // If the fan is already spinning, we keep it spinning until it
                // falls below the target by the margin, so we don't go
                // on-off-on-off too much.
                target_pwm = min_pwm;
            } else {
                target_pwm = 0;
            }
        } else {
            target_pwm = desired;
        }
    } // else -> no auto_control -> keep target_pwm intact

    // Phase 2: adjust to minima and spinning up.

    // If the fans are not spinning yet and should be, give them a bit of a
    // kick to get turning. Unfortunately, we can't do that to each of them
    // individually, they share the PWM, even though they have separate RPM
    // measurement.
    if (!already_spinning && target_pwm > 0 && target_pwm < spin_up_pwm) {
        return spin_up_pwm;
    }

    if (target_pwm > 0 && target_pwm < min_pwm) {
        // Even if the user sets it to some low %, keep them at least on the
        // minimum (the auto thing never sets it between 0 and min, so it's
        // only in the manual case).
        return min_pwm;
    }

    return target_pwm;
}

} // namespace buddy

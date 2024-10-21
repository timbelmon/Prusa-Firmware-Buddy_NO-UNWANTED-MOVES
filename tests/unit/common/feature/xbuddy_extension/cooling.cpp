#include <feature/xbuddy_extension/cooling.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Cooling PWM") {
    buddy::Cooling cooling;

    SECTION("Manual, full pwm") {
        cooling.auto_control = false;
        cooling.target_pwm = 255;

        SECTION("With target temp") {
            cooling.target_temperature = 60;
        }

        SECTION("Without target temp") {}

        REQUIRE(cooling.fan_pwm(true, 54) == 255);
        REQUIRE(cooling.fan_pwm(false, 54) == 255);
    }

    SECTION("Manual, low PWM") {
        cooling.auto_control = false;
        cooling.target_pwm = 5;

        SECTION("Already running") {
            REQUIRE(cooling.fan_pwm(true, 54) == 70);
        }

        SECTION("Initial kick") {
            REQUIRE(cooling.fan_pwm(false, 54) == 200);
        }
    }

    SECTION("Auto cooling, no target temp") {
        cooling.target_temperature = std::nullopt;

        REQUIRE(cooling.fan_pwm(true, 45) == 0);
        REQUIRE(cooling.target_pwm == 0);
    }

    SECTION("Auto cooling, cold chamber") {
        cooling.target_temperature = 60;

        REQUIRE(cooling.fan_pwm(false, 20) == 0);
        REQUIRE(cooling.target_pwm == 0);
    }

    SECTION("Auto cooling, slightly cool") {
        cooling.target_temperature = 60;

        SECTION("Not running, don't start") {
            REQUIRE(cooling.fan_pwm(false, 59) == 0);
            REQUIRE(cooling.target_pwm == 0);
        }

        SECTION("Already running, keep it up") {
            cooling.target_pwm = 70;
            REQUIRE(cooling.fan_pwm(true, 59) == 70);
            REQUIRE(cooling.target_pwm == 70);
        }
    }

    SECTION("Auto cooling, exact temp") {
        cooling.target_temperature = 60;

        SECTION("Start") {
            REQUIRE(cooling.fan_pwm(false, 60) == 200);
            REQUIRE(cooling.target_pwm == 70);
        }

        SECTION("Already running") {
            REQUIRE(cooling.fan_pwm(true, 60) == 70);
            REQUIRE(cooling.target_pwm == 70);
        }
    }

    SECTION("Auto cooling, really hot") {
        cooling.target_temperature = 60;

        REQUIRE(cooling.fan_pwm(true, 80) == 255);
        REQUIRE(cooling.target_pwm == 255);
    }

    SECTION("Auto cooling, slightly hot") {
        cooling.target_temperature = 60;

        SECTION("Already running") {
            REQUIRE(cooling.fan_pwm(true, 61) == 88);
            REQUIRE(cooling.target_pwm == 88);
        }

        SECTION("Kick up") {
            REQUIRE(cooling.fan_pwm(false, 61) == 200);
            REQUIRE(cooling.target_pwm == 88);
        }
    }
}

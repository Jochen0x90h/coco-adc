#include <coco/platform/gpio.hpp>


namespace coco {
namespace debug {

// LEDs on ProgBox board
constexpr auto redPin = gpio::Config::PF10 | gpio::Config::INVERT;
constexpr auto greenPin = gpio::Config::PF9 | gpio::Config::INVERT;
constexpr auto bluePin = gpio::Config::PD15 | gpio::Config::INVERT;

void init() {
	// Initialize debug LEDs
	gpio::configureOutput(redPin, false);
	gpio::configureOutput(greenPin, false);
	gpio::configureOutput(bluePin, false);
}

void set(uint32_t bits, uint32_t function) {
	gpio::setOutput(redPin, (bits & 1) != 0, (function & 1) != 0);
	gpio::setOutput(greenPin, (bits & 2) != 0, (function & 2) != 0);
	gpio::setOutput(bluePin, (bits & 4) != 0, (function & 4) != 0);
}

} // debug
} // coco

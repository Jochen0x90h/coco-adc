#include "AdcDummy.hpp"


namespace coco {

AdcDummy::~AdcDummy() {
}

int AdcDummy::get(int channel) {
    return this->value;
}

} // namespace coco

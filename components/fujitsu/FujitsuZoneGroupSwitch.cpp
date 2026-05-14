#include "FujitsuZoneGroupSwitch.h"
#include "FujitsuClimate.h"

namespace esphome {
namespace fujitsu {

void FujitsuZoneGroupSwitch::loop() {
    if (climate_ == nullptr) return;

    bool current = false;
    if (xSemaphoreTake(climate_->lock, (TickType_t)10) == pdTRUE) {
        current = (climate_->sharedZoneState.zoneGroup == group_);
        xSemaphoreGive(climate_->lock);
    }
    if (current != this->state) {
        this->publish_state(current);
    }
}

void FujitsuZoneGroupSwitch::write_state(bool state) {
    if (climate_ == nullptr) return;

    if (xSemaphoreTake(climate_->lock, (TickType_t)50) == pdTRUE) {
        // Turning on this group activates it; turning off reverts to NONE
        climate_->heatPump.setZoneGroup(state ? group_ : FujiZoneGroup::NONE);
        xSemaphoreGive(climate_->lock);
    }
    this->publish_state(state);
}

}  // namespace fujitsu
}  // namespace esphome

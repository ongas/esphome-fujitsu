#include "FujitsuZoneSwitch.h"
#include "FujitsuClimate.h"

namespace esphome {
namespace fujitsu {

void FujitsuZoneSwitch::loop() {
    if (climate_ == nullptr) return;

    bool current = false;
    if (xSemaphoreTake(climate_->lock, (TickType_t)10) == pdTRUE) {
        current = climate_->sharedZoneState.zones[zone_];
        xSemaphoreGive(climate_->lock);
    }
    if (current != this->state) {
        this->publish_state(current);
    }
}

void FujitsuZoneSwitch::write_state(bool state) {
    if (climate_ == nullptr) return;

    if (xSemaphoreTake(climate_->lock, (TickType_t)50) == pdTRUE) {
        climate_->heatPump.setZoneOnOff(zone_, state);
        xSemaphoreGive(climate_->lock);
    }
    this->publish_state(state);
}

}  // namespace fujitsu
}  // namespace esphome

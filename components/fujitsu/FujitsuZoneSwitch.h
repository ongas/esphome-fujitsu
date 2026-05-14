#pragma once
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace fujitsu {

class FujitsuClimate;

class FujitsuZoneSwitch : public switch_::Switch, public Component {
   public:
    void set_climate(FujitsuClimate *climate) { this->climate_ = climate; }
    void set_zone(int zone) { this->zone_ = zone; }
    void loop() override;

   protected:
    void write_state(bool state) override;
    FujitsuClimate *climate_{nullptr};
    int zone_{0};
};

}  // namespace fujitsu
}  // namespace esphome

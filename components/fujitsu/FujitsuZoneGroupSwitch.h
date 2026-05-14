#pragma once
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"
#include "FujiHeatPump.h"

namespace esphome {
namespace fujitsu {

class FujitsuClimate;

class FujitsuZoneGroupSwitch : public switch_::Switch, public Component {
   public:
    void set_climate(FujitsuClimate *climate) { this->climate_ = climate; }
    void set_group(FujiZoneGroup group) { this->group_ = group; }
    void loop() override;

   protected:
    void write_state(bool state) override;
    FujitsuClimate *climate_{nullptr};
    FujiZoneGroup group_{FujiZoneGroup::NONE};
};

}  // namespace fujitsu
}  // namespace esphome

#pragma once
#include "FujiHeatPump.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace fujitsu {

class FujitsuClimate : public climate::Climate, public Component {
   public:
    void setup() override;
    void loop() override;
    void control(const climate::ClimateCall &call) override;
    climate::ClimateTraits traits() override;
    TaskHandle_t taskHandle;
    FujiHeatPump heatPump;
    FujiFrame sharedState;
    SemaphoreHandle_t lock;
    bool pendingUpdate;

    void set_rx_pin(int pin) { this->rx_pin_ = pin; }
    void set_tx_pin(int pin) { this->tx_pin_ = pin; }
    void set_en_pin(int pin) { this->en_pin_ = pin; }
    void set_nrst_pin(int pin) { this->nrst_pin_ = pin; }
    void set_status_sensor(text_sensor::TextSensor *sensor) { this->status_sensor_ = sensor; }
    void attempt_login() { 
        ESP_LOGW("fujitsu", "=== MANUAL LOGIN BUTTON PRESSED ===");
        this->heatPump.attemptSecondaryLogin(); 
    }

   protected:
    int rx_pin_ = 16;
    int tx_pin_ = 17;
    int en_pin_ = -1;
    int nrst_pin_ = -1;
    text_sensor::TextSensor *status_sensor_{nullptr};
    std::string last_status_;

    void updateState();
    optional<climate::ClimateMode> fujiToEspMode(FujiMode fujiMode);
    optional<FujiMode> espToFujiMode(climate::ClimateMode espMode);
    
    optional<climate::ClimateFanMode> fujiToEspFanMode(FujiFanMode fujiFanMode);
    optional<FujiFanMode> espToFujiFanMode(climate::ClimateFanMode espFanMode);
};

}  // namespace fujitsu
}  // namespace esphome

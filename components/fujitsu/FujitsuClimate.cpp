#include "FujitsuClimate.h"
#include "esphome/core/log.h"

namespace esphome {
namespace fujitsu {

void serialTask(void *pvParameters) {
    FujitsuClimate *climate = (FujitsuClimate *)pvParameters;
    ESP_LOGW("fuji", "reached task");
    ESP_LOGW("fuji", "serialTask started on core %d", xPortGetCoreID());

    uint32_t frameCount = 0;
    uint32_t loopCount = 0;
    uint32_t lastDiag = millis();
    uint32_t bytesReceived = 0;
    uint32_t lastByteCount = 0;

    for (;;) {
        loopCount++;
        
        if (climate->heatPump.waitForFrame()) {
            frameCount++;
            delay(60);  // upstream: 60ms delay before sending response
            climate->heatPump.sendPendingFrame();
            climate->pendingUpdate = false;
        }
        if (xSemaphoreTake(climate->lock, (TickType_t)200) == pdTRUE) {
            memcpy(&(climate->sharedState), climate->heatPump.getCurrentState(),
                   sizeof(FujiFrame));
            xSemaphoreGive(climate->lock);
        }

        // Diagnostic: log every 10 seconds
        if (millis() - lastDiag > 10000) {
            FujiFrame *st = climate->heatPump.getCurrentState();
            // Build string of unique frame patterns seen
            char patterns[256] = "";
            for (int i = 0; i < climate->heatPump.seenPatternCount; i++) {
                auto &p = climate->heatPump.seenPatterns[i];
                char tmp[48];
                snprintf(tmp, sizeof(tmp), "%s%d>%d:t%d:cP%d(%lu)", 
                         i > 0 ? " " : "", p.src, p.dst, p.type, p.cP, p.count);
                strncat(patterns, tmp, sizeof(patterns) - strlen(patterns) - 1);
            }
            ESP_LOGW("fuji", "DIAG: frames=%u bytes_rx=%u delta=%u dst:P=%lu/S=%lu/O=%lu bound=%d probe=%lu@%lums resp=%lu@%lums echo=%d/match=%d rx_bytes=%lu incomplete=%lu valid=%lu last_rx=%lums patterns=[%s]",
                     frameCount,
                     bytesReceived, bytesReceived - lastByteCount,
                     climate->heatPump.frameDestPrimary, climate->heatPump.frameDestSecondary, climate->heatPump.frameDestOther,
                     climate->heatPump.isBound() ? 1 : 0,
                     climate->heatPump.probeReceivedCount, climate->heatPump.probeReceivedMs,
                     climate->heatPump.responseSentCount, climate->heatPump.responseSentMs,
                     climate->heatPump.lastEchoCount, climate->heatPump.lastEchoMatch ? 1 : 0,
                     climate->heatPump.rawRxBytesSeen, climate->heatPump.incompleteFrameCount, climate->heatPump.validFrameCount, climate->heatPump.lastRxByteMs,
                     patterns);
            lastByteCount = bytesReceived;
            lastDiag = millis();
        }
        
        // Yield to allow other tasks (including watchdog) to run
        vTaskDelay(1);
    }
}

void FujitsuClimate::setup() {
    ESP_LOGW("fuji", "setup() at %lums - RX:%d TX:%d (priority=BUS, before WiFi)", 
             millis(), this->rx_pin_, this->tx_pin_);
    this->lock = xSemaphoreCreateBinary();
    xSemaphoreGive(this->lock);
    this->pendingUpdate = false;
    memcpy(&(this->sharedState), this->heatPump.getCurrentState(),
           sizeof(FujiFrame));

    // EN (Pin 2) and NRST (Pin 7) are hardwired to 5V - no GPIO control needed

    this->heatPump.connect(&Serial2, true, this->rx_pin_, this->tx_pin_);
    
    // EN (Pin 2) and NRST (Pin 7) are hardwired to 5V and do NOT need GPIO control
    // They were previously controlled by GPIO25 and GPIO26, which caused unnecessary
    // power dissipation and thermal issues. Simply tying them to 5V is the correct solution.
    // See datasheet section 4.1.5 (Mode transition via EN pin) and 4.3.2 (Reset output)
    
    ESP_LOGD("fuji", "starting task");
    xTaskCreatePinnedToCore(serialTask, "FujiTask", 10000, (void *)this,
                            2, &(this->taskHandle), 1);
    ESP_LOGD("fuji", "setup complete - serialTask started at priority 2 on core 1");
}

void FujitsuClimate::on_shutdown() {
    // Log shutdown to HA activity feed via status sensor
    if (this->status_sensor_ != nullptr) {
        this->status_sensor_->publish_state("Shutting down...");
    }

    // EN pin is hardwired to 5V, so we cannot disable the transceiver via GPIO
    // The transceiver will remain in normal operation during OTA reboot.
    // The AC unit will detect the secondary controller going silent and handle re-registration.
    if (this->taskHandle != nullptr) {
        vTaskDelete(this->taskHandle);
        this->taskHandle = nullptr;
    }
}

optional<climate::ClimateMode> FujitsuClimate::fujiToEspMode(
    FujiMode fujiMode) {
    if (fujiMode == FujiMode::FAN) {
        return climate::ClimateMode::CLIMATE_MODE_FAN_ONLY;
    }
    if (fujiMode == FujiMode::DRY) {
        return climate::ClimateMode::CLIMATE_MODE_DRY;
    }
    if (fujiMode == FujiMode::COOL) {
        return climate::ClimateMode::CLIMATE_MODE_COOL;
    }
    if (fujiMode == FujiMode::HEAT) {
        return climate::ClimateMode::CLIMATE_MODE_HEAT;
    }
    if (fujiMode == FujiMode::AUTO) {
        return climate::ClimateMode::CLIMATE_MODE_AUTO;
    }
    return {};
}

optional<FujiMode> FujitsuClimate::espToFujiMode(climate::ClimateMode espMode) {
    if (espMode == climate::ClimateMode::CLIMATE_MODE_FAN_ONLY) {
        return FujiMode::FAN;
    }
    if (espMode == climate::ClimateMode::CLIMATE_MODE_DRY) {
        return FujiMode::DRY;
    }
    if (espMode == climate::ClimateMode::CLIMATE_MODE_COOL) {
        return FujiMode::COOL;
    }
    if (espMode == climate::ClimateMode::CLIMATE_MODE_HEAT) {
        return FujiMode::HEAT;
    }
    if (espMode == climate::ClimateMode::CLIMATE_MODE_AUTO) {
        return FujiMode::AUTO;
    }
    return {};
}

optional<climate::ClimateFanMode> FujitsuClimate::fujiToEspFanMode(
    FujiFanMode fujiFanMode) {
    if (fujiFanMode == FujiFanMode::FAN_AUTO) {
        return climate::ClimateFanMode::CLIMATE_FAN_AUTO;
    }

    if (fujiFanMode == FujiFanMode::FAN_HIGH) {
        return climate::ClimateFanMode::CLIMATE_FAN_HIGH;
    }

    if (fujiFanMode == FujiFanMode::FAN_MEDIUM) {
        return climate::ClimateFanMode::CLIMATE_FAN_MEDIUM;
    }

    if (fujiFanMode == FujiFanMode::FAN_LOW) {
        return climate::ClimateFanMode::CLIMATE_FAN_LOW;
    }

    return {};
}

optional<FujiFanMode> FujitsuClimate::espToFujiFanMode(
    climate::ClimateFanMode espFanMode) {
    if (espFanMode == climate::ClimateFanMode::CLIMATE_FAN_AUTO) {
        return FujiFanMode::FAN_AUTO;
    }

    if (espFanMode == climate::ClimateFanMode::CLIMATE_FAN_HIGH) {
        return FujiFanMode::FAN_HIGH;
    }

    if (espFanMode == climate::ClimateFanMode::CLIMATE_FAN_MEDIUM) {
        return FujiFanMode::FAN_MEDIUM;
    }

    if (espFanMode == climate::ClimateFanMode::CLIMATE_FAN_LOW) {
        return FujiFanMode::FAN_LOW;
    }

    return {};
}

void FujitsuClimate::updateState() {
    if (this->pendingUpdate) {  // wait till update is sent
        return;
    }
    
    // Check if a pending retry needs re-sending
    this->checkRetry();
    if (this->pendingUpdate) return;  // checkRetry may have re-sent
    
    // While retry is active, don't sync fields we're waiting on —
    // this prevents the UI from bouncing back to old values before
    // the unit has had time to process our write.
    byte holdFields = this->retryActive_ ? this->retryFields_ : 0;
    
    bool updated = false;
    if (xSemaphoreTake(this->lock, TickType_t(200)) == pdTRUE) {
        // Room temp (always sync — this is read-only sensor data)
        if (this->current_temperature != this->sharedState.controllerTemp) {
            this->current_temperature = this->sharedState.controllerTemp;
            updated = true;
        }

        // Target temp
        if (!(holdFields & kTempUpdateMask) &&
            this->sharedState.temperature != this->target_temperature) {
            ESP_LOGD("fuji", "ctrl temp %d vs my temp %d",
                     this->sharedState.temperature, this->target_temperature);
            this->target_temperature = this->sharedState.temperature;
            updated = true;
        }

        // Mode
        if (!(holdFields & (kModeUpdateMask | kOnOffUpdateMask))) {
            auto newMode = fujiToEspMode((FujiMode)this->sharedState.acMode);
            if (newMode.has_value() && this->sharedState.onOff &&
                newMode.value() != this->mode) {
                ESP_LOGD("fuji", "ctrl mode %d vs my mode %d", newMode.value(),
                         this->mode);
                this->mode = newMode.value();
                updated = true;
            }
            
            if (!this->sharedState.onOff &&
                this->mode != climate::ClimateMode::CLIMATE_MODE_OFF) {
                ESP_LOGD("fuji",
                         "Controller turned off AC, adding mode change to call");
                this->mode = climate::ClimateMode::CLIMATE_MODE_OFF;
                updated = true;
            }
        }

        // Fan speed
        if (!(holdFields & kFanModeUpdateMask)) {
            auto newFanMode =
                fujiToEspFanMode((FujiFanMode)this->sharedState.fanMode);
            if (newFanMode.has_value() && newFanMode.value() != this->fan_mode) {
                ESP_LOGD("fujitsu", "ctrl fan mode %d vs my fan mode %d",
                         static_cast<int>(newFanMode.value()),
                         this->fan_mode.has_value() ? static_cast<int>(this->fan_mode.value()) : -1);
                this->fan_mode = newFanMode.value();
                updated = true;
            }
        }

        if (!(holdFields & kEconomyModeUpdateMask)) {
            if (this->sharedState.economyMode &&
                this->preset != climate::ClimatePreset::CLIMATE_PRESET_ECO) {
                ESP_LOGD("fujitsu",
                         "ECO mode turned on by controller, adding preset change "
                         "to call %d ",
                         this->sharedState.economyMode);

                this->preset = climate::ClimatePreset::CLIMATE_PRESET_ECO;
                updated = true;
            } else if (!this->sharedState.economyMode &&
                       this->preset == climate::ClimatePreset::CLIMATE_PRESET_ECO) {
                ESP_LOGD("fujitsu",
                         "ECO mode turned off by controller, adding preset change "
                         "to call, %d",
                         this->sharedState.economyMode);

                this->preset = climate::ClimatePreset::CLIMATE_PRESET_NONE;
                updated = true;
            }
        }

        xSemaphoreGive(this->lock);
    }

    if (updated) {
        ESP_LOGD("fuji", "publishing state");
        this->publish_state();
    }

    // Update connection status sensor
    if (this->status_sensor_ != nullptr) {
        std::string status;
        if (this->heatPump.isBound()) {
            status = "Connected (Read-Write)";
        } else if (this->sharedState.controllerPresent) {
            status = "Read-Only Mode";
        } else {
            status = "No LIN Data";
        }
        if (status != this->last_status_) {
            this->status_sensor_->publish_state(status);
            this->last_status_ = status;
        }
    }
}

void FujitsuClimate::loop() { this->updateState(); }

void FujitsuClimate::startRetry(byte fields) {
    memcpy(&this->retryState_, &this->sharedState, sizeof(FujiFrame));
    this->retryFields_ = fields;
    this->retryCount_ = 0;
    this->retryRequestTime_ = millis();
    this->retryLastAttempt_ = millis();
    this->retryActive_ = true;
    ESP_LOGD("fuji", "Retry: tracking fields=0x%02X", fields);
}

void FujitsuClimate::checkRetry() {
    if (!this->retryActive_ || this->pendingUpdate) return;
    
    // Wait at least kRetryIntervalMs since last attempt
    if (millis() - this->retryLastAttempt_ < kRetryIntervalMs) return;
    
    // Check if the values we wanted are now reflected in the unit's state
    bool matched = true;
    if (xSemaphoreTake(this->lock, TickType_t(200)) == pdTRUE) {
        if ((this->retryFields_ & kTempUpdateMask) && 
            this->sharedState.temperature != this->retryState_.temperature) {
            matched = false;
        }
        if ((this->retryFields_ & kModeUpdateMask) && 
            this->sharedState.acMode != this->retryState_.acMode) {
            matched = false;
        }
        if ((this->retryFields_ & kOnOffUpdateMask) && 
            this->sharedState.onOff != this->retryState_.onOff) {
            matched = false;
        }
        if ((this->retryFields_ & kFanModeUpdateMask) && 
            this->sharedState.fanMode != this->retryState_.fanMode) {
            matched = false;
        }
        if ((this->retryFields_ & kEconomyModeUpdateMask) && 
            this->sharedState.economyMode != this->retryState_.economyMode) {
            matched = false;
        }
        if ((this->retryFields_ & kSwingModeUpdateMask) && 
            this->sharedState.swingMode != this->retryState_.swingMode) {
            matched = false;
        }
        
        if (matched) {
            ESP_LOGI("fuji", "Retry: change confirmed by unit after %d attempt(s)", 
                     this->retryCount_);
            this->retryActive_ = false;
        } else if (this->retryCount_ >= kMaxRetries) {
            ESP_LOGW("fuji", "Retry: giving up after %d attempts, unit rejected change", 
                     kMaxRetries);
            this->retryActive_ = false;
        } else {
            // Re-apply the desired values
            this->retryCount_++;
            this->retryLastAttempt_ = millis();
            memcpy(&this->sharedState, &this->retryState_, sizeof(FujiFrame));
            this->heatPump.setState(&this->sharedState);
            this->pendingUpdate = true;
            ESP_LOGW("fuji", "Retry: attempt %d/%d, re-sending fields=0x%02X", 
                     this->retryCount_, kMaxRetries, this->retryFields_);
        }
        xSemaphoreGive(this->lock);
    }
}

void FujitsuClimate::control(const climate::ClimateCall &call) {
    if (xSemaphoreTake(this->lock, 1000) == pdTRUE) {
        bool updated = false;
        if (call.get_mode().has_value()) {
            climate::ClimateMode callMode = call.get_mode().value();
            ESP_LOGD("fuji", "Fuji setting mode %d", callMode);

            auto fujiMode = this->espToFujiMode(callMode);

            if (fujiMode.has_value()) {
                this->sharedState.acMode = static_cast<byte>(fujiMode.value());
                if (callMode != climate::ClimateMode::CLIMATE_MODE_OFF) {
                    this->sharedState.onOff = 1;
                }
                this->mode = callMode;
                updated = true;
            }

            if (callMode == climate::ClimateMode::CLIMATE_MODE_OFF) {
                this->sharedState.onOff = 0;
                this->mode = climate::ClimateMode::CLIMATE_MODE_OFF;
                updated = true;
            }
        }
        if (call.get_target_temperature().has_value()) {
            auto callTargetTemp = call.get_target_temperature().value();
            this->sharedState.temperature = callTargetTemp;
            this->target_temperature = callTargetTemp;
            updated = true;
            ESP_LOGD("fuji", "Fuji setting temperature %f", callTargetTemp);
        }

        if (call.get_preset().has_value()) {
            auto callPreset = call.get_preset().value();
            this->sharedState.economyMode = static_cast<byte>(
                callPreset == climate::ClimatePreset::CLIMATE_PRESET_ECO ? 1
                                                                         : 0);
            this->preset = callPreset;
            updated = true;
            ESP_LOGD("fuji", "Fuji setting preset %d", callPreset);
        }

        if (call.get_fan_mode().has_value()) {
            auto callFanMode = call.get_fan_mode().value();
            auto fujiFanMode = this->espToFujiFanMode(callFanMode);
            if (fujiFanMode.has_value()) {
                this->sharedState.fanMode = static_cast<byte>(fujiFanMode.value());
            }
            this->fan_mode = callFanMode;
            updated = true;
            ESP_LOGD("fuji", "Fuji setting fan mode %d", this->fan_mode);
        }
        if (updated) {
            this->heatPump.setState(&(this->sharedState));
            this->pendingUpdate = true;
            // Publish immediately so HA logs the change in the activity feed
            this->publish_state();
            // Start retry tracking with the desired values
            this->startRetry(this->heatPump.getUpdateFields());
        }
        xSemaphoreGive(this->lock);
    }
}

climate::ClimateTraits FujitsuClimate::traits() {
    auto traits = climate::ClimateTraits();

    traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
    traits.set_supported_modes({
        climate::CLIMATE_MODE_AUTO,
        climate::CLIMATE_MODE_HEAT,
        climate::CLIMATE_MODE_FAN_ONLY,
        climate::CLIMATE_MODE_DRY,
        climate::CLIMATE_MODE_COOL,
        climate::CLIMATE_MODE_OFF,
    });

    traits.set_visual_temperature_step(1);
    traits.set_visual_min_temperature(16);
    traits.set_visual_max_temperature(30);

    traits.set_supported_fan_modes(
        {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW,
         climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH});
    traits.set_supported_presets({
        climate::CLIMATE_PRESET_ECO,
        climate::CLIMATE_PRESET_NONE,
    });

    return traits;
}
}  // namespace fujitsu
}  // namespace esphome
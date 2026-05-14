/* This file is based on unreality's FujiHeatPump project */

// #define DEBUG_FUJI
#include "FujiHeatPump.h"
#include "esphome/core/log.h"
#include "driver/uart.h"  // ESP-IDF UART for explicit pin mapping

namespace esphome {
namespace fujitsu {

FujiFrame FujiHeatPump::decodeFrame()
{
    FujiFrame ff;

    ff.messageSource = readBuf[0];
    ff.messageDest = readBuf[1] & 0b01111111;
    ff.messageType = (readBuf[2] & 0b01110000) >> 4;

    ff.acError = (readBuf[kErrorIndex] & kErrorMask) >> kErrorOffset;
    ff.temperature =
        (readBuf[kTemperatureIndex] & kTemperatureMask) >> kTemperatureOffset;
    ff.acMode = (readBuf[kModeIndex] & kModeMask) >> kModeOffset;
    ff.fanMode = (readBuf[kFanIndex] & kFanMask) >> kFanOffset;
    ff.economyMode = (readBuf[kEconomyIndex] & kEconomyMask) >> kEconomyOffset;
    ff.swingMode = (readBuf[kSwingIndex] & kSwingMask) >> kSwingOffset;
    ff.swingStep =
        (readBuf[kSwingStepIndex] & kSwingStepMask) >> kSwingStepOffset;
    ff.controllerPresent =
        (readBuf[kControllerPresentIndex] & kControllerPresentMask) >>
        kControllerPresentOffset;
    ff.updateMagic =
        (readBuf[kUpdateMagicIndex] & kUpdateMagicMask) >> kUpdateMagicOffset;
    ff.onOff = (readBuf[kEnabledIndex] & kEnabledMask) >> kEnabledOffset;
    ff.controllerTemp = (readBuf[kControllerTempIndex] & kControllerTempMask) >> kControllerTempOffset; // there is one leading bit here that is unknown - probably a sign bit for negative temps?

    ff.writeBit = (readBuf[2] & 0b00001000) != 0;
    ff.loginBit = (readBuf[1] & 0b00100000) != 0;
    ff.unknownBit = (readBuf[1] & 0b10000000) > 0;

    return ff;
}

void FujiHeatPump::decodeHeader(byte *buf, byte &src, byte &dst, byte &type,
                                 bool &writeBit, bool &loginBit, bool &unknownBit) {
    src = buf[0];
    dst = buf[1] & 0b01111111;
    type = (buf[2] & 0b01110000) >> 4;
    writeBit = (buf[2] & 0b00001000) != 0;
    loginBit = (buf[1] & 0b00100000) != 0;
    unknownBit = (buf[1] & 0b10000000) > 0;
}

void FujiHeatPump::encodeHeader(byte *buf, byte src, byte dst, byte type,
                                 bool writeBit, bool loginBit, bool unknownBit) {
    buf[0] = src;
    buf[1] = (dst & 0b01111111);
    if (unknownBit) buf[1] |= 0b10000000;
    if (loginBit)   buf[1] |= 0b00100000;
    buf[2] = (type << 4);
    if (writeBit) buf[2] |= 0b00001000;
}

ZoneFrame FujiHeatPump::decodeZoneFrame() {
    ZoneFrame zf;
    zf.messageSource = readBuf[0];
    zf.messageDest = readBuf[1] & 0b01111111;
    zf.messageType = (readBuf[2] & 0b01110000) >> 4;
    zf.writeBit = (readBuf[2] & 0b00001000) != 0;
    zf.loginBit = (readBuf[1] & 0b00100000) != 0;
    zf.unknownBit = (readBuf[1] & 0b10000000) > 0;

    // Decode enabled zones (byte 3, one bit per zone)
    for (int i = 0; i < (int)kZoneCount; i++) {
        zf.zones[i] = (readBuf[3] >> i) & 0b00000001;
    }

    zf.zoneWriteBit = (readBuf[kZoneWriteBitIndex] & kZoneWriteBitMask) >> kZoneWriteBitOffset;
    zf.zoneGroup = static_cast<FujiZoneGroup>((readBuf[kZoneGroupIndex] & kZoneGroupMask) >> kZoneGroupOffset);

    // Bytes 5-6: day/night zone configs (interleaved, 2 bits per zone)
    for (int i = 0; i < (int)kZoneCount; i++) {
        bool zoneEnabled = (readBuf[5] >> i) & 0b00000001;
        int index = i / 2;
        if (i % 2 == 0) {
            zf.dayZones[index] = zoneEnabled;
        } else {
            zf.nightZones[index] = zoneEnabled;
        }
    }
    for (int i = 0; i < (int)kZoneCount; i++) {
        bool zoneEnabled = (readBuf[6] >> i) & 0b00000001;
        int index = (i + 8) / 2;
        if ((i + 8) % 2 == 0) {
            zf.dayZones[index] = zoneEnabled;
        } else {
            zf.nightZones[index] = zoneEnabled;
        }
    }

    ESP_LOGD("fuji", "Zone decode: zones=%d%d%d%d%d%d%d%d group=%d write=%d",
             zf.zones[0], zf.zones[1], zf.zones[2], zf.zones[3],
             zf.zones[4], zf.zones[5], zf.zones[6], zf.zones[7],
             static_cast<int>(zf.zoneGroup), zf.zoneWriteBit);
    return zf;
}

void FujiHeatPump::encodeZoneFrame(ZoneFrame &zf) {
    memset(zoneWriteBuf, 0, 8);

    encodeHeader(zoneWriteBuf, zf.messageSource, zf.messageDest, zf.messageType,
                 zf.writeBit, zf.loginBit, zf.unknownBit);

    // Encode zone on/off bits (byte 3)
    for (int i = 0; i < (int)kZoneCount; i++) {
        zoneWriteBuf[3] |= zf.zones[i] << i;
    }

    zoneWriteBuf[kZoneWriteBitIndex] |= zf.zoneWriteBit << kZoneWriteBitOffset;
    zoneWriteBuf[kZoneGroupIndex] |= static_cast<byte>(zf.zoneGroup) << kZoneGroupOffset;

    // Encode day/night zone configs (bytes 5-6)
    for (int i = 0; i < (int)kZoneCount; i++) {
        int index = i / 2;
        if (i % 2 == 0) {
            zoneWriteBuf[5] |= zf.dayZones[index] << i;
        } else {
            zoneWriteBuf[5] |= zf.nightZones[index] << i;
        }
    }
    for (int i = 0; i < (int)kZoneCount; i++) {
        int index = (i + 8) / 2;
        if ((i + 8) % 2 == 0) {
            zoneWriteBuf[6] |= zf.dayZones[index] << i;
        } else {
            zoneWriteBuf[6] |= zf.nightZones[index] << i;
        }
    }

    zoneWriteBuf[7] = 0x40;  // fixed value per protocol

    ESP_LOGD("fuji", "Zone encode: %02X %02X %02X %02X %02X %02X %02X %02X",
             zoneWriteBuf[0], zoneWriteBuf[1], zoneWriteBuf[2], zoneWriteBuf[3],
             zoneWriteBuf[4], zoneWriteBuf[5], zoneWriteBuf[6], zoneWriteBuf[7]);
}

void FujiHeatPump::encodeFrame(FujiFrame ff)
{
    memset(writeBuf, 0, 8);

    writeBuf[0] = ff.messageSource;

    writeBuf[1] &= 0b10000000;
    writeBuf[1] |= ff.messageDest & 0b01111111;

    writeBuf[2] &= 0b11001111;
    writeBuf[2] |= ff.messageType << 4;

    if (ff.writeBit)
    {
        writeBuf[2] |= 0b00001000;
    }
    else
    {
        writeBuf[2] &= 0b11110111;
    }

    writeBuf[1] &= 0b01111111;
    if (ff.unknownBit)
    {
        writeBuf[1] |= 0b10000000;
    }

    if (ff.loginBit)
    {
        writeBuf[1] |= 0b00100000;
    }
    else
    {
        writeBuf[1] &= 0b11011111;
    }

    writeBuf[kModeIndex] =
        (writeBuf[kModeIndex] & ~kModeMask) | (ff.acMode << kModeOffset);
    writeBuf[kModeIndex] = (writeBuf[kEnabledIndex] & ~kEnabledMask) |
                           (ff.onOff << kEnabledOffset);
    writeBuf[kFanIndex] =
        (writeBuf[kFanIndex] & ~kFanMask) | (ff.fanMode << kFanOffset);
    writeBuf[kErrorIndex] =
        (writeBuf[kErrorIndex] & ~kErrorMask) | (ff.acError << kErrorOffset);
    writeBuf[kEconomyIndex] = (writeBuf[kEconomyIndex] & ~kEconomyMask) |
                              (ff.economyMode << kEconomyOffset);
    writeBuf[kTemperatureIndex] =
        (writeBuf[kTemperatureIndex] & ~kTemperatureMask) |
        (ff.temperature << kTemperatureOffset);
    writeBuf[kSwingIndex] =
        (writeBuf[kSwingIndex] & ~kSwingMask) | (ff.swingMode << kSwingOffset);
    writeBuf[kSwingStepIndex] = (writeBuf[kSwingStepIndex] & ~kSwingStepMask) |
                                (ff.swingStep << kSwingStepOffset);
    writeBuf[kControllerPresentIndex] =
        (writeBuf[kControllerPresentIndex] & ~kControllerPresentMask) |
        (ff.controllerPresent << kControllerPresentOffset);
    writeBuf[kUpdateMagicIndex] =
        (writeBuf[kUpdateMagicIndex] & ~kUpdateMagicMask) |
        (ff.updateMagic << kUpdateMagicOffset);
    writeBuf[kControllerTempIndex] =
        (writeBuf[kControllerTempIndex] & ~kControllerTempMask) |
        (ff.controllerTemp << kControllerTempOffset);
}

void FujiHeatPump::connect(HardwareSerial *serial, bool secondary)
{
    return this->connect(serial, secondary, -1, -1);
}

void FujiHeatPump::connect(HardwareSerial *serial, bool secondary,
                           int rxPin = -1, int txPin = -1)
{
    _serial = serial;
    if (rxPin != -1 && txPin != -1)
    {
#ifdef ESP32
        _serial->begin(500, SERIAL_8E1, rxPin, txPin);
        
        // Force TX pin mapping via ESP-IDF — Arduino Core 3.x may not
        // route the TX pin correctly on Serial2.
        esp_err_t err = uart_set_pin(UART_NUM_2, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        ESP_LOGW("fuji", "uart_set_pin(TX=%d, RX=%d) = %s", txPin, rxPin, esp_err_to_name(err));
#else
        Serial.print("Setting RX/TX pin unsupported, using defaults.\n");
        _serial->begin(500, SERIAL_8E1);
#endif
    }
    else
    {
        _serial->begin(500, SERIAL_8E1);
    }
    _serial->setTimeout(200);

    if (secondary)
    {
        controllerIsPrimary = false;
        controllerAddress = static_cast<byte>(FujiAddress::SECONDARY);
        ESP_LOGW("fuji", "=== SECONDARY mode (addr 33) at %lums: waiting for PRIMARY probe ===", millis());
    }
    else
    {
        controllerIsPrimary = true;
        controllerAddress = static_cast<byte>(FujiAddress::PRIMARY);
    }

    lastFrameReceived = 0;
}

void FujiHeatPump::printFrame(byte buf[8], FujiFrame ff)
{
    ESP_LOGD("fuji", "%X %X %X %X %X %X %X %X  ", buf[0], buf[1], buf[2],
             buf[3], buf[4], buf[5], buf[6], buf[7]);
    ESP_LOGD(
        "fuji",
        " mSrc: %d mDst: %d mType: %d write: %d login: %d unknown: %d onOff: "
        "%d temp: %d, mode: %d cP:%d uM:%d cTemp:%d acError:%d \n",
        ff.messageSource, ff.messageDest, ff.messageType, ff.writeBit,
        ff.loginBit, ff.unknownBit, ff.onOff, ff.temperature, ff.acMode,
        ff.controllerPresent, ff.updateMagic, ff.controllerTemp, ff.acError);
}

void FujiHeatPump::sendPendingFrame()
{
    if (pendingFrame && (millis() - lastFrameReceived) > 50)
    {
        // Save TX bytes for echo comparison
        byte txCopy[8];
        memcpy(txCopy, writeBuf, 8);
        
        unsigned long txStart = millis();
        _serial->write(writeBuf, 8);
        _serial->flush();
        unsigned long txEnd = millis();
        pendingFrame = false;
        updateFields = 0;
        
        // Track response for diagnostics
        responseSentCount++;
        responseSentMs = millis();

        // Read echo into separate buffer and verify
        byte echoBuf[8];
        memset(echoBuf, 0, 8);
        int echoBytes = _serial->readBytes(echoBuf, 8);
        unsigned long echoEnd = millis();
        
        // Compare echo with what we sent
        bool echoMatch = (echoBytes == 8);
        if (echoMatch) {
            for (int i = 0; i < 8; i++) {
                if (echoBuf[i] != txCopy[i]) {
                    echoMatch = false;
                    break;
                }
            }
        }
        
        lastEchoCount = echoBytes;
        if (echoMatch) lastEchoMatch = true;
        
        // Log every 50th TX or if echo mismatch
        if (!echoMatch || (responseSentCount % 50 == 1)) {
            ESP_LOGW("fuji", "TX: sent=%02X %02X %02X %02X %02X %02X %02X %02X echo=%02X %02X %02X %02X %02X %02X %02X %02X echoBytes=%d match=%d txMs=%lu-%lu echoMs=%lu",
                txCopy[0], txCopy[1], txCopy[2], txCopy[3], txCopy[4], txCopy[5], txCopy[6], txCopy[7],
                echoBuf[0], echoBuf[1], echoBuf[2], echoBuf[3], echoBuf[4], echoBuf[5], echoBuf[6], echoBuf[7],
                echoBytes, echoMatch ? 1 : 0, txStart, txEnd, echoEnd);
        }
    }
    
    // Auto-login injection: send SECONDARY login after bus goes idle
    if (autoLoginPending && !pendingFrame && (millis() - lastFrameReceived) > 50)
    {
        autoLoginPending = false;
        autoLoginLastAttempt = millis();
        autoLoginCount++;
        
        FujiFrame loginFrame;
        memset(&loginFrame, 0, sizeof(FujiFrame));
        loginFrame.messageDest = static_cast<byte>(FujiAddress::UNIT);
        loginFrame.messageSource = static_cast<byte>(FujiAddress::SECONDARY);
        loginFrame.messageType = static_cast<byte>(FujiMessageType::STATUS);
        loginFrame.controllerPresent = 1;
        loginFrame.loginBit = false;
        loginFrame.updateMagic = 2;
        loginFrame.unknownBit = true;
        loginFrame.writeBit = 0;
        
        encodeFrame(loginFrame);
        for (int i = 0; i < 8; i++) {
            writeBuf[i] ^= 0xFF;
        }
        _serial->write(writeBuf, 8);
        _serial->flush();
        _serial->readBytes(writeBuf, 8);  // read back our own frame
        
        ESP_LOGW("fuji", ">>> AUTO-LOGIN #%lu sent (SECONDARY→UNIT STATUS cP=1) uptime=%lu", 
                 autoLoginCount, millis());
    }
}

void FujiHeatPump::sendPendingZoneFrame()
{
    if (pendingZoneFrame && initialZoneStateReceived && !pendingFrame &&
        (millis() - lastFrameReceived) > 50)
    {
        // XOR with 0xFF for wire format
        for (int i = 0; i < 8; i++) {
            zoneWriteBuf[i] ^= 0xFF;
        }

        _serial->write(zoneWriteBuf, 8);
        _serial->flush();
        pendingZoneFrame = false;
        zoneUpdateFields = 0;
        zoneGroupUpdated = false;

        // Read back echo
        byte echoBuf[8];
        int echoBytes = _serial->readBytes(echoBuf, 8);

        ESP_LOGI("fuji", "Zone TX: sent %d echo=%d", 8, echoBytes);
    }
}

bool FujiHeatPump::waitForFrame()
{
    FujiFrame ff;

    if (_serial->available())
    {
        memset(readBuf, 0, 8);
        int bytesRead = _serial->readBytes(readBuf, 8);

        if (bytesRead < 8)
        {
            // skip incomplete frame
            return false;
        }

        for (int i = 0; i < 8; i++)
        {
            readBuf[i] ^= 0xFF;
        }

        // Check message type before full decode — zone frames have different layout
        byte peekType = (readBuf[2] & 0b01110000) >> 4;
        if (peekType == static_cast<byte>(FujiMessageType::ZONE)) {
            lastFrameReceived = millis();
            ESP_LOGI("fuji", "Zone frame received");
            ZoneFrame zf = decodeZoneFrame();
            currentZoneState = zf;
            initialZoneStateReceived = true;
            return true;
        }

        ff = decodeFrame();

        // === Track unique frame patterns (src/dst/type/cP) ===
        {
            bool found = false;
            for (int i = 0; i < seenPatternCount; i++) {
                if (seenPatterns[i].src == ff.messageSource && seenPatterns[i].dst == ff.messageDest &&
                    seenPatterns[i].type == ff.messageType && seenPatterns[i].cP == ff.controllerPresent) {
                    seenPatterns[i].count++;
                    found = true;
                    break;
                }
            }
            if (!found && seenPatternCount < 16) {
                seenPatterns[seenPatternCount] = {ff.messageSource, ff.messageDest, ff.messageType, ff.controllerPresent, 1};
                ESP_LOGW("fuji", "NEW PATTERN: src=%d dst=%d type=%d cP=%d login=%d raw=%02X %02X %02X %02X %02X %02X %02X %02X",
                    ff.messageSource, ff.messageDest, ff.messageType, ff.controllerPresent, ff.loginBit,
                    readBuf[0], readBuf[1], readBuf[2], readBuf[3], readBuf[4], readBuf[5], readBuf[6], readBuf[7]);
                seenPatternCount++;
            }
        }

        // === LOG ALL LOGIN FRAMES on the bus (regardless of destination) ===
        if (ff.messageType == static_cast<byte>(FujiMessageType::LOGIN)) {
            ESP_LOGW("fuji", "!!! LOGIN FRAME on bus: src=%d dst=%d cP=%d login=%d raw=%02X %02X %02X %02X %02X %02X %02X %02X uptime=%lu",
                ff.messageSource, ff.messageDest, ff.controllerPresent, ff.loginBit,
                readBuf[0], readBuf[1], readBuf[2], readBuf[3], readBuf[4], readBuf[5], readBuf[6], readBuf[7],
                millis());
        }

        // === Auto-login: after seeing PRIMARY→UNIT response, queue our SECONDARY login ===
        if (autoLoginEnabled && !isBound() && 
            ff.messageSource == static_cast<byte>(FujiAddress::PRIMARY) && 
            ff.messageDest == static_cast<byte>(FujiAddress::UNIT) &&
            (millis() - autoLoginLastAttempt) > 10000) {  // max once per 10 sec
            autoLoginPending = true;
        }

        // Log frames addressed to SECONDARY (33) or with unexpected destinations
        if (ff.messageDest == static_cast<byte>(FujiAddress::SECONDARY)) {
            ESP_LOGW("fuji", "!!! PROBE FOR SECONDARY: src=%d type=%d cP=%d login=%d uM=%d raw=%02X %02X %02X %02X %02X %02X %02X %02X uptime=%lu",
                ff.messageSource, ff.messageType,
                ff.controllerPresent, ff.loginBit, ff.updateMagic,
                readBuf[0], readBuf[1], readBuf[2], readBuf[3], readBuf[4], readBuf[5], readBuf[6], readBuf[7],
                millis());
            frameDestSecondary++;
        } else if (ff.messageDest == static_cast<byte>(FujiAddress::PRIMARY)) {
            frameDestPrimary++;
        } else {
            // Log first occurrence of each unique "other" destination
            bool alreadyLogged = false;
            for (int i = 0; i < loggedOtherCount; i++) {
                if (loggedOtherDests[i] == ff.messageDest) { alreadyLogged = true; break; }
            }
            if (!alreadyLogged && loggedOtherCount < 8) {
                loggedOtherDests[loggedOtherCount++] = ff.messageDest;
                ESP_LOGW("fuji", "OTHER dst=%d src=%d type=%d raw=%02X %02X %02X %02X %02X %02X %02X %02X",
                    ff.messageDest, ff.messageSource, ff.messageType,
                    readBuf[0], readBuf[1], readBuf[2], readBuf[3], readBuf[4], readBuf[5], readBuf[6], readBuf[7]);
            }
            frameDestOther++;
        }

#ifdef DEBUG_FUJI
        ESP_LOGD("fuji", "<-- ");
        printFrame(readBuf, ff);
#endif

        // Log ZONE frames to capture zone controller traffic
        if (ff.messageType == static_cast<byte>(FujiMessageType::ZONE))
        {
            ESP_LOGI("fuji", "ZONE FRAME src=%d dest=%d byte3=%02X byte4=%02X byte5=%02X byte6=%02X byte7=%02X",
                ff.messageSource, ff.messageDest,
                readBuf[3], readBuf[4], readBuf[5], readBuf[6], readBuf[7]);
        }

        if (ff.messageDest == controllerAddress)
        {
            lastFrameReceived = millis();
            
            // Track probe reception for SECONDARY diagnostics
            if (!controllerIsPrimary) {
                probeReceivedCount++;
                probeReceivedMs = millis();
                ESP_LOGW("fuji", "!!! FRAME FOR US at %lums: src=%d type=%d cP=%d uM=%d", 
                    millis(), ff.messageSource, ff.messageType, ff.controllerPresent, ff.updateMagic);
            }

            if (ff.messageType == static_cast<byte>(FujiMessageType::STATUS))
            {
                if (ff.controllerPresent == 1)
                {
                    // we have logged into the indoor unit
                    // this is what most frames are
                    ff.messageSource = controllerAddress;

                    if (seenSecondaryController)
                    {
                        ff.messageDest =
                            static_cast<byte>(FujiAddress::SECONDARY);
                        ff.loginBit = true;
                        ff.controllerPresent = 0;
                    }
                    else
                    {
                        ff.messageDest = static_cast<byte>(FujiAddress::UNIT);
                        ff.loginBit = false;
                        ff.controllerPresent = 1;
                    }

                    // SECONDARY: updateMagic=0 for steady-state (uM=2 only in first response, handled in cP==0 path)
                    // PRIMARY: updateMagic=0 always
                    ff.updateMagic = 0;
                    ff.unknownBit = true;
                    ff.writeBit = 0;
                    ff.messageType = static_cast<byte>(FujiMessageType::STATUS);
                }
                else
                {
                    if (controllerIsPrimary)
                    {
                        // if this is the first message we have received,
                        // announce ourselves to the indoor unit
                        ff.messageSource = controllerAddress;
                        ff.messageDest = static_cast<byte>(FujiAddress::UNIT);
                        ff.loginBit = false;
                        ff.controllerPresent = 0;
                        ff.updateMagic = 0;
                        ff.unknownBit = true;
                        ff.writeBit = 0;
                        ff.messageType =
                            static_cast<byte>(FujiMessageType::LOGIN);

                        ff.onOff = 0;
                        ff.temperature = 0;
                        ff.acMode = 0;
                        ff.fanMode = 0;
                        ff.swingMode = 0;
                        ff.swingStep = 0;
                        ff.acError = 0;
                    }
                    else
                    {
                        // secondary controller never seems to get any other
                        // message types, only status with controllerPresent ==
                        // 0 the secondary controller seems to send the same
                        // flags no matter which message type

                        ff.messageSource = controllerAddress;
                        ff.messageDest = static_cast<byte>(FujiAddress::UNIT);
                        ff.loginBit = false;
                        ff.controllerPresent = 1;
                        ff.updateMagic = 2;
                        ff.unknownBit = true;
                        ff.writeBit = 0;
                    }
                }

                // if we have any updates, set the flags
                if (updateFields)
                {
                    ff.writeBit = 1;
                }

                if (updateFields & kOnOffUpdateMask)
                {
                    ff.onOff = updateState.onOff;
                }

                if (updateFields & kTempUpdateMask)
                {
                    ff.temperature = updateState.temperature;
                }

                if (updateFields & kModeUpdateMask)
                {
                    ff.acMode = updateState.acMode;
                }

                if (updateFields & kFanModeUpdateMask)
                {
                    ff.fanMode = updateState.fanMode;
                }

                if (updateFields & kSwingModeUpdateMask)
                {
                    ff.swingMode = updateState.swingMode;
                }

                if (updateFields & kSwingStepUpdateMask)
                {
                    ff.swingStep = updateState.swingStep;
                }

                if (updateFields & kEconomyModeUpdateMask)
                {
                    ff.economyMode = updateState.economyMode;
                }

                memcpy(&currentState, &ff, sizeof(FujiFrame));
            }
            else if (ff.messageType ==
                     static_cast<byte>(FujiMessageType::LOGIN))
            {
                // received a login frame OK frame
                // the primary will send packet to a secondary controller to see
                // if it exists
                ff.messageSource = controllerAddress;
                ff.messageDest = static_cast<byte>(FujiAddress::UNIT);
                ff.loginBit = true;
                ff.controllerPresent = 1;
                ff.updateMagic = 0;
                ff.unknownBit = true;
                ff.writeBit = 0;

                ff.onOff = currentState.onOff;
                ff.temperature = currentState.temperature;
                ff.acMode = currentState.acMode;
                ff.fanMode = currentState.fanMode;
                ff.swingMode = currentState.swingMode;
                ff.swingStep = currentState.swingStep;
                ff.acError = currentState.acError;
            }
            else if (ff.messageType ==
                     static_cast<byte>(FujiMessageType::ERROR))
            {
                ESP_LOGD("fuji", "AC ERROR RECV: ");
                printFrame(readBuf, ff);
                // handle errors here
                return false;
            }

            encodeFrame(ff);

#ifdef DEBUG_FUJI
            ESP_LOGD("fuji", "--> ");
            printFrame(writeBuf, ff);
#endif

            for (int i = 0; i < 8; i++)
            {
                writeBuf[i] ^= 0xFF;
            }

            // Use deferred send for all modes — sendPendingFrame() will
            // transmit after a 50ms bus-idle gap (original upstream approach).
            pendingFrame = true;

            // Prepare pending zone frame if zone updates are queued
            if (zoneUpdateFields || zoneGroupUpdated) {
                ZoneFrame zf = currentZoneState;
                zf.zoneWriteBit = true;

                if (zoneGroupUpdated) {
                    zf.zoneGroup = zoneUpdateState.zoneGroup;
                } else {
                    zf.zoneGroup = currentZoneState.zoneGroup;
                }

                switch (zf.zoneGroup) {
                    case FujiZoneGroup::NONE:
                        for (int i = 0; i < (int)kZoneCount; i++) {
                            if (zoneUpdateFields & (1 << i)) {
                                zf.zones[i] = zoneUpdateState.zones[i];
                            } else {
                                zf.zones[i] = currentZoneState.zones[i];
                            }
                        }
                        break;
                    case FujiZoneGroup::DAY:
                        memcpy(zf.zones, zf.dayZones, kZoneCount);
                        break;
                    case FujiZoneGroup::NIGHT:
                        memcpy(zf.zones, zf.nightZones, kZoneCount);
                        break;
                    case FujiZoneGroup::ALL:
                        for (int i = 0; i < (int)kZoneCount; i++) {
                            zf.zones[i] = true;
                        }
                        break;
                }

                zf.messageSource = controllerAddress;
                zf.messageDest = static_cast<byte>(FujiAddress::UNIT);
                zf.messageType = static_cast<byte>(FujiMessageType::ZONE);
                zf.loginBit = false;
                zf.unknownBit = true;
                zf.writeBit = true;
                encodeZoneFrame(zf);
                pendingZoneFrame = true;
            }
        }
        else if (ff.messageDest ==
                 static_cast<byte>(FujiAddress::SECONDARY))
        {
            seenSecondaryController = true;
            currentState.controllerTemp =
                ff.controllerTemp; // we dont have a temp sensor, use the temp
                                   // reading from the secondary controller
        }
        else if (!controllerIsPrimary &&
                 ff.messageSource == static_cast<byte>(FujiAddress::PRIMARY) &&
                 ff.messageDest == static_cast<byte>(FujiAddress::UNIT))
        {
            // Passive monitoring: when SECONDARY is not being polled by the
            // unit, capture state from PRIMARY→UNIT frames visible on the bus.
            // The PRIMARY's response contains room temp and current AC state.
            currentState.controllerTemp = ff.controllerTemp;
            currentState.temperature = ff.temperature;
            currentState.acMode = ff.acMode;
            currentState.fanMode = ff.fanMode;
            currentState.onOff = ff.onOff;
            currentState.economyMode = ff.economyMode;
            currentState.swingMode = ff.swingMode;
            currentState.swingStep = ff.swingStep;
            currentState.controllerPresent = 1;
        }

        return true;
    }

    return false;
}

bool FujiHeatPump::isBound()
{
    if (millis() - lastFrameReceived < 1000)
    {
        return true;
    }
    return false;
}

bool FujiHeatPump::updatePending()
{
    if (updateFields)
    {
        return true;
    }
    return false;
}

void FujiHeatPump::setOnOff(bool o)
{
    updateFields |= kOnOffUpdateMask;
    updateState.onOff = o ? 1 : 0;
}
void FujiHeatPump::setTemp(byte t)
{
    updateFields |= kTempUpdateMask;
    updateState.temperature = t;
}
void FujiHeatPump::setMode(byte m)
{
    updateFields |= kModeUpdateMask;
    updateState.acMode = m;
}
void FujiHeatPump::setFanMode(byte fm)
{
    updateFields |= kFanModeUpdateMask;
    updateState.fanMode = fm;
}
void FujiHeatPump::setEconomyMode(byte em)
{
    updateFields |= kEconomyModeUpdateMask;
    updateState.economyMode = em;
}
void FujiHeatPump::setSwingMode(byte sm)
{
    updateFields |= kSwingModeUpdateMask;
    updateState.swingMode = sm;
}
void FujiHeatPump::setSwingStep(byte ss)
{
    updateFields |= kSwingStepUpdateMask;
    updateState.swingStep = ss;
}

bool FujiHeatPump::getOnOff() { return currentState.onOff == 1 ? true : false; }
byte FujiHeatPump::getTemp() { return currentState.temperature; }
byte FujiHeatPump::getMode() { return currentState.acMode; }
byte FujiHeatPump::getFanMode() { return currentState.fanMode; }
byte FujiHeatPump::getEconomyMode() { return currentState.economyMode; }
byte FujiHeatPump::getSwingMode() { return currentState.swingMode; }
byte FujiHeatPump::getSwingStep() { return currentState.swingStep; }
byte FujiHeatPump::getControllerTemp() { return currentState.controllerTemp; }

FujiFrame *FujiHeatPump::getCurrentState() { return &currentState; }

FujiFrame *FujiHeatPump::getUpdateState() { return &updateState; }

void FujiHeatPump::setState(FujiFrame *state)
{
    FujiFrame *current = this->getCurrentState();
    if (state->onOff != current->onOff)
    {
        this->setOnOff(state->onOff);
    }

    if (state->temperature != current->temperature)
    {
        this->setTemp(state->temperature);
    }

    if (state->acMode != current->acMode)
    {
        this->setMode(state->acMode);
    }

    if (state->fanMode != current->fanMode)
    {
        this->setFanMode(state->fanMode);
    }

    if (state->economyMode != current->economyMode)
    {
        this->setEconomyMode(state->economyMode);
    }

    if (state->swingMode != current->swingMode)
    {
        this->setSwingMode(state->swingMode);
    }

    if (state->swingStep != current->swingStep)
    {
        this->setSwingStep(state->swingStep);
    }
}

byte FujiHeatPump::getUpdateFields() { return updateFields; }

void FujiHeatPump::setZoneOnOff(int zone, bool o) {
    if (zone < 0 || zone >= (int)kZoneCount) return;
    zoneUpdateFields |= (1 << zone);
    zoneUpdateState.zones[zone] = o;
}

void FujiHeatPump::setZoneGroup(FujiZoneGroup zoneGroup) {
    zoneGroupUpdated = true;
    zoneUpdateState.zoneGroup = zoneGroup;
}

bool FujiHeatPump::getZoneOnOff(int zone) {
    if (zone < 0 || zone >= (int)kZoneCount) return false;
    return currentZoneState.zones[zone];
}

FujiZoneGroup FujiHeatPump::getZoneGroup() { return currentZoneState.zoneGroup; }

ZoneFrame *FujiHeatPump::getCurrentZoneState() { return &currentZoneState; }

}  // namespace fujitsu
}  // namespace esphome

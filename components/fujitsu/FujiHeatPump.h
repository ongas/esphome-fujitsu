/* This file is based on unreality's FujiHeatPump project */
#pragma once

#include <Arduino.h>

namespace esphome {
namespace fujitsu {

const byte kModeIndex = 3;
const byte kModeMask = 0b00001110;
const byte kModeOffset = 1;

const byte kFanIndex = 3;
const byte kFanMask = 0b01110000;
const byte kFanOffset = 4;

const byte kEnabledIndex = 3;
const byte kEnabledMask = 0b00000001;
const byte kEnabledOffset = 0;

const byte kErrorIndex = 3;
const byte kErrorMask = 0b10000000;
const byte kErrorOffset = 7;

const byte kEconomyIndex = 4;
const byte kEconomyMask = 0b10000000;
const byte kEconomyOffset = 7;

const byte kTemperatureIndex = 4;
const byte kTemperatureMask = 0b01111111;
const byte kTemperatureOffset = 0;

const byte kUpdateMagicIndex = 5;
const byte kUpdateMagicMask = 0b11110000;
const byte kUpdateMagicOffset = 4;

const byte kSwingIndex = 5;
const byte kSwingMask = 0b00000100;
const byte kSwingOffset = 2;

const byte kSwingStepIndex = 5;
const byte kSwingStepMask = 0b00000010;
const byte kSwingStepOffset = 1;

const byte kControllerPresentIndex = 6;
const byte kControllerPresentMask = 0b00000001;
const byte kControllerPresentOffset = 0;

const byte kControllerTempIndex = 6;
const byte kControllerTempMask = 0b01111110;
const byte kControllerTempOffset = 1;

typedef struct FujiFrames {
    byte onOff = 0;
    byte temperature = 16;
    byte acMode = 0;
    byte fanMode = 0;
    byte acError = 0;
    byte economyMode = 0;
    byte swingMode = 0;
    byte swingStep = 0;
    byte controllerPresent = 0;
    byte updateMagic = 0;  // unsure what this value indicates
    byte controllerTemp = 16;

    bool writeBit = false;
    bool loginBit = false;
    bool unknownBit = false;  // unsure what this bit indicates

    byte messageType = 0;
    byte messageSource = 0;
    byte messageDest = 0;
} FujiFrame;

class FujiHeatPump {
   private:
    HardwareSerial *_serial;
    byte readBuf[8];
    byte writeBuf[8];

    byte controllerAddress;
    bool controllerIsPrimary = true;
    bool seenSecondaryController = false;
    bool controllerLoggedIn = false;
    unsigned long lastFrameReceived;

    byte updateFields;
    FujiFrame updateState;
    FujiFrame currentState;

    FujiFrame decodeFrame();
    void encodeFrame(FujiFrame ff);
    void printFrame(byte buf[8], FujiFrame ff);

    bool pendingFrame = false;
    
    // Auto-login: inject SECONDARY login after seeing bus activity
    bool autoLoginPending = false;
    unsigned long autoLoginLastAttempt = 0;

   public:
    unsigned long autoLoginCount = 0;
    // Track unique frame patterns seen (src|dst|type|cP packed into byte)
    struct FramePattern { byte src; byte dst; byte type; byte cP; unsigned long count; };
    FramePattern seenPatterns[16] = {};
    byte seenPatternCount = 0;
    
    // Track which "other" destinations we've already logged
    byte loggedOtherDests[8] = {0};
    byte loggedOtherCount = 0;

    // Frame destination counters for diagnostics
    unsigned long frameDestPrimary = 0;
    unsigned long frameDestSecondary = 0;
    unsigned long frameDestOther = 0;
    
    // Probe/response tracking (survives until next reboot, visible in DIAG)
    unsigned long probeReceivedCount = 0;
    unsigned long probeReceivedMs = 0;
    unsigned long responseSentCount = 0;
    unsigned long responseSentMs = 0;
    int lastEchoCount = -1;  // -1 = no TX yet, 0-8 = echo bytes received
    bool lastEchoMatch = false;
    bool firstSecondaryResponse = true; // track first response for uM=2
    int updateRetryCount = 0; // how many frames we've sent writeBit=1 without ACK
    bool frameSynced = false; // true after detecting inter-frame gap at boot
    int bootQuietCount = 0;   // probes to observe silently before responding
    int enPin = -1;           // LIN transceiver EN pin (enabled after frame sync)
    
    // Enable/disable auto-login injection (disabled: unsolicited frames don't work)
    bool autoLoginEnabled = false;

    void connect(HardwareSerial *serial, bool secondary);
    void connect(HardwareSerial *serial, bool secondary, int rxPin, int txPin);
    void connect(HardwareSerial *serial, bool secondary, int rxPin, int txPin, int enPin);

    bool waitForFrame();
    void sendPendingFrame();
    bool isBound();
    bool updatePending();

    void setOnOff(bool o);
    void setTemp(byte t);
    void setMode(byte m);
    void setFanMode(byte fm);
    void setEconomyMode(byte em);
    void setSwingMode(byte sm);
    void setSwingStep(byte ss);
    void setState(FujiFrame * state);

    bool getOnOff();
    byte getTemp();
    byte getMode();
    byte getFanMode();
    byte getEconomyMode();
    byte getSwingMode();
    byte getSwingStep();
    byte getControllerTemp();

    FujiFrame *getCurrentState();
    FujiFrame *getUpdateState();
    
    byte getUpdateFields();

};

enum class FujiMode : byte {
    UNKNOWN = 0,
    FAN = 1,
    DRY = 2,
    COOL = 3,
    HEAT = 4,
    AUTO = 5,
};

enum class FujiMessageType : byte {
    STATUS = 0,
    ERROR = 1,
    LOGIN = 2,
    UNKNOWN = 3,
    ZONE = 5,
};

enum class FujiAddress : byte {
    START = 0,
    UNIT = 1,
    PRIMARY = 32,
    SECONDARY = 33,
};

enum class FujiFanMode : byte {
    FAN_AUTO = 0,
    FAN_QUIET = 1,
    FAN_LOW = 2,
    FAN_MEDIUM = 3,
    FAN_HIGH = 4
};

const byte kOnOffUpdateMask = 0b10000000;
const byte kTempUpdateMask = 0b01000000;
const byte kModeUpdateMask = 0b00100000;
const byte kFanModeUpdateMask = 0b00010000;
const byte kEconomyModeUpdateMask = 0b00001000;
const byte kSwingModeUpdateMask = 0b00000100;
const byte kSwingStepUpdateMask = 0b00000010;

}  // namespace fujitsu
}  // namespace esphome

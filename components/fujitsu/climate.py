import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, text_sensor, button
from esphome.const import CONF_ID
from esphome import pins

AUTO_LOAD = ["climate", "text_sensor", "button"]

CONF_RX_PIN = "rx_pin"
CONF_TX_PIN = "tx_pin"
CONF_EN_PIN = "en_pin"
CONF_NRST_PIN = "nrst_pin"
CONF_STATUS_SENSOR = "status_sensor"
CONF_ATTEMPT_LOGIN_BUTTON = "attempt_login_button"

fujitsu_ns = cg.esphome_ns.namespace("fujitsu")
FujitsuClimate = fujitsu_ns.class_("FujitsuClimate", climate.Climate, cg.Component)
AttemptLoginButton = fujitsu_ns.class_("AttemptLoginButton", button.Button, cg.Component)

CONFIG_SCHEMA = (
    climate.climate_schema(FujitsuClimate)
    .extend(cv.COMPONENT_SCHEMA)
    .extend(
        {
            cv.Optional(CONF_RX_PIN, default=16): cv.int_,
            cv.Optional(CONF_TX_PIN, default=17): cv.int_,
            cv.Optional(CONF_EN_PIN): cv.int_,
            cv.Optional(CONF_NRST_PIN): cv.int_,
            cv.Optional(CONF_STATUS_SENSOR): text_sensor.text_sensor_schema(
                icon="mdi:information-outline",
                entity_category="diagnostic",
            ),
            cv.Optional(CONF_ATTEMPT_LOGIN_BUTTON): button.button_schema(
                AttemptLoginButton,
                icon="mdi:login",
                entity_category="diagnostic",
            ),
        }
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    cg.add(var.set_rx_pin(config[CONF_RX_PIN]))
    cg.add(var.set_tx_pin(config[CONF_TX_PIN]))
    if CONF_EN_PIN in config:
        cg.add(var.set_en_pin(config[CONF_EN_PIN]))
    if CONF_NRST_PIN in config:
        cg.add(var.set_nrst_pin(config[CONF_NRST_PIN]))
    if CONF_STATUS_SENSOR in config:
        sens = await text_sensor.new_text_sensor(config[CONF_STATUS_SENSOR])
        cg.add(var.set_status_sensor(sens))
    
    if CONF_ATTEMPT_LOGIN_BUTTON in config:
        btn = await button.new_button(config[CONF_ATTEMPT_LOGIN_BUTTON], var)
        await cg.register_component(btn, config)
        cg.add(btn.set_parent(var))

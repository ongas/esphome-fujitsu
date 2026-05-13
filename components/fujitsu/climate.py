import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, text_sensor
from esphome.const import CONF_ID
from esphome import pins, automation

AUTO_LOAD = ["climate", "text_sensor"]

CONF_RX_PIN = "rx_pin"
CONF_TX_PIN = "tx_pin"
CONF_EN_PIN = "en_pin"
CONF_NRST_PIN = "nrst_pin"
CONF_STATUS_SENSOR = "status_sensor"

fujitsu_ns = cg.esphome_ns.namespace("fujitsu")
FujitsuClimate = fujitsu_ns.class_("FujitsuClimate", climate.Climate, cg.Component)

# Action for attempt_login
AttemptLoginAction = fujitsu_ns.class_("AttemptLoginAction", automation.Action)

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

    # Register attempt_login action for use in automations/services
    automation.register_action(
        "fujitsu.attempt_login",
        AttemptLoginAction,
        cv.Schema({cv.GenerateID(): cv.use_id(FujitsuClimate)}),
    )(lambda config: cg.new_Pvariable(AttemptLoginAction, cg.cast(
        cg.get_variable(config[CONF_ID]), FujitsuClimate
    )))

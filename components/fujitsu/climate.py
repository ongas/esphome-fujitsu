import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ID
from esphome import pins

AUTO_LOAD = ["climate"]

CONF_RX_PIN = "rx_pin"
CONF_TX_PIN = "tx_pin"
CONF_EN_PIN = "en_pin"
CONF_NRST_PIN = "nrst_pin"
CONF_DEBUG = "debug"

fujitsu_ns = cg.esphome_ns.namespace("fujitsu")
FujitsuClimate = fujitsu_ns.class_("FujitsuClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = (
    climate.climate_schema(FujitsuClimate)
    .extend(cv.COMPONENT_SCHEMA)
    .extend(
        {
            cv.Optional(CONF_RX_PIN, default=3): cv.int_,
            cv.Optional(CONF_TX_PIN, default=1): cv.int_,
            cv.Optional(CONF_EN_PIN): cv.int_,
            cv.Optional(CONF_NRST_PIN): cv.int_,
            cv.Optional(CONF_DEBUG, default=False): cv.boolean,
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
    if CONF_DEBUG in config:
        cg.add(var.set_debug(config[CONF_DEBUG]))


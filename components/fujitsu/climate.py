import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, text_sensor, switch
from esphome.const import CONF_ID, CONF_NAME, CONF_ICON
from esphome import pins

AUTO_LOAD = ["climate", "text_sensor", "switch"]

CONF_RX_PIN = "rx_pin"
CONF_TX_PIN = "tx_pin"
CONF_EN_PIN = "en_pin"
CONF_NRST_PIN = "nrst_pin"
CONF_STATUS_SENSOR = "status_sensor"
CONF_ZONES = "zones"
CONF_ZONE_NUMBER = "zone_number"
CONF_ZONE_GROUPS = "zone_groups"
CONF_GROUP_TYPE = "group_type"

fujitsu_ns = cg.esphome_ns.namespace("fujitsu")
FujitsuClimate = fujitsu_ns.class_("FujitsuClimate", climate.Climate, cg.Component)
FujitsuZoneSwitch = fujitsu_ns.class_("FujitsuZoneSwitch", switch.Switch, cg.Component)
FujitsuZoneGroupSwitch = fujitsu_ns.class_(
    "FujitsuZoneGroupSwitch", switch.Switch, cg.Component
)
ZONE_GROUP_ENUM = {
    "DAY": cg.RawExpression("esphome::fujitsu::FujiZoneGroup::DAY"),
    "NIGHT": cg.RawExpression("esphome::fujitsu::FujiZoneGroup::NIGHT"),
    "ALL": cg.RawExpression("esphome::fujitsu::FujiZoneGroup::ALL"),
}

ZONE_SWITCH_SCHEMA = switch.switch_schema(FujitsuZoneSwitch, icon="mdi:view-grid").extend(
    cv.COMPONENT_SCHEMA
).extend(
    {
        cv.Required(CONF_ZONE_NUMBER): cv.int_range(min=1, max=8),
    }
)

ZONE_GROUP_SWITCH_SCHEMA = switch.switch_schema(
    FujitsuZoneGroupSwitch, icon="mdi:view-grid-plus"
).extend(cv.COMPONENT_SCHEMA).extend(
    {
        cv.Required(CONF_GROUP_TYPE): cv.enum(
            {"DAY": "DAY", "NIGHT": "NIGHT", "ALL": "ALL"}, upper=True
        ),
    }
)

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
            cv.Optional(CONF_ZONES): cv.ensure_list(ZONE_SWITCH_SCHEMA),
            cv.Optional(CONF_ZONE_GROUPS): cv.ensure_list(ZONE_GROUP_SWITCH_SCHEMA),
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

    if CONF_ZONES in config:
        for zone_conf in config[CONF_ZONES]:
            sw = await switch.new_switch(zone_conf)
            await cg.register_component(sw, zone_conf)
            cg.add(sw.set_climate(var))
            cg.add(sw.set_zone(zone_conf[CONF_ZONE_NUMBER] - 1))

    if CONF_ZONE_GROUPS in config:
        for group_conf in config[CONF_ZONE_GROUPS]:
            sw = await switch.new_switch(group_conf)
            await cg.register_component(sw, group_conf)
            cg.add(sw.set_climate(var))
            group_type = group_conf[CONF_GROUP_TYPE]
            cg.add(sw.set_group(ZONE_GROUP_ENUM[group_type]))

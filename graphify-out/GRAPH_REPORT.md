# Graph Report - /mnt/e/source/repos/personal/homeassistant/custom_components/fujitsu/esphome-fujitsu  (2026-05-14)

## Corpus Check
- Corpus is ~4,122 words - fits in a single context window. You may not need a graph.

## Summary
- 68 nodes · 89 edges · 11 communities (10 shown, 1 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 5 edges (avg confidence: 0.5)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Community 1|Community 1]]
- [[_COMMUNITY_Community 2|Community 2]]
- [[_COMMUNITY_Community 3|Community 3]]
- [[_COMMUNITY_Community 4|Community 4]]
- [[_COMMUNITY_Community 5|Community 5]]
- [[_COMMUNITY_Community 10|Community 10]]

## God Nodes (most connected - your core abstractions)
1. `setState()` - 9 edges
2. `waitForFrame()` - 5 edges
3. `encodeFrame()` - 4 edges
4. `updateState()` - 4 edges
5. `isBound()` - 3 edges
6. `attemptSecondaryLogin()` - 3 edges
7. `control()` - 3 edges
8. `decodeFrame()` - 2 edges
9. `printFrame()` - 2 edges
10. `sendPendingFrame()` - 2 edges

## Surprising Connections (you probably didn't know these)
- `README.md` --documents--> `FujiHeatPump.h`  [INFERRED]
   →   _Bridges community 4 → community 2_

## Hyperedges (group relationships)
- **Fujitsu AC Control System** —  [INFERRED]
- **ESPHome Integration Layer** —  [INFERRED]
- **LIN Protocol Implementation** —  [INFERRED]

## Communities (11 total, 1 thin omitted)

### Community 1 - "Community 1"
Cohesion: 0.0
Nodes (7): control(), espToFujiFanMode(), espToFujiMode(), fujiToEspFanMode(), fujiToEspMode(), loop(), updateState()

### Community 2 - "Community 2"
Cohesion: 0.0
Nodes (10): fujitsuacfirebeetle.yaml, climate.Climate, Component, text_sensor.TextSensor, FujitsuClimate.h, climate.py, __init__.py, FujitsuClimate (+2 more)

### Community 3 - "Community 3"
Cohesion: 0.0
Nodes (9): getCurrentState(), setEconomyMode(), setFanMode(), setMode(), setOnOff(), setState(), setSwingMode(), setSwingStep() (+1 more)

### Community 4 - "Community 4"
Cohesion: 0.0
Nodes (9): FujiAddress, FujitsuClimate.cpp, FujiFanMode, FujiFrame, FujiHeatPump, FujiHeatPump.h, FujiMessageType, FujiMode (+1 more)

### Community 5 - "Community 5"
Cohesion: 0.0
Nodes (7): attemptSecondaryLogin(), decodeFrame(), encodeFrame(), isBound(), printFrame(), sendPendingFrame(), waitForFrame()

## Knowledge Gaps
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.
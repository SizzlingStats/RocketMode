# RocketMode

## Build Status
| System | Compiler | Status |
| ------ | -------- | ------ |
| Ubuntu 20.04 | Clang-10 | [![Build status](https://ci.appveyor.com/api/projects/status/vij4rx4kfsfa73ai/branch/master?svg=true)](https://ci.appveyor.com/project/SizzlingCalamari/demboyz-linux/branch/master) |
| Windows | VS2022 | [![Build status](https://ci.appveyor.com/api/projects/status/pc63pbl9b0t5tygl/branch/master?svg=true)](https://ci.appveyor.com/project/SizzlingCalamari/demboyz/branch/master) |

## About
RocketMode By SizzlingCalamari

Created as an April Fools' day surprise for Oprah's Petrol Station TF2 Server ([petrol.tf](https://petrol.tf))

Video by Carthage (https://www.youtube.com/watch?v=zTmNzUM3mGY)

[![finally, I am the projectile](https://img.youtube.com/vi/zTmNzUM3mGY/0.jpg)](https://www.youtube.com/watch?v=zTmNzUM3mGY)

## Installation
```
srcds/
├── tf/
│   ├── addons/
│       ├── rocketmode/
│           ├── rocketmode.vdf
│           ├── rocketmode.dll (windows only)
│           └── rocketmode.so  (linux only)
```

## Cvars
```
"sizz_rocketmode_spawn_initialdelay", "15.0f", "Delay before spawning the first set of launchers after round start (seconds)."
"sizz_rocketmode_spawn_interval", "60.0f", "After initial spawns, periodically spawn launchers on this interval (seconds)."
"sizz_rocketmode_spawn_enabled", "1", "Enables periodic spawning of rocket mode rocket launchers (0/1)."
"sizz_rocketmode_spawn_command_enabled", "0", "Enables clients to spawn a launcher at their feet with 'spawnlauncher' (0/1)"
"sizz_rocketmode_damagemult", "1.5f", "Damage multiplier over base rocket damage [0.1f, 10.0f]."
"sizz_rocketmode_ammomult", "1.0f", "Reserve ammo multiplier over base rocket launcher [0.1f, 1.0f]."
"sizz_rocketmode_rocketspecialist", "1", "Enables the Rocket Specialist attribute on sizz launchers (0/1)."
"sizz_rocketmode_speedmult", "0.5f", "Speed multiplier over base rocket speed [0.1f, 10.0f]."
"sizz_rocketmode_pickup_lifetime", "60.0f", "tf_dropped_weapon_lifetime, but specific to sizz launchers (seconds)."
```

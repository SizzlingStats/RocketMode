# RocketMode

## Build Status
| System | Compiler | Status |
| ------ | -------- | ------ |
| Ubuntu 20.04 | Clang-10 | [![Build status](https://ci.appveyor.com/api/projects/status/wu09gbyll5t0oabf?svg=true)](https://ci.appveyor.com/project/SizzlingCalamari/rocketmode-linux) |
| Windows | VS2022 | [![Build status](https://ci.appveyor.com/api/projects/status/5mda31hkqmpb3aos?svg=true)](https://ci.appveyor.com/project/SizzlingCalamari/rocketmode) |

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

## License
- - -

MIT License

Copyright (c) 2023 SizzlingStats

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

- - -
All individuals associated with SizzlingStats do not claim ownership of 
the contents of the directory "/external",
which does not fall under the previous license.

RocketMode includes code Copyright (c) Valve Corporation and is licensed separately.

Source and Team Fortress are trademarks and/or registered trademarks of Valve Corporation.

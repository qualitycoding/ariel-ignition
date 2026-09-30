# Environment (pinned) and verified setup log

Verified in the planning sandbox (Ubuntu 24.04, 2026-09-30):

| Tool | Version | Use |
|---|---|---|
| gcc | 13.3.0 | host unit tests, harness |
| avr-gcc | 7.3.0 (gcc-avr 1:7.3.0+Atmel3.7.0-1) | firmware |
| avr-libc | 1:2.0.0+Atmel3.7.0-1 | firmware headers (fuse bit positions verified, C-024) |
| binutils-avr | 2.26.20160125+Atmel3.7.0-2 | avr-size/avr-nm/objcopy |
| simavr / libsimavr-dev | 1.6+dfsg-3build2 | firmware-in-the-loop tests |
| libelf | Ubuntu 24.04 default | simavr ELF loading |
| avrdude | 7.1+dfsg-3build2 | flashing (USBasp) |
| Unity | v2.6.0, vendored in vendor/unity (hashes below, identical to upstream tag, C-032) | test framework |
| python3 | 3.12 | spikes, diagram generator |
| arduino-cli | latest 1.x (not installed at planning time) | S-019 only |
| KiCad | 8.x (optional, not installed at planning time) | S-009 only |

Setup on a fresh Ubuntu 24.04:
```
sudo apt-get install -y build-essential gcc-avr avr-libc binutils-avr avrdude simavr libsimavr-dev libelf-dev python3
make test          # expect 31 failures + T-301/T-302 failures before implementation
```
Vendored file hashes (sha256):
```
865b12a0747d1e8973612a04ea978767aa8fa8aab1466c662807b54bfe517d2c  vendor/unity/LICENSE.txt
c00c9012f3a170d99133cc8a80036bc3c52e0782d5a286f93b31bb1106dce65a  vendor/unity/meson.build
ee69607f977cf2653762cb3a84da8a122a0a5bbe6c3308ca1f59e443b1890cf5  vendor/unity/unity.c
e20bed56a8172986ff9f65933351b8b9df62996b77045ed8787d631223fe6eb3  vendor/unity/unity.h
c4220e36bc7c66f81a0f0b026e694865f3f33e6c8b48113dedd8db1d50ce0716  vendor/unity/unity_internals.h
```
Dependency scan (security): the firmware links only avr-libc; host tests link
Unity (vendored, hash-pinned) and libsimavr/libelf from the OS. No network
access at runtime; the only external interface is the physical serial port (TM-1).

#!/usr/bin/env python
"""
Force SCons to use response files (@file.rsp) for the g++ compile command.

Why: Arduino-ESP32 S3 build's g++ command line is ~32K chars (244 -I paths
from S3 SDK), hitting Win32 CreateProcess's 32K limit. Symptoms:
"xtensa-esp32s3-elf-g++: error: CreateProcess: No such file or directory".

What this does:
- Monkey-patches the Compile / C++ builders in every env
- For each compile invocation, writes the long argument list to a .rsp file
- Replaces the g++ command with `g++ @path/to/file.rsp`
- Result: command line is ~150 chars regardless of how many -I/-D flags
- Win32 limit no longer hit; identical flags passed through verbatim

Drop-in: add to platformio.ini under [env:tinylora_mv_game]:
    extra_scripts = pre:scripts/force_rspfile.py
"""

Import("env")

import os
import sys

# Response file magic — SCons recognizes this and auto-handles @file.rsp
RSPFILE_SYNTAX = ["@$TARGET.rsp"]


def _patch_env(e):
    """Enable rspfile for the g++ driver in this env."""
    # Compiler and C++ compiler (for C++ files like BuzzerFeedbackThread.cpp)
    for tool in ("CC", "CXX", "AS"):
        if tool not in e:
            continue
        # The g++ / gcc binary
        e[f"{tool}_RSPFILE"] = "${TARGET.rsp}"
        e[f"{tool}_RSPFILE_SYNTAX"] = RSPFILE_SYNTAX
    # Also link-time — same 32K limit applies, will help -Wl flags too
    for tool in ("LINK", "AR"):
        if tool not in e:
            continue
        e[f"{tool}_RSPFILE"] = "${TARGET.rsp}"
        e[f"{tool}_RSPFILE_SYNTAX"] = RSPFILE_SYNTAX


# Patch all envs PIO creates
_patch_env(env)
# Some PIO builders create sub-envs on the fly (e.g. SharedLib); they inherit
# from `env` so the *_RSPFILE settings should propagate. If you see
# "rspfile not honored" errors, also patch DefaultEnvironment().
try:
    _patch_env(DefaultEnvironment())
except Exception:
    pass

# Sanity log so we can confirm in pio run -v that this loaded
print("[force_rspfile] g++/gcc/linker rspfile ENABLED (Win32 32K cmdline workaround)")

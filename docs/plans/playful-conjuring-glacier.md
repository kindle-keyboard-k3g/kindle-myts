# Implementation Plan: Idempotent Kindle Update & Deployment Scripts (Linux / WSL)

## 1. Context

Updating or installing `kindle-myts` (with modern `myts-ng` as default) on a Kindle device currently requires manual multi-step operations (`scp`, `chmod`, editing `/mnt/us/launchpad/myts.ini`, managing running processes to avoid `ETXTBSY` locks, and restoring framework daemons).

When users attempt to deploy without proper orchestration:
1. Active `myts` processes lock the executable file on the Kindle filesystem, causing `scp: dest open "/mnt/us/myts/myts": Failure` (`ETXTBSY`).
2. Custom user configurations (e.g. keybindings, margins, custom fonts in `myts.ini`) can be accidentally overwritten.
3. Launchpad configurations might point to stale binaries or fail to reload.
4. Users on WSL or native Linux may connect via USBNetwork/SSH or USB Mass Storage (drive mount), requiring different transfer mechanisms.

The objective is to provide an idempotent, robust, and user-friendly deployment tool (`scripts/update-kindle.sh` and `make deploy` / `make update`) that works seamlessly on any Linux or WSL environment.

---

## 2. Core Requirements & Idempotence Guarantees

1. **Dual-Transport Support**:
   - **Mode A (SSH / USBNetwork / Wi-Fi)**: Automated connection probing across standard Kindle targets (`kindle` SSH alias, `192.168.2.2`, `192.168.15.2`, or user-specified `--host`).
   - **Mode B (USB Mass Storage / Drive Mount)**: Automatic detection of mounted Kindle partitions under Linux (`/media/$USER/*`, `/run/media/$USER/*`, `/mnt/*`) and WSL (`/mnt/d`, `/mnt/e`, etc., scanning for Kindle folder signatures: `documents`, `system`, or `launchpad`), or user-specified `--usb-path`.

2. **Idempotence & Safety Guarantees**:
   - **Safe Process Termination**: Terminates any active `myts`, `myts-ng`, or `matrix` instances on the device prior to copying, eliminating `ETXTBSY` file-busy errors.
   - **Legacy Binary Preservation**: Backs up the original 2010 C binary to `/mnt/us/myts/myts-legacy` only if `myts-legacy` does not already exist (never clobbers existing backups).
   - **User Configuration Protection**: Preserves existing `/mnt/us/myts/myts.ini` by default to avoid losing custom keybindings. Provides `--reset-config` for clean installs.
   - **Atomic/Clean File Transfer**: Deploys `tools/myts` as `/mnt/us/myts/myts`, `myts-ng-kindle` as `/mnt/us/myts/myts-ng`, `tools/launch_kindle.sh`, fonts (`ter-u12n.hex`), `keydefs.ini`, and optionally `matrix`.
   - **Launchpad Registration**: Configures `/mnt/us/launchpad/myts.ini` with correct hotkey bindings (`Shift + T, T`) and triggers a non-disruptive reload (`killall -HUP launchpad`).
   - **Permission Enforcement**: Sets executable bits (`chmod +x`) on all binaries and scripts.
   - **Post-Deploy Sanity Verification**: Over SSH, automatically executes `/mnt/us/myts/myts --dry-run` to verify startup, display initialization, and framework restoration.

3. **Artifact Resolution (No Hard Build Dependencies for End Users)**:
   - Priority 1: Use existing cross-compiled `myts-ng-kindle` if present in repository root.
   - Priority 2: Extract prebuilt binaries from release archive `myts.zip` if available.
   - Priority 3: Cross-compile on demand using `armv6-linux-musleabi-g++` if the toolchain is installed.
   - Fallback: Clearly report if no precompiled ARM binary is found and guide the user on obtaining `myts.zip`.

---

## 3. Proposed Architecture & Critical Files

### 3.1 New Script: `scripts/update-kindle.sh`
- Pure POSIX-compliant shell script with bash compatibility.
- CLI flags:
  - `--ssh [HOST]`: Force SSH mode (default auto-probes `kindle`, `192.168.2.2`, `192.168.15.2`).
  - `--usb [PATH]`: Force USB Mass Storage mode (auto-detects Kindle mount point if omitted).
  - `--port [PORT]`: Custom SSH port (default: 22).
  - `--reset-config`: Overwrite existing `myts.ini` on Kindle with repository defaults.
  - `--build`: Force recompilation of ARM binaries before deploying.
  - `--no-verify`: Skip post-deploy dry-run verification.
  - `--help, -h`: Usage documentation.

### 3.2 Symlink / Convenience Entry: `tools/update-kindle.sh`
- Symlink to `scripts/update-kindle.sh` for developer discovery inside `tools/`.

### 3.3 Integration with `Makefile`
- Add phony targets:
  - `make deploy`: Builds prerequisites (if toolchain available) and invokes `./scripts/update-kindle.sh`.
  - `make update`: Alias for `make deploy`.

### 3.4 Documentation Update
- Update `README.md` and `docs/build-and-test.md` with simple one-liner instructions for updating the Kindle from Linux or WSL.

---

## 4. Detailed Implementation Flow for `scripts/update-kindle.sh`

### Step 1: Pre-flight & Environment Detection
- Detect WSL environment (`grep -qi microsoft /proc/version 2>/dev/null`).
- Parse CLI arguments and validate inputs immediately (Fail Fast).

### Step 2: Binary Resolution
- Locate `myts-ng-kindle`, `tools/matrix-kindle` (or `tools/matrix`), `tools/myts`, `tools/launch_kindle.sh`, `myts.l.ini`, `ter-u12n.hex`, `keydefs.ini`, `myts.ini`.
- If `myts-ng-kindle` is missing:
  - Check if `myts.zip` exists and unpack `myts/myts-ng` and `myts/matrix`.
  - If not, check if `armv6-linux-musleabi-g++` is in `PATH` and run `make myts-ng-kindle tools/matrix-kindle`.
  - If impossible, exit with error message explaining where to get `myts.zip`.

### Step 3: Target Connection Discovery
- If mode is unspecified, auto-detect:
  1. Try probing SSH (`ssh -o BatchMode=yes -o ConnectTimeout=2 kindle true` or `192.168.2.2` or `192.168.15.2`).
  2. If SSH responds, select SSH mode.
  3. If SSH fails, scan for mounted Kindle storage:
     - On Linux: `/media/$USER/*`, `/run/media/$USER/*`, `/mnt/*`.
     - On WSL: `/mnt/[a-z]`.
     - Check for presence of `documents/` or `launchpad/` or `system/`.
     - If found, select USB Mass Storage mode.
  4. If neither is found, exit with diagnostic guidance showing how to enable USBNetwork or mount the Kindle drive.

### Step 4: Execution - SSH Mode
1. **Kill Active Processes**:
   `ssh $KINDLE_SSH "killall -9 myts myts-ng matrix 2>/dev/null || true"`
2. **Ensure Directories**:
   `ssh $KINDLE_SSH "mkdir -p /mnt/us/myts /mnt/us/launchpad"`
3. **Backup Legacy Binary (Idempotent)**:
   `ssh $KINDLE_SSH "[ -f /mnt/us/myts/myts ] && [ ! -f /mnt/us/myts/myts-legacy ] && ! grep -q '#!/bin/sh' /mnt/us/myts/myts && cp /mnt/us/myts/myts /mnt/us/myts/myts-legacy || true"`
4. **Transfer Files via SCP**:
   - Transfer `tools/myts` -> `/mnt/us/myts/myts`
   - Transfer `myts-ng-kindle` -> `/mnt/us/myts/myts-ng` and `/mnt/us/myts/myts-ng-kindle`
   - Transfer `tools/launch_kindle.sh` -> `/mnt/us/myts/launch_kindle.sh`
   - Transfer `myts.l.ini` -> `/mnt/us/launchpad/myts.ini`
   - Transfer fonts (`ter-u12n.hex`) and `keydefs.ini` -> `/mnt/us/myts/`
   - If `matrix-kindle` exists: transfer to `/mnt/us/myts/matrix`
   - If `--reset-config` OR `/mnt/us/myts/myts.ini` does not exist: transfer `myts.ini` -> `/mnt/us/myts/myts.ini`
5. **Set Permissions**:
   `ssh $KINDLE_SSH "chmod +x /mnt/us/myts/myts /mnt/us/myts/myts-ng /mnt/us/myts/launch_kindle.sh /mnt/us/myts/matrix 2>/dev/null || true"`
6. **Reload Launchpad**:
   `ssh $KINDLE_SSH "killall -HUP launchpad 2>/dev/null || true"`
7. **Verify**:
   Execute `/mnt/us/myts/myts --dry-run` and verify exit code 0.

### Step 5: Execution - USB Mass Storage Mode
1. Validate target directory: `$KINDLE_USB_PATH/myts` and `$KINDLE_USB_PATH/launchpad`.
2. Backup legacy binary if present.
3. Copy all files into `$KINDLE_USB_PATH/myts/` and `$KINDLE_USB_PATH/launchpad/myts.ini`.
4. Flush buffers (`sync`).
5. Output clear instructions: safely eject the Kindle, unplug USB cable, and press `Shift Shift Space` on Kindle keyboard to reload Launchpad.

---

## 5. Verification Plan

1. **Automated Unit & Shell Checks**:
   - Syntax validation: `bash -n scripts/update-kindle.sh`
   - Test help and argument validation: `./scripts/update-kindle.sh --help`
   - Test non-existent path errors: `./scripts/update-kindle.sh --usb /tmp/nonexistent`
2. **Real-Device SSH Verification**:
   - Run `./scripts/update-kindle.sh --ssh kindle` against connected Kindle.
   - Verify idempotence by running it 3 times consecutively; ensure zero failures, no file corruption, and dry-run tests pass every time.
3. **USB Mass Storage Mode Verification**:
   - Create mock Kindle directory structure in temporary folder (`mkdir -p /tmp/mock-kindle/{documents,launchpad,myts}`).
   - Run `./scripts/update-kindle.sh --usb /tmp/mock-kindle`.
   - Verify all required files, permissions, and launchpad configs are deployed correctly without errors.
4. **Makefile Target Verification**:
   - Run `make deploy` and verify seamless orchestration.

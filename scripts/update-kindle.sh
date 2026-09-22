#!/bin/sh
# ==============================================================================
# update-kindle.sh
# Idempotent installer and updater for kindle-myts (myts-ng) on Kindle devices.
# Supports Linux and Windows Subsystem for Linux (WSL).
#
# Transports supported:
#   1. SSH / USBNetwork / Wi-Fi (recommended: live process handling & verification)
#   2. USB Mass Storage (drive mount: /media/..., /mnt/d, etc.)
# ==============================================================================

set -e

# ANSI Color Codes
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    COLOR_RESET="\033[0m"
    COLOR_BOLD="\033[1m"
    COLOR_GREEN="\033[32m"
    COLOR_YELLOW="\033[33m"
    COLOR_RED="\033[31m"
    COLOR_CYAN="\033[36m"
else
    COLOR_RESET=""
    COLOR_BOLD=""
    COLOR_GREEN=""
    COLOR_YELLOW=""
    COLOR_RED=""
    COLOR_CYAN=""
fi

log_info() {
    printf "${COLOR_CYAN}[INFO]${COLOR_RESET} %s\n" "$*"
}

log_success() {
    printf "${COLOR_GREEN}[SUCCESS]${COLOR_RESET} %s\n" "$*"
}

log_warn() {
    printf "${COLOR_YELLOW}[WARN]${COLOR_RESET} %s\n" "$*"
}

log_error() {
    printf "${COLOR_RED}[ERROR]${COLOR_RESET} %s\n" "$*" >&2
}

print_banner() {
    printf "${COLOR_BOLD}======================================================${COLOR_RESET}\n"
    printf "${COLOR_BOLD}     kindle-myts (myts-ng) - Kindle Update Tool      ${COLOR_RESET}\n"
    printf "${COLOR_BOLD}======================================================${COLOR_RESET}\n\n"
}

usage() {
    cat <<EOF
Usage: $0 [OPTIONS]

Idempotently deploys or updates kindle-myts on an Amazon Kindle Keyboard or DX.
Compatible with Linux and WSL.

Options:
  --ssh [HOST]         Use SSH deployment mode. Optionally specify target host/IP.
                       If HOST is omitted, auto-probes: 'kindle', '192.168.2.2', '192.168.15.2'.
  --usb [PATH]         Use USB Mass Storage mode. Optionally specify mount path.
                       If PATH is omitted, auto-scans /media, /run/media, or WSL drive letters.
  --port PORT          Custom SSH port (default: 22).
  --reset-config       Overwrite /mnt/us/myts/myts.ini with default config.
                       (By default, existing custom configurations are preserved).
  --build              Force cross-compilation of ARMv6 binaries before deployment.
  --no-verify          Skip post-deployment dry-run verification over SSH.
  -h, --help           Display this help message and exit.

Environment Variables:
  KINDLE_HOST          Default SSH host or IP (fallback when --ssh has no argument).
  KINDLE_PORT          Default SSH port (default: 22).
  KINDLE_USB_PATH      Default mount directory for USB Mass Storage mode.

Examples:
  $0                   # Autodetect connection (SSH or mounted USB drive) and deploy
  $0 --ssh kindle      # Deploy over SSH using the 'kindle' SSH config alias
  $0 --ssh 192.168.2.2 # Deploy over SSH directly to USBNetwork IP
  $0 --usb /mnt/e      # Deploy to Kindle mounted on Windows drive E: via WSL
  $0 --reset-config    # Deploy and reset myts.ini to repository defaults
EOF
}

# Resolve script repository root directory
SCRIPT_PATH="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_PATH/.." && pwd)"

# Parse options
MODE="auto"
SSH_TARGET=""
SSH_PORT="${KINDLE_PORT:-22}"
USB_MOUNT_PATH="${KINDLE_USB_PATH:-}"
RESET_CONFIG=0
FORCE_BUILD=0
VERIFY=1

while [ $# -gt 0 ]; do
    case "$1" in
        --ssh)
            MODE="ssh"
            if [ $# -gt 1 ] && [ "${2#-}" = "$2" ]; then
                SSH_TARGET="$2"
                shift
            fi
            ;;
        --usb)
            MODE="usb"
            if [ $# -gt 1 ] && [ "${2#-}" = "$2" ]; then
                USB_MOUNT_PATH="$2"
                shift
            fi
            ;;
        --port)
            if [ $# -lt 2 ]; then
                log_error "Option --port requires an argument."
                exit 1
            fi
            SSH_PORT="$2"
            shift
            ;;
        --reset-config)
            RESET_CONFIG=1
            ;;
        --build)
            FORCE_BUILD=1
            ;;
        --no-verify)
            VERIFY=0
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
    shift
done

is_wsl() {
    [ -f /proc/version ] && grep -qi microsoft /proc/version
}

# ------------------------------------------------------------------------------
# 1. Resolve & Verify Binaries and Assets
# ------------------------------------------------------------------------------
resolve_binaries() {
    log_info "Resolving deployment artifacts in $REPO_DIR..."

    MYTS_NG_BIN="$REPO_DIR/myts-ng-kindle"
    MATRIX_BIN="$REPO_DIR/tools/matrix-kindle"

    # Cross-compile if requested or if myts-ng-kindle is absent
    if [ "$FORCE_BUILD" -eq 1 ] || [ ! -f "$MYTS_NG_BIN" ]; then
        TOOLCHAIN_CXX="$HOME/.local/toolchains/armv6-linux-musleabi-cross/bin/armv6-linux-musleabi-g++"
        if [ -x "$TOOLCHAIN_CXX" ] || command -v armv6-linux-musleabi-g++ >/dev/null 2>&1; then
            log_info "Building ARMv6 Kindle binaries..."
            make -C "$REPO_DIR" myts-ng-kindle tools/matrix-kindle
        elif [ -f "$REPO_DIR/myts.zip" ]; then
            log_info "Extracting precompiled Kindle binaries from myts.zip..."
            unzip -p "$REPO_DIR/myts.zip" myts/myts-ng > "$MYTS_NG_BIN"
            chmod +x "$MYTS_NG_BIN"
            unzip -p "$REPO_DIR/myts.zip" myts/matrix > "$MATRIX_BIN" 2>/dev/null || true
            if [ -f "$MATRIX_BIN" ]; then chmod +x "$MATRIX_BIN"; fi
        else
            log_error "Missing ARMv6 binary ($MYTS_NG_BIN) and no cross-compiler or myts.zip found!"
            log_error "Run 'make myts.zip' or install the armv6-linux-musleabi toolchain."
            exit 1
        fi
    fi

    # Check for required assets
    for required_file in \
        "$REPO_DIR/tools/myts" \
        "$REPO_DIR/tools/myts.sh" \
        "$REPO_DIR/tools/launch_kindle.sh" \
        "$REPO_DIR/myts.l.ini" \
        "$REPO_DIR/myts.ini" \
        "$REPO_DIR/keydefs.ini" \
        "$REPO_DIR/ter-u12n.hex"
    do
        if [ ! -f "$required_file" ]; then
            log_error "Required file missing: $required_file"
            exit 1
        fi
    done

    log_success "All required deployment artifacts verified."
}

# ------------------------------------------------------------------------------
# 2. Connection Probing (SSH and USB Mass Storage)
# ------------------------------------------------------------------------------
probe_ssh_target() {
    target="$1"
    port="$2"
    # Test SSH connection with a quick 2-second timeout
    if ssh -o BatchMode=yes -o ConnectTimeout=2 -p "$port" "$target" "true" >/dev/null 2>&1; then
        return 0
    fi
    return 1
}

find_ssh_target() {
    if [ -n "$SSH_TARGET" ]; then
        if probe_ssh_target "$SSH_TARGET" "$SSH_PORT"; then
            echo "$SSH_TARGET"
            return 0
        else
            log_warn "Specified SSH target '$SSH_TARGET' is not reachable on port $SSH_PORT."
            return 1
        fi
    fi

    # If KINDLE_HOST is set, try that first
    if [ -n "${KINDLE_HOST:-}" ]; then
        if probe_ssh_target "$KINDLE_HOST" "$SSH_PORT"; then
            echo "$KINDLE_HOST"
            return 0
        fi
    fi

    # Probe standard Kindle hosts
    for candidate in "kindle" "root@192.168.2.2" "root@192.168.15.2"; do
        if probe_ssh_target "$candidate" "$SSH_PORT"; then
            echo "$candidate"
            return 0
        fi
    done

    return 1
}

find_usb_storage() {
    if [ -n "$USB_MOUNT_PATH" ]; then
        if [ -d "$USB_MOUNT_PATH" ] && { [ -d "$USB_MOUNT_PATH/documents" ] || [ -d "$USB_MOUNT_PATH/system" ] || [ -d "$USB_MOUNT_PATH/launchpad" ]; }; then
            echo "$USB_MOUNT_PATH"
            return 0
        else
            log_warn "Specified USB path '$USB_MOUNT_PATH' does not appear to be a Kindle drive."
            return 1
        fi
    fi

    CANDIDATES=""

    # Linux standard mount paths
    if [ -n "${USER:-}" ]; then
        CANDIDATES="$CANDIDATES /media/$USER/* /run/media/$USER/*"
    fi
    CANDIDATES="$CANDIDATES /media/* /mnt/*"

    # WSL drive letters
    if is_wsl; then
        for drive in d e f g h i j k l m n o p q r s t u v w x y z; do
            if [ -d "/mnt/$drive" ]; then
                CANDIDATES="$CANDIDATES /mnt/$drive"
            fi
        done
    fi

    for candidate in $CANDIDATES; do
        if [ -d "$candidate" ] && { [ -d "$candidate/documents" ] || [ -d "$candidate/system" ] || [ -d "$candidate/launchpad" ]; }; then
            echo "$candidate"
            return 0
        fi
    done

    return 1
}

# ------------------------------------------------------------------------------
# 3. SSH Deployment Flow
# ------------------------------------------------------------------------------
deploy_ssh() {
    target="$1"
    port="$2"
    log_info "Deploying to Kindle via SSH target: ${COLOR_BOLD}$target${COLOR_RESET} (port $port)..."

    # Step A: Stop active myts sessions to avoid ETXTBSY ("Text file busy")
    log_info "Stopping any active myts processes on Kindle..."
    ssh -p "$port" "$target" "killall -9 myts myts-ng matrix 2>/dev/null || true"
    ssh -p "$port" "$target" "rm -f /var/tmp/myts.special 2>/dev/null || true"

    # Step B: Create directories
    log_info "Ensuring target directories exist (/mnt/us/myts, /mnt/us/launchpad)..."
    ssh -p "$port" "$target" "mkdir -p /mnt/us/myts /mnt/us/launchpad"

    # Step C: Idempotent backup of original legacy binary
    log_info "Checking legacy binary backup status..."
    ssh -p "$port" "$target" "if [ -f /mnt/us/myts/myts ] && [ ! -f /mnt/us/myts/myts-legacy ] && ! grep -q '#!/bin/sh' /mnt/us/myts/myts 2>/dev/null; then mv /mnt/us/myts/myts /mnt/us/myts/myts-legacy; fi"

    # Step D: Transfer core executables and scripts
    log_info "Copying modern wrapper and native binaries..."
    scp -P "$port" "$REPO_DIR/tools/myts" "$target:/mnt/us/myts/myts"
    scp -P "$port" "$REPO_DIR/tools/myts.sh" "$target:/mnt/us/myts/myts.sh"
    scp -P "$port" "$MYTS_NG_BIN" "$target:/mnt/us/myts/myts-ng"
    scp -P "$port" "$MYTS_NG_BIN" "$target:/mnt/us/myts/myts-ng-kindle"
    scp -P "$port" "$REPO_DIR/tools/launch_kindle.sh" "$target:/mnt/us/myts/launch_kindle.sh"

    # Step E: Transfer Launchpad configuration
    log_info "Updating Launchpad configuration (/mnt/us/launchpad/myts.ini)..."
    scp -P "$port" "$REPO_DIR/myts.l.ini" "$target:/mnt/us/launchpad/myts.ini"
    scp -P "$port" "$REPO_DIR/myts.l.ini" "$target:/mnt/us/launchpad/myts.l.ini"
    scp -P "$port" "$REPO_DIR/myts.l.ini" "$target:/mnt/us/myts/myts.l.ini"

    # Step F: Transfer assets and fonts
    log_info "Copying font definitions and keymaps..."
    scp -P "$port" "$REPO_DIR/ter-u12n.hex" "$REPO_DIR/keydefs.ini" "$target:/mnt/us/myts/"

    # Transfer matrix binary if available
    if [ -f "$MATRIX_BIN" ]; then
        log_info "Copying matrix animation binary..."
        scp -P "$port" "$MATRIX_BIN" "$target:/mnt/us/myts/matrix"
        scp -P "$port" "$MATRIX_BIN" "$target:/mnt/us/myts/matrix-kindle"
    fi
    if [ -f "$REPO_DIR/tools/matrix-anim.sh" ]; then
        scp -P "$port" "$REPO_DIR/tools/matrix-anim.sh" "$target:/mnt/us/myts/"
    fi

    # Step G: Handle configuration file (myts.ini)
    if [ "$RESET_CONFIG" -eq 1 ]; then
        log_warn "Overwriting /mnt/us/myts/myts.ini with default configuration (--reset-config specified)..."
        scp -P "$port" "$REPO_DIR/myts.ini" "$target:/mnt/us/myts/myts.ini"
    else
        # Only copy myts.ini if it does not already exist on the Kindle
        if ssh -p "$port" "$target" "[ ! -f /mnt/us/myts/myts.ini ]"; then
            log_info "Installing initial default /mnt/us/myts/myts.ini..."
            scp -P "$port" "$REPO_DIR/myts.ini" "$target:/mnt/us/myts/myts.ini"
        else
            log_info "Preserving existing /mnt/us/myts/myts.ini (custom user settings intact)."
        fi
    fi

    # Step H: Set execution permissions
    log_info "Setting executable permissions..."
    ssh -p "$port" "$target" "chmod +x /mnt/us/myts/myts /mnt/us/myts/myts.sh /mnt/us/myts/myts-ng /mnt/us/myts/myts-ng-kindle /mnt/us/myts/launch_kindle.sh /mnt/us/myts/matrix 2>/dev/null || true"

    # Step I: Reload Launchpad
    log_info "Reloading Kindle Launchpad daemon..."
    ssh -p "$port" "$target" "killall -HUP launchpad 2>/dev/null || true"

    # Step J: Post-deployment verification
    if [ "$VERIFY" -eq 1 ]; then
        log_info "Performing post-deploy sanity check (/mnt/us/myts/myts --dry-run)..."
        if ssh -p "$port" "$target" "/mnt/us/myts/myts --dry-run"; then
            log_success "Sanity check passed: myts-ng started and restored daemons cleanly!"
        else
            log_warn "Sanity check returned non-zero exit code. Please check device logs."
        fi
    fi

    log_success "Deployment completed successfully via SSH!"
    printf "\n${COLOR_BOLD}Next Steps:${COLOR_RESET}\n"
    printf "  On your Kindle Keyboard, press: ${COLOR_CYAN}Shift + T, then T${COLOR_RESET} to start myts-ng.\n\n"
}

# ------------------------------------------------------------------------------
# 4. USB Mass Storage Deployment Flow
# ------------------------------------------------------------------------------
deploy_usb() {
    mount_path="$1"
    log_info "Deploying to Kindle via USB drive mount: ${COLOR_BOLD}$mount_path${COLOR_RESET}..."

    MYTS_DIR="$mount_path/myts"
    LAUNCHPAD_DIR="$mount_path/launchpad"

    mkdir -p "$MYTS_DIR" "$LAUNCHPAD_DIR"

    # Step A: Backup legacy binary if present and unbacked
    if [ -f "$MYTS_DIR/myts" ] && [ ! -f "$MYTS_DIR/myts-legacy" ]; then
        if ! grep -q '#!/bin/sh' "$MYTS_DIR/myts" 2>/dev/null; then
            log_info "Backing up legacy myts binary to $MYTS_DIR/myts-legacy..."
            cp -p "$MYTS_DIR/myts" "$MYTS_DIR/myts-legacy"
        fi
    fi

    # Step B: Copy wrapper and native binaries
    log_info "Copying modern wrapper and binaries to $MYTS_DIR/..."
    cp -p "$REPO_DIR/tools/myts" "$MYTS_DIR/myts"
    cp -p "$REPO_DIR/tools/myts.sh" "$MYTS_DIR/myts.sh"
    cp -p "$MYTS_NG_BIN" "$MYTS_DIR/myts-ng"
    cp -p "$MYTS_NG_BIN" "$MYTS_DIR/myts-ng-kindle"
    cp -p "$REPO_DIR/tools/launch_kindle.sh" "$MYTS_DIR/launch_kindle.sh"

    # Step C: Copy Launchpad configuration
    log_info "Updating Launchpad configuration ($LAUNCHPAD_DIR/myts.ini)..."
    cp -p "$REPO_DIR/myts.l.ini" "$LAUNCHPAD_DIR/myts.ini"
    cp -p "$REPO_DIR/myts.l.ini" "$LAUNCHPAD_DIR/myts.l.ini"
    cp -p "$REPO_DIR/myts.l.ini" "$MYTS_DIR/myts.l.ini"

    # Step D: Copy assets and fonts
    log_info "Copying fonts and keymaps..."
    cp -p "$REPO_DIR/ter-u12n.hex" "$MYTS_DIR/"
    cp -p "$REPO_DIR/keydefs.ini" "$MYTS_DIR/"

    if [ -f "$MATRIX_BIN" ]; then
        log_info "Copying matrix animation binary..."
        cp -p "$MATRIX_BIN" "$MYTS_DIR/matrix"
        cp -p "$MATRIX_BIN" "$MYTS_DIR/matrix-kindle"
    fi
    if [ -f "$REPO_DIR/tools/matrix-anim.sh" ]; then
        cp -p "$REPO_DIR/tools/matrix-anim.sh" "$MYTS_DIR/"
    fi

    # Step E: Handle myts.ini
    if [ "$RESET_CONFIG" -eq 1 ] || [ ! -f "$MYTS_DIR/myts.ini" ]; then
        log_info "Writing default $MYTS_DIR/myts.ini..."
        cp -p "$REPO_DIR/myts.ini" "$MYTS_DIR/myts.ini"
    else
        log_info "Preserving existing $MYTS_DIR/myts.ini."
    fi

    # Step F: Flush buffers to device
    log_info "Flushing filesystem buffers to USB drive (sync)..."
    sync

    log_success "Files successfully copied to USB mount $mount_path!"
    printf "\n${COLOR_BOLD}Next Steps:${COLOR_RESET}\n"
    printf "  1. Safely eject/unmount the Kindle USB drive from your PC.\n"
    printf "  2. Unplug the USB cable from the Kindle.\n"
    printf "  3. On the Kindle physical keyboard, press: ${COLOR_CYAN}Shift Shift Space${COLOR_RESET} to reload Launchpad.\n"
    printf "  4. Press: ${COLOR_CYAN}Shift + T, then T${COLOR_RESET} to start myts-ng.\n\n"
}

# ------------------------------------------------------------------------------
# Main Dispatcher
# ------------------------------------------------------------------------------
print_banner
resolve_binaries

if [ "$MODE" = "ssh" ]; then
    RESOLVED_TARGET="$(find_ssh_target || true)"
    if [ -z "$RESOLVED_TARGET" ]; then
        log_error "Could not establish SSH connection to Kindle."
        log_error "Make sure USBNetwork is active on Kindle and cable is connected."
        log_error "Try running: $0 --ssh root@192.168.2.2 or check your ~/.ssh/config."
        exit 1
    fi
    deploy_ssh "$RESOLVED_TARGET" "$SSH_PORT"
elif [ "$MODE" = "usb" ]; then
    RESOLVED_PATH="$(find_usb_storage || true)"
    if [ -z "$RESOLVED_PATH" ]; then
        log_error "Kindle USB storage drive could not be detected."
        log_error "Please connect the Kindle via USB and ensure it is mounted as a drive."
        log_error "Or specify the mount path explicitly: $0 --usb /path/to/kindle"
        exit 1
    fi
    deploy_usb "$RESOLVED_PATH"
else
    # Auto-detection mode: try SSH first, then USB Mass Storage
    log_info "Scanning for Kindle connection (checking SSH / USBNetwork)..."
    RESOLVED_TARGET="$(find_ssh_target || true)"
    if [ -n "$RESOLVED_TARGET" ]; then
        deploy_ssh "$RESOLVED_TARGET" "$SSH_PORT"
    else
        log_info "SSH target not responding. Checking for mounted Kindle USB drive..."
        RESOLVED_PATH="$(find_usb_storage || true)"
        if [ -n "$RESOLVED_PATH" ]; then
            deploy_usb "$RESOLVED_PATH"
        else
            log_error "Neither an active SSH connection nor a mounted Kindle USB drive was found."
            log_error "How to connect:"
            log_error "  Option 1 (SSH / USBNetwork): Turn on USBNetwork, connect cable, and run: $0 --ssh kindle"
            log_error "  Option 2 (USB Drive): Plug in Kindle as USB drive, mount it, and run: $0 --usb /path/to/mount"
            if is_wsl; then
                log_warn "Running in WSL: If Kindle is drive E: in Windows, run: $0 --usb /mnt/e"
            fi
            exit 1
        fi
    fi
fi

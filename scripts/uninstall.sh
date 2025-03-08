#!/bin/bash

# uninstall.sh - Remove Nutshell installation
# Usage: ./scripts/uninstall.sh [--user]

USER_UNINSTALL=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --user)
            USER_UNINSTALL=true
            shift
            ;;
        *)
            echo "Unknown option: $key"
            echo "Usage: $0 [--user]"
            exit 1
            ;;
    esac
done

if [ "$USER_UNINSTALL" = true ]; then
    # User installation removal
    echo "Removing user installation..."
    if [ -f "$HOME/bin/nutshell" ]; then
        rm -f "$HOME/bin/nutshell"
        echo "Removed binary from $HOME/bin/nutshell"
    fi
    
    if [ -d "$HOME/.nutshell" ]; then
        rm -rf "$HOME/.nutshell"
        echo "Removed configuration directory $HOME/.nutshell"
    fi
else
    # System installation removal
    if [ "$(id -u)" -ne 0 ]; then
        echo "System uninstallation requires root privileges"
        echo "Please run with sudo or use --user for user uninstallation"
        exit 1
    fi
    
    echo "Removing system installation..."
    if [ -f "/usr/local/bin/nutshell" ]; then
        rm -f "/usr/local/bin/nutshell"
        echo "Removed binary from /usr/local/bin/nutshell"
    fi
    
    if [ -d "/usr/local/share/nutshell" ]; then
        rm -rf "/usr/local/share/nutshell"
        echo "Removed package directory /usr/local/share/nutshell"
    fi
fi

echo "Nutshell has been uninstalled successfully."

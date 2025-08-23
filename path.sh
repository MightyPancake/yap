#!/usr/bin/env bash
#
# Installer script for YAP_HOME_PATH
# - Sets YAP_HOME_PATH to this script’s directory
# - Permanently adds it to PATH for bash and fish
# - Avoids duplicates

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]:-${(%):-%x}}" )" && pwd )"

add_to_bashrc() {
    local RCFILE="$HOME/.bashrc"
    if ! grep -q "YAP_HOME_PATH=$SCRIPT_DIR" "$RCFILE" 2>/dev/null; then
        echo "" >> "$RCFILE"
        echo "# Added by YAP installer" >> "$RCFILE"
        echo "export YAP_HOME_PATH=\"$SCRIPT_DIR\"" >> "$RCFILE"
        echo "case \":\$PATH:\" in" >> "$RCFILE"
        echo "    *\":\$YAP_HOME_PATH:\"*) ;; " >> "$RCFILE"
        echo "    *) export PATH=\"\$YAP_HOME_PATH:\$PATH\" ;; " >> "$RCFILE"
        echo "esac" >> "$RCFILE"
        echo "Updated $RCFILE"
    else
        echo "YAP_HOME_PATH already present in $RCFILE"
    fi
}

add_to_fish_config() {
    local CONFIG="$HOME/.config/fish/config.fish"
    mkdir -p "$(dirname "$CONFIG")"
    if ! grep -q "set -x YAP_HOME_PATH $SCRIPT_DIR" "$CONFIG" 2>/dev/null; then
        echo "" >> "$CONFIG"
        echo "# Added by YAP installer" >> "$CONFIG"
        echo "set -x YAP_HOME_PATH $SCRIPT_DIR" >> "$CONFIG"
        echo "if not contains \$YAP_HOME_PATH \$PATH" >> "$CONFIG"
        echo "    set -x PATH \$YAP_HOME_PATH \$PATH" >> "$CONFIG"
        echo "end" >> "$CONFIG"
        echo "Updated $CONFIG"
    else
        echo "YAP_HOME_PATH already present in $CONFIG"
    fi
}

# Detect shell
if [ -n "$BASH_VERSION" ] || [ -n "$ZSH_VERSION" ]; then
    add_to_bashrc
elif [ -n "$FISH_VERSION" ]; then
    add_to_fish_config
else
    # Default to bashrc if unsure
    add_to_bashrc
fi

echo "Done. Restart your shell or run 'source ~/.bashrc' (bash) or 'source ~/.config/fish/config.fish' (fish)."

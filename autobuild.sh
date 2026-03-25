#!/bin/bash

# =======================
# Color definitions
# =======================
RED="\e[31m"
GREEN="\e[32m"
YELLOW="\e[33m"
BLUE="\e[34m"
BOLD="\e[1m"
RESET="\e[0m"

# Path to ESP-IDF export script
IDF_EXPORT="/home/alvin/works/ESP-IDF/.espressif/v6.0/esp-idf/export.sh"
OUTPUT_FILE="build/esp32s3_tinyusb.bin"

# Determine action: default is 'build'
ACTION=${1:-build}

case $ACTION in
    build)
        echo -e "\n${BOLD}${BLUE}===== 🚀 Starting Build =====${RESET}\n"
        START_TIME=$(date +%s)
        
        # Load ESP-IDF environment and build
        . "$IDF_EXPORT" && idf.py build
        
        END_TIME=$(date +%s)
        BUILD_TIME=$((END_TIME - START_TIME))
        
        # Print build time
        echo -e "\n${BOLD}${BLUE}===== ⏱  Build Time =====${RESET}"
        printf "Total build time: ${GREEN}%02d:%02d:%02d${RESET} (hh:mm:ss)\n" $((BUILD_TIME/3600)) $(( (BUILD_TIME%3600)/60 )) $((BUILD_TIME%60))

        # Output file info
        echo -e "\n${BOLD}${BLUE}===== 📦 Output File =====${RESET}"
        if [ -f "$OUTPUT_FILE" ]; then
            echo -e "File generated at: ${GREEN}$OUTPUT_FILE${RESET}"
            ls -lh "$OUTPUT_FILE"
        else
            echo -e "${YELLOW}⚠️  Build finished but output file not found!${RESET}"
        fi
        echo -e "\n${BOLD}${BLUE}===========================${RESET}\n"
        ;;
    clean)
        echo -e "\n${BOLD}${BLUE}🧹 Cleaning project...${RESET}"
        . "$IDF_EXPORT" && idf.py fullclean
        echo -e "${GREEN}Clean completed.${RESET}\n"
        ;;
    *)
        echo -e "${RED}Unknown action: $ACTION${RESET}"
        echo "Usage: $0 [clean]"
        exit 1
        ;;
esac
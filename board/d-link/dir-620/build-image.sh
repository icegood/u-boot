#!/bin/sh

# Build U-Boot flash image for DIR-620
#
# Usage:
#   $0 <output> <spl> <dtb> <lzma> <spl_base> \
#      <dtb_offset> <dtb_max_size> <spl_max_size> <part_end> \
#      <profile_layout> <env_file> [<eeprom_file>]

OUTPUT="$1"
SPL_FILE="$2"
DTB_FILE="$3"
LZMA_FILE="$4"

# Informational only
SPL_BASE="$5"

DTB_OFFSET_RAW="$6"
DTB_MAX_SIZE_RAW="$7"
SPL_MAX_SIZE_RAW="$8"
UBOOT_PART_END_RAW="$9"

PROFILE_LAYOUT="${10}"
ENV_FILE="${11}"
EEPROM_FILE="${12}"

usage() {
    echo "Usage:"
    echo "  $0 <output> <spl_image> <dtb_file> <lzma_payload> <spl_base> \\"
    echo "     <dtb_offset> <dtb_max_size> <spl_max_size> <part_end> \\"
    echo "     <profile_layout> <env_file> [<eeprom_file>]"
    exit 1
}

# ==============================================================================
# --- ARGUMENT VALIDATION ---
# ==============================================================================

[ $# -lt 11 ] && usage
[ $# -gt 12 ] && usage

for arg in \
    "$OUTPUT" \
    "$SPL_FILE" \
    "$DTB_FILE" \
    "$LZMA_FILE" \
    "$SPL_BASE" \
    "$DTB_OFFSET_RAW" \
    "$DTB_MAX_SIZE_RAW" \
    "$SPL_MAX_SIZE_RAW" \
    "$UBOOT_PART_END_RAW" \
    "$PROFILE_LAYOUT" \
    "$ENV_FILE"
do
    [ -z "$arg" ] && usage
done

# ==============================================================================
# --- LAYOUT CALCULATIONS ---
# ==============================================================================

# Shell arithmetic converts hex constants like 0x10000 automatically
SPL_OFFSET=0
EEPROM_OFFSET=$((SPL_MAX_SIZE_RAW))

DTB_OFFSET=$((DTB_OFFSET_RAW))
DTB_MAX_SIZE=$((DTB_MAX_SIZE_RAW))
SPL_MAX_SIZE=$((SPL_MAX_SIZE_RAW))
UBOOT_PART_END=$((UBOOT_PART_END_RAW))

LZMA_OFFSET=$((DTB_OFFSET + DTB_MAX_SIZE))
LZMA_MAX_SIZE=$((UBOOT_PART_END - LZMA_OFFSET))

# ==============================================================================
# --- LAYOUT VALIDATION ---
# ==============================================================================

fail() {
    echo "ERROR: $*"
    exit 1
}

[ "$DTB_OFFSET" -lt 0 ] && fail "Negative DTB offset"
[ "$DTB_MAX_SIZE" -le 0 ] && fail "Invalid DTB max size"
[ "$SPL_MAX_SIZE" -le 0 ] && fail "Invalid SPL max size"
[ "$UBOOT_PART_END" -le 0 ] && fail "Invalid partition end"

[ "$DTB_OFFSET" -lt "$SPL_MAX_SIZE" ] && \
    fail "DTB overlaps SPL region"

[ "$LZMA_OFFSET" -ge "$UBOOT_PART_END" ] && \
    fail "LZMA offset exceeds partition"

[ "$LZMA_MAX_SIZE" -le 0 ] && \
    fail "Invalid LZMA max size"

echo "  LAYOUT \\"
echo "    spl_base=0x$(printf '%x' "$SPL_BASE") \\"
echo "    spl_max=0x$(printf '%x' "$SPL_MAX_SIZE") \\"
echo "    dtb_ofs=0x$(printf '%x' "$DTB_OFFSET") \\"
echo "    dtb_max=0x$(printf '%x' "$DTB_MAX_SIZE") \\"
echo "    lzma_ofs=0x$(printf '%x' "$LZMA_OFFSET") \\"
echo "    lzma_max=0x$(printf '%x' "$LZMA_MAX_SIZE") \\"
echo "    part_end=0x$(printf '%x' "$UBOOT_PART_END")"

# ==============================================================================
# --- HELPERS ---
# ==============================================================================

get_size() {
    wc -c < "$1" | tr -d ' '
}

check_component() {
    name="$1"
    file_path="$2"
    offset="$3"
    max_size="$4"

    if [ ! -f "$file_path" ]; then
        echo "  $name: MISSING at $file_path"
        return 1
    fi

    file_size=$(get_size "$file_path")

    if [ "$file_size" -gt "$max_size" ]; then
        echo "  $name ($file_path): ${file_size}B OVERFLOW (max ${max_size}B)"
        return 1
    fi

    end_offset=$((offset + file_size))

    if [ "$end_offset" -gt "$UBOOT_PART_END" ]; then
        echo "  $name ($file_path): exceeds partition end"
        return 1
    fi

    echo "  $name ($file_path): ${file_size}B at offset 0x$(printf '%x' "$offset"), max=${max_size}B"
    return 0
}

write_component() {
    name="$1"
    file_path="$2"
    offset="$3"

    file_size=$(get_size "$file_path")

    dd \
        if="$file_path" \
        of="$OUTPUT" \
        bs=1 \
        seek="$offset" \
        conv=notrunc \
        2>/dev/null

    if [ $? -ne 0 ]; then
        fail "Failed to write $name to $OUTPUT"
    fi

    written_size=$(get_size "$OUTPUT")

    if [ "$written_size" -gt "$UBOOT_PART_END" ]; then
        fail "$OUTPUT grew beyond partition size"
    fi
}

create_filled_image() {
    output="$1"
    size="$2"

    rm -f "$output" || exit 1

    # Create file fully filled with 0xFF
    tr '\000' '\377' < /dev/zero | \
        dd of="$output" bs=1 count="$size" 2>/dev/null

    if [ $? -ne 0 ]; then
        fail "Failed to create output image"
    fi
}

# ==============================================================================
# --- CHECK COMPONENTS ---
# ==============================================================================

echo "Checking components..."

error=0

check_component "SPL" \
    "$SPL_FILE" \
    "$SPL_OFFSET" \
    "$SPL_MAX_SIZE" || error=1

check_component "DTB" \
    "$DTB_FILE" \
    "$DTB_OFFSET" \
    "$DTB_MAX_SIZE" || error=1

check_component "LZMA" \
    "$LZMA_FILE" \
    "$LZMA_OFFSET" \
    "$LZMA_MAX_SIZE" || error=1

if [ -n "$EEPROM_FILE" ]; then
    if [ ! -f "$EEPROM_FILE" ]; then
        echo "  EEPROM: not found ($EEPROM_FILE), skipping"
    else
        check_component "EEPROM" \
            "$EEPROM_FILE" \
            "$EEPROM_OFFSET" \
            512 || error=1
    fi
fi

[ "$error" -ne 0 ] && exit 1

# ==============================================================================
# --- BUILD IMAGE ---
# ==============================================================================

echo "Creating flash image..."

create_filled_image "$OUTPUT" "$UBOOT_PART_END"

write_component "SPL" \
    "$SPL_FILE" \
    "$SPL_OFFSET"

if [ -n "$EEPROM_FILE" ] && [ -f "$EEPROM_FILE" ]; then
    write_component "EEPROM" \
        "$EEPROM_FILE" \
        "$EEPROM_OFFSET"
fi

write_component "DTB" \
    "$DTB_FILE" \
    "$DTB_OFFSET"

write_component "LZMA" \
    "$LZMA_FILE" \
    "$LZMA_OFFSET"

FINAL_SIZE=$(get_size "$OUTPUT")

echo "Created $OUTPUT (${FINAL_SIZE} bytes)"
echo "MD5: $(md5sum "$OUTPUT" | cut -d' ' -f1)"
echo ""

# ==============================================================================
# --- ENVIRONMENT IMAGE ---
# ==============================================================================

echo ""
echo "Generating environment images..."

# Locate mkenvimage
MKENVIMAGE="tools/mkenvimage"
[ ! -f "$MKENVIMAGE" ] && MKENVIMAGE="$(dirname "$0")/../../tools/mkenvimage"
[ ! -f "$MKENVIMAGE" ] && MKENVIMAGE="mkenvimage"

# Extract environment layout from PROFILE_LAYOUT
ENV_ADDR=""
ENV_SIZE=""
ENV_SECT_SIZE=""
for token in $PROFILE_LAYOUT; do
    case "$token" in
        ENV_ADDR=*) ENV_ADDR="${token#ENV_ADDR=}" ;;
        ENV_SIZE=*) ENV_SIZE="${token#ENV_SIZE=}" ;;
        ENV_SECT_SIZE=*) ENV_SECT_SIZE="${token#ENV_SECT_SIZE=}" ;;
    esac
done

if [ -z "$ENV_ADDR" ] || [ -z "$ENV_SIZE" ] || [ -z "$ENV_SECT_SIZE" ]; then
    echo "  WARNING: ENV_ADDR, ENV_SIZE or ENV_SECT_SIZE not found in profile, skipping env image"
elif ! command -v "$MKENVIMAGE" >/dev/null 2>&1 && [ ! -x "$MKENVIMAGE" ]; then
    echo "  WARNING: mkenvimage not found, skipping env image"
else
    ENV_IMAGE="dir620.env"

    "$MKENVIMAGE" -s "$ENV_SIZE" -o "$ENV_IMAGE" "$ENV_FILE"
    if [ $? -ne 0 ]; then
        echo "  ERROR: mkenvimage failed"
    else
        echo "  $ENV_FILE -> $ENV_IMAGE ($(get_size "$ENV_IMAGE") bytes)"
        echo "  Usage: ext4load usb 0:1 \$loadaddr $ENV_IMAGE && env import -c \$loadaddr \$filesize && saveenv"
        echo "  MD5: $(md5sum "$ENV_IMAGE" | cut -d' ' -f1)"

        ENV_OFFSET=$((ENV_ADDR - 0xbf000000))
        ENV_END=$((ENV_OFFSET + ENV_SIZE))
        if [ "$ENV_OFFSET" -ge 0 ] && [ "$ENV_END" -le "$UBOOT_PART_END" ]; then
            write_component "ENV" "$ENV_IMAGE" "$ENV_OFFSET"
        fi
    fi
fi

echo ""
echo "PROFILE_VARS='LZMA_OFFSET=$(printf '0x%x' "$LZMA_OFFSET") LZMA_MAX=$(printf '0x%x' "$LZMA_MAX_SIZE") $PROFILE_LAYOUT'"

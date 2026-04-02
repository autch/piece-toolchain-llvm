#!/bin/bash
# LTO effect measurement script
# Builds each example app with LTO=off/thin/full and compares code sizes.
#
# Usage: ./tools/measure-lto.sh [--verbose]

set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build/bin"
SYSROOT="$ROOT/sysroot/s1c33-none-elf"
LD_SCRIPT="$ROOT/tools/piece.ld"
SDK_INCLUDE="$ROOT/sdk/include"
TMPDIR="$(mktemp -d /tmp/lto-measure.XXXXXX)"
trap "rm -rf $TMPDIR" EXIT

CLANG="$BIN/clang"
LLD="$BIN/ld.lld"
SIZE="$BIN/llvm-size"

CFLAGS_COMMON="--target=s1c33-none-elf --sysroot=$SYSROOT -O2 -Wall -I$SDK_INCLUDE -Wno-incompatible-library-redeclaration"
LDFLAGS_COMMON="-m elf32ls1c33 -T $LD_SCRIPT"
LIBS="-L$SYSROOT/lib -lpceapi -lio -llib -lmath -lstring -lctype -lfp -lidiv"
CRT="$SYSROOT/lib/crt0.o $SYSROOT/lib/crti.o"

VERBOSE="${1:+1}"
VERBOSE="${VERBOSE:-0}"

log() { [ "$VERBOSE" = "1" ] && echo "$@" >&2; }
die() { echo "ERROR: $@" >&2; exit 1; }

# Verify tools exist
[ -x "$CLANG" ] || die "clang not found: $CLANG"
[ -x "$LLD"   ] || die "lld not found: $LLD"
[ -x "$SIZE"  ] || die "llvm-size not found: $SIZE"

# get_section_size <elf> <section>
get_section_size() {
    local elf="$1" sec="$2"
    "$BIN/llvm-objdump" --section-headers "$elf" 2>/dev/null \
        | awk -v s="$sec" '$2 == s {printf "%d", strtonum("0x"$3)}'
}

# get_sizes <elf>  → "text rodata data bss total"
get_sizes() {
    local elf="$1"
    "$SIZE" "$elf" 2>/dev/null | awk 'NR==2 {print $1, $2, $3, $4}'
}

# elf_file_size <elf>
elf_file_size() { wc -c < "$1"; }

# build <name> <lto_mode> <src1> [src2 ...] → prints "text rodata data bss"
build_app() {
    local name="$1" lto_mode="$2"
    shift 2
    local srcs=("$@")
    local out_elf="$TMPDIR/${name}_${lto_mode}.elf"
    local objs=()

    case "$lto_mode" in
        off)   lto_flag=""          ;;
        thin)  lto_flag="-flto=thin" ;;
        full)  lto_flag="-flto=full" ;;
        *)     die "unknown lto_mode: $lto_mode" ;;
    esac

    # Compile each source
    for src in "${srcs[@]}"; do
        local base
        base="$(basename "${src%.*}")"
        local obj="$TMPDIR/${name}_${lto_mode}_${base}.o"
        log "  cc $lto_flag $src → $obj"
        $CLANG $CFLAGS_COMMON ${APP_EXTRA_CFLAGS[$name]:-} $lto_flag -c "$src" -o "$obj" 2>/dev/null || {
            echo "COMPILE_FAIL"
            return 1
        }
        objs+=("$obj")
    done

    # Link
    log "  link → $out_elf"
    local extra_objs=(${APP_EXTRA_OBJS[$name]:-})
    if [ "$lto_mode" != "off" ]; then
        $LLD $LDFLAGS_COMMON --plugin-opt=O2 $CRT "${objs[@]}" "${extra_objs[@]}" $LIBS -o "$out_elf" 2>/dev/null || {
            echo "LINK_FAIL"
            return 1
        }
    else
        $LLD $LDFLAGS_COMMON $CRT "${objs[@]}" "${extra_objs[@]}" $LIBS -o "$out_elf" 2>/dev/null || {
            echo "LINK_FAIL"
            return 1
        }
    fi

    get_sizes "$out_elf"
}

# Print a table row: app | off text | thin text | full text | thin% | full%
print_header() {
    printf "%-20s  %10s  %10s  %10s  %8s  %8s  %8s  %8s\n" \
        "App" "text(off)" "text(thin)" "text(full)" "Δthin(B)" "Δfull(B)" "Δthin%" "Δfull%"
    printf "%s\n" "$(printf '%.0s-' {1..90})"
}

print_row() {
    local name="$1"
    local text_off="$2"  rodata_off="$3"
    local text_thin="$4" rodata_thin="$5"
    local text_full="$6" rodata_full="$7"

    # Replace non-numeric values with 0 for calculation, but display "FAIL"
    local thin_disp="$text_thin" full_disp="$text_full"
    [[ "$text_thin" =~ ^[0-9]+$ ]] || text_thin="0"
    [[ "$text_full" =~ ^[0-9]+$ ]] || text_full="0"

    if [ "${text_off:-0}" -gt 0 ] 2>/dev/null; then
        local dthin_b=$(( text_thin - text_off ))
        local dfull_b=$(( text_full - text_off ))
        local dthin_pct=$(( dthin_b * 100 / text_off ))
        local dfull_pct=$(( dfull_b * 100 / text_off ))
        local sign_thin=""; [ "$dthin_b" -gt 0 ] && sign_thin="+"
        local sign_full=""; [ "$dfull_b" -gt 0 ] && sign_full="+"
        [ "$thin_disp" = "0" ] && { dthin_b=0; dthin_pct=0; sign_thin=""; thin_disp="FAIL"; }
        [ "$full_disp" = "0" ] && { dfull_b=0; dfull_pct=0; sign_full=""; full_disp="FAIL"; }
        printf "%-20s  %10d  %10s  %10s  %8s  %8s  %7s  %7s\n" \
            "$name" "$text_off" "$thin_disp" "$full_disp" \
            "${sign_thin}${dthin_b}" "${sign_full}${dfull_b}" \
            "${sign_thin}${dthin_pct}%" "${sign_full}${dfull_pct}%"
    else
        printf "%-20s  %10s  %10s  %10s  %8s  %8s  %7s  %7s\n" \
            "$name" "FAIL" "${thin_disp:-FAIL}" "${full_disp:-FAIL}" "-" "-" "-" "-"
    fi
}

# ─────────────────────────────────────────────────────────
# App definitions: name + sources
# ─────────────────────────────────────────────────────────

declare -A APP_SRCS

declare -A APP_EXTRA_CFLAGS
declare -A APP_EXTRA_OBJS

APP_SRCS["hello"]="$ROOT/hello/hello.c"

APP_SRCS["minimal"]="$ROOT/minimal/minimal.c"

APP_SRCS["print"]="$ROOT/print/print.c"

APP_SRCS["jien"]="$ROOT/jien/jien.c $ROOT/jien/jien_bmp.c"

APP_SRCS["fpkplay"]="$ROOT/fpkplay/fpkplay.c $ROOT/fpkplay/decode.c $ROOT/fpkplay/fpk.c \
    $ROOT/fpkplay/music/exp12.c $ROOT/fpkplay/music/instdef.c \
    $ROOT/fpkplay/music/mus.c   $ROOT/fpkplay/music/seq.c \
    $ROOT/fpkplay/music/wavetable.c"
APP_EXTRA_CFLAGS["fpkplay"]="-I$ROOT/fpkplay -I$ROOT/fpkplay/music"
# wave/*.o and musfast.o are precompiled (data + asm); link them in directly
APP_EXTRA_OBJS["fpkplay"]="$ROOT/fpkplay/music/musfast.o $(echo $ROOT/fpkplay/music/wave/*.o)"

APP_SRCS["pmdplay"]="$ROOT/pmdplay/pmdplay.c $ROOT/pmdplay/pmdplay_debug.c \
    $ROOT/pmdplay/music/exp12.c $ROOT/pmdplay/music/instdef.c \
    $ROOT/pmdplay/music/mus.c   $ROOT/pmdplay/music/seq.c \
    $ROOT/pmdplay/music/wavetable.c"
APP_EXTRA_CFLAGS["pmdplay"]="-I$ROOT/pmdplay -I$ROOT/pmdplay/music -I$ROOT/pmdplay/pmd"
APP_EXTRA_OBJS["pmdplay"]="$ROOT/pmdplay/music/musfast.o $(echo $ROOT/pmdplay/music/wave/*.o) $ROOT/pmdplay/pmd/pmd_data.a"

APPS="hello minimal print jien fpkplay pmdplay"

# ─────────────────────────────────────────────────────────
# Main measurement loop
# ─────────────────────────────────────────────────────────

echo ""
echo "S1C33 LTO Effect Measurement"
echo "Compiler: $($CLANG --version 2>&1 | head -1)"
echo "Target:   s1c33-none-elf, -O2"
echo "Date:     $(date)"
echo ""
print_header

declare -A RESULTS

for app in $APPS; do
    srcs="${APP_SRCS[$app]}"
    echo -n "Building $app ... " >&2

    sizes_off=""
    sizes_thin=""
    sizes_full=""

    sizes_off=$(build_app "$app" "off"  $srcs 2>/dev/null) && echo -n "." >&2 || echo -n "!" >&2
    sizes_thin=$(build_app "$app" "thin" $srcs 2>/dev/null) && echo -n "." >&2 || echo -n "!" >&2
    sizes_full=$(build_app "$app" "full" $srcs 2>/dev/null) && echo -n "." >&2 || echo -n "!" >&2
    echo " done" >&2

    read text_off  rodata_off  data_off  bss_off  <<< "$sizes_off"
    read text_thin rodata_thin data_thin bss_thin <<< "$sizes_thin"
    read text_full rodata_full data_full bss_full <<< "$sizes_full"

    print_row "$app" \
        "${text_off:-0}"  "${rodata_off:-0}" \
        "${text_thin:-0}" "${rodata_thin:-0}" \
        "${text_full:-0}" "${rodata_full:-0}"

    RESULTS["${app}_off"]="$sizes_off"
    RESULTS["${app}_thin"]="$sizes_thin"
    RESULTS["${app}_full"]="$sizes_full"
done

echo ""
echo "Notes:"
echo "  Δthin / Δfull = (text_lto - text_off) * 100 / text_off"
echo "  Negative values = smaller code (LTO benefit)"
echo "  Positive values = larger code (LTO overhead / inlining)"
echo "  text includes user code + SDK library code pulled in by --gc-sections"
echo "  muslib.a wave/*.o (precompiled instrument samples) not included in fpkplay LTO"
echo ""

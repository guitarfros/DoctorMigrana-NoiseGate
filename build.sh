#!/usr/bin/env bash
# Compila o plugin en Ubuntu/Debian.
#   ./build.sh            -> instala dependencias (pide sudo), compila Release, corre os tests
#   ./build.sh --no-deps  -> salta a instalación de dependencias
#   ./build.sh --clean    -> borra build/ antes de compilar
#   ./build.sh --no-install -> non copia o VST3 a ~/.vst3
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

INSTALL_DEPS=1
CLEAN=0
COPY=ON
for arg in "$@"; do
    case "$arg" in
        --no-deps)    INSTALL_DEPS=0 ;;
        --clean)      CLEAN=1 ;;
        --no-install) COPY=OFF ;;
        -h|--help)    sed -n '2,7p' "$0"; exit 0 ;;
        *) echo "Opción descoñecida: $arg"; exit 1 ;;
    esac
done

if [ "$INSTALL_DEPS" -eq 1 ]; then
    echo "==> Instalando dependencias (apt)…"
    sudo apt-get update
    sudo apt-get install -y \
        build-essential cmake ninja-build pkg-config git \
        libasound2-dev libjack-jackd2-dev \
        libfreetype-dev libfontconfig1-dev \
        libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
        libxinerama-dev libxrandr-dev libxrender-dev \
        libglu1-mesa-dev mesa-common-dev
fi

[ "$CLEAN" -eq 1 ] && rm -rf build

GEN=()
command -v ninja >/dev/null 2>&1 && GEN=(-G Ninja)

echo "==> Configurando (a primeira vez descarga JUCE, tarda un pouco)…"
cmake -S . -B build "${GEN[@]}" -DCMAKE_BUILD_TYPE=Release -DCALMA_COPY_AFTER_BUILD="$COPY"

echo "==> Compilando…"
cmake --build build --config Release --parallel

echo "==> Tests do DSP…"
ctest --test-dir build --output-on-failure

echo
echo "==> Listo. Resultados:"
find build -path '*_artefacts/Release/*' \( -name '*.vst3' -o -type f -name 'Doctor*Noise*Gate' \) -maxdepth 6 2>/dev/null
[ "$COPY" = ON ] && echo "    (o VST3 tamén se copiou a ~/.vst3)"

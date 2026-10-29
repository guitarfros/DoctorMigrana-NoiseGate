# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 GuitarFros
cmake_minimum_required(VERSION 3.22)

# Debe ir antes de project() para que o macOS o respecte
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Versión mínima de macOS")
    # Binario universal (Apple Silicon + Intel). Vai aquí e non na liña de comandos
    # do workflow, porque o «;» da lista é un separador de comandos para o shell.
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Arquitecturas de macOS")
endif()

project(DoctorMigranaNoiseGate VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CALMA_BUILD_PLUGIN     "Compilar o plugin (VST3 + Standalone)"                 ON)
option(CALMA_BUILD_TESTS      "Compilar e rexistrar os tests do DSP (sen JUCE)"       ON)
option(CALMA_COPY_AFTER_BUILD "Copiar o VST3 ao cartafol de plugins do usuario"       ON)
set(CALMA_JUCE_TAG "8.0.6" CACHE STRING "Versión (tag) de JUCE a descargar")

# ----------------------------------------------------------------------------
# Tests do DSP: C++ puro, compilan en segundos e non precisan JUCE
# ----------------------------------------------------------------------------
if(CALMA_BUILD_TESTS)
    enable_testing()
    add_executable(calma_dsp_test tests/dsp_test.cpp)
    target_include_directories(calma_dsp_test PRIVATE Source)
    add_test(NAME dsp COMMAND calma_dsp_test)
endif()

if(NOT CALMA_BUILD_PLUGIN)
    return()
endif()

# ----------------------------------------------------------------------------
# JUCE (descárgase automaticamente)
# ----------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        ${CALMA_JUCE_TAG}
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

# ----------------------------------------------------------------------------
# Imaxe do pedal incrustada no binario (assets/pedal.jpg)
# ----------------------------------------------------------------------------
set(CALMA_PEDAL_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/assets/pedal.jpg")
if(EXISTS "${CALMA_PEDAL_IMAGE}")
    juce_add_binary_data(CalmaAssets SOURCES "${CALMA_PEDAL_IMAGE}")
    set_target_properties(CalmaAssets PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set(CALMA_HAS_IMAGE 1)
else()
    message(WARNING "Non atopei assets/pedal.jpg: o plugin usará un fondo liso.")
    set(CALMA_HAS_IMAGE 0)
endif()

# ----------------------------------------------------------------------------
# Plugin
# ----------------------------------------------------------------------------
if(CALMA_COPY_AFTER_BUILD)
    set(_copy TRUE)
else()
    set(_copy FALSE)
endif()

juce_add_plugin(DoctorMigranaNoiseGate
    COMPANY_NAME                "GuitarFros"
    BUNDLE_ID                   com.guitarfros.noisegate
    PRODUCT_NAME                "Doctor Migrana - Noise Gate"
    PLUGIN_MANUFACTURER_CODE    Dmig
    PLUGIN_CODE                 Ngt1
    FORMATS                     VST3 Standalone
    VST3_CATEGORIES             Fx Dynamics
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${_copy})

target_sources(DoctorMigranaNoiseGate PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/NoiseGate.h
    Source/PedalLayout.h)

target_compile_definitions(DoctorMigranaNoiseGate PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODAL_LOOPS_PERMITTED=0
    CALMA_HAS_PEDAL_IMAGE=${CALMA_HAS_IMAGE})

target_include_directories(DoctorMigranaNoiseGate PRIVATE Source)

if(CALMA_HAS_IMAGE)
    target_link_libraries(DoctorMigranaNoiseGate PRIVATE CalmaAssets)
endif()

target_link_libraries(DoctorMigranaNoiseGate
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

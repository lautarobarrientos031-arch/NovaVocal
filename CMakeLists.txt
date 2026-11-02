cmake_minimum_required(VERSION 3.22)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Version minima de macOS")
# Windows: runtime de C++ enlazado estatico (el usuario no necesita instalar Visual C++ Redistributable)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(NovaVocal VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- JUCE: usa una copia local (-DJUCE_DIR=/ruta/a/JUCE) o la descarga automaticamente
include(FetchContent)
set(JUCE_DIR "" CACHE PATH "Ruta a una copia local de JUCE (opcional)")
if(JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_DIR}" JUCE)
else()
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(NOVA_FORMATS VST3 AU Standalone)
else()
    set(NOVA_FORMATS VST3 Standalone)
endif()

# --- AAX (solo Pro Tools; FL Studio NO lo carga). Requiere el AAX SDK de Avid (cuenta de desarrollador).
#     Uso: -DAAX_SDK_PATH=/ruta/al/AAX_SDK
set(AAX_SDK_PATH "" CACHE PATH "Ruta al AAX SDK de Avid (opcional)")
if(AAX_SDK_PATH AND EXISTS "${AAX_SDK_PATH}")
    juce_set_aax_sdk_path("${AAX_SDK_PATH}")
    list(APPEND NOVA_FORMATS AAX)
    message(STATUS "AAX habilitado")
endif()

juce_add_plugin(NovaVocal
    COMPANY_NAME "NovaAudio"
    BUNDLE_ID com.novaaudio.novavocal
    PLUGIN_MANUFACTURER_CODE Nova
    PLUGIN_CODE Nvvc
    FORMATS ${NOVA_FORMATS}
    PRODUCT_NAME "Nova Vocal"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Nova Vocal usa el microfono en modo Standalone.")

target_sources(NovaVocal PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(NovaVocal PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(NovaVocal
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Test del nucleo DSP (sin JUCE):  cmake -DNOVA_BUILD_TESTS=ON ...
option(NOVA_BUILD_TESTS "Compilar el test del nucleo DSP" OFF)
if(NOVA_BUILD_TESTS)
    add_executable(nova_core_test Tests/test_core.cpp)
endif()

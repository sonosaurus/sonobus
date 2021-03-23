# Automatically generated makefile, created by the Projucer
# Don't edit this file! Your changes will be overwritten when you re-save the Projucer project!

cmake_minimum_required(VERSION 3.4.1)

set(BINARY_NAME "juce_jni")

set(OBOE_DIR "../../../deps/juce/modules/juce_audio_devices/native/oboe")

add_subdirectory (${OBOE_DIR} ./oboe)

add_library("cpufeatures" STATIC "${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c")
set_source_files_properties("${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c" PROPERTIES COMPILE_FLAGS "-Wno-sign-conversion -Wno-gnu-statement-expression")

add_definitions("-DJUCE_ANDROID=1" "-DJUCE_ANDROID_API_VERSION=24" "-DJUCE_PUSH_NOTIFICATIONS=1" "-DJUCE_PUSH_NOTIFICATIONS_ACTIVITY=\"com/sonosaurus/sonobus/SonoBusActivity\"" "-DJUCE_CONTENT_SHARING=1" "-DJUCE_ANDROID_GL_ES_VERSION_3_0=1" "-DJUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1" "-DSONOBUS_BUILD_VERSION=\"1.4.20-61\"" "-DUSE_CODEC_OPUS=1" "-DAOO_TIMEFILTER_CHECK=0" "-DAOO_STATIC" "-DJUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE=0" "-DJUCE_ANDROID_ACTIVITY_OVERRIDE_CLASSNAME=\"com_sonosaurus_sonobus_SonoBusActivity\"" "-DJUCE_ANDROID_ACTIVITY_OVERRIDE_CLASSPATH=\"com/sonosaurus/sonobus/SonoBusActivity\"" "-DJUCER_ANDROIDSTUDIO_5128E54=1" "-DJUCE_APP_VERSION=1.4.1" "-DJUCE_APP_VERSION_HEX=0x10401")

include_directories( AFTER
    "../../../mobile/JuceLibraryCode"
    "../../../deps"
    "../../../deps/juce/modules"
    "${ANDROID_NDK}/sources/android/cpufeatures"
)

enable_language(ASM)

if(JUCE_BUILD_CONFIGURATION MATCHES "DEBUG")
    link_directories(
        "../../../deps/android/${ANDROID_ABI}"
    )

    add_definitions("-DJUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1" "-DSONOBUS_BUILD_VERSION=\"1.4.20-61\"" "-DLOGLEVEL=2" "-DUSE_CODEC_OPUS=1" "-DAOO_TIMEFILTER_CHECK=0" "-DAOO_STATIC" "-DJUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE=0" "-DJUCE_ANDROID_ACTIVITY_OVERRIDE_CLASSNAME=\"com_sonosaurus_sonobus_SonoBusActivity\"" "-DJUCE_ANDROID_ACTIVITY_OVERRIDE_CLASSPATH=\"com/sonosaurus/sonobus/SonoBusActivity\"" "-DJUCER_ANDROIDSTUDIO_5128E54=1" "-DJUCE_APP_VERSION=1.4.1" "-DJUCE_APP_VERSION_HEX=0x10401" "-DDEBUG=1" "-D_DEBUG=1")
    include_directories( AFTER
        "../../../deps/aoo/lib"
        "../../../deps/aoo/deps"
        "../../../deps/android"
    )

    find_library(opus "opus" PATHS
        "../../../deps/android/${ANDROID_ABI}"
        NO_CMAKE_FIND_ROOT_PATH)


elseif(JUCE_BUILD_CONFIGURATION MATCHES "RELEASE")
    link_directories(
        "../../../deps/android/${ANDROID_ABI}"
    )

    add_definitions("-DJUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1" "-DSONOBUS_BUILD_VERSION=\"1.4.20-61\"" "-DUSE_CODEC_OPUS=1" "-DAOO_TIMEFILTER_CHECK=0" "-DAOO_STATIC" "-DJUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE=0" "-DJUCE_ANDROID_ACTIVITY_OVERRIDE_CLASSNAME=\"com_sonosaurus_sonobus_SonoBusActivity\"" "-DJUCE_ANDROID_ACTIVITY_OVERRIDE_CLASSPATH=\"com/sonosaurus/sonobus/SonoBusActivity\"" "-DJUCER_ANDROIDSTUDIO_5128E54=1" "-DJUCE_APP_VERSION=1.4.1" "-DJUCE_APP_VERSION_HEX=0x10401" "-DNDEBUG=1")
    include_directories( AFTER
        "../../../deps/aoo/lib"
        "../../../deps/aoo/deps"
        "../../../deps/android"
    )

    find_library(opus "opus" PATHS
        "../../../deps/android/${ANDROID_ABI}"
        NO_CMAKE_FIND_ROOT_PATH)


else()
    message( FATAL_ERROR "No matching build-configuration found." )
endif()

add_library( ${BINARY_NAME}

    SHARED

    "../../../deps/aoo/lib/aoo/aoo.h"
    "../../../deps/aoo/lib/aoo/aoo.hpp"
    "../../../deps/aoo/lib/aoo/aoo_net.h"
    "../../../deps/aoo/lib/aoo/aoo_net.hpp"
    "../../../deps/aoo/lib/aoo/aoo_opus.h"
    "../../../deps/aoo/lib/aoo/aoo_pcm.h"
    "../../../deps/aoo/lib/aoo/aoo_types.h"
    "../../../deps/aoo/lib/aoo/aoo_utils.hpp"
    "../../../deps/aoo/lib/src/client.cpp"
    "../../../deps/aoo/lib/src/client.hpp"
    "../../../deps/aoo/lib/src/codec_opus.cpp"
    "../../../deps/aoo/lib/src/codec_pcm.cpp"
    "../../../deps/aoo/lib/src/common.cpp"
    "../../../deps/aoo/lib/src/common.hpp"
    "../../../deps/aoo/lib/src/lockfree.hpp"
    "../../../deps/aoo/lib/src/net_utils.cpp"
    "../../../deps/aoo/lib/src/net_utils.hpp"
    "../../../deps/aoo/lib/src/server.cpp"
    "../../../deps/aoo/lib/src/server.hpp"
    "../../../deps/aoo/lib/src/sink.cpp"
    "../../../deps/aoo/lib/src/sink.hpp"
    "../../../deps/aoo/lib/src/SLIP.hpp"
    "../../../deps/aoo/lib/src/source.cpp"
    "../../../deps/aoo/lib/src/source.hpp"
    "../../../deps/aoo/lib/src/sync.cpp"
    "../../../deps/aoo/lib/src/sync.hpp"
    "../../../deps/aoo/lib/src/time.cpp"
    "../../../deps/aoo/lib/src/time.hpp"
    "../../../deps/aoo/lib/src/time_dll.hpp"
    "../../../deps/aoo/deps/md5/md5.c"
    "../../../deps/aoo/deps/md5/md5.h"
    "../../../deps/aoo/deps/oscpack/osc/MessageMappingOscPacketListener.h"
    "../../../deps/aoo/deps/oscpack/osc/OscException.h"
    "../../../deps/aoo/deps/oscpack/osc/OscHostEndianness.h"
    "../../../deps/aoo/deps/oscpack/osc/OscOutboundPacketStream.cpp"
    "../../../deps/aoo/deps/oscpack/osc/OscOutboundPacketStream.h"
    "../../../deps/aoo/deps/oscpack/osc/OscPacketListener.h"
    "../../../deps/aoo/deps/oscpack/osc/OscPrintReceivedElements.cpp"
    "../../../deps/aoo/deps/oscpack/osc/OscPrintReceivedElements.h"
    "../../../deps/aoo/deps/oscpack/osc/OscReceivedElements.cpp"
    "../../../deps/aoo/deps/oscpack/osc/OscReceivedElements.h"
    "../../../deps/aoo/deps/oscpack/osc/OscTypes.cpp"
    "../../../deps/aoo/deps/oscpack/osc/OscTypes.h"
    "../../../localization/localized_de.txt"
    "../../../localization/localized_es.txt"
    "../../../localization/localized_fr.txt"
    "../../../localization/localized_it.txt"
    "../../../localization/localized_ja.txt"
    "../../../localization/localized_nl.txt"
    "../../../localization/localized_pt-br.txt"
    "../../../localization/localized_pt-pt.txt"
    "../../../images/bar_click.wav"
    "../../../images/beat_click.wav"
    "../../../images/chat.svg"
    "../../../images/chat_dots.svg"
    "../../../images/chevron_forward.svg"
    "../../../images/copy_icon.svg"
    "../../../Source/DejaVuSans.ttf"
    "../../../images/dice_icon_128.png"
    "../../../images/dispfull.svg"
    "../../../images/dispminimal.svg"
    "../../../images/dots.svg"
    "../../../images/dots_icon.png"
    "../../../images/expand_arrow_active.svg"
    "../../../images/expand_arrow_inactive.svg"
    "../../../images/folder_icon.svg"
    "../../../images/hear-others.svg"
    "../../../images/incoming_allowed.svg"
    "../../../images/incoming_allowed_active.svg"
    "../../../images/incoming_disallowed.svg"
    "../../../images/lgc_bar.wav"
    "../../../images/link.svg"
    "../../../images/link_all.svg"
    "../../../images/link_up.svg"
    "../../../images/loop_icon.svg"
    "../../../images/met.svg"
    "../../../images/mic.svg"
    "../../../images/mic_disabled.svg"
    "../../../images/mic_pointing.svg"
    "../../../images/move_updown.svg"
    "../../../images/mute-others.svg"
    "../../../images/network.svg"
    "../../../images/outgoing_allowed.svg"
    "../../../images/outgoing_allowed_active.svg"
    "../../../images/outgoing_disallowed.svg"
    "../../../images/paste_icon.svg"
    "../../../images/pause_icon.svg"
    "../../../images/people.png"
    "../../../images/people.svg"
    "../../../images/person.png"
    "../../../images/person.svg"
    "../../../images/play_icon.svg"
    "../../../images/power.svg"
    "../../../images/power_sel.svg"
    "../../../images/record.svg"
    "../../../images/record_active.svg"
    "../../../images/record_active_alt.svg"
    "../../../images/rectape.svg"
    "../../../images/reset_buffer_icon.svg"
    "../../../images/send_group.svg"
    "../../../images/send_group_small.svg"
    "../../../images/settings_icon.svg"
    "../../../images/skipback_icon.svg"
    "../../../images/sonobus_icon_ios_256.png"
    "../../../images/sonobus_icon_ios_1024.png"
    "../../../images/sonobus_logo_96.png"
    "../../../images/sonobus_title_small.png"
    "../../../images/speaker.svg"
    "../../../images/speaker_disabled.svg"
    "../../../images/triangle_disclosure.svg"
    "../../../images/triangle_disclosure_right.svg"
    "../../../images/urei_main.wav"
    "../../../Source/wordmaker.g"
    "../../../images/x_icon.svg"
    "../../../Source/android/SonoBusActivity.cpp"
    "../../../Source/android/SonoBusActivity.h"
    "../../../Source/AutoUpdater.cpp"
    "../../../Source/AutoUpdater.h"
    "../../../Source/BeatToggleGrid.cpp"
    "../../../Source/BeatToggleGrid.h"
    "../../../Source/ChannelGroup.cpp"
    "../../../Source/ChannelGroup.h"
    "../../../Source/ChannelGroupsView.cpp"
    "../../../Source/ChannelGroupsView.h"
    "../../../Source/ChatView.cpp"
    "../../../Source/ChatView.h"
    "../../../Source/CompressorView.h"
    "../../../Source/ConnectView.cpp"
    "../../../Source/ConnectView.h"
    "../../../Source/CrossPlatformUtils.h"
    "../../../Source/CrossPlatformUtilsAndroid.cpp"
    "../../../Source/CrossPlatformUtilsIOS.mm"
    "../../../Source/CrossPlatformUtilsLinux.cpp"
    "../../../Source/CrossPlatformUtilsMac.mm"
    "../../../Source/CrossPlatformUtilsWindows.cpp"
    "../../../Source/DebugLogC.h"
    "../../../Source/EffectParams.cpp"
    "../../../Source/EffectParams.h"
    "../../../Source/EffectsBaseView.h"
    "../../../Source/ExpanderView.h"
    "../../../Source/faustCompressor.h"
    "../../../Source/faustExpander.h"
    "../../../Source/faustLimiter.h"
    "../../../Source/faustParametricEQ.h"
    "../../../Source/GenericItemChooser.cpp"
    "../../../Source/GenericItemChooser.h"
    "../../../Source/JitterBufferMeter.cpp"
    "../../../Source/JitterBufferMeter.h"
    "../../../Source/LatencyMatchView.cpp"
    "../../../Source/LatencyMatchView.h"
    "../../../Source/LatencyMeasurer.cpp"
    "../../../Source/LatencyMeasurer.h"
    "../../../Source/LevelMeterLookAndFeelMethods.h"
    "../../../Source/Metronome.cpp"
    "../../../Source/Metronome.h"
    "../../../Source/MonitorDelayView.h"
    "../../../Source/mtdm.cc"
    "../../../Source/mtdm.h"
    "../../../Source/MVerb.h"
    "../../../Source/OptionsView.cpp"
    "../../../Source/OptionsView.h"
    "../../../Source/ParametricEqView.h"
    "../../../Source/PeersContainerView.cpp"
    "../../../Source/PeersContainerView.h"
    "../../../Source/RandomSentenceGenerator.cpp"
    "../../../Source/RandomSentenceGenerator.h"
    "../../../Source/ReverbSendView.h"
    "../../../Source/RunCumulantor.cpp"
    "../../../Source/RunCumulantor.h"
    "../../../Source/RunningCumulant.c"
    "../../../Source/RunningCumulant.h"
    "../../../Source/SonobusPluginEditor.cpp"
    "../../../Source/SonobusPluginEditor.h"
    "../../../Source/SonobusPluginProcessor.cpp"
    "../../../Source/SonobusPluginProcessor.h"
    "../../../Source/SonobusTypes.h"
    "../../../Source/SonoChoiceButton.cpp"
    "../../../Source/SonoChoiceButton.h"
    "../../../Source/SonoDrawableButton.cpp"
    "../../../Source/SonoDrawableButton.h"
    "../../../Source/SonoLookAndFeel.cpp"
    "../../../Source/SonoLookAndFeel.h"
    "../../../Source/SonoStandaloneFilterApp.cpp"
    "../../../Source/SonoStandaloneFilterWindow.h"
    "../../../Source/SonoTextButton.cpp"
    "../../../Source/SonoTextButton.h"
    "../../../Source/SonoUtility.h"
    "../../../Source/VersionInfo.cpp"
    "../../../Source/VersionInfo.h"
    "../../../Source/WaveformTransportComponent.h"
    "../../../Source/zitaRev.h"
    "../../../deps/ff_meters/LevelMeter/LevelMeter.cpp"
    "../../../deps/ff_meters/LevelMeter/LevelMeter.h"
    "../../../deps/ff_meters/LevelMeter/LevelMeterSource.h"
    "../../../deps/ff_meters/LookAndFeel/LevelMeterLookAndFeel.h"
    "../../../deps/ff_meters/LookAndFeel/LevelMeterLookAndFeelMethods.h"
    "../../../deps/ff_meters/LookAndFeel/SoundFieldLookAndFeelMethods.h"
    "../../../deps/ff_meters/Visualisers/OutlineBuffer.h"
    "../../../deps/ff_meters/Visualisers/StereoFieldBuffer.h"
    "../../../deps/ff_meters/Visualisers/StereoFieldComponent.h"
    "../../../deps/ff_meters/ff_meters.cpp"
    "../../../deps/ff_meters/LICENSE.md"
    "../../../deps/ff_meters/README.md"
    "../../../deps/ff_meters/ff_meters.h"
    "../../../deps/juce/modules/juce_audio_basics/audio_play_head/juce_AudioPlayHead.h"
    "../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioChannelSet.cpp"
    "../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioChannelSet.h"
    "../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioDataConverters.cpp"
    "../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioDataConverters.h"
    "../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioProcessLoadMeasurer.cpp"
    "../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioProcessLoadMeasurer.h"
    "../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioSampleBuffer.h"
    "../../../deps/juce/modules/juce_audio_basics/buffers/juce_FloatVectorOperations.cpp"
    "../../../deps/juce/modules/juce_audio_basics/buffers/juce_FloatVectorOperations.h"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiBuffer.cpp"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiBuffer.h"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiFile.cpp"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiFile.h"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiKeyboardState.cpp"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiKeyboardState.h"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiMessage.cpp"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiMessage.h"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiMessageSequence.cpp"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiMessageSequence.h"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiRPN.cpp"
    "../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiRPN.h"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEInstrument.cpp"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEInstrument.h"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEMessages.cpp"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEMessages.h"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPENote.cpp"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPENote.h"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiser.cpp"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiser.h"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiserBase.cpp"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiserBase.h"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiserVoice.cpp"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiserVoice.h"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEUtils.cpp"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEUtils.h"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEValue.cpp"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEValue.h"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEZoneLayout.cpp"
    "../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEZoneLayout.h"
    "../../../deps/juce/modules/juce_audio_basics/native/juce_mac_CoreAudioLayouts.h"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_AudioSource.h"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_BufferingAudioSource.cpp"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_BufferingAudioSource.h"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_ChannelRemappingAudioSource.cpp"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_ChannelRemappingAudioSource.h"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_IIRFilterAudioSource.cpp"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_IIRFilterAudioSource.h"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_MemoryAudioSource.cpp"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_MemoryAudioSource.h"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_MixerAudioSource.cpp"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_MixerAudioSource.h"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_PositionableAudioSource.h"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_ResamplingAudioSource.cpp"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_ResamplingAudioSource.h"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_ReverbAudioSource.cpp"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_ReverbAudioSource.h"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_ToneGeneratorAudioSource.cpp"
    "../../../deps/juce/modules/juce_audio_basics/sources/juce_ToneGeneratorAudioSource.h"
    "../../../deps/juce/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp"
    "../../../deps/juce/modules/juce_audio_basics/synthesisers/juce_Synthesiser.h"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_ADSR.h"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_Decibels.h"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_GenericInterpolator.h"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_IIRFilter.cpp"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_IIRFilter.h"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_Interpolators.cpp"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_Interpolators.h"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_LagrangeInterpolator.cpp"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_Reverb.h"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_SmoothedValue.cpp"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_SmoothedValue.h"
    "../../../deps/juce/modules/juce_audio_basics/utilities/juce_WindowedSincInterpolator.cpp"
    "../../../deps/juce/modules/juce_audio_basics/juce_audio_basics.cpp"
    "../../../deps/juce/modules/juce_audio_basics/juce_audio_basics.mm"
    "../../../deps/juce/modules/juce_audio_basics/juce_audio_basics.h"
    "../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioDeviceManager.cpp"
    "../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioDeviceManager.h"
    "../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioIODevice.cpp"
    "../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioIODevice.h"
    "../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioIODeviceType.cpp"
    "../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioIODeviceType.h"
    "../../../deps/juce/modules/juce_audio_devices/audio_io/juce_SystemAudioVolume.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPacket.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPackets.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPBytestreamInputHandler.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPConversion.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPConverters.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPDispatcher.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPFactory.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPIterator.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPMidi1ToBytestreamTranslator.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPMidi1ToMidi2DefaultTranslator.cpp"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPMidi1ToMidi2DefaultTranslator.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPProtocols.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPReceiver.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPSysEx7.cpp"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPSysEx7.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPTests.cpp"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPU32InputHandler.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPUtils.cpp"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPUtils.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPView.cpp"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPView.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/juce_MidiDevices.cpp"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/juce_MidiDevices.h"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/juce_MidiMessageCollector.cpp"
    "../../../deps/juce/modules/juce_audio_devices/midi_io/juce_MidiMessageCollector.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/AudioStream.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/AudioStreamBase.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/AudioStreamBuilder.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/AudioStreamCallback.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/Definitions.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/LatencyTuner.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/Oboe.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/ResultWithValue.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/StabilizedCallback.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/Utilities.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/Version.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/aaudio/AAudioLoader.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/aaudio/AAudioLoader.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/aaudio/AudioStreamAAudio.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/aaudio/AudioStreamAAudio.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/AudioClock.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/AudioSourceCaller.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/AudioSourceCaller.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/AudioStream.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/AudioStreamBuilder.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/DataConversionFlowGraph.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/DataConversionFlowGraph.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FilterAudioStream.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FilterAudioStream.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockAdapter.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockAdapter.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockReader.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockReader.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockWriter.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockWriter.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/LatencyTuner.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/MonotonicCounter.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/OboeDebug.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/QuirksManager.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/QuirksManager.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/SourceFloatCaller.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/SourceFloatCaller.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/SourceI16Caller.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/SourceI16Caller.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/StabilizedCallback.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/Trace.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/Trace.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/Utilities.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/Version.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoBuffer.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoBuffer.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoController.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoController.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoControllerBase.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoControllerBase.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoControllerIndirect.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoControllerIndirect.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/HyperbolicCosineWindow.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/IntegerRatio.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/IntegerRatio.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/KaiserWindow.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/LinearResampler.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/LinearResampler.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/MultiChannelResampler.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/MultiChannelResampler.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResampler.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResampler.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResamplerMono.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResamplerMono.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResamplerStereo.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResamplerStereo.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/SincResampler.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/SincResampler.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/SincResamplerStereo.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/SincResamplerStereo.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ChannelCountConverter.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ChannelCountConverter.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ClipToRange.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ClipToRange.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/FlowGraphNode.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/FlowGraphNode.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ManyToMultiConverter.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ManyToMultiConverter.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/MonoToMultiConverter.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/MonoToMultiConverter.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/MultiToMonoConverter.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/MultiToMonoConverter.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/RampLinear.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/RampLinear.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SampleRateConverter.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SampleRateConverter.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkFloat.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkFloat.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkI16.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkI16.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkI24.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkI24.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceFloat.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceFloat.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceI16.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceI16.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceI24.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceI24.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioInputStreamOpenSLES.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioInputStreamOpenSLES.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioOutputStreamOpenSLES.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioOutputStreamOpenSLES.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioStreamBuffered.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioStreamBuffered.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioStreamOpenSLES.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioStreamOpenSLES.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/EngineOpenSLES.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/EngineOpenSLES.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/OpenSLESUtilities.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/OpenSLESUtilities.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/OutputMixerOpenSLES.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/OutputMixerOpenSLES.h"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/CMakeLists.txt"
    "../../../deps/juce/modules/juce_audio_devices/native/oboe/readme.md"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_android_Audio.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_android_HighPerformanceAudioHelpers.h"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_android_Midi.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_android_Oboe.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_android_OpenSL.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_ios_Audio.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_ios_Audio.h"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_linux_ALSA.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_linux_Bela.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_linux_JackAudio.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_linux_Midi.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_mac_CoreAudio.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_mac_CoreMidi.mm"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_MidiDataConcatenator.h"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_win32_ASIO.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_win32_DirectSound.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_win32_Midi.cpp"
    "../../../deps/juce/modules/juce_audio_devices/native/juce_win32_WASAPI.cpp"
    "../../../deps/juce/modules/juce_audio_devices/sources/juce_AudioSourcePlayer.cpp"
    "../../../deps/juce/modules/juce_audio_devices/sources/juce_AudioSourcePlayer.h"
    "../../../deps/juce/modules/juce_audio_devices/sources/juce_AudioTransportSource.cpp"
    "../../../deps/juce/modules/juce_audio_devices/sources/juce_AudioTransportSource.h"
    "../../../deps/juce/modules/juce_audio_devices/juce_audio_devices.cpp"
    "../../../deps/juce/modules/juce_audio_devices/juce_audio_devices.mm"
    "../../../deps/juce/modules/juce_audio_devices/juce_audio_devices.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/all.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/bitmath.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/bitreader.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/bitwriter.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/cpu.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/crc.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/fixed.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/float.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/format.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/lpc.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/md5.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/memory.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/metadata.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/stream_encoder.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/stream_encoder_framing.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/window.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/protected/all.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/protected/stream_decoder.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/protected/stream_encoder.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/bitmath.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/bitreader.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/bitwriter.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/cpu.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/crc.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/fixed.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/float.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/format.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/lpc_flac.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/md5.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/memory.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/stream_decoder.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/stream_encoder.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/stream_encoder_framing.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/window_flac.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/all.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/alloc.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/assert.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/callback.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/compat.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/endswap.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/export.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/Flac Licence.txt"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/format.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/metadata.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/ordinals.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/stream_decoder.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/stream_encoder.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/flac/win_utf8_io.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/books/coupled/res_books_51.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/books/coupled/res_books_stereo.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/books/floor/floor_books.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/books/uncoupled/res_books_uncoupled.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/floor_all.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/psych_8.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/psych_11.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/psych_16.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/psych_44.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/residue_8.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/residue_16.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/residue_44.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/residue_44p51.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/residue_44u.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_8.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_11.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_16.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_22.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_32.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_44.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_44p51.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_44u.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_X.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/analysis.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/backends.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/bitrate.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/bitrate.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/block.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/codebook.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/codebook.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/codec_internal.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/envelope.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/envelope.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/floor0.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/floor1.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/highlevel.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/info.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lookup.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lookup.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lookup_data.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lpc.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lpc.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lsp.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lsp.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/mapping0.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/masking.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/mdct.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/mdct.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/misc.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/misc.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/os.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/psy.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/psy.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/registry.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/registry.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/res0.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/scales.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/sharedbook.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/smallft.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/smallft.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/synthesis.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/vorbisenc.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/vorbisfile.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/window.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/window.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/README.md"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/bitwise.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/codec.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/config_types.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/crctable.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/framing.c"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/Ogg Vorbis Licence.txt"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/ogg.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/os_types.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/vorbisenc.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/vorbisfile.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_AiffAudioFormat.cpp"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_AiffAudioFormat.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_CoreAudioFormat.cpp"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_CoreAudioFormat.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_FlacAudioFormat.cpp"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_FlacAudioFormat.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_LAMEEncoderAudioFormat.cpp"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_LAMEEncoderAudioFormat.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_MP3AudioFormat.cpp"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_MP3AudioFormat.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_OggVorbisAudioFormat.cpp"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_OggVorbisAudioFormat.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_WavAudioFormat.cpp"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_WavAudioFormat.h"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_WindowsMediaAudioFormat.cpp"
    "../../../deps/juce/modules/juce_audio_formats/codecs/juce_WindowsMediaAudioFormat.h"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormat.cpp"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormat.h"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatManager.cpp"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatManager.h"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatReader.cpp"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatReader.h"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatReaderSource.cpp"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatReaderSource.h"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatWriter.cpp"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatWriter.h"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioSubsectionReader.cpp"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_AudioSubsectionReader.h"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_BufferingAudioFormatReader.cpp"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_BufferingAudioFormatReader.h"
    "../../../deps/juce/modules/juce_audio_formats/format/juce_MemoryMappedAudioFormatReader.h"
    "../../../deps/juce/modules/juce_audio_formats/sampler/juce_Sampler.cpp"
    "../../../deps/juce/modules/juce_audio_formats/sampler/juce_Sampler.h"
    "../../../deps/juce/modules/juce_audio_formats/juce_audio_formats.cpp"
    "../../../deps/juce/modules/juce_audio_formats/juce_audio_formats.mm"
    "../../../deps/juce/modules/juce_audio_formats/juce_audio_formats.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AAX/juce_AAX_Modifier_Injector.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AAX/juce_AAX_Wrapper.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUBase.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUBase.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUBaseHelper.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUBuffer.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUBuffer.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUCarbonViewBase.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUCarbonViewBase.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUCarbonViewControl.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUCarbonViewControl.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUCarbonViewDispatch.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUDispatch.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUDispatch.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUInputElement.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUInputElement.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUInputFormatConverter.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUMIDIBase.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUMIDIBase.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUOutputBase.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUOutputBase.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUOutputElement.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUOutputElement.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUPlugInDispatch.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUPlugInDispatch.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUScopeElement.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUScopeElement.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUSilentTimeout.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUTimestampGenerator.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUViewLocalizedStringKeys.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAtomic.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAtomicStack.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAudioChannelLayout.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAudioChannelLayout.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAUParameter.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAUParameter.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAutoDisposer.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CADebugMacros.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CADebugPrintf.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAException.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAHostTimeBase.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAMath.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAMutex.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAMutex.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CarbonEventHandler.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CarbonEventHandler.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAReferenceCounted.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAStreamBasicDescription.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAStreamBasicDescription.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAThreadSafeList.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAVectorUnit.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAVectorUnit.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAVectorUnitTypes.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAXException.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/ComponentBase.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/ComponentBase.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/MusicDeviceBase.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/MusicDeviceBase.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/juce_AU_Wrapper.mm"
    "../../../deps/juce/modules/juce_audio_plugin_client/AU/juce_AUv3_Wrapper.mm"
    "../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_DigiCode_Header.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_DigiCode1.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_DigiCode2.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_DigiCode3.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_MacUtilities.mm"
    "../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_WinUtilities.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_Wrapper.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/Standalone/juce_StandaloneFilterApp.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/Unity/juce_Unity_Wrapper.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/Unity/juce_UnityPluginInterface.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_CarbonVisibility.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_CheckSettingMacros.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_CreatePluginFilter.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_FakeMouseMoveGenerator.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_IncludeModuleHeaders.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_IncludeSystemHeaders.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_PluginHostType.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_PluginUtilities.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_WindowsHooks.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/VST/juce_VST_Wrapper.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/VST/juce_VST_Wrapper.mm"
    "../../../deps/juce/modules/juce_audio_plugin_client/VST/juce_VSTCallbackHandler.h"
    "../../../deps/juce/modules/juce_audio_plugin_client/VST3/juce_VST3_Wrapper.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_AAX.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_AAX.mm"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_AU_1.mm"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_AU_2.mm"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_AUv3.mm"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_1.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_2.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_3.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_4.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_utils.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_utils.mm"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_Standalone.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_Unity.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_utils.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST_utils.mm"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST2.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp"
    "../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client.h"
    "../../../deps/juce/modules/juce_audio_processors/format/juce_AudioPluginFormat.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format/juce_AudioPluginFormat.h"
    "../../../deps/juce/modules/juce_audio_processors/format/juce_AudioPluginFormatManager.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format/juce_AudioPluginFormatManager.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/baseiids.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/classfactoryhelpers.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fbuffer.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fbuffer.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fdebug.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fdebug.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fobject.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fobject.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fstreamer.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fstreamer.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fstring.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fstring.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/updatehandler.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/updatehandler.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/thread/include/flock.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/thread/source/flock.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/LICENSE.txt"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/README.md"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/conststringtable.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/conststringtable.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/coreiids.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/falignpop.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/falignpush.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/fplatform.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/fstrdefs.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ftypes.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/funknown.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/funknown.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/futils.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/fvariant.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ibstream.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/icloneable.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ipersistent.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ipluginbase.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/istringresult.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/iupdatehandler.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/smartpointer.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/typesizecheck.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ustring.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ustring.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/gui/iplugview.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/gui/iplugviewcontentscalesupport.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstattributes.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstaudioprocessor.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstautomationstate.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstchannelcontextinfo.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstcomponent.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstcontextmenu.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivsteditcontroller.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstevents.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivsthostapplication.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstinterappaudio.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstmessage.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstmidicontrollers.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstmidilearn.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstnoteexpression.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstparameterchanges.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstphysicalui.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstpluginterfacesupport.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstplugview.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstprefetchablesupport.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstprocesscontext.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstrepresentation.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivsttestplugprovider.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstunits.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/vstpshpack4.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/vstspeaker.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/vsttypes.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/LICENSE.txt"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/README.md"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/common/memorystream.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/common/memorystream.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/common/pluginview.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/common/pluginview.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/hosting/hostclasses.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/hosting/hostclasses.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/hosting/pluginterfacesupport.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/hosting/pluginterfacesupport.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstbus.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstbus.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstcomponent.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstcomponent.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstcomponentbase.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstcomponentbase.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vsteditcontroller.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vsteditcontroller.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstinitiids.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstparameters.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstparameters.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstpresetfile.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstpresetfile.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/LICENSE.txt"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/README.md"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt"
    "../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/README.md"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_AU_Shared.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_AudioUnitPluginFormat.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_AudioUnitPluginFormat.mm"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_LADSPAPluginFormat.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_LADSPAPluginFormat.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_LegacyAudioParameter.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_VST3Common.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_VST3Headers.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_VST3PluginFormat.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_VST3PluginFormat.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_VSTCommon.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_VSTMidiEventList.h"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_VSTPluginFormat.cpp"
    "../../../deps/juce/modules/juce_audio_processors/format_types/juce_VSTPluginFormat.h"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioPluginInstance.cpp"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioPluginInstance.h"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessor.cpp"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessor.h"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorEditor.cpp"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorEditor.h"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorGraph.cpp"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorGraph.h"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorListener.h"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorParameter.h"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorParameterGroup.cpp"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorParameterGroup.h"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_GenericAudioProcessorEditor.cpp"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_GenericAudioProcessorEditor.h"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_PluginDescription.cpp"
    "../../../deps/juce/modules/juce_audio_processors/processors/juce_PluginDescription.h"
    "../../../deps/juce/modules/juce_audio_processors/scanning/juce_KnownPluginList.cpp"
    "../../../deps/juce/modules/juce_audio_processors/scanning/juce_KnownPluginList.h"
    "../../../deps/juce/modules/juce_audio_processors/scanning/juce_PluginDirectoryScanner.cpp"
    "../../../deps/juce/modules/juce_audio_processors/scanning/juce_PluginDirectoryScanner.h"
    "../../../deps/juce/modules/juce_audio_processors/scanning/juce_PluginListComponent.cpp"
    "../../../deps/juce/modules/juce_audio_processors/scanning/juce_PluginListComponent.h"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterBool.cpp"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterBool.h"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterChoice.cpp"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterChoice.h"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterFloat.cpp"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterFloat.h"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterInt.cpp"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterInt.h"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioProcessorParameterWithID.cpp"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioProcessorParameterWithID.h"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.cpp"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.h"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_ParameterAttachments.cpp"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_ParameterAttachments.h"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_RangedAudioParameter.cpp"
    "../../../deps/juce/modules/juce_audio_processors/utilities/juce_RangedAudioParameter.h"
    "../../../deps/juce/modules/juce_audio_processors/juce_audio_processors.cpp"
    "../../../deps/juce/modules/juce_audio_processors/juce_audio_processors.mm"
    "../../../deps/juce/modules/juce_audio_processors/juce_audio_processors.h"
    "../../../deps/juce/modules/juce_audio_utils/audio_cd/juce_AudioCDBurner.h"
    "../../../deps/juce/modules/juce_audio_utils/audio_cd/juce_AudioCDReader.cpp"
    "../../../deps/juce/modules/juce_audio_utils/audio_cd/juce_AudioCDReader.h"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioAppComponent.cpp"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioAppComponent.h"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioDeviceSelectorComponent.cpp"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioDeviceSelectorComponent.h"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioThumbnail.cpp"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioThumbnail.h"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioThumbnailBase.h"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioThumbnailCache.cpp"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioThumbnailCache.h"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioVisualiserComponent.cpp"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioVisualiserComponent.h"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_BluetoothMidiDevicePairingDialogue.h"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_MidiKeyboardComponent.cpp"
    "../../../deps/juce/modules/juce_audio_utils/gui/juce_MidiKeyboardComponent.h"
    "../../../deps/juce/modules/juce_audio_utils/native/juce_android_BluetoothMidiDevicePairingDialogue.cpp"
    "../../../deps/juce/modules/juce_audio_utils/native/juce_ios_BluetoothMidiDevicePairingDialogue.mm"
    "../../../deps/juce/modules/juce_audio_utils/native/juce_linux_AudioCDReader.cpp"
    "../../../deps/juce/modules/juce_audio_utils/native/juce_linux_BluetoothMidiDevicePairingDialogue.cpp"
    "../../../deps/juce/modules/juce_audio_utils/native/juce_mac_AudioCDBurner.mm"
    "../../../deps/juce/modules/juce_audio_utils/native/juce_mac_AudioCDReader.mm"
    "../../../deps/juce/modules/juce_audio_utils/native/juce_mac_BluetoothMidiDevicePairingDialogue.mm"
    "../../../deps/juce/modules/juce_audio_utils/native/juce_win_BluetoothMidiDevicePairingDialogue.cpp"
    "../../../deps/juce/modules/juce_audio_utils/native/juce_win32_AudioCDBurner.cpp"
    "../../../deps/juce/modules/juce_audio_utils/native/juce_win32_AudioCDReader.cpp"
    "../../../deps/juce/modules/juce_audio_utils/players/juce_AudioProcessorPlayer.cpp"
    "../../../deps/juce/modules/juce_audio_utils/players/juce_AudioProcessorPlayer.h"
    "../../../deps/juce/modules/juce_audio_utils/players/juce_SoundPlayer.cpp"
    "../../../deps/juce/modules/juce_audio_utils/players/juce_SoundPlayer.h"
    "../../../deps/juce/modules/juce_audio_utils/juce_audio_utils.cpp"
    "../../../deps/juce/modules/juce_audio_utils/juce_audio_utils.mm"
    "../../../deps/juce/modules/juce_audio_utils/juce_audio_utils.h"
    "../../../deps/juce/modules/juce_core/containers/juce_AbstractFifo.cpp"
    "../../../deps/juce/modules/juce_core/containers/juce_AbstractFifo.h"
    "../../../deps/juce/modules/juce_core/containers/juce_Array.h"
    "../../../deps/juce/modules/juce_core/containers/juce_ArrayAllocationBase.h"
    "../../../deps/juce/modules/juce_core/containers/juce_ArrayBase.cpp"
    "../../../deps/juce/modules/juce_core/containers/juce_ArrayBase.h"
    "../../../deps/juce/modules/juce_core/containers/juce_DynamicObject.cpp"
    "../../../deps/juce/modules/juce_core/containers/juce_DynamicObject.h"
    "../../../deps/juce/modules/juce_core/containers/juce_ElementComparator.h"
    "../../../deps/juce/modules/juce_core/containers/juce_HashMap.h"
    "../../../deps/juce/modules/juce_core/containers/juce_HashMap_test.cpp"
    "../../../deps/juce/modules/juce_core/containers/juce_LinkedListPointer.h"
    "../../../deps/juce/modules/juce_core/containers/juce_ListenerList.h"
    "../../../deps/juce/modules/juce_core/containers/juce_NamedValueSet.cpp"
    "../../../deps/juce/modules/juce_core/containers/juce_NamedValueSet.h"
    "../../../deps/juce/modules/juce_core/containers/juce_OwnedArray.cpp"
    "../../../deps/juce/modules/juce_core/containers/juce_OwnedArray.h"
    "../../../deps/juce/modules/juce_core/containers/juce_PropertySet.cpp"
    "../../../deps/juce/modules/juce_core/containers/juce_PropertySet.h"
    "../../../deps/juce/modules/juce_core/containers/juce_ReferenceCountedArray.cpp"
    "../../../deps/juce/modules/juce_core/containers/juce_ReferenceCountedArray.h"
    "../../../deps/juce/modules/juce_core/containers/juce_ScopedValueSetter.h"
    "../../../deps/juce/modules/juce_core/containers/juce_SortedSet.h"
    "../../../deps/juce/modules/juce_core/containers/juce_SparseSet.cpp"
    "../../../deps/juce/modules/juce_core/containers/juce_SparseSet.h"
    "../../../deps/juce/modules/juce_core/containers/juce_Variant.cpp"
    "../../../deps/juce/modules/juce_core/containers/juce_Variant.h"
    "../../../deps/juce/modules/juce_core/files/juce_DirectoryIterator.cpp"
    "../../../deps/juce/modules/juce_core/files/juce_DirectoryIterator.h"
    "../../../deps/juce/modules/juce_core/files/juce_File.cpp"
    "../../../deps/juce/modules/juce_core/files/juce_File.h"
    "../../../deps/juce/modules/juce_core/files/juce_FileFilter.cpp"
    "../../../deps/juce/modules/juce_core/files/juce_FileFilter.h"
    "../../../deps/juce/modules/juce_core/files/juce_FileInputStream.cpp"
    "../../../deps/juce/modules/juce_core/files/juce_FileInputStream.h"
    "../../../deps/juce/modules/juce_core/files/juce_FileOutputStream.cpp"
    "../../../deps/juce/modules/juce_core/files/juce_FileOutputStream.h"
    "../../../deps/juce/modules/juce_core/files/juce_FileSearchPath.cpp"
    "../../../deps/juce/modules/juce_core/files/juce_FileSearchPath.h"
    "../../../deps/juce/modules/juce_core/files/juce_MemoryMappedFile.h"
    "../../../deps/juce/modules/juce_core/files/juce_RangedDirectoryIterator.cpp"
    "../../../deps/juce/modules/juce_core/files/juce_RangedDirectoryIterator.h"
    "../../../deps/juce/modules/juce_core/files/juce_TemporaryFile.cpp"
    "../../../deps/juce/modules/juce_core/files/juce_TemporaryFile.h"
    "../../../deps/juce/modules/juce_core/files/juce_WildcardFileFilter.cpp"
    "../../../deps/juce/modules/juce_core/files/juce_WildcardFileFilter.h"
    "../../../deps/juce/modules/juce_core/javascript/juce_Javascript.cpp"
    "../../../deps/juce/modules/juce_core/javascript/juce_Javascript.h"
    "../../../deps/juce/modules/juce_core/javascript/juce_JSON.cpp"
    "../../../deps/juce/modules/juce_core/javascript/juce_JSON.h"
    "../../../deps/juce/modules/juce_core/logging/juce_FileLogger.cpp"
    "../../../deps/juce/modules/juce_core/logging/juce_FileLogger.h"
    "../../../deps/juce/modules/juce_core/logging/juce_Logger.cpp"
    "../../../deps/juce/modules/juce_core/logging/juce_Logger.h"
    "../../../deps/juce/modules/juce_core/maths/juce_BigInteger.cpp"
    "../../../deps/juce/modules/juce_core/maths/juce_BigInteger.h"
    "../../../deps/juce/modules/juce_core/maths/juce_Expression.cpp"
    "../../../deps/juce/modules/juce_core/maths/juce_Expression.h"
    "../../../deps/juce/modules/juce_core/maths/juce_MathsFunctions.h"
    "../../../deps/juce/modules/juce_core/maths/juce_NormalisableRange.h"
    "../../../deps/juce/modules/juce_core/maths/juce_Random.cpp"
    "../../../deps/juce/modules/juce_core/maths/juce_Random.h"
    "../../../deps/juce/modules/juce_core/maths/juce_Range.h"
    "../../../deps/juce/modules/juce_core/maths/juce_StatisticsAccumulator.h"
    "../../../deps/juce/modules/juce_core/memory/juce_AllocationHooks.cpp"
    "../../../deps/juce/modules/juce_core/memory/juce_AllocationHooks.h"
    "../../../deps/juce/modules/juce_core/memory/juce_Atomic.h"
    "../../../deps/juce/modules/juce_core/memory/juce_ByteOrder.h"
    "../../../deps/juce/modules/juce_core/memory/juce_ContainerDeletePolicy.h"
    "../../../deps/juce/modules/juce_core/memory/juce_HeapBlock.h"
    "../../../deps/juce/modules/juce_core/memory/juce_HeavyweightLeakedObjectDetector.h"
    "../../../deps/juce/modules/juce_core/memory/juce_LeakedObjectDetector.h"
    "../../../deps/juce/modules/juce_core/memory/juce_Memory.h"
    "../../../deps/juce/modules/juce_core/memory/juce_MemoryBlock.cpp"
    "../../../deps/juce/modules/juce_core/memory/juce_MemoryBlock.h"
    "../../../deps/juce/modules/juce_core/memory/juce_OptionalScopedPointer.h"
    "../../../deps/juce/modules/juce_core/memory/juce_ReferenceCountedObject.h"
    "../../../deps/juce/modules/juce_core/memory/juce_ScopedPointer.h"
    "../../../deps/juce/modules/juce_core/memory/juce_SharedResourcePointer.h"
    "../../../deps/juce/modules/juce_core/memory/juce_Singleton.h"
    "../../../deps/juce/modules/juce_core/memory/juce_WeakReference.h"
    "../../../deps/juce/modules/juce_core/misc/juce_ConsoleApplication.cpp"
    "../../../deps/juce/modules/juce_core/misc/juce_ConsoleApplication.h"
    "../../../deps/juce/modules/juce_core/misc/juce_Result.cpp"
    "../../../deps/juce/modules/juce_core/misc/juce_Result.h"
    "../../../deps/juce/modules/juce_core/misc/juce_RuntimePermissions.cpp"
    "../../../deps/juce/modules/juce_core/misc/juce_RuntimePermissions.h"
    "../../../deps/juce/modules/juce_core/misc/juce_Uuid.cpp"
    "../../../deps/juce/modules/juce_core/misc/juce_Uuid.h"
    "../../../deps/juce/modules/juce_core/misc/juce_WindowsRegistry.h"
    "../../../deps/juce/modules/juce_core/native/java/README.txt"
    "../../../deps/juce/modules/juce_core/native/juce_android_Files.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_android_JNIHelpers.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_android_JNIHelpers.h"
    "../../../deps/juce/modules/juce_core/native/juce_android_Misc.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_android_Network.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_android_RuntimePermissions.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_android_SystemStats.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_android_Threads.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_BasicNativeHeaders.h"
    "../../../deps/juce/modules/juce_core/native/juce_curl_Network.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_linux_CommonFile.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_linux_Files.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_linux_Network.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_linux_SystemStats.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_linux_Threads.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_mac_ClangBugWorkaround.h"
    "../../../deps/juce/modules/juce_core/native/juce_mac_Files.mm"
    "../../../deps/juce/modules/juce_core/native/juce_mac_Network.mm"
    "../../../deps/juce/modules/juce_core/native/juce_mac_Strings.mm"
    "../../../deps/juce/modules/juce_core/native/juce_mac_SystemStats.mm"
    "../../../deps/juce/modules/juce_core/native/juce_mac_Threads.mm"
    "../../../deps/juce/modules/juce_core/native/juce_osx_ObjCHelpers.h"
    "../../../deps/juce/modules/juce_core/native/juce_posix_IPAddress.h"
    "../../../deps/juce/modules/juce_core/native/juce_posix_NamedPipe.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_posix_SharedCode.h"
    "../../../deps/juce/modules/juce_core/native/juce_wasm_SystemStats.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_win32_ComSmartPtr.h"
    "../../../deps/juce/modules/juce_core/native/juce_win32_Files.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_win32_Network.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_win32_Registry.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_win32_SystemStats.cpp"
    "../../../deps/juce/modules/juce_core/native/juce_win32_Threads.cpp"
    "../../../deps/juce/modules/juce_core/network/juce_IPAddress.cpp"
    "../../../deps/juce/modules/juce_core/network/juce_IPAddress.h"
    "../../../deps/juce/modules/juce_core/network/juce_MACAddress.cpp"
    "../../../deps/juce/modules/juce_core/network/juce_MACAddress.h"
    "../../../deps/juce/modules/juce_core/network/juce_NamedPipe.cpp"
    "../../../deps/juce/modules/juce_core/network/juce_NamedPipe.h"
    "../../../deps/juce/modules/juce_core/network/juce_Socket.cpp"
    "../../../deps/juce/modules/juce_core/network/juce_Socket.h"
    "../../../deps/juce/modules/juce_core/network/juce_URL.cpp"
    "../../../deps/juce/modules/juce_core/network/juce_URL.h"
    "../../../deps/juce/modules/juce_core/network/juce_WebInputStream.cpp"
    "../../../deps/juce/modules/juce_core/network/juce_WebInputStream.h"
    "../../../deps/juce/modules/juce_core/streams/juce_BufferedInputStream.cpp"
    "../../../deps/juce/modules/juce_core/streams/juce_BufferedInputStream.h"
    "../../../deps/juce/modules/juce_core/streams/juce_FileInputSource.cpp"
    "../../../deps/juce/modules/juce_core/streams/juce_FileInputSource.h"
    "../../../deps/juce/modules/juce_core/streams/juce_InputSource.h"
    "../../../deps/juce/modules/juce_core/streams/juce_InputStream.cpp"
    "../../../deps/juce/modules/juce_core/streams/juce_InputStream.h"
    "../../../deps/juce/modules/juce_core/streams/juce_MemoryInputStream.cpp"
    "../../../deps/juce/modules/juce_core/streams/juce_MemoryInputStream.h"
    "../../../deps/juce/modules/juce_core/streams/juce_MemoryOutputStream.cpp"
    "../../../deps/juce/modules/juce_core/streams/juce_MemoryOutputStream.h"
    "../../../deps/juce/modules/juce_core/streams/juce_OutputStream.cpp"
    "../../../deps/juce/modules/juce_core/streams/juce_OutputStream.h"
    "../../../deps/juce/modules/juce_core/streams/juce_SubregionStream.cpp"
    "../../../deps/juce/modules/juce_core/streams/juce_SubregionStream.h"
    "../../../deps/juce/modules/juce_core/streams/juce_URLInputSource.cpp"
    "../../../deps/juce/modules/juce_core/streams/juce_URLInputSource.h"
    "../../../deps/juce/modules/juce_core/system/juce_CompilerSupport.h"
    "../../../deps/juce/modules/juce_core/system/juce_CompilerWarnings.h"
    "../../../deps/juce/modules/juce_core/system/juce_PlatformDefs.h"
    "../../../deps/juce/modules/juce_core/system/juce_StandardHeader.h"
    "../../../deps/juce/modules/juce_core/system/juce_SystemStats.cpp"
    "../../../deps/juce/modules/juce_core/system/juce_SystemStats.h"
    "../../../deps/juce/modules/juce_core/system/juce_TargetPlatform.h"
    "../../../deps/juce/modules/juce_core/text/juce_Base64.cpp"
    "../../../deps/juce/modules/juce_core/text/juce_Base64.h"
    "../../../deps/juce/modules/juce_core/text/juce_CharacterFunctions.cpp"
    "../../../deps/juce/modules/juce_core/text/juce_CharacterFunctions.h"
    "../../../deps/juce/modules/juce_core/text/juce_CharPointer_ASCII.h"
    "../../../deps/juce/modules/juce_core/text/juce_CharPointer_UTF8.h"
    "../../../deps/juce/modules/juce_core/text/juce_CharPointer_UTF16.h"
    "../../../deps/juce/modules/juce_core/text/juce_CharPointer_UTF32.h"
    "../../../deps/juce/modules/juce_core/text/juce_Identifier.cpp"
    "../../../deps/juce/modules/juce_core/text/juce_Identifier.h"
    "../../../deps/juce/modules/juce_core/text/juce_LocalisedStrings.cpp"
    "../../../deps/juce/modules/juce_core/text/juce_LocalisedStrings.h"
    "../../../deps/juce/modules/juce_core/text/juce_NewLine.h"
    "../../../deps/juce/modules/juce_core/text/juce_String.cpp"
    "../../../deps/juce/modules/juce_core/text/juce_String.h"
    "../../../deps/juce/modules/juce_core/text/juce_StringArray.cpp"
    "../../../deps/juce/modules/juce_core/text/juce_StringArray.h"
    "../../../deps/juce/modules/juce_core/text/juce_StringPairArray.cpp"
    "../../../deps/juce/modules/juce_core/text/juce_StringPairArray.h"
    "../../../deps/juce/modules/juce_core/text/juce_StringPool.cpp"
    "../../../deps/juce/modules/juce_core/text/juce_StringPool.h"
    "../../../deps/juce/modules/juce_core/text/juce_StringRef.h"
    "../../../deps/juce/modules/juce_core/text/juce_TextDiff.cpp"
    "../../../deps/juce/modules/juce_core/text/juce_TextDiff.h"
    "../../../deps/juce/modules/juce_core/threads/juce_ChildProcess.cpp"
    "../../../deps/juce/modules/juce_core/threads/juce_ChildProcess.h"
    "../../../deps/juce/modules/juce_core/threads/juce_CriticalSection.h"
    "../../../deps/juce/modules/juce_core/threads/juce_DynamicLibrary.h"
    "../../../deps/juce/modules/juce_core/threads/juce_HighResolutionTimer.cpp"
    "../../../deps/juce/modules/juce_core/threads/juce_HighResolutionTimer.h"
    "../../../deps/juce/modules/juce_core/threads/juce_InterProcessLock.h"
    "../../../deps/juce/modules/juce_core/threads/juce_Process.h"
    "../../../deps/juce/modules/juce_core/threads/juce_ReadWriteLock.cpp"
    "../../../deps/juce/modules/juce_core/threads/juce_ReadWriteLock.h"
    "../../../deps/juce/modules/juce_core/threads/juce_ScopedLock.h"
    "../../../deps/juce/modules/juce_core/threads/juce_ScopedReadLock.h"
    "../../../deps/juce/modules/juce_core/threads/juce_ScopedWriteLock.h"
    "../../../deps/juce/modules/juce_core/threads/juce_SpinLock.h"
    "../../../deps/juce/modules/juce_core/threads/juce_Thread.cpp"
    "../../../deps/juce/modules/juce_core/threads/juce_Thread.h"
    "../../../deps/juce/modules/juce_core/threads/juce_ThreadLocalValue.h"
    "../../../deps/juce/modules/juce_core/threads/juce_ThreadPool.cpp"
    "../../../deps/juce/modules/juce_core/threads/juce_ThreadPool.h"
    "../../../deps/juce/modules/juce_core/threads/juce_TimeSliceThread.cpp"
    "../../../deps/juce/modules/juce_core/threads/juce_TimeSliceThread.h"
    "../../../deps/juce/modules/juce_core/threads/juce_WaitableEvent.cpp"
    "../../../deps/juce/modules/juce_core/threads/juce_WaitableEvent.h"
    "../../../deps/juce/modules/juce_core/time/juce_PerformanceCounter.cpp"
    "../../../deps/juce/modules/juce_core/time/juce_PerformanceCounter.h"
    "../../../deps/juce/modules/juce_core/time/juce_RelativeTime.cpp"
    "../../../deps/juce/modules/juce_core/time/juce_RelativeTime.h"
    "../../../deps/juce/modules/juce_core/time/juce_Time.cpp"
    "../../../deps/juce/modules/juce_core/time/juce_Time.h"
    "../../../deps/juce/modules/juce_core/unit_tests/juce_UnitTest.cpp"
    "../../../deps/juce/modules/juce_core/unit_tests/juce_UnitTest.h"
    "../../../deps/juce/modules/juce_core/unit_tests/juce_UnitTestCategories.h"
    "../../../deps/juce/modules/juce_core/xml/juce_XmlDocument.cpp"
    "../../../deps/juce/modules/juce_core/xml/juce_XmlDocument.h"
    "../../../deps/juce/modules/juce_core/xml/juce_XmlElement.cpp"
    "../../../deps/juce/modules/juce_core/xml/juce_XmlElement.h"
    "../../../deps/juce/modules/juce_core/zip/zlib/adler32.c"
    "../../../deps/juce/modules/juce_core/zip/zlib/compress.c"
    "../../../deps/juce/modules/juce_core/zip/zlib/crc32.c"
    "../../../deps/juce/modules/juce_core/zip/zlib/crc32.h"
    "../../../deps/juce/modules/juce_core/zip/zlib/deflate.c"
    "../../../deps/juce/modules/juce_core/zip/zlib/deflate.h"
    "../../../deps/juce/modules/juce_core/zip/zlib/infback.c"
    "../../../deps/juce/modules/juce_core/zip/zlib/inffast.c"
    "../../../deps/juce/modules/juce_core/zip/zlib/inffast.h"
    "../../../deps/juce/modules/juce_core/zip/zlib/inffixed.h"
    "../../../deps/juce/modules/juce_core/zip/zlib/inflate.c"
    "../../../deps/juce/modules/juce_core/zip/zlib/inflate.h"
    "../../../deps/juce/modules/juce_core/zip/zlib/inftrees.c"
    "../../../deps/juce/modules/juce_core/zip/zlib/inftrees.h"
    "../../../deps/juce/modules/juce_core/zip/zlib/trees.c"
    "../../../deps/juce/modules/juce_core/zip/zlib/trees.h"
    "../../../deps/juce/modules/juce_core/zip/zlib/uncompr.c"
    "../../../deps/juce/modules/juce_core/zip/zlib/zconf.h"
    "../../../deps/juce/modules/juce_core/zip/zlib/zconf.in.h"
    "../../../deps/juce/modules/juce_core/zip/zlib/zlib.h"
    "../../../deps/juce/modules/juce_core/zip/zlib/zutil.c"
    "../../../deps/juce/modules/juce_core/zip/zlib/zutil.h"
    "../../../deps/juce/modules/juce_core/zip/juce_GZIPCompressorOutputStream.cpp"
    "../../../deps/juce/modules/juce_core/zip/juce_GZIPCompressorOutputStream.h"
    "../../../deps/juce/modules/juce_core/zip/juce_GZIPDecompressorInputStream.cpp"
    "../../../deps/juce/modules/juce_core/zip/juce_GZIPDecompressorInputStream.h"
    "../../../deps/juce/modules/juce_core/zip/juce_ZipFile.cpp"
    "../../../deps/juce/modules/juce_core/zip/juce_ZipFile.h"
    "../../../deps/juce/modules/juce_core/juce_core.cpp"
    "../../../deps/juce/modules/juce_core/juce_core.mm"
    "../../../deps/juce/modules/juce_core/juce_core.h"
    "../../../deps/juce/modules/juce_cryptography/encryption/juce_BlowFish.cpp"
    "../../../deps/juce/modules/juce_cryptography/encryption/juce_BlowFish.h"
    "../../../deps/juce/modules/juce_cryptography/encryption/juce_Primes.cpp"
    "../../../deps/juce/modules/juce_cryptography/encryption/juce_Primes.h"
    "../../../deps/juce/modules/juce_cryptography/encryption/juce_RSAKey.cpp"
    "../../../deps/juce/modules/juce_cryptography/encryption/juce_RSAKey.h"
    "../../../deps/juce/modules/juce_cryptography/hashing/juce_MD5.cpp"
    "../../../deps/juce/modules/juce_cryptography/hashing/juce_MD5.h"
    "../../../deps/juce/modules/juce_cryptography/hashing/juce_SHA256.cpp"
    "../../../deps/juce/modules/juce_cryptography/hashing/juce_SHA256.h"
    "../../../deps/juce/modules/juce_cryptography/hashing/juce_Whirlpool.cpp"
    "../../../deps/juce/modules/juce_cryptography/hashing/juce_Whirlpool.h"
    "../../../deps/juce/modules/juce_cryptography/juce_cryptography.cpp"
    "../../../deps/juce/modules/juce_cryptography/juce_cryptography.mm"
    "../../../deps/juce/modules/juce_cryptography/juce_cryptography.h"
    "../../../deps/juce/modules/juce_data_structures/app_properties/juce_ApplicationProperties.cpp"
    "../../../deps/juce/modules/juce_data_structures/app_properties/juce_ApplicationProperties.h"
    "../../../deps/juce/modules/juce_data_structures/app_properties/juce_PropertiesFile.cpp"
    "../../../deps/juce/modules/juce_data_structures/app_properties/juce_PropertiesFile.h"
    "../../../deps/juce/modules/juce_data_structures/undomanager/juce_UndoableAction.h"
    "../../../deps/juce/modules/juce_data_structures/undomanager/juce_UndoManager.cpp"
    "../../../deps/juce/modules/juce_data_structures/undomanager/juce_UndoManager.h"
    "../../../deps/juce/modules/juce_data_structures/values/juce_CachedValue.cpp"
    "../../../deps/juce/modules/juce_data_structures/values/juce_CachedValue.h"
    "../../../deps/juce/modules/juce_data_structures/values/juce_Value.cpp"
    "../../../deps/juce/modules/juce_data_structures/values/juce_Value.h"
    "../../../deps/juce/modules/juce_data_structures/values/juce_ValueTree.cpp"
    "../../../deps/juce/modules/juce_data_structures/values/juce_ValueTree.h"
    "../../../deps/juce/modules/juce_data_structures/values/juce_ValueTreeSynchroniser.cpp"
    "../../../deps/juce/modules/juce_data_structures/values/juce_ValueTreeSynchroniser.h"
    "../../../deps/juce/modules/juce_data_structures/values/juce_ValueWithDefault.cpp"
    "../../../deps/juce/modules/juce_data_structures/values/juce_ValueWithDefault.h"
    "../../../deps/juce/modules/juce_data_structures/juce_data_structures.cpp"
    "../../../deps/juce/modules/juce_data_structures/juce_data_structures.mm"
    "../../../deps/juce/modules/juce_data_structures/juce_data_structures.h"
    "../../../deps/juce/modules/juce_dsp/containers/juce_AudioBlock.h"
    "../../../deps/juce/modules/juce_dsp/containers/juce_AudioBlock_test.cpp"
    "../../../deps/juce/modules/juce_dsp/containers/juce_FixedSizeFunction.h"
    "../../../deps/juce/modules/juce_dsp/containers/juce_FixedSizeFunction_test.cpp"
    "../../../deps/juce/modules/juce_dsp/containers/juce_SIMDRegister.h"
    "../../../deps/juce/modules/juce_dsp/containers/juce_SIMDRegister_Impl.h"
    "../../../deps/juce/modules/juce_dsp/containers/juce_SIMDRegister_test.cpp"
    "../../../deps/juce/modules/juce_dsp/filter_design/juce_FilterDesign.cpp"
    "../../../deps/juce/modules/juce_dsp/filter_design/juce_FilterDesign.h"
    "../../../deps/juce/modules/juce_dsp/frequency/juce_Convolution.cpp"
    "../../../deps/juce/modules/juce_dsp/frequency/juce_Convolution.h"
    "../../../deps/juce/modules/juce_dsp/frequency/juce_Convolution_test.cpp"
    "../../../deps/juce/modules/juce_dsp/frequency/juce_FFT.cpp"
    "../../../deps/juce/modules/juce_dsp/frequency/juce_FFT.h"
    "../../../deps/juce/modules/juce_dsp/frequency/juce_FFT_test.cpp"
    "../../../deps/juce/modules/juce_dsp/frequency/juce_Windowing.cpp"
    "../../../deps/juce/modules/juce_dsp/frequency/juce_Windowing.h"
    "../../../deps/juce/modules/juce_dsp/maths/juce_FastMathApproximations.h"
    "../../../deps/juce/modules/juce_dsp/maths/juce_LogRampedValue.h"
    "../../../deps/juce/modules/juce_dsp/maths/juce_LogRampedValue_test.cpp"
    "../../../deps/juce/modules/juce_dsp/maths/juce_LookupTable.cpp"
    "../../../deps/juce/modules/juce_dsp/maths/juce_LookupTable.h"
    "../../../deps/juce/modules/juce_dsp/maths/juce_Matrix.cpp"
    "../../../deps/juce/modules/juce_dsp/maths/juce_Matrix.h"
    "../../../deps/juce/modules/juce_dsp/maths/juce_Matrix_test.cpp"
    "../../../deps/juce/modules/juce_dsp/maths/juce_Phase.h"
    "../../../deps/juce/modules/juce_dsp/maths/juce_Polynomial.h"
    "../../../deps/juce/modules/juce_dsp/maths/juce_SpecialFunctions.cpp"
    "../../../deps/juce/modules/juce_dsp/maths/juce_SpecialFunctions.h"
    "../../../deps/juce/modules/juce_dsp/native/juce_avx_SIMDNativeOps.cpp"
    "../../../deps/juce/modules/juce_dsp/native/juce_avx_SIMDNativeOps.h"
    "../../../deps/juce/modules/juce_dsp/native/juce_fallback_SIMDNativeOps.h"
    "../../../deps/juce/modules/juce_dsp/native/juce_neon_SIMDNativeOps.cpp"
    "../../../deps/juce/modules/juce_dsp/native/juce_neon_SIMDNativeOps.h"
    "../../../deps/juce/modules/juce_dsp/native/juce_sse_SIMDNativeOps.cpp"
    "../../../deps/juce/modules/juce_dsp/native/juce_sse_SIMDNativeOps.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_BallisticsFilter.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_BallisticsFilter.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_DelayLine.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_DelayLine.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_DryWetMixer.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_DryWetMixer.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_FIRFilter.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_FIRFilter.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_FIRFilter_test.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_FirstOrderTPTFilter.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_FirstOrderTPTFilter.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_IIRFilter.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_IIRFilter.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_IIRFilter_Impl.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_LinkwitzRileyFilter.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_LinkwitzRileyFilter.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_Oversampling.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_Oversampling.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_Panner.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_Panner.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_ProcessContext.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_ProcessorChain.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_ProcessorChain_test.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_ProcessorDuplicator.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_ProcessorWrapper.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_StateVariableFilter.h"
    "../../../deps/juce/modules/juce_dsp/processors/juce_StateVariableTPTFilter.cpp"
    "../../../deps/juce/modules/juce_dsp/processors/juce_StateVariableTPTFilter.h"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Bias.h"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Chorus.cpp"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Chorus.h"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Compressor.cpp"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Compressor.h"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Gain.h"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_LadderFilter.cpp"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_LadderFilter.h"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Limiter.cpp"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Limiter.h"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_NoiseGate.cpp"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_NoiseGate.h"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Oscillator.h"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Phaser.cpp"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Phaser.h"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_Reverb.h"
    "../../../deps/juce/modules/juce_dsp/widgets/juce_WaveShaper.h"
    "../../../deps/juce/modules/juce_dsp/juce_dsp.cpp"
    "../../../deps/juce/modules/juce_dsp/juce_dsp.mm"
    "../../../deps/juce/modules/juce_dsp/juce_dsp.h"
    "../../../deps/juce/modules/juce_events/broadcasters/juce_ActionBroadcaster.cpp"
    "../../../deps/juce/modules/juce_events/broadcasters/juce_ActionBroadcaster.h"
    "../../../deps/juce/modules/juce_events/broadcasters/juce_ActionListener.h"
    "../../../deps/juce/modules/juce_events/broadcasters/juce_AsyncUpdater.cpp"
    "../../../deps/juce/modules/juce_events/broadcasters/juce_AsyncUpdater.h"
    "../../../deps/juce/modules/juce_events/broadcasters/juce_ChangeBroadcaster.cpp"
    "../../../deps/juce/modules/juce_events/broadcasters/juce_ChangeBroadcaster.h"
    "../../../deps/juce/modules/juce_events/broadcasters/juce_ChangeListener.h"
    "../../../deps/juce/modules/juce_events/interprocess/juce_ConnectedChildProcess.cpp"
    "../../../deps/juce/modules/juce_events/interprocess/juce_ConnectedChildProcess.h"
    "../../../deps/juce/modules/juce_events/interprocess/juce_InterprocessConnection.cpp"
    "../../../deps/juce/modules/juce_events/interprocess/juce_InterprocessConnection.h"
    "../../../deps/juce/modules/juce_events/interprocess/juce_InterprocessConnectionServer.cpp"
    "../../../deps/juce/modules/juce_events/interprocess/juce_InterprocessConnectionServer.h"
    "../../../deps/juce/modules/juce_events/interprocess/juce_NetworkServiceDiscovery.cpp"
    "../../../deps/juce/modules/juce_events/interprocess/juce_NetworkServiceDiscovery.h"
    "../../../deps/juce/modules/juce_events/messages/juce_ApplicationBase.cpp"
    "../../../deps/juce/modules/juce_events/messages/juce_ApplicationBase.h"
    "../../../deps/juce/modules/juce_events/messages/juce_CallbackMessage.h"
    "../../../deps/juce/modules/juce_events/messages/juce_DeletedAtShutdown.cpp"
    "../../../deps/juce/modules/juce_events/messages/juce_DeletedAtShutdown.h"
    "../../../deps/juce/modules/juce_events/messages/juce_Initialisation.h"
    "../../../deps/juce/modules/juce_events/messages/juce_Message.h"
    "../../../deps/juce/modules/juce_events/messages/juce_MessageListener.cpp"
    "../../../deps/juce/modules/juce_events/messages/juce_MessageListener.h"
    "../../../deps/juce/modules/juce_events/messages/juce_MessageManager.cpp"
    "../../../deps/juce/modules/juce_events/messages/juce_MessageManager.h"
    "../../../deps/juce/modules/juce_events/messages/juce_MountedVolumeListChangeDetector.h"
    "../../../deps/juce/modules/juce_events/messages/juce_NotificationType.h"
    "../../../deps/juce/modules/juce_events/native/juce_android_Messaging.cpp"
    "../../../deps/juce/modules/juce_events/native/juce_ios_MessageManager.mm"
    "../../../deps/juce/modules/juce_events/native/juce_linux_EventLoop.h"
    "../../../deps/juce/modules/juce_events/native/juce_linux_Messaging.cpp"
    "../../../deps/juce/modules/juce_events/native/juce_mac_MessageManager.mm"
    "../../../deps/juce/modules/juce_events/native/juce_osx_MessageQueue.h"
    "../../../deps/juce/modules/juce_events/native/juce_win32_HiddenMessageWindow.h"
    "../../../deps/juce/modules/juce_events/native/juce_win32_Messaging.cpp"
    "../../../deps/juce/modules/juce_events/native/juce_win32_WinRTWrapper.cpp"
    "../../../deps/juce/modules/juce_events/native/juce_win32_WinRTWrapper.h"
    "../../../deps/juce/modules/juce_events/timers/juce_MultiTimer.cpp"
    "../../../deps/juce/modules/juce_events/timers/juce_MultiTimer.h"
    "../../../deps/juce/modules/juce_events/timers/juce_Timer.cpp"
    "../../../deps/juce/modules/juce_events/timers/juce_Timer.h"
    "../../../deps/juce/modules/juce_events/juce_events.cpp"
    "../../../deps/juce/modules/juce_events/juce_events.mm"
    "../../../deps/juce/modules/juce_events/juce_events.h"
    "../../../deps/juce/modules/juce_graphics/colour/juce_Colour.cpp"
    "../../../deps/juce/modules/juce_graphics/colour/juce_Colour.h"
    "../../../deps/juce/modules/juce_graphics/colour/juce_ColourGradient.cpp"
    "../../../deps/juce/modules/juce_graphics/colour/juce_ColourGradient.h"
    "../../../deps/juce/modules/juce_graphics/colour/juce_Colours.cpp"
    "../../../deps/juce/modules/juce_graphics/colour/juce_Colours.h"
    "../../../deps/juce/modules/juce_graphics/colour/juce_FillType.cpp"
    "../../../deps/juce/modules/juce_graphics/colour/juce_FillType.h"
    "../../../deps/juce/modules/juce_graphics/colour/juce_PixelFormats.h"
    "../../../deps/juce/modules/juce_graphics/contexts/juce_GraphicsContext.cpp"
    "../../../deps/juce/modules/juce_graphics/contexts/juce_GraphicsContext.h"
    "../../../deps/juce/modules/juce_graphics/contexts/juce_LowLevelGraphicsContext.h"
    "../../../deps/juce/modules/juce_graphics/contexts/juce_LowLevelGraphicsPostScriptRenderer.cpp"
    "../../../deps/juce/modules/juce_graphics/contexts/juce_LowLevelGraphicsPostScriptRenderer.h"
    "../../../deps/juce/modules/juce_graphics/contexts/juce_LowLevelGraphicsSoftwareRenderer.cpp"
    "../../../deps/juce/modules/juce_graphics/contexts/juce_LowLevelGraphicsSoftwareRenderer.h"
    "../../../deps/juce/modules/juce_graphics/effects/juce_DropShadowEffect.cpp"
    "../../../deps/juce/modules/juce_graphics/effects/juce_DropShadowEffect.h"
    "../../../deps/juce/modules/juce_graphics/effects/juce_GlowEffect.cpp"
    "../../../deps/juce/modules/juce_graphics/effects/juce_GlowEffect.h"
    "../../../deps/juce/modules/juce_graphics/effects/juce_ImageEffectFilter.h"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_AttributedString.cpp"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_AttributedString.h"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_CustomTypeface.cpp"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_CustomTypeface.h"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_Font.cpp"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_Font.h"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_GlyphArrangement.cpp"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_GlyphArrangement.h"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_TextLayout.cpp"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_TextLayout.h"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_Typeface.cpp"
    "../../../deps/juce/modules/juce_graphics/fonts/juce_Typeface.h"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_AffineTransform.cpp"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_AffineTransform.h"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_BorderSize.h"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_EdgeTable.cpp"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_EdgeTable.h"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_Line.h"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_Parallelogram.h"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_Path.cpp"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_Path.h"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_PathIterator.cpp"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_PathIterator.h"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_PathStrokeType.cpp"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_PathStrokeType.h"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_Point.h"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_Rectangle.h"
    "../../../deps/juce/modules/juce_graphics/geometry/juce_RectangleList.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/cderror.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/changes to libjpeg for JUCE.txt"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcapimin.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcapistd.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jccoefct.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jccolor.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcdctmgr.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jchuff.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jchuff.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcinit.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcmainct.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcmarker.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcmaster.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcomapi.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jconfig.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcparam.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcphuff.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcprepct.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcsample.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jctrans.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdapimin.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdapistd.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdatasrc.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdcoefct.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdcolor.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdct.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jddctmgr.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdhuff.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdhuff.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdinput.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdmainct.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdmarker.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdmaster.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdmerge.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdphuff.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdpostct.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdsample.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdtrans.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jerror.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jerror.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jfdctflt.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jfdctfst.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jfdctint.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jidctflt.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jidctfst.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jidctint.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jidctred.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jinclude.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jmemmgr.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jmemnobs.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jmemsys.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jmorecfg.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jpegint.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jpeglib.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jquant1.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jquant2.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jutils.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jversion.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/transupp.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/jpglib/transupp.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/libpng_readme.txt"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/png.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/png.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngconf.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngdebug.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngerror.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngget.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pnginfo.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngmem.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngpread.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngpriv.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngread.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngrio.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngrtran.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngrutil.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngset.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngstruct.h"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngtrans.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngwio.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngwrite.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngwtran.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngwutil.c"
    "../../../deps/juce/modules/juce_graphics/image_formats/juce_GIFLoader.cpp"
    "../../../deps/juce/modules/juce_graphics/image_formats/juce_JPEGLoader.cpp"
    "../../../deps/juce/modules/juce_graphics/image_formats/juce_PNGLoader.cpp"
    "../../../deps/juce/modules/juce_graphics/images/juce_Image.cpp"
    "../../../deps/juce/modules/juce_graphics/images/juce_Image.h"
    "../../../deps/juce/modules/juce_graphics/images/juce_ImageCache.cpp"
    "../../../deps/juce/modules/juce_graphics/images/juce_ImageCache.h"
    "../../../deps/juce/modules/juce_graphics/images/juce_ImageConvolutionKernel.cpp"
    "../../../deps/juce/modules/juce_graphics/images/juce_ImageConvolutionKernel.h"
    "../../../deps/juce/modules/juce_graphics/images/juce_ImageFileFormat.cpp"
    "../../../deps/juce/modules/juce_graphics/images/juce_ImageFileFormat.h"
    "../../../deps/juce/modules/juce_graphics/native/juce_android_Fonts.cpp"
    "../../../deps/juce/modules/juce_graphics/native/juce_android_GraphicsContext.cpp"
    "../../../deps/juce/modules/juce_graphics/native/juce_android_IconHelpers.cpp"
    "../../../deps/juce/modules/juce_graphics/native/juce_freetype_Fonts.cpp"
    "../../../deps/juce/modules/juce_graphics/native/juce_linux_Fonts.cpp"
    "../../../deps/juce/modules/juce_graphics/native/juce_linux_IconHelpers.cpp"
    "../../../deps/juce/modules/juce_graphics/native/juce_mac_CoreGraphicsContext.h"
    "../../../deps/juce/modules/juce_graphics/native/juce_mac_CoreGraphicsContext.mm"
    "../../../deps/juce/modules/juce_graphics/native/juce_mac_CoreGraphicsHelpers.h"
    "../../../deps/juce/modules/juce_graphics/native/juce_mac_Fonts.mm"
    "../../../deps/juce/modules/juce_graphics/native/juce_mac_IconHelpers.cpp"
    "../../../deps/juce/modules/juce_graphics/native/juce_RenderingHelpers.h"
    "../../../deps/juce/modules/juce_graphics/native/juce_win32_Direct2DGraphicsContext.cpp"
    "../../../deps/juce/modules/juce_graphics/native/juce_win32_Direct2DGraphicsContext.h"
    "../../../deps/juce/modules/juce_graphics/native/juce_win32_DirectWriteTypeface.cpp"
    "../../../deps/juce/modules/juce_graphics/native/juce_win32_DirectWriteTypeLayout.cpp"
    "../../../deps/juce/modules/juce_graphics/native/juce_win32_Fonts.cpp"
    "../../../deps/juce/modules/juce_graphics/native/juce_win32_IconHelpers.cpp"
    "../../../deps/juce/modules/juce_graphics/placement/juce_Justification.h"
    "../../../deps/juce/modules/juce_graphics/placement/juce_RectanglePlacement.cpp"
    "../../../deps/juce/modules/juce_graphics/placement/juce_RectanglePlacement.h"
    "../../../deps/juce/modules/juce_graphics/juce_graphics.cpp"
    "../../../deps/juce/modules/juce_graphics/juce_graphics.mm"
    "../../../deps/juce/modules/juce_graphics/juce_graphics.h"
    "../../../deps/juce/modules/juce_gui_basics/application/juce_Application.cpp"
    "../../../deps/juce/modules/juce_gui_basics/application/juce_Application.h"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_ArrowButton.cpp"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_ArrowButton.h"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_Button.cpp"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_Button.h"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_DrawableButton.cpp"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_DrawableButton.h"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_HyperlinkButton.cpp"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_HyperlinkButton.h"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_ImageButton.cpp"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_ImageButton.h"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_ShapeButton.cpp"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_ShapeButton.h"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_TextButton.cpp"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_TextButton.h"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_ToggleButton.cpp"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_ToggleButton.h"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_ToolbarButton.cpp"
    "../../../deps/juce/modules/juce_gui_basics/buttons/juce_ToolbarButton.h"
    "../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandID.h"
    "../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandInfo.cpp"
    "../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandInfo.h"
    "../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandManager.cpp"
    "../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandManager.h"
    "../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandTarget.cpp"
    "../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandTarget.h"
    "../../../deps/juce/modules/juce_gui_basics/commands/juce_KeyPressMappingSet.cpp"
    "../../../deps/juce/modules/juce_gui_basics/commands/juce_KeyPressMappingSet.h"
    "../../../deps/juce/modules/juce_gui_basics/components/juce_CachedComponentImage.h"
    "../../../deps/juce/modules/juce_gui_basics/components/juce_Component.cpp"
    "../../../deps/juce/modules/juce_gui_basics/components/juce_Component.h"
    "../../../deps/juce/modules/juce_gui_basics/components/juce_ComponentListener.cpp"
    "../../../deps/juce/modules/juce_gui_basics/components/juce_ComponentListener.h"
    "../../../deps/juce/modules/juce_gui_basics/components/juce_ModalComponentManager.cpp"
    "../../../deps/juce/modules/juce_gui_basics/components/juce_ModalComponentManager.h"
    "../../../deps/juce/modules/juce_gui_basics/desktop/juce_Desktop.cpp"
    "../../../deps/juce/modules/juce_gui_basics/desktop/juce_Desktop.h"
    "../../../deps/juce/modules/juce_gui_basics/desktop/juce_Displays.cpp"
    "../../../deps/juce/modules/juce_gui_basics/desktop/juce_Displays.h"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_Drawable.cpp"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_Drawable.h"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableComposite.cpp"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableComposite.h"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableImage.cpp"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableImage.h"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawablePath.cpp"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawablePath.h"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableRectangle.cpp"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableRectangle.h"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableShape.cpp"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableShape.h"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableText.cpp"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableText.h"
    "../../../deps/juce/modules/juce_gui_basics/drawables/juce_SVGParser.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_ContentSharer.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_ContentSharer.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_DirectoryContentsDisplayComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_DirectoryContentsDisplayComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_DirectoryContentsList.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_DirectoryContentsList.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileBrowserComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileBrowserComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileBrowserListener.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileChooser.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileChooser.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileChooserDialogBox.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileChooserDialogBox.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileListComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileListComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FilenameComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FilenameComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FilePreviewComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileSearchPathListComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileSearchPathListComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileTreeComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileTreeComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_ImagePreviewComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_ImagePreviewComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_CaretComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_CaretComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyboardFocusTraverser.cpp"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyboardFocusTraverser.h"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyListener.cpp"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyListener.h"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyPress.cpp"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyPress.h"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_ModifierKeys.cpp"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_ModifierKeys.h"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_SystemClipboard.h"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_TextEditorKeyMapper.h"
    "../../../deps/juce/modules/juce_gui_basics/keyboard/juce_TextInputTarget.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_AnimatedPosition.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_AnimatedPositionBehaviours.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentAnimator.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentAnimator.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentBoundsConstrainer.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentBoundsConstrainer.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentBuilder.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentBuilder.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentMovementWatcher.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentMovementWatcher.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ConcertinaPanel.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ConcertinaPanel.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_FlexBox.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_FlexBox.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_FlexItem.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_Grid.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_Grid.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_GridItem.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_GridItem.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_GroupComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_GroupComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_MultiDocumentPanel.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_MultiDocumentPanel.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableBorderComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableBorderComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableCornerComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableCornerComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableEdgeComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableEdgeComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ScrollBar.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_ScrollBar.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_SidePanel.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_SidePanel.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableLayoutManager.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableLayoutManager.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableLayoutResizerBar.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableLayoutResizerBar.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableObjectResizer.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableObjectResizer.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_TabbedButtonBar.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_TabbedButtonBar.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_TabbedComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_TabbedComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_Viewport.cpp"
    "../../../deps/juce/modules/juce_gui_basics/layout/juce_Viewport.h"
    "../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel.cpp"
    "../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel.h"
    "../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V1.cpp"
    "../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V1.h"
    "../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V2.cpp"
    "../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V2.h"
    "../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V3.cpp"
    "../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V3.h"
    "../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V4.cpp"
    "../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V4.h"
    "../../../deps/juce/modules/juce_gui_basics/menus/juce_BurgerMenuComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/menus/juce_BurgerMenuComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/menus/juce_MenuBarComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/menus/juce_MenuBarComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/menus/juce_MenuBarModel.cpp"
    "../../../deps/juce/modules/juce_gui_basics/menus/juce_MenuBarModel.h"
    "../../../deps/juce/modules/juce_gui_basics/menus/juce_PopupMenu.cpp"
    "../../../deps/juce/modules/juce_gui_basics/menus/juce_PopupMenu.h"
    "../../../deps/juce/modules/juce_gui_basics/misc/juce_BubbleComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/misc/juce_BubbleComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/misc/juce_DropShadower.cpp"
    "../../../deps/juce/modules/juce_gui_basics/misc/juce_DropShadower.h"
    "../../../deps/juce/modules/juce_gui_basics/misc/juce_JUCESplashScreen.cpp"
    "../../../deps/juce/modules/juce_gui_basics/misc/juce_JUCESplashScreen.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_ComponentDragger.cpp"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_ComponentDragger.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_DragAndDropContainer.cpp"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_DragAndDropContainer.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_DragAndDropTarget.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_FileDragAndDropTarget.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_LassoComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseCursor.cpp"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseCursor.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseEvent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseEvent.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseInactivityDetector.cpp"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseInactivityDetector.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseInputSource.cpp"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseInputSource.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseListener.cpp"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseListener.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_SelectedItemSet.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_TextDragAndDropTarget.h"
    "../../../deps/juce/modules/juce_gui_basics/mouse/juce_TooltipClient.h"
    "../../../deps/juce/modules/juce_gui_basics/native/x11/juce_linux_X11_DragAndDrop.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/x11/juce_linux_X11_Symbols.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/x11/juce_linux_X11_Symbols.h"
    "../../../deps/juce/modules/juce_gui_basics/native/x11/juce_linux_XWindowSystem.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/x11/juce_linux_XWindowSystem.h"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_android_ContentSharer.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_android_FileChooser.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_android_Windowing.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_common_MimeTypes.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_ios_ContentSharer.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_ios_FileChooser.mm"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_ios_UIViewComponentPeer.mm"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_ios_Windowing.mm"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_linux_FileChooser.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_linux_Windowing.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_mac_FileChooser.mm"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_mac_MainMenu.mm"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_mac_MouseCursor.mm"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_mac_NSViewComponentPeer.mm"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_mac_Windowing.mm"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_MultiTouchMapper.h"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_win32_DragAndDrop.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_win32_FileChooser.cpp"
    "../../../deps/juce/modules/juce_gui_basics/native/juce_win32_Windowing.cpp"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_MarkerList.cpp"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_MarkerList.h"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeCoordinate.cpp"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeCoordinate.h"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeCoordinatePositioner.cpp"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeCoordinatePositioner.h"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeParallelogram.cpp"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeParallelogram.h"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativePoint.cpp"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativePoint.h"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativePointPath.cpp"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativePointPath.h"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeRectangle.cpp"
    "../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeRectangle.h"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_BooleanPropertyComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_BooleanPropertyComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_ButtonPropertyComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_ButtonPropertyComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_ChoicePropertyComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_ChoicePropertyComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_MultiChoicePropertyComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_MultiChoicePropertyComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_PropertyComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_PropertyComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_PropertyPanel.cpp"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_PropertyPanel.h"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_SliderPropertyComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_SliderPropertyComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_TextPropertyComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/properties/juce_TextPropertyComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ComboBox.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ComboBox.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ImageComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ImageComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_Label.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_Label.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ListBox.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ListBox.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ProgressBar.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ProgressBar.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_Slider.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_Slider.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_TableHeaderComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_TableHeaderComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_TableListBox.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_TableListBox.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_TextEditor.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_TextEditor.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_Toolbar.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_Toolbar.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ToolbarItemComponent.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ToolbarItemComponent.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ToolbarItemFactory.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ToolbarItemPalette.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_ToolbarItemPalette.h"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_TreeView.cpp"
    "../../../deps/juce/modules/juce_gui_basics/widgets/juce_TreeView.h"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_AlertWindow.cpp"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_AlertWindow.h"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_CallOutBox.cpp"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_CallOutBox.h"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_ComponentPeer.cpp"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_ComponentPeer.h"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_DialogWindow.cpp"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_DialogWindow.h"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_DocumentWindow.cpp"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_DocumentWindow.h"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_NativeMessageBox.h"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_ResizableWindow.cpp"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_ResizableWindow.h"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_ThreadWithProgressWindow.cpp"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_ThreadWithProgressWindow.h"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_TooltipWindow.cpp"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_TooltipWindow.h"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_TopLevelWindow.cpp"
    "../../../deps/juce/modules/juce_gui_basics/windows/juce_TopLevelWindow.h"
    "../../../deps/juce/modules/juce_gui_basics/juce_gui_basics.cpp"
    "../../../deps/juce/modules/juce_gui_basics/juce_gui_basics.mm"
    "../../../deps/juce/modules/juce_gui_basics/juce_gui_basics.h"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CodeDocument.cpp"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CodeDocument.h"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CodeEditorComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CodeEditorComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CodeTokeniser.h"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CPlusPlusCodeTokeniser.cpp"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CPlusPlusCodeTokeniser.h"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CPlusPlusCodeTokeniserFunctions.h"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_LuaCodeTokeniser.cpp"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_LuaCodeTokeniser.h"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_XMLCodeTokeniser.cpp"
    "../../../deps/juce/modules/juce_gui_extra/code_editor/juce_XMLCodeTokeniser.h"
    "../../../deps/juce/modules/juce_gui_extra/documents/juce_FileBasedDocument.cpp"
    "../../../deps/juce/modules/juce_gui_extra/documents/juce_FileBasedDocument.h"
    "../../../deps/juce/modules/juce_gui_extra/embedding/juce_ActiveXControlComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/embedding/juce_AndroidViewComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/embedding/juce_HWNDComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/embedding/juce_NSViewComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/embedding/juce_ScopedDPIAwarenessDisabler.h"
    "../../../deps/juce/modules/juce_gui_extra/embedding/juce_UIViewComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/embedding/juce_XEmbedComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_AnimatedAppComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_AnimatedAppComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_AppleRemote.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_BubbleMessageComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_BubbleMessageComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_ColourSelector.cpp"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_ColourSelector.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_KeyMappingEditorComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_KeyMappingEditorComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_LiveConstantEditor.cpp"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_LiveConstantEditor.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_PreferencesPanel.cpp"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_PreferencesPanel.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_PushNotifications.cpp"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_PushNotifications.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_RecentlyOpenedFilesList.cpp"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_RecentlyOpenedFilesList.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_SplashScreen.cpp"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_SplashScreen.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_SystemTrayIconComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_SystemTrayIconComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/misc/juce_WebBrowserComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_android_PushNotifications.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_android_WebBrowserComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_AndroidViewComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_ios_PushNotifications.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_ios_UIViewComponent.mm"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_linux_X11_SystemTrayIcon.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_linux_X11_WebBrowserComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_linux_XEmbedComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_mac_AppleRemote.mm"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_mac_CarbonViewWrapperComponent.h"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_mac_NSViewComponent.mm"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_mac_PushNotifications.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_mac_SystemTrayIcon.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_mac_WebBrowserComponent.mm"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_win32_ActiveXComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_win32_HWNDComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_win32_SystemTrayIcon.cpp"
    "../../../deps/juce/modules/juce_gui_extra/native/juce_win32_WebBrowserComponent.cpp"
    "../../../deps/juce/modules/juce_gui_extra/juce_gui_extra.cpp"
    "../../../deps/juce/modules/juce_gui_extra/juce_gui_extra.mm"
    "../../../deps/juce/modules/juce_gui_extra/juce_gui_extra.h"
    "../../../deps/juce/modules/juce_opengl/geometry/juce_Draggable3DOrientation.h"
    "../../../deps/juce/modules/juce_opengl/geometry/juce_Matrix3D.h"
    "../../../deps/juce/modules/juce_opengl/geometry/juce_Quaternion.h"
    "../../../deps/juce/modules/juce_opengl/geometry/juce_Vector3D.h"
    "../../../deps/juce/modules/juce_opengl/native/juce_MissingGLDefinitions.h"
    "../../../deps/juce/modules/juce_opengl/native/juce_OpenGL_android.h"
    "../../../deps/juce/modules/juce_opengl/native/juce_OpenGL_ios.h"
    "../../../deps/juce/modules/juce_opengl/native/juce_OpenGL_linux_X11.h"
    "../../../deps/juce/modules/juce_opengl/native/juce_OpenGL_osx.h"
    "../../../deps/juce/modules/juce_opengl/native/juce_OpenGL_win32.h"
    "../../../deps/juce/modules/juce_opengl/native/juce_OpenGLExtensions.h"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLContext.cpp"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLContext.h"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLFrameBuffer.cpp"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLFrameBuffer.h"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLGraphicsContext.cpp"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLGraphicsContext.h"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLHelpers.cpp"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLHelpers.h"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLImage.cpp"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLImage.h"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLPixelFormat.cpp"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLPixelFormat.h"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLRenderer.h"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLShaderProgram.cpp"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLShaderProgram.h"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLTexture.cpp"
    "../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLTexture.h"
    "../../../deps/juce/modules/juce_opengl/utils/juce_OpenGLAppComponent.cpp"
    "../../../deps/juce/modules/juce_opengl/utils/juce_OpenGLAppComponent.h"
    "../../../deps/juce/modules/juce_opengl/juce_opengl.cpp"
    "../../../deps/juce/modules/juce_opengl/juce_opengl.mm"
    "../../../deps/juce/modules/juce_opengl/juce_opengl.h"
    "../../../mobile/JuceLibraryCode/AppConfig.h"
    "../../../mobile/JuceLibraryCode/BinaryData.cpp"
    "../../../mobile/JuceLibraryCode/BinaryData.h"
    "../../../mobile/JuceLibraryCode/include_ff_meters.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_audio_basics.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_audio_devices.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_audio_formats.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_audio_plugin_client_Standalone.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_audio_plugin_client_utils.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_audio_processors.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_audio_utils.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_core.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_cryptography.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_data_structures.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_dsp.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_events.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_graphics.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_gui_basics.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_gui_extra.cpp"
    "../../../mobile/JuceLibraryCode/include_juce_opengl.cpp"
    "../../../mobile/JuceLibraryCode/JuceHeader.h"
    "../../../mobile/JuceLibraryCode/JucePluginDefines.h"
)

set_source_files_properties("../../../deps/aoo/lib/aoo/aoo.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/aoo/aoo.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/aoo/aoo_net.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/aoo/aoo_net.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/aoo/aoo_opus.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/aoo/aoo_pcm.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/aoo/aoo_types.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/aoo/aoo_utils.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/src/client.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/src/common.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/src/lockfree.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/src/net_utils.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/src/server.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/src/sink.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/src/SLIP.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/src/source.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/src/sync.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/src/time.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/lib/src/time_dll.hpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/deps/md5/md5.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/deps/oscpack/osc/MessageMappingOscPacketListener.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/deps/oscpack/osc/OscException.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/deps/oscpack/osc/OscHostEndianness.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/deps/oscpack/osc/OscOutboundPacketStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/deps/oscpack/osc/OscPacketListener.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/deps/oscpack/osc/OscPrintReceivedElements.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/deps/oscpack/osc/OscReceivedElements.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/aoo/deps/oscpack/osc/OscTypes.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../localization/localized_de.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../localization/localized_es.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../localization/localized_fr.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../localization/localized_it.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../localization/localized_ja.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../localization/localized_nl.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../localization/localized_pt-br.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../localization/localized_pt-pt.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/bar_click.wav" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/beat_click.wav" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/chat.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/chat_dots.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/chevron_forward.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/copy_icon.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/DejaVuSans.ttf" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/dice_icon_128.png" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/dispfull.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/dispminimal.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/dots.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/dots_icon.png" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/expand_arrow_active.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/expand_arrow_inactive.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/folder_icon.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/hear-others.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/incoming_allowed.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/incoming_allowed_active.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/incoming_disallowed.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/lgc_bar.wav" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/link.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/link_all.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/link_up.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/loop_icon.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/met.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/mic.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/mic_disabled.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/mic_pointing.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/move_updown.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/mute-others.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/network.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/outgoing_allowed.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/outgoing_allowed_active.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/outgoing_disallowed.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/paste_icon.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/pause_icon.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/people.png" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/people.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/person.png" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/person.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/play_icon.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/power.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/power_sel.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/record.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/record_active.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/record_active_alt.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/rectape.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/reset_buffer_icon.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/send_group.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/send_group_small.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/settings_icon.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/skipback_icon.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/sonobus_icon_ios_256.png" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/sonobus_icon_ios_1024.png" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/sonobus_logo_96.png" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/sonobus_title_small.png" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/speaker.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/speaker_disabled.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/triangle_disclosure.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/triangle_disclosure_right.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/urei_main.wav" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/wordmaker.g" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../images/x_icon.svg" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/android/SonoBusActivity.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/AutoUpdater.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/BeatToggleGrid.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/ChannelGroup.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/ChannelGroupsView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/ChatView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/CompressorView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/ConnectView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/CrossPlatformUtils.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/CrossPlatformUtilsIOS.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/CrossPlatformUtilsMac.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/DebugLogC.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/EffectParams.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/EffectsBaseView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/ExpanderView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/faustCompressor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/faustExpander.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/faustLimiter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/faustParametricEQ.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/GenericItemChooser.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/JitterBufferMeter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/LatencyMatchView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/LatencyMeasurer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/LevelMeterLookAndFeelMethods.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/Metronome.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/MonitorDelayView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/mtdm.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/MVerb.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/OptionsView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/ParametricEqView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/PeersContainerView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/RandomSentenceGenerator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/ReverbSendView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/RunCumulantor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/RunningCumulant.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/SonobusPluginEditor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/SonobusPluginProcessor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/SonobusTypes.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/SonoChoiceButton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/SonoDrawableButton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/SonoLookAndFeel.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/SonoStandaloneFilterWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/SonoTextButton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/SonoUtility.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/VersionInfo.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/WaveformTransportComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../Source/zitaRev.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/LevelMeter/LevelMeter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/LevelMeter/LevelMeter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/LevelMeter/LevelMeterSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/LookAndFeel/LevelMeterLookAndFeel.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/LookAndFeel/LevelMeterLookAndFeelMethods.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/LookAndFeel/SoundFieldLookAndFeelMethods.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/Visualisers/OutlineBuffer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/Visualisers/StereoFieldBuffer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/Visualisers/StereoFieldComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/ff_meters.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/LICENSE.md" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/README.md" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/ff_meters/ff_meters.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/audio_play_head/juce_AudioPlayHead.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioChannelSet.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioChannelSet.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioDataConverters.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioDataConverters.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioProcessLoadMeasurer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioProcessLoadMeasurer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/buffers/juce_AudioSampleBuffer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/buffers/juce_FloatVectorOperations.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/buffers/juce_FloatVectorOperations.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiBuffer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiBuffer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiFile.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiFile.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiKeyboardState.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiKeyboardState.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiMessage.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiMessage.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiMessageSequence.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiMessageSequence.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiRPN.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/midi/juce_MidiRPN.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEInstrument.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEInstrument.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEMessages.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEMessages.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPENote.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPENote.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiser.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiserBase.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiserBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiserVoice.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPESynthesiserVoice.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEUtils.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEUtils.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEValue.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEValue.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEZoneLayout.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/mpe/juce_MPEZoneLayout.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/native/juce_mac_CoreAudioLayouts.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_AudioSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_BufferingAudioSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_BufferingAudioSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_ChannelRemappingAudioSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_ChannelRemappingAudioSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_IIRFilterAudioSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_IIRFilterAudioSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_MemoryAudioSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_MemoryAudioSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_MixerAudioSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_MixerAudioSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_PositionableAudioSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_ResamplingAudioSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_ResamplingAudioSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_ReverbAudioSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_ReverbAudioSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_ToneGeneratorAudioSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/sources/juce_ToneGeneratorAudioSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/synthesisers/juce_Synthesiser.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_ADSR.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_Decibels.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_GenericInterpolator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_IIRFilter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_IIRFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_Interpolators.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_Interpolators.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_LagrangeInterpolator.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_Reverb.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_SmoothedValue.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_SmoothedValue.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/utilities/juce_WindowedSincInterpolator.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/juce_audio_basics.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/juce_audio_basics.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_basics/juce_audio_basics.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioDeviceManager.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioDeviceManager.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioIODevice.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioIODevice.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioIODeviceType.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/audio_io/juce_AudioIODeviceType.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/audio_io/juce_SystemAudioVolume.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPacket.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPackets.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPBytestreamInputHandler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPConversion.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPConverters.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPDispatcher.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPFactory.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPIterator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPMidi1ToBytestreamTranslator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPMidi1ToMidi2DefaultTranslator.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPMidi1ToMidi2DefaultTranslator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPProtocols.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPReceiver.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPSysEx7.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPSysEx7.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPTests.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPU32InputHandler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPUtils.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPUtils.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPView.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/ump/juce_UMPView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/juce_MidiDevices.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/juce_MidiDevices.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/juce_MidiMessageCollector.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/midi_io/juce_MidiMessageCollector.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/AudioStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/AudioStreamBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/AudioStreamBuilder.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/AudioStreamCallback.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/Definitions.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/LatencyTuner.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/Oboe.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/ResultWithValue.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/StabilizedCallback.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/Utilities.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/include/oboe/Version.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/aaudio/AAudioLoader.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/aaudio/AAudioLoader.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/aaudio/AudioStreamAAudio.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/aaudio/AudioStreamAAudio.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/AudioClock.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/AudioSourceCaller.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/AudioSourceCaller.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/AudioStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/AudioStreamBuilder.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/DataConversionFlowGraph.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/DataConversionFlowGraph.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FilterAudioStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FilterAudioStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockAdapter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockAdapter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockReader.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockReader.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockWriter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/FixedBlockWriter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/LatencyTuner.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/MonotonicCounter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/OboeDebug.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/QuirksManager.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/QuirksManager.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/SourceFloatCaller.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/SourceFloatCaller.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/SourceI16Caller.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/SourceI16Caller.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/StabilizedCallback.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/Trace.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/Trace.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/Utilities.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/common/Version.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoBuffer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoBuffer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoController.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoController.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoControllerBase.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoControllerBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoControllerIndirect.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/fifo/FifoControllerIndirect.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/HyperbolicCosineWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/IntegerRatio.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/IntegerRatio.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/KaiserWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/LinearResampler.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/LinearResampler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/MultiChannelResampler.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/MultiChannelResampler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResampler.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResampler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResamplerMono.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResamplerMono.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResamplerStereo.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/PolyphaseResamplerStereo.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/SincResampler.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/SincResampler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/SincResamplerStereo.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/resampler/SincResamplerStereo.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ChannelCountConverter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ChannelCountConverter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ClipToRange.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ClipToRange.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/FlowGraphNode.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/FlowGraphNode.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ManyToMultiConverter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/ManyToMultiConverter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/MonoToMultiConverter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/MonoToMultiConverter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/MultiToMonoConverter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/MultiToMonoConverter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/RampLinear.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/RampLinear.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SampleRateConverter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SampleRateConverter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkFloat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkFloat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkI16.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkI16.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkI24.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SinkI24.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceFloat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceFloat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceI16.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceI16.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceI24.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/flowgraph/SourceI24.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioInputStreamOpenSLES.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioInputStreamOpenSLES.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioOutputStreamOpenSLES.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioOutputStreamOpenSLES.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioStreamBuffered.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioStreamBuffered.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioStreamOpenSLES.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/AudioStreamOpenSLES.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/EngineOpenSLES.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/EngineOpenSLES.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/OpenSLESUtilities.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/OpenSLESUtilities.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/OutputMixerOpenSLES.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/src/opensles/OutputMixerOpenSLES.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/CMakeLists.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/oboe/readme.md" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_android_Audio.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_android_HighPerformanceAudioHelpers.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_android_Midi.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_android_Oboe.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_android_OpenSL.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_ios_Audio.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_ios_Audio.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_linux_ALSA.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_linux_Bela.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_linux_JackAudio.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_linux_Midi.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_mac_CoreAudio.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_mac_CoreMidi.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_MidiDataConcatenator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_win32_ASIO.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_win32_DirectSound.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_win32_Midi.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/native/juce_win32_WASAPI.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/sources/juce_AudioSourcePlayer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/sources/juce_AudioSourcePlayer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/sources/juce_AudioTransportSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/sources/juce_AudioTransportSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/juce_audio_devices.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/juce_audio_devices.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_devices/juce_audio_devices.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/all.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/bitmath.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/bitreader.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/bitwriter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/cpu.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/crc.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/fixed.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/float.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/format.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/lpc.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/md5.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/memory.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/metadata.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/stream_encoder.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/stream_encoder_framing.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/private/window.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/protected/all.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/protected/stream_decoder.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/include/protected/stream_encoder.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/bitmath.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/bitreader.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/bitwriter.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/cpu.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/crc.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/fixed.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/float.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/format.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/lpc_flac.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/md5.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/memory.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/stream_decoder.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/stream_encoder.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/stream_encoder_framing.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/libFLAC/window_flac.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/all.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/alloc.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/assert.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/callback.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/compat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/endswap.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/export.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/Flac Licence.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/format.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/metadata.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/ordinals.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/stream_decoder.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/stream_encoder.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/flac/win_utf8_io.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/books/coupled/res_books_51.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/books/coupled/res_books_stereo.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/books/floor/floor_books.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/books/uncoupled/res_books_uncoupled.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/floor_all.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/psych_8.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/psych_11.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/psych_16.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/psych_44.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/residue_8.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/residue_16.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/residue_44.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/residue_44p51.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/residue_44u.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_8.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_11.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_16.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_22.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_32.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_44.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_44p51.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_44u.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/modes/setup_X.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/analysis.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/backends.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/bitrate.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/bitrate.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/block.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/codebook.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/codebook.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/codec_internal.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/envelope.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/envelope.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/floor0.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/floor1.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/highlevel.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/info.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lookup.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lookup.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lookup_data.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lpc.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lpc.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lsp.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/lsp.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/mapping0.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/masking.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/mdct.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/mdct.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/misc.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/misc.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/os.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/psy.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/psy.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/registry.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/registry.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/res0.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/scales.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/sharedbook.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/smallft.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/smallft.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/synthesis.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/vorbisenc.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/vorbisfile.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/window.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/lib/window.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/libvorbis-1.3.7/README.md" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/bitwise.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/codec.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/config_types.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/crctable.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/framing.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/Ogg Vorbis Licence.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/ogg.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/os_types.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/vorbisenc.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/oggvorbis/vorbisfile.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_AiffAudioFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_AiffAudioFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_CoreAudioFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_CoreAudioFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_FlacAudioFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_FlacAudioFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_LAMEEncoderAudioFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_LAMEEncoderAudioFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_MP3AudioFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_MP3AudioFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_OggVorbisAudioFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_OggVorbisAudioFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_WavAudioFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_WavAudioFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_WindowsMediaAudioFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/codecs/juce_WindowsMediaAudioFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatManager.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatManager.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatReader.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatReader.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatReaderSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatReaderSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatWriter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioFormatWriter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioSubsectionReader.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_AudioSubsectionReader.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_BufferingAudioFormatReader.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_BufferingAudioFormatReader.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/format/juce_MemoryMappedAudioFormatReader.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/sampler/juce_Sampler.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/sampler/juce_Sampler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/juce_audio_formats.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/juce_audio_formats.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_formats/juce_audio_formats.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AAX/juce_AAX_Modifier_Injector.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AAX/juce_AAX_Wrapper.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUBase.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUBaseHelper.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUBuffer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUBuffer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUCarbonViewBase.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUCarbonViewBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUCarbonViewControl.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUCarbonViewControl.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUCarbonViewDispatch.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUDispatch.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUDispatch.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUInputElement.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUInputElement.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUInputFormatConverter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUMIDIBase.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUMIDIBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUOutputBase.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUOutputBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUOutputElement.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUOutputElement.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUPlugInDispatch.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUPlugInDispatch.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUScopeElement.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUScopeElement.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUSilentTimeout.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUTimestampGenerator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/AUViewLocalizedStringKeys.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAtomic.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAtomicStack.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAudioChannelLayout.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAudioChannelLayout.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAUParameter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAUParameter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAAutoDisposer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CADebugMacros.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CADebugPrintf.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAException.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAHostTimeBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAMath.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAMutex.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAMutex.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CarbonEventHandler.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CarbonEventHandler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAReferenceCounted.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAStreamBasicDescription.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAStreamBasicDescription.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAThreadSafeList.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAVectorUnit.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAVectorUnit.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAVectorUnitTypes.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/CAXException.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/ComponentBase.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/ComponentBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/MusicDeviceBase.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/CoreAudioUtilityClasses/MusicDeviceBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/juce_AU_Wrapper.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/AU/juce_AUv3_Wrapper.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_DigiCode_Header.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_DigiCode1.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_DigiCode2.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_DigiCode3.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_MacUtilities.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_WinUtilities.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/RTAS/juce_RTAS_Wrapper.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/Standalone/juce_StandaloneFilterApp.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/Unity/juce_Unity_Wrapper.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/Unity/juce_UnityPluginInterface.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_CarbonVisibility.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_CheckSettingMacros.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_CreatePluginFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_FakeMouseMoveGenerator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_IncludeModuleHeaders.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_IncludeSystemHeaders.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_PluginHostType.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_PluginUtilities.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/utility/juce_WindowsHooks.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/VST/juce_VST_Wrapper.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/VST/juce_VST_Wrapper.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/VST/juce_VSTCallbackHandler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/VST3/juce_VST3_Wrapper.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_AAX.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_AAX.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_AU_1.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_AU_2.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_AUv3.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_1.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_2.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_3.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_4.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_utils.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_RTAS_utils.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_Standalone.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_Unity.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_utils.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST_utils.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST2.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format/juce_AudioPluginFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format/juce_AudioPluginFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format/juce_AudioPluginFormatManager.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format/juce_AudioPluginFormatManager.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/baseiids.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/classfactoryhelpers.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fbuffer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fbuffer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fdebug.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fdebug.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fobject.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fobject.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fstreamer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fstreamer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fstring.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/fstring.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/updatehandler.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/source/updatehandler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/thread/include/flock.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/thread/source/flock.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/LICENSE.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/base/README.md" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/conststringtable.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/conststringtable.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/coreiids.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/falignpop.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/falignpush.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/fplatform.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/fstrdefs.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ftypes.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/funknown.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/funknown.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/futils.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/fvariant.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ibstream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/icloneable.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ipersistent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ipluginbase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/istringresult.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/iupdatehandler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/smartpointer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/typesizecheck.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ustring.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/base/ustring.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/gui/iplugview.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/gui/iplugviewcontentscalesupport.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstattributes.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstaudioprocessor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstautomationstate.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstchannelcontextinfo.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstcomponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstcontextmenu.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivsteditcontroller.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstevents.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivsthostapplication.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstinterappaudio.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstmessage.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstmidicontrollers.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstmidilearn.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstnoteexpression.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstparameterchanges.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstphysicalui.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstpluginterfacesupport.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstplugview.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstprefetchablesupport.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstprocesscontext.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstrepresentation.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivsttestplugprovider.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstunits.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/vstpshpack4.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/vstspeaker.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/vsttypes.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/LICENSE.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/README.md" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/common/memorystream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/common/memorystream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/common/pluginview.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/common/pluginview.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/hosting/hostclasses.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/hosting/hostclasses.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/hosting/pluginterfacesupport.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/hosting/pluginterfacesupport.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstbus.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstbus.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstcomponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstcomponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstcomponentbase.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstcomponentbase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vsteditcontroller.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vsteditcontroller.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstinitiids.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstparameters.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstparameters.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstpresetfile.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/source/vst/vstpresetfile.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/LICENSE.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/public.sdk/README.md" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/VST3_SDK/README.md" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_AU_Shared.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_AudioUnitPluginFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_AudioUnitPluginFormat.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_LADSPAPluginFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_LADSPAPluginFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_LegacyAudioParameter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_VST3Common.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_VST3Headers.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_VST3PluginFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_VST3PluginFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_VSTCommon.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_VSTMidiEventList.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_VSTPluginFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/format_types/juce_VSTPluginFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioPluginInstance.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioPluginInstance.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessor.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorEditor.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorEditor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorGraph.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorGraph.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorListener.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorParameter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorParameterGroup.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_AudioProcessorParameterGroup.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_GenericAudioProcessorEditor.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_GenericAudioProcessorEditor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_PluginDescription.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/processors/juce_PluginDescription.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/scanning/juce_KnownPluginList.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/scanning/juce_KnownPluginList.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/scanning/juce_PluginDirectoryScanner.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/scanning/juce_PluginDirectoryScanner.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/scanning/juce_PluginListComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/scanning/juce_PluginListComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterBool.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterBool.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterChoice.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterChoice.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterFloat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterFloat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterInt.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioParameterInt.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioProcessorParameterWithID.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioProcessorParameterWithID.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_ParameterAttachments.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_ParameterAttachments.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_RangedAudioParameter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/utilities/juce_RangedAudioParameter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/juce_audio_processors.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/juce_audio_processors.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_processors/juce_audio_processors.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/audio_cd/juce_AudioCDBurner.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/audio_cd/juce_AudioCDReader.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/audio_cd/juce_AudioCDReader.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioAppComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioAppComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioDeviceSelectorComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioDeviceSelectorComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioThumbnail.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioThumbnail.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioThumbnailBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioThumbnailCache.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioThumbnailCache.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioVisualiserComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_AudioVisualiserComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_BluetoothMidiDevicePairingDialogue.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_MidiKeyboardComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/gui/juce_MidiKeyboardComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/native/juce_android_BluetoothMidiDevicePairingDialogue.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/native/juce_ios_BluetoothMidiDevicePairingDialogue.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/native/juce_linux_AudioCDReader.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/native/juce_linux_BluetoothMidiDevicePairingDialogue.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/native/juce_mac_AudioCDBurner.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/native/juce_mac_AudioCDReader.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/native/juce_mac_BluetoothMidiDevicePairingDialogue.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/native/juce_win_BluetoothMidiDevicePairingDialogue.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/native/juce_win32_AudioCDBurner.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/native/juce_win32_AudioCDReader.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/players/juce_AudioProcessorPlayer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/players/juce_AudioProcessorPlayer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/players/juce_SoundPlayer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/players/juce_SoundPlayer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/juce_audio_utils.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/juce_audio_utils.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_audio_utils/juce_audio_utils.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_AbstractFifo.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_AbstractFifo.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_Array.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_ArrayAllocationBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_ArrayBase.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_ArrayBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_DynamicObject.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_DynamicObject.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_ElementComparator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_HashMap.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_HashMap_test.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_LinkedListPointer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_ListenerList.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_NamedValueSet.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_NamedValueSet.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_OwnedArray.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_OwnedArray.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_PropertySet.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_PropertySet.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_ReferenceCountedArray.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_ReferenceCountedArray.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_ScopedValueSetter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_SortedSet.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_SparseSet.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_SparseSet.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_Variant.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/containers/juce_Variant.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_DirectoryIterator.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_DirectoryIterator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_File.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_File.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_FileFilter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_FileFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_FileInputStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_FileInputStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_FileOutputStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_FileOutputStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_FileSearchPath.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_FileSearchPath.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_MemoryMappedFile.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_RangedDirectoryIterator.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_RangedDirectoryIterator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_TemporaryFile.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_TemporaryFile.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_WildcardFileFilter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/files/juce_WildcardFileFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/javascript/juce_Javascript.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/javascript/juce_Javascript.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/javascript/juce_JSON.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/javascript/juce_JSON.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/logging/juce_FileLogger.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/logging/juce_FileLogger.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/logging/juce_Logger.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/logging/juce_Logger.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/maths/juce_BigInteger.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/maths/juce_BigInteger.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/maths/juce_Expression.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/maths/juce_Expression.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/maths/juce_MathsFunctions.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/maths/juce_NormalisableRange.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/maths/juce_Random.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/maths/juce_Random.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/maths/juce_Range.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/maths/juce_StatisticsAccumulator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_AllocationHooks.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_AllocationHooks.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_Atomic.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_ByteOrder.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_ContainerDeletePolicy.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_HeapBlock.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_HeavyweightLeakedObjectDetector.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_LeakedObjectDetector.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_Memory.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_MemoryBlock.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_MemoryBlock.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_OptionalScopedPointer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_ReferenceCountedObject.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_ScopedPointer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_SharedResourcePointer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_Singleton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/memory/juce_WeakReference.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/misc/juce_ConsoleApplication.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/misc/juce_ConsoleApplication.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/misc/juce_Result.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/misc/juce_Result.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/misc/juce_RuntimePermissions.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/misc/juce_RuntimePermissions.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/misc/juce_Uuid.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/misc/juce_Uuid.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/misc/juce_WindowsRegistry.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/java/README.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_android_Files.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_android_JNIHelpers.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_android_JNIHelpers.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_android_Misc.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_android_Network.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_android_RuntimePermissions.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_android_SystemStats.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_android_Threads.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_BasicNativeHeaders.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_curl_Network.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_linux_CommonFile.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_linux_Files.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_linux_Network.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_linux_SystemStats.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_linux_Threads.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_mac_ClangBugWorkaround.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_mac_Files.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_mac_Network.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_mac_Strings.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_mac_SystemStats.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_mac_Threads.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_osx_ObjCHelpers.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_posix_IPAddress.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_posix_NamedPipe.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_posix_SharedCode.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_wasm_SystemStats.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_win32_ComSmartPtr.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_win32_Files.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_win32_Network.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_win32_Registry.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_win32_SystemStats.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/native/juce_win32_Threads.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_IPAddress.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_IPAddress.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_MACAddress.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_MACAddress.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_NamedPipe.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_NamedPipe.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_Socket.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_Socket.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_URL.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_URL.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_WebInputStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/network/juce_WebInputStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_BufferedInputStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_BufferedInputStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_FileInputSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_FileInputSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_InputSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_InputStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_InputStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_MemoryInputStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_MemoryInputStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_MemoryOutputStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_MemoryOutputStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_OutputStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_OutputStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_SubregionStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_SubregionStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_URLInputSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/streams/juce_URLInputSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/system/juce_CompilerSupport.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/system/juce_CompilerWarnings.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/system/juce_PlatformDefs.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/system/juce_StandardHeader.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/system/juce_SystemStats.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/system/juce_SystemStats.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/system/juce_TargetPlatform.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_Base64.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_Base64.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_CharacterFunctions.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_CharacterFunctions.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_CharPointer_ASCII.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_CharPointer_UTF8.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_CharPointer_UTF16.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_CharPointer_UTF32.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_Identifier.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_Identifier.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_LocalisedStrings.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_LocalisedStrings.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_NewLine.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_String.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_String.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_StringArray.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_StringArray.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_StringPairArray.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_StringPairArray.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_StringPool.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_StringPool.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_StringRef.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_TextDiff.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/text/juce_TextDiff.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_ChildProcess.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_ChildProcess.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_CriticalSection.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_DynamicLibrary.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_HighResolutionTimer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_HighResolutionTimer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_InterProcessLock.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_Process.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_ReadWriteLock.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_ReadWriteLock.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_ScopedLock.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_ScopedReadLock.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_ScopedWriteLock.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_SpinLock.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_Thread.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_Thread.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_ThreadLocalValue.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_ThreadPool.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_ThreadPool.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_TimeSliceThread.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_TimeSliceThread.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_WaitableEvent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/threads/juce_WaitableEvent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/time/juce_PerformanceCounter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/time/juce_PerformanceCounter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/time/juce_RelativeTime.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/time/juce_RelativeTime.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/time/juce_Time.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/time/juce_Time.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/unit_tests/juce_UnitTest.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/unit_tests/juce_UnitTest.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/unit_tests/juce_UnitTestCategories.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/xml/juce_XmlDocument.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/xml/juce_XmlDocument.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/xml/juce_XmlElement.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/xml/juce_XmlElement.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/adler32.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/compress.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/crc32.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/crc32.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/deflate.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/deflate.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/infback.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/inffast.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/inffast.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/inffixed.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/inflate.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/inflate.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/inftrees.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/inftrees.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/trees.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/trees.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/uncompr.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/zconf.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/zconf.in.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/zlib.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/zutil.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/zlib/zutil.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/juce_GZIPCompressorOutputStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/juce_GZIPCompressorOutputStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/juce_GZIPDecompressorInputStream.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/juce_GZIPDecompressorInputStream.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/juce_ZipFile.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/zip/juce_ZipFile.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/juce_core.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/juce_core.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_core/juce_core.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/encryption/juce_BlowFish.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/encryption/juce_BlowFish.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/encryption/juce_Primes.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/encryption/juce_Primes.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/encryption/juce_RSAKey.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/encryption/juce_RSAKey.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/hashing/juce_MD5.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/hashing/juce_MD5.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/hashing/juce_SHA256.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/hashing/juce_SHA256.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/hashing/juce_Whirlpool.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/hashing/juce_Whirlpool.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/juce_cryptography.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/juce_cryptography.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_cryptography/juce_cryptography.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/app_properties/juce_ApplicationProperties.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/app_properties/juce_ApplicationProperties.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/app_properties/juce_PropertiesFile.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/app_properties/juce_PropertiesFile.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/undomanager/juce_UndoableAction.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/undomanager/juce_UndoManager.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/undomanager/juce_UndoManager.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/values/juce_CachedValue.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/values/juce_CachedValue.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/values/juce_Value.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/values/juce_Value.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/values/juce_ValueTree.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/values/juce_ValueTree.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/values/juce_ValueTreeSynchroniser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/values/juce_ValueTreeSynchroniser.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/values/juce_ValueWithDefault.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/values/juce_ValueWithDefault.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/juce_data_structures.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/juce_data_structures.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_data_structures/juce_data_structures.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/containers/juce_AudioBlock.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/containers/juce_AudioBlock_test.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/containers/juce_FixedSizeFunction.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/containers/juce_FixedSizeFunction_test.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/containers/juce_SIMDRegister.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/containers/juce_SIMDRegister_Impl.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/containers/juce_SIMDRegister_test.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/filter_design/juce_FilterDesign.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/filter_design/juce_FilterDesign.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/frequency/juce_Convolution.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/frequency/juce_Convolution.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/frequency/juce_Convolution_test.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/frequency/juce_FFT.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/frequency/juce_FFT.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/frequency/juce_FFT_test.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/frequency/juce_Windowing.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/frequency/juce_Windowing.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_FastMathApproximations.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_LogRampedValue.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_LogRampedValue_test.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_LookupTable.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_LookupTable.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_Matrix.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_Matrix.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_Matrix_test.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_Phase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_Polynomial.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_SpecialFunctions.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/maths/juce_SpecialFunctions.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/native/juce_avx_SIMDNativeOps.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/native/juce_avx_SIMDNativeOps.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/native/juce_fallback_SIMDNativeOps.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/native/juce_neon_SIMDNativeOps.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/native/juce_neon_SIMDNativeOps.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/native/juce_sse_SIMDNativeOps.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/native/juce_sse_SIMDNativeOps.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_BallisticsFilter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_BallisticsFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_DelayLine.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_DelayLine.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_DryWetMixer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_DryWetMixer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_FIRFilter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_FIRFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_FIRFilter_test.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_FirstOrderTPTFilter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_FirstOrderTPTFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_IIRFilter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_IIRFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_IIRFilter_Impl.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_LinkwitzRileyFilter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_LinkwitzRileyFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_Oversampling.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_Oversampling.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_Panner.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_Panner.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_ProcessContext.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_ProcessorChain.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_ProcessorChain_test.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_ProcessorDuplicator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_ProcessorWrapper.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_StateVariableFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_StateVariableTPTFilter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/processors/juce_StateVariableTPTFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Bias.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Chorus.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Chorus.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Compressor.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Compressor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Gain.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_LadderFilter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_LadderFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Limiter.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Limiter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_NoiseGate.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_NoiseGate.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Oscillator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Phaser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Phaser.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_Reverb.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/widgets/juce_WaveShaper.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/juce_dsp.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/juce_dsp.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_dsp/juce_dsp.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/broadcasters/juce_ActionBroadcaster.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/broadcasters/juce_ActionBroadcaster.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/broadcasters/juce_ActionListener.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/broadcasters/juce_AsyncUpdater.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/broadcasters/juce_AsyncUpdater.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/broadcasters/juce_ChangeBroadcaster.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/broadcasters/juce_ChangeBroadcaster.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/broadcasters/juce_ChangeListener.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/interprocess/juce_ConnectedChildProcess.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/interprocess/juce_ConnectedChildProcess.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/interprocess/juce_InterprocessConnection.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/interprocess/juce_InterprocessConnection.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/interprocess/juce_InterprocessConnectionServer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/interprocess/juce_InterprocessConnectionServer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/interprocess/juce_NetworkServiceDiscovery.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/interprocess/juce_NetworkServiceDiscovery.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_ApplicationBase.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_ApplicationBase.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_CallbackMessage.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_DeletedAtShutdown.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_DeletedAtShutdown.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_Initialisation.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_Message.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_MessageListener.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_MessageListener.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_MessageManager.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_MessageManager.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_MountedVolumeListChangeDetector.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/messages/juce_NotificationType.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/native/juce_android_Messaging.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/native/juce_ios_MessageManager.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/native/juce_linux_EventLoop.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/native/juce_linux_Messaging.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/native/juce_mac_MessageManager.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/native/juce_osx_MessageQueue.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/native/juce_win32_HiddenMessageWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/native/juce_win32_Messaging.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/native/juce_win32_WinRTWrapper.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/native/juce_win32_WinRTWrapper.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/timers/juce_MultiTimer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/timers/juce_MultiTimer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/timers/juce_Timer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/timers/juce_Timer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/juce_events.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/juce_events.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_events/juce_events.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/colour/juce_Colour.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/colour/juce_Colour.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/colour/juce_ColourGradient.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/colour/juce_ColourGradient.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/colour/juce_Colours.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/colour/juce_Colours.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/colour/juce_FillType.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/colour/juce_FillType.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/colour/juce_PixelFormats.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/contexts/juce_GraphicsContext.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/contexts/juce_GraphicsContext.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/contexts/juce_LowLevelGraphicsContext.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/contexts/juce_LowLevelGraphicsPostScriptRenderer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/contexts/juce_LowLevelGraphicsPostScriptRenderer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/contexts/juce_LowLevelGraphicsSoftwareRenderer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/contexts/juce_LowLevelGraphicsSoftwareRenderer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/effects/juce_DropShadowEffect.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/effects/juce_DropShadowEffect.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/effects/juce_GlowEffect.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/effects/juce_GlowEffect.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/effects/juce_ImageEffectFilter.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_AttributedString.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_AttributedString.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_CustomTypeface.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_CustomTypeface.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_Font.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_Font.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_GlyphArrangement.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_GlyphArrangement.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_TextLayout.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_TextLayout.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_Typeface.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/fonts/juce_Typeface.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_AffineTransform.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_AffineTransform.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_BorderSize.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_EdgeTable.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_EdgeTable.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_Line.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_Parallelogram.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_Path.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_Path.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_PathIterator.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_PathIterator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_PathStrokeType.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_PathStrokeType.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_Point.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_Rectangle.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/geometry/juce_RectangleList.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/cderror.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/changes to libjpeg for JUCE.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcapimin.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcapistd.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jccoefct.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jccolor.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcdctmgr.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jchuff.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jchuff.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcinit.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcmainct.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcmarker.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcmaster.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcomapi.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jconfig.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcparam.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcphuff.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcprepct.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jcsample.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jctrans.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdapimin.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdapistd.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdatasrc.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdcoefct.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdcolor.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdct.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jddctmgr.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdhuff.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdhuff.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdinput.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdmainct.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdmarker.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdmaster.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdmerge.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdphuff.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdpostct.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdsample.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jdtrans.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jerror.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jerror.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jfdctflt.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jfdctfst.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jfdctint.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jidctflt.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jidctfst.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jidctint.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jidctred.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jinclude.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jmemmgr.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jmemnobs.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jmemsys.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jmorecfg.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jpegint.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jpeglib.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jquant1.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jquant2.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jutils.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/jversion.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/transupp.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/jpglib/transupp.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/libpng_readme.txt" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/png.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/png.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngconf.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngdebug.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngerror.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngget.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pnginfo.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngmem.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngpread.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngpriv.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngread.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngrio.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngrtran.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngrutil.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngset.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngstruct.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngtrans.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngwio.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngwrite.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngwtran.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/pnglib/pngwutil.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/juce_GIFLoader.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/juce_JPEGLoader.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/image_formats/juce_PNGLoader.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/images/juce_Image.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/images/juce_Image.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/images/juce_ImageCache.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/images/juce_ImageCache.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/images/juce_ImageConvolutionKernel.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/images/juce_ImageConvolutionKernel.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/images/juce_ImageFileFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/images/juce_ImageFileFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_android_Fonts.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_android_GraphicsContext.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_android_IconHelpers.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_freetype_Fonts.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_linux_Fonts.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_linux_IconHelpers.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_mac_CoreGraphicsContext.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_mac_CoreGraphicsContext.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_mac_CoreGraphicsHelpers.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_mac_Fonts.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_mac_IconHelpers.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_RenderingHelpers.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_win32_Direct2DGraphicsContext.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_win32_Direct2DGraphicsContext.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_win32_DirectWriteTypeface.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_win32_DirectWriteTypeLayout.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_win32_Fonts.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/native/juce_win32_IconHelpers.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/placement/juce_Justification.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/placement/juce_RectanglePlacement.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/placement/juce_RectanglePlacement.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/juce_graphics.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/juce_graphics.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_graphics/juce_graphics.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/application/juce_Application.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/application/juce_Application.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_ArrowButton.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_ArrowButton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_Button.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_Button.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_DrawableButton.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_DrawableButton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_HyperlinkButton.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_HyperlinkButton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_ImageButton.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_ImageButton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_ShapeButton.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_ShapeButton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_TextButton.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_TextButton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_ToggleButton.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_ToggleButton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_ToolbarButton.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/buttons/juce_ToolbarButton.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandID.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandInfo.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandInfo.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandManager.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandManager.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandTarget.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/commands/juce_ApplicationCommandTarget.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/commands/juce_KeyPressMappingSet.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/commands/juce_KeyPressMappingSet.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/components/juce_CachedComponentImage.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/components/juce_Component.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/components/juce_Component.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/components/juce_ComponentListener.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/components/juce_ComponentListener.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/components/juce_ModalComponentManager.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/components/juce_ModalComponentManager.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/desktop/juce_Desktop.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/desktop/juce_Desktop.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/desktop/juce_Displays.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/desktop/juce_Displays.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_Drawable.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_Drawable.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableComposite.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableComposite.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableImage.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableImage.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawablePath.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawablePath.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableRectangle.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableRectangle.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableShape.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableShape.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableText.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_DrawableText.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/drawables/juce_SVGParser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_ContentSharer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_ContentSharer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_DirectoryContentsDisplayComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_DirectoryContentsDisplayComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_DirectoryContentsList.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_DirectoryContentsList.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileBrowserComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileBrowserComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileBrowserListener.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileChooser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileChooser.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileChooserDialogBox.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileChooserDialogBox.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileListComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileListComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FilenameComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FilenameComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FilePreviewComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileSearchPathListComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileSearchPathListComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileTreeComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_FileTreeComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_ImagePreviewComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/filebrowser/juce_ImagePreviewComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_CaretComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_CaretComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyboardFocusTraverser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyboardFocusTraverser.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyListener.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyListener.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyPress.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_KeyPress.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_ModifierKeys.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_ModifierKeys.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_SystemClipboard.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_TextEditorKeyMapper.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/keyboard/juce_TextInputTarget.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_AnimatedPosition.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_AnimatedPositionBehaviours.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentAnimator.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentAnimator.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentBoundsConstrainer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentBoundsConstrainer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentBuilder.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentBuilder.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentMovementWatcher.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ComponentMovementWatcher.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ConcertinaPanel.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ConcertinaPanel.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_FlexBox.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_FlexBox.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_FlexItem.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_Grid.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_Grid.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_GridItem.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_GridItem.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_GroupComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_GroupComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_MultiDocumentPanel.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_MultiDocumentPanel.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableBorderComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableBorderComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableCornerComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableCornerComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableEdgeComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ResizableEdgeComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ScrollBar.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_ScrollBar.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_SidePanel.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_SidePanel.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableLayoutManager.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableLayoutManager.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableLayoutResizerBar.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableLayoutResizerBar.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableObjectResizer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_StretchableObjectResizer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_TabbedButtonBar.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_TabbedButtonBar.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_TabbedComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_TabbedComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_Viewport.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/layout/juce_Viewport.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V1.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V1.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V2.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V2.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V3.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V3.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V4.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V4.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/menus/juce_BurgerMenuComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/menus/juce_BurgerMenuComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/menus/juce_MenuBarComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/menus/juce_MenuBarComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/menus/juce_MenuBarModel.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/menus/juce_MenuBarModel.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/menus/juce_PopupMenu.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/menus/juce_PopupMenu.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/misc/juce_BubbleComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/misc/juce_BubbleComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/misc/juce_DropShadower.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/misc/juce_DropShadower.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/misc/juce_JUCESplashScreen.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/misc/juce_JUCESplashScreen.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_ComponentDragger.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_ComponentDragger.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_DragAndDropContainer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_DragAndDropContainer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_DragAndDropTarget.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_FileDragAndDropTarget.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_LassoComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseCursor.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseCursor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseEvent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseEvent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseInactivityDetector.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseInactivityDetector.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseInputSource.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseInputSource.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseListener.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_MouseListener.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_SelectedItemSet.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_TextDragAndDropTarget.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/mouse/juce_TooltipClient.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/x11/juce_linux_X11_DragAndDrop.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/x11/juce_linux_X11_Symbols.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/x11/juce_linux_X11_Symbols.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/x11/juce_linux_XWindowSystem.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/x11/juce_linux_XWindowSystem.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_android_ContentSharer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_android_FileChooser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_android_Windowing.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_common_MimeTypes.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_ios_ContentSharer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_ios_FileChooser.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_ios_UIViewComponentPeer.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_ios_Windowing.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_linux_FileChooser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_linux_Windowing.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_mac_FileChooser.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_mac_MainMenu.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_mac_MouseCursor.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_mac_NSViewComponentPeer.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_mac_Windowing.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_MultiTouchMapper.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_win32_DragAndDrop.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_win32_FileChooser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/native/juce_win32_Windowing.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_MarkerList.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_MarkerList.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeCoordinate.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeCoordinate.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeCoordinatePositioner.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeCoordinatePositioner.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeParallelogram.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeParallelogram.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativePoint.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativePoint.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativePointPath.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativePointPath.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeRectangle.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/positioning/juce_RelativeRectangle.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_BooleanPropertyComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_BooleanPropertyComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_ButtonPropertyComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_ButtonPropertyComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_ChoicePropertyComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_ChoicePropertyComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_MultiChoicePropertyComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_MultiChoicePropertyComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_PropertyComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_PropertyComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_PropertyPanel.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_PropertyPanel.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_SliderPropertyComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_SliderPropertyComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_TextPropertyComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/properties/juce_TextPropertyComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ComboBox.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ComboBox.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ImageComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ImageComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_Label.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_Label.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ListBox.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ListBox.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ProgressBar.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ProgressBar.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_Slider.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_Slider.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_TableHeaderComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_TableHeaderComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_TableListBox.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_TableListBox.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_TextEditor.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_TextEditor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_Toolbar.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_Toolbar.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ToolbarItemComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ToolbarItemComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ToolbarItemFactory.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ToolbarItemPalette.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_ToolbarItemPalette.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_TreeView.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/widgets/juce_TreeView.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_AlertWindow.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_AlertWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_CallOutBox.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_CallOutBox.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_ComponentPeer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_ComponentPeer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_DialogWindow.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_DialogWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_DocumentWindow.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_DocumentWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_NativeMessageBox.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_ResizableWindow.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_ResizableWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_ThreadWithProgressWindow.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_ThreadWithProgressWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_TooltipWindow.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_TooltipWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_TopLevelWindow.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/windows/juce_TopLevelWindow.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/juce_gui_basics.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/juce_gui_basics.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_basics/juce_gui_basics.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CodeDocument.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CodeDocument.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CodeEditorComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CodeEditorComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CodeTokeniser.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CPlusPlusCodeTokeniser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CPlusPlusCodeTokeniser.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_CPlusPlusCodeTokeniserFunctions.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_LuaCodeTokeniser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_LuaCodeTokeniser.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_XMLCodeTokeniser.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/code_editor/juce_XMLCodeTokeniser.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/documents/juce_FileBasedDocument.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/documents/juce_FileBasedDocument.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/embedding/juce_ActiveXControlComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/embedding/juce_AndroidViewComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/embedding/juce_HWNDComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/embedding/juce_NSViewComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/embedding/juce_ScopedDPIAwarenessDisabler.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/embedding/juce_UIViewComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/embedding/juce_XEmbedComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_AnimatedAppComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_AnimatedAppComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_AppleRemote.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_BubbleMessageComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_BubbleMessageComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_ColourSelector.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_ColourSelector.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_KeyMappingEditorComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_KeyMappingEditorComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_LiveConstantEditor.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_LiveConstantEditor.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_PreferencesPanel.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_PreferencesPanel.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_PushNotifications.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_PushNotifications.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_RecentlyOpenedFilesList.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_RecentlyOpenedFilesList.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_SplashScreen.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_SplashScreen.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_SystemTrayIconComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_SystemTrayIconComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/misc/juce_WebBrowserComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_android_PushNotifications.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_android_WebBrowserComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_AndroidViewComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_ios_PushNotifications.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_ios_UIViewComponent.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_linux_X11_SystemTrayIcon.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_linux_X11_WebBrowserComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_linux_XEmbedComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_mac_AppleRemote.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_mac_CarbonViewWrapperComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_mac_NSViewComponent.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_mac_PushNotifications.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_mac_SystemTrayIcon.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_mac_WebBrowserComponent.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_win32_ActiveXComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_win32_HWNDComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_win32_SystemTrayIcon.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/native/juce_win32_WebBrowserComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/juce_gui_extra.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/juce_gui_extra.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_gui_extra/juce_gui_extra.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/geometry/juce_Draggable3DOrientation.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/geometry/juce_Matrix3D.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/geometry/juce_Quaternion.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/geometry/juce_Vector3D.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/native/juce_MissingGLDefinitions.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/native/juce_OpenGL_android.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/native/juce_OpenGL_ios.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/native/juce_OpenGL_linux_X11.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/native/juce_OpenGL_osx.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/native/juce_OpenGL_win32.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/native/juce_OpenGLExtensions.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLContext.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLContext.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLFrameBuffer.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLFrameBuffer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLGraphicsContext.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLGraphicsContext.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLHelpers.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLHelpers.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLImage.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLImage.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLPixelFormat.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLPixelFormat.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLRenderer.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLShaderProgram.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLShaderProgram.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLTexture.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/opengl/juce_OpenGLTexture.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/utils/juce_OpenGLAppComponent.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/utils/juce_OpenGLAppComponent.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/juce_opengl.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/juce_opengl.mm" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../deps/juce/modules/juce_opengl/juce_opengl.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../mobile/JuceLibraryCode/AppConfig.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../mobile/JuceLibraryCode/BinaryData.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../mobile/JuceLibraryCode/JuceHeader.h" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../../../mobile/JuceLibraryCode/JucePluginDefines.h" PROPERTIES HEADER_FILE_ONLY TRUE)

target_compile_options( ${BINARY_NAME} PRIVATE "-fsigned-char" )

if( JUCE_BUILD_CONFIGURATION MATCHES "DEBUG" )
    target_compile_options( ${BINARY_NAME} PRIVATE)
endif()

if( JUCE_BUILD_CONFIGURATION MATCHES "RELEASE" )
    target_compile_options( ${BINARY_NAME} PRIVATE)
endif()

find_library(log "log")
find_library(android "android")
find_library(glesv3 "GLESv3")
find_library(egl "EGL")

target_link_libraries( ${BINARY_NAME}

    ${log}
    ${android}
    ${glesv3}
    ${egl}
    ${opus}
    "cpufeatures"
    "oboe"
)

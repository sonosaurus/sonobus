#!/bin/bash -x

# Comprehensive OSC Test Script for SonoBus
# Sends OSC messages to localhost port 6000
# Each message is followed by a sleep for UI control verification

echo "=== Audio Controls ==="
oscsend localhost 6000 /DrySlider f 1.0
sleep 2
oscsend localhost 6000 /DrySlider f 0.5
sleep 2

oscsend localhost 6000 /OutGainSlider f 1.0
sleep 2
oscsend localhost 6000 /OutGainSlider f 0.5
sleep 2

echo "=== Main Control Buttons ==="
oscsend localhost 6000 /MainMuteButton i 1
sleep 2
oscsend localhost 6000 /MainMuteButton i 0
sleep 2

oscsend localhost 6000 /MainRecvMuteButton i 1
sleep 2
oscsend localhost 6000 /MainRecvMuteButton i 0
sleep 2

oscsend localhost 6000 /MainPushToTalkButton i 1
sleep 2
oscsend localhost 6000 /MainPushToTalkButton i 0
sleep 2

echo "=== Input Monitor Controls ==="
oscsend localhost 6000 /InMuteButton i 1
sleep 2
oscsend localhost 6000 /InMuteButton i 0
sleep 2

oscsend localhost 6000 /InSoloButton i 1
sleep 2
oscsend localhost 6000 /InSoloButton i 0
sleep 2

echo "=== Metronome Controls ==="
oscsend localhost 6000 /MetEnableButton i 1
sleep 2
oscsend localhost 6000 /MetEnableButton i 0
sleep 2

oscsend localhost 6000 /MetLevelSlider f 0.8
sleep 2
oscsend localhost 6000 /MetLevelSlider f 0.4
sleep 2

oscsend localhost 6000 /MetTempoSlider f 120.0
sleep 2
oscsend localhost 6000 /MetTempoSlider f 60.0
sleep 2

oscsend localhost 6000 /MetSyncButton i 1
sleep 2
oscsend localhost 6000 /MetSyncButton i 0
sleep 2

oscsend localhost 6000 /MetSyncFileButton i 1
sleep 2
oscsend localhost 6000 /MetSyncFileButton i 0
sleep 2

oscsend localhost 6000 /MetSendButton i 1
sleep 2
oscsend localhost 6000 /MetSendButton i 0
sleep 2

oscsend localhost 6000 /MetPanSlider f -0.5
sleep 2
oscsend localhost 6000 /MetPanSlider f 0.5
sleep 2

oscsend localhost 6000 /MetMonitorSlider f 0.8
sleep 2
oscsend localhost 6000 /MetMonitorSlider f 0.4
sleep 2

echo "=== Utility Buttons ==="
oscsend localhost 6000 /BufferMinButton i 1
sleep 2

oscsend localhost 6000 /RecvSyncButton i 1
sleep 2

oscsend localhost 6000 /RecordingButton i 1
sleep 2
oscsend localhost 6000 /RecordingButton i 0
sleep 2

echo "=== Main Reverb Controls ==="
oscsend localhost 6000 /MainReverbEnabled i 1
sleep 2
oscsend localhost 6000 /MainReverbEnabled i 0
sleep 2

oscsend localhost 6000 /ReverbLevelSlider f 1.0
sleep 2
oscsend localhost 6000 /ReverbLevelSlider f 0.5
sleep 2

oscsend localhost 6000 /ReverbSizeSlider f 1.0
sleep 2
oscsend localhost 6000 /ReverbSizeSlider f 0.5
sleep 2

oscsend localhost 6000 /ReverbDampingSlider f 1.0
sleep 2
oscsend localhost 6000 /ReverbDampingSlider f 0.5
sleep 2

oscsend localhost 6000 /ReverbPreDelaySlider f 20.0
sleep 2
oscsend localhost 6000 /ReverbPreDelaySlider f 60.0
sleep 2

echo "=== Input Reverb Controls ==="
oscsend localhost 6000 /InReverbButton i 1
sleep 2
oscsend localhost 6000 /InReverbButton i 0
sleep 2

oscsend localhost 6000 /InputReverbLevel f 0.8
sleep 2
oscsend localhost 6000 /InputReverbLevel f 0.4
sleep 2

oscsend localhost 6000 /InputReverbSize f 0.9
sleep 2
oscsend localhost 6000 /InputReverbSize f 0.5
sleep 2

oscsend localhost 6000 /InputReverbDamping f 0.7
sleep 2
oscsend localhost 6000 /InputReverbDamping f 0.3
sleep 2

oscsend localhost 6000 /InputReverbPreDelay f 30.0
sleep 2
oscsend localhost 6000 /InputReverbPreDelay f 10.0
sleep 2

echo "=== Monitor Delay ==="
oscsend localhost 6000 /MonDelayButton i 1
sleep 2
oscsend localhost 6000 /MonDelayButton i 0
sleep 2

echo "=== File Playback Controls ==="
oscsend localhost 6000 /FileSendButton i 1
sleep 2
oscsend localhost 6000 /FileSendButton i 0
sleep 2

oscsend localhost 6000 /PlaybackSlider f 0.9
sleep 2
oscsend localhost 6000 /PlaybackSlider f 0.5
sleep 2

oscsend localhost 6000 /FileMonitorSlider f 0.8
sleep 2
oscsend localhost 6000 /FileMonitorSlider f 0.4
sleep 2

oscsend localhost 6000 /FilePlaybackPreLevel f 0.7
sleep 2
oscsend localhost 6000 /FilePlaybackPreLevel f 0.3
sleep 2

echo "=== Soundboard Controls ==="
oscsend localhost 6000 /SoundboardSendButton i 1
sleep 2
oscsend localhost 6000 /SoundboardSendButton i 0
sleep 2

oscsend localhost 6000 /SoundboardLevelSlider f 0.9
sleep 2
oscsend localhost 6000 /SoundboardLevelSlider f 0.5
sleep 2

oscsend localhost 6000 /SoundboardMonitorSlider f 0.8
sleep 2
oscsend localhost 6000 /SoundboardMonitorSlider f 0.4
sleep 2

echo "=== Input Group 1 Controls ==="
oscsend localhost 6000 /InputGroup1PreLevel f 0.8
sleep 2
oscsend localhost 6000 /InputGroup1PreLevel f 0.5
sleep 2

oscsend localhost 6000 /InputGroup1Pan f -0.5
sleep 2
oscsend localhost 6000 /InputGroup1Pan f 0.5
sleep 2

oscsend localhost 6000 /InputGroup1PanLeft f -0.3
sleep 2
oscsend localhost 6000 /InputGroup1PanLeft f 0.3
sleep 2

oscsend localhost 6000 /InputGroup1PanRight f -0.4
sleep 2
oscsend localhost 6000 /InputGroup1PanRight f 0.4
sleep 2

oscsend localhost 6000 /InputGroup1Monitor f 0.7
sleep 2
oscsend localhost 6000 /InputGroup1Monitor f 0.3
sleep 2

echo "=== Input Group 1 M.FX Controls ==="
oscsend localhost 6000 /InputGroup1MonDelayEnable i 1
sleep 2
oscsend localhost 6000 /InputGroup1MonDelayEnable i 0
sleep 2

oscsend localhost 6000 /InputGroup1MonDelayTime f 100.0
sleep 2
oscsend localhost 6000 /InputGroup1MonDelayTime f 50.0
sleep 2

oscsend localhost 6000 /InputGroup1MonDelayLink i 1
sleep 2
oscsend localhost 6000 /InputGroup1MonDelayLink i 0
sleep 2

oscsend localhost 6000 /InputGroup1MonReverbSend f 0.8
sleep 2
oscsend localhost 6000 /InputGroup1MonReverbSend f 0.4
sleep 2

echo "=== Input Group 1 Mute/Solo Controls ==="
oscsend localhost 6000 /InputGroup1Mute i 1
sleep 2
oscsend localhost 6000 /InputGroup1Mute i 0
sleep 2

oscsend localhost 6000 /InputGroup1Solo i 1
sleep 2
oscsend localhost 6000 /InputGroup1Solo i 0
sleep 2

echo "=== Input Group 1 FX Controls ==="
oscsend localhost 6000 /InputGroup1InputReverbSend f 0.6
sleep 2
oscsend localhost 6000 /InputGroup1InputReverbSend f 0.3
sleep 2

oscsend localhost 6000 /InputGroup1PolarityInvert i 1
sleep 2
oscsend localhost 6000 /InputGroup1PolarityInvert i 0
sleep 2

echo "=== Input Group 1 Compressor Controls ==="
oscsend localhost 6000 /InputGroup1CompressorEnable i 1
sleep 2
oscsend localhost 6000 /InputGroup1CompressorThreshold f -20.0
sleep 2
oscsend localhost 6000 /InputGroup1CompressorRatio f 4.0
sleep 2
oscsend localhost 6000 /InputGroup1CompressorAttack f 15.0
sleep 2
oscsend localhost 6000 /InputGroup1CompressorRelease f 100.0
sleep 2
oscsend localhost 6000 /InputGroup1CompressorMakeupGain f 10.0
sleep 2
oscsend localhost 6000 /InputGroup1CompressorAuto i 1
sleep 2
oscsend localhost 6000 /InputGroup1CompressorAuto i 0
sleep 2
oscsend localhost 6000 /InputGroup1CompressorEnable i 0
sleep 2

echo "=== Input Group 1 Expander (Noise Gate) Controls ==="
oscsend localhost 6000 /InputGroup1ExpanderEnable i 1
sleep 2
oscsend localhost 6000 /InputGroup1ExpanderNoiseFloor f -40.0
sleep 2
oscsend localhost 6000 /InputGroup1ExpanderRatio f 3.0
sleep 2
oscsend localhost 6000 /InputGroup1ExpanderAttack f 5.0
sleep 2
oscsend localhost 6000 /InputGroup1ExpanderRelease f 150.0
sleep 2
oscsend localhost 6000 /InputGroup1ExpanderEnable i 0
sleep 2

echo "=== Input Group 1 Parametric EQ Controls ==="
oscsend localhost 6000 /InputGroup1EqEnable i 1
sleep 2
oscsend localhost 6000 /InputGroup1EqLowShelfFreq f 80.0
sleep 2
oscsend localhost 6000 /InputGroup1EqLowShelfGain f 3.0
sleep 2
oscsend localhost 6000 /InputGroup1EqPara1Freq f 200.0
sleep 2
oscsend localhost 6000 /InputGroup1EqPara1Gain f -2.0
sleep 2
oscsend localhost 6000 /InputGroup1EqPara1Q f 2.0
sleep 2
oscsend localhost 6000 /InputGroup1EqHighShelfFreq f 8000.0
sleep 2
oscsend localhost 6000 /InputGroup1EqHighShelfGain f -3.0
sleep 2
oscsend localhost 6000 /InputGroup1EqPara2Freq f 500.0
sleep 2
oscsend localhost 6000 /InputGroup1EqPara2Gain f 4.0
sleep 2
oscsend localhost 6000 /InputGroup1EqPara2Q f 3.0
sleep 2
oscsend localhost 6000 /InputGroup1EqEnable i 0
sleep 2

echo "=== Input Group 2 Controls (if exists) ==="
oscsend localhost 6000 /InputGroup2PreLevel f 0.7
sleep 2
oscsend localhost 6000 /InputGroup2PreLevel f 0.4
sleep 2

oscsend localhost 6000 /InputGroup2Pan f -0.3
sleep 2
oscsend localhost 6000 /InputGroup2Pan f 0.3
sleep 2

oscsend localhost 6000 /InputGroup2Monitor f 0.6
sleep 2
oscsend localhost 6000 /InputGroup2Monitor f 0.2
sleep 2

echo "=== Options Tab Controls ==="
oscsend localhost 6000 /OptionsDynamicResamplingButton i 1
sleep 2
oscsend localhost 6000 /OptionsDynamicResamplingButton i 0
sleep 2

oscsend localhost 6000 /OptionsUseSpecificUdpPortButton i 1
sleep 2
oscsend localhost 6000 /OptionsUseSpecificUdpPortButton i 0
sleep 2

oscsend localhost 6000 /OptionsUdpPortEditor i 12000
sleep 2
oscsend localhost 6000 /OptionsUdpPortEditor i 13000
sleep 2

oscsend localhost 6000 /OptionsOverrideSamplerateButton i 1
sleep 2
oscsend localhost 6000 /OptionsOverrideSamplerateButton i 0
sleep 2

oscsend localhost 6000 /OptionsShouldCheckForUpdateButton i 1
sleep 2
oscsend localhost 6000 /OptionsShouldCheckForUpdateButton i 0
sleep 2

oscsend localhost 6000 /OptionsAutoReconnectButton i 1
sleep 2
oscsend localhost 6000 /OptionsAutoReconnectButton i 0
sleep 2

oscsend localhost 6000 /OptionsSliderSnapToMouseButton i 1
sleep 2
oscsend localhost 6000 /OptionsSliderSnapToMouseButton i 0
sleep 2

oscsend localhost 6000 /OptionsDisableShortcutButton i 1
sleep 2
oscsend localhost 6000 /OptionsDisableShortcutButton i 0
sleep 2

oscsend localhost 6000 /OptionsInputLimiterButton i 1
sleep 2
oscsend localhost 6000 /OptionsInputLimiterButton i 0
sleep 2

oscsend localhost 6000 /OptionsDefaultLevelSlider f 0.73
sleep 2
oscsend localhost 6000 /OptionsDefaultLevelSlider f 0.1
sleep 2

oscsend localhost 6000 /OptionsMaxRecvPaddingSlider f 20.0
sleep 2
oscsend localhost 6000 /OptionsMaxRecvPaddingSlider f 10.0
sleep 2

oscsend localhost 6000 /OptionsAutoDropThreshSlider f 1.0
sleep 2
oscsend localhost 6000 /OptionsAutoDropThreshSlider f 33.0
sleep 2

oscsend localhost 6000 /BufferTimeSlider f 1.0
sleep 2
oscsend localhost 6000 /BufferTimeSlider f 33.0
sleep 2

oscsend localhost 6000 /OptionsChangeAllFormatButton i 1
sleep 2
oscsend localhost 6000 /OptionsChangeAllFormatButton i 0
sleep 2

oscsend localhost 6000 /OptionsAutosizeDefaultChoice i 1
sleep 2
oscsend localhost 6000 /OptionsAutosizeDefaultChoice i 0
sleep 2

oscsend localhost 6000 /OptionsFormatChoiceDefaultChoice i 2
sleep 2
oscsend localhost 6000 /OptionsFormatChoiceDefaultChoice i 1
sleep 2

echo "=== OSC Configuration Controls ==="
oscsend localhost 6000 /OSCTargetIPAddress s "127.0.0.2"
sleep 2
oscsend localhost 6000 /OSCTargetIPAddress s "127.0.0.1"
sleep 2

oscsend localhost 6000 /OSCTargetPort i 9999
sleep 2
oscsend localhost 6000 /OSCTargetPort i 6001
sleep 2

# Note: OSCReceivePort commented out to avoid changing port during test
# oscsend localhost 6000 /OSCReceivePort i 9998
# sleep 2
# oscsend localhost 6000 /OSCReceivePort i 6000
# sleep 2

echo "=== Recording Options ==="
oscsend localhost 6000 /OptionsRecStealth i 1
sleep 2
oscsend localhost 6000 /OptionsRecStealth i 0
sleep 2

oscsend localhost 6000 /OptionsRecSelfSilenceMutedButton i 1
sleep 2
oscsend localhost 6000 /OptionsRecSelfSilenceMutedButton i 0
sleep 2

oscsend localhost 6000 /OptionsRecSelfPostFxButton i 1
sleep 2
oscsend localhost 6000 /OptionsRecSelfPostFxButton i 0
sleep 2

oscsend localhost 6000 /OptionsMetRecordedButton i 1
sleep 2
oscsend localhost 6000 /OptionsMetRecordedButton i 0
sleep 2

oscsend localhost 6000 /OptionsRecMixButton i 1
sleep 2
oscsend localhost 6000 /OptionsRecMixButton i 0
sleep 2

oscsend localhost 6000 /OptionsRecSelfButton i 1
sleep 2
oscsend localhost 6000 /OptionsRecSelfButton i 0
sleep 2

oscsend localhost 6000 /OptionsRecOthersButton i 1
sleep 2
oscsend localhost 6000 /OptionsRecOthersButton i 0
sleep 2

oscsend localhost 6000 /OptionsRecMixMinusButton i 1
sleep 2
oscsend localhost 6000 /OptionsRecMixMinusButton i 0
sleep 2

echo "=== Peer 1 Controls ==="
oscsend localhost 6000 /Peer1Mute f 1.0
sleep 2
oscsend localhost 6000 /Peer1Mute f 0.0
sleep 2

oscsend localhost 6000 /Peer1Solo f 1.0
sleep 2
oscsend localhost 6000 /Peer1Solo f 0.0
sleep 2

echo "=== Peer 1 Utility Controls ==="
oscsend localhost 6000 /Peer1BufferMin i 1
sleep 2

oscsend localhost 6000 /Peer1ResetDrop i 1
sleep 2

echo "=== Peer 1 Level Control ==="
oscsend localhost 6000 /Peer1Level f 0.8
sleep 2
oscsend localhost 6000 /Peer1Level f 1.0
sleep 2
oscsend localhost 6000 /Peer1Level f 1.2
sleep 2

echo "=== Peer 1 Pan Control ==="
oscsend localhost 6000 /Peer1Pan f -1.0
sleep 2
oscsend localhost 6000 /Peer1Pan f 0.0
sleep 2
oscsend localhost 6000 /Peer1Pan f 1.0
sleep 2

echo "=== Peer 1 FX Controls ==="
oscsend localhost 6000 /Peer1InputReverbSend f 0.6
sleep 2
oscsend localhost 6000 /Peer1InputReverbSend f 0.3
sleep 2

oscsend localhost 6000 /Peer1PolarityInvert i 1
sleep 2
oscsend localhost 6000 /Peer1PolarityInvert i 0
sleep 2

echo "=== Peer 1 Compressor Controls ==="
oscsend localhost 6000 /Peer1CompressorEnable i 1
sleep 2
oscsend localhost 6000 /Peer1CompressorThreshold f -20.0
sleep 2
oscsend localhost 6000 /Peer1CompressorRatio f 4.0
sleep 2
oscsend localhost 6000 /Peer1CompressorAttack f 15.0
sleep 2
oscsend localhost 6000 /Peer1CompressorRelease f 100.0
sleep 2
oscsend localhost 6000 /Peer1CompressorMakeupGain f 10.0
sleep 2
oscsend localhost 6000 /Peer1CompressorAuto i 1
sleep 2
oscsend localhost 6000 /Peer1CompressorAuto i 0
sleep 2
oscsend localhost 6000 /Peer1CompressorEnable i 0
sleep 2

echo "=== Peer 1 Expander (Noise Gate) Controls ==="
oscsend localhost 6000 /Peer1ExpanderEnable i 1
sleep 2
oscsend localhost 6000 /Peer1ExpanderNoiseFloor f -40.0
sleep 2
oscsend localhost 6000 /Peer1ExpanderRatio f 3.0
sleep 2
oscsend localhost 6000 /Peer1ExpanderAttack f 5.0
sleep 2
oscsend localhost 6000 /Peer1ExpanderRelease f 150.0
sleep 2
oscsend localhost 6000 /Peer1ExpanderEnable i 0
sleep 2

echo "=== Peer 1 Parametric EQ Controls ==="
oscsend localhost 6000 /Peer1EqEnable i 1
sleep 2
oscsend localhost 6000 /Peer1EqLowShelfFreq f 80.0
sleep 2
oscsend localhost 6000 /Peer1EqLowShelfGain f 3.0
sleep 2
oscsend localhost 6000 /Peer1EqPara1Freq f 200.0
sleep 2
oscsend localhost 6000 /Peer1EqPara1Gain f -2.0
sleep 2
oscsend localhost 6000 /Peer1EqPara1Q f 2.0
sleep 2
oscsend localhost 6000 /Peer1EqHighShelfFreq f 8000.0
sleep 2
oscsend localhost 6000 /Peer1EqHighShelfGain f -3.0
sleep 2
oscsend localhost 6000 /Peer1EqPara2Freq f 500.0
sleep 2
oscsend localhost 6000 /Peer1EqPara2Gain f 4.0
sleep 2
oscsend localhost 6000 /Peer1EqPara2Q f 3.0
sleep 2
oscsend localhost 6000 /Peer1EqEnable i 0
sleep 2

echo "=== Peer 2 Controls (if connected) ==="
oscsend localhost 6000 /Peer2Mute f 1.0
sleep 2
oscsend localhost 6000 /Peer2Mute f 0.0
sleep 2

oscsend localhost 6000 /Peer2Solo f 1.0
sleep 2
oscsend localhost 6000 /Peer2Solo f 0.0
sleep 2

oscsend localhost 6000 /Peer2InputReverbSend f 0.5
sleep 2
oscsend localhost 6000 /Peer2InputReverbSend f 0.2
sleep 2

oscsend localhost 6000 /Peer2Level f 0.9
sleep 2

oscsend localhost 6000 /Peer2Pan f 0.5
sleep 2

echo "=== OSC Test Complete ==="

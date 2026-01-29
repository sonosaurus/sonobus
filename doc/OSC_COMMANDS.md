# SonoBus OSC Commands Documentation

This document describes all available OSC (Open Sound Control) commands for controlling SonoBus remotely.

## Overview

SonoBus supports bidirectional OSC communication, allowing external controllers to both send commands to control the application and receive status updates when controls change.

## Configuration

OSC can be configured in the Options tab:
- **Enable OSC**: Toggle OSC functionality on/off
- **Target IP Address**: IP address to send OSC messages to (default: 127.0.0.1)
- **Target Port**: Port to send OSC messages to (default: 9000)
- **Receive Port**: Port to listen for incoming OSC messages (default: 9001)

## OSC Address Format

All OSC addresses follow the pattern: `/<ControlName>`

## Control Types

### Sliders (Float Values)
Sliders accept and send float values representing the parameter value.
- **Send**: `/address <float value>`
- **Receive**: Application sends float value when slider changes

### Toggle Buttons (On/Off State)
Toggle buttons accept and send integer values (0 = off, 1 = on).
- **Send**: `/address <int 0|1>`
- **Receive**: Application sends 0 or 1 when toggle state changes

### Push Buttons (Trigger Actions)
Push buttons accept integer value 1 to trigger their action.
- **Send**: `/address <int 1>`
- **Receive**: Application sends 1 when button is clicked

### Push-Hold Buttons (Press/Release)
Push-hold buttons accept integer values (1 = press, 0 = release).
- **Send**: `/address <int 0|1>`
- **Receive**: Application sends 1 on press, 0 on release

### Choice Buttons (Selection)
Choice buttons accept and send integer values representing the selected option ID.
- **Send**: `/address <int option_id>`
- **Receive**: Application sends option ID when selection changes

## Main UI Controls

### Audio Controls

#### `/DrySlider`
**Type**: Slider  
**Description**: Controls the monitor (dry) level - your own audio that you hear  
**Data Type**: Float  
**Range**: Audio level in decibels  
**Parameter**: `paramDry`

#### `/OutGainSlider`
**Type**: Slider  
**Description**: Controls the output (wet) level - mix of all incoming audio  
**Data Type**: Float  
**Range**: Audio level in decibels  
**Parameter**: `paramWet`

### Main Control Buttons

#### `/MainMuteButton`
**Type**: Toggle Button  
**Description**: Mutes/unmutes sending your audio to others  
**Data Type**: Integer (0 = unmuted/sending, 1 = muted/not sending)  
**Parameter**: `paramMainSendMute`

#### `/MainRecvMuteButton`
**Type**: Toggle Button  
**Description**: Mutes/unmutes receiving audio from all other users  
**Data Type**: Integer (0 = unmuted/receiving, 1 = muted/not receiving)  
**Parameter**: `paramMainRecvMute`

#### `/MainPushToTalkButton`
**Type**: Push-Hold Button  
**Description**: Push to talk - temporarily unmutes sending and mutes receiving  
**Data Type**: Integer (1 = press/talk, 0 = release/listen)  
**Behavior**: 
- When pressed (1): Unmutes send, mutes receive
- When released (0): Restores previous send mute state, unmutes receive

### Metronome Controls

#### `/MetEnableButton`
**Type**: Toggle Button  
**Description**: Enables/disables the metronome  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Parameter**: `paramMetEnabled`

#### `/MetLevelSlider`
**Type**: Slider  
**Description**: Controls the metronome level/volume  
**Data Type**: Float  
**Range**: Audio level  
**Parameter**: `paramMetGain`

#### `/MetTempoSlider`
**Type**: Slider  
**Description**: Controls the metronome tempo in BPM  
**Data Type**: Float  
**Range**: Tempo in beats per minute  
**Parameter**: `paramMetTempo`

#### `/MetSyncButton`
**Type**: Toggle Button  
**Description**: Synchronizes metronome to host DAW tempo  
**Data Type**: Integer (0 = not synced, 1 = synced to host)  
**Parameter**: `paramSyncMetToHost`

#### `/MetSyncFileButton`
**Type**: Toggle Button  
**Description**: Synchronizes metronome to file playback tempo  
**Data Type**: Integer (0 = not synced, 1 = synced to file)  
**Parameter**: `paramSyncMetToFilePlayback`

### Input Monitor Controls

#### `/InMuteButton`
**Type**: Toggle Button  
**Description**: Mutes/unmutes your input monitor (what you hear of your own audio)  
**Data Type**: Integer (0 = unmuted, 1 = muted)  
**Parameter**: `paramMainInMute`

#### `/InSoloButton`
**Type**: Toggle Button  
**Description**: Solos your input monitor  
**Data Type**: Integer (0 = not soloed, 1 = soloed)  
**Parameter**: `paramMainMonitorSolo`

### Utility Buttons

#### `/BufferMinButton`
**Type**: Push Button  
**Description**: Resets the jitter buffer to minimum for all connected users  
**Data Type**: Integer (1 to trigger)  
**Action**: Resets all peer jitter buffers

#### `/RecvSyncButton`
**Type**: Push Button  
**Description**: Synchronizes receive latency for all users  
**Data Type**: Integer (1 to trigger)  
**Action**: Adjusts all peer jitter buffers to match the highest latency

#### `/RecordingButton`
**Type**: Toggle Button  
**Description**: Starts/stops recording audio to file  
**Data Type**: Integer (0 = stop recording, 1 = start recording)  
**Behavior**: Toggles recording state
- When turned on: Starts recording with current settings
- When turned off: Stops recording and saves file

### Main Reverb Controls

#### `/MainReverbEnabled`
**Type**: Toggle Button  
**Description**: Enables/disables the main reverb effect  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Parameter**: `paramMainReverbEnabled`

#### `/ReverbLevelSlider`
**Type**: Slider  
**Description**: Controls the reverb level/mix  
**Data Type**: Float  
**Range**: Audio level  
**Parameter**: `paramMainReverbLevel`

#### `/ReverbSizeSlider`
**Type**: Slider  
**Description**: Controls the reverb room size  
**Data Type**: Float  
**Range**: Room size parameter  
**Parameter**: `paramMainReverbSize`

#### `/ReverbDampingSlider`
**Type**: Slider  
**Description**: Controls the reverb damping (high frequency absorption)  
**Data Type**: Float  
**Range**: Damping parameter  
**Parameter**: `paramMainReverbDamping`

#### `/ReverbPreDelaySlider`
**Type**: Slider  
**Description**: Controls the reverb pre-delay time  
**Data Type**: Float  
**Range**: Pre-delay time in milliseconds  
**Parameter**: `paramMainReverbPreDelay`

### File Playback Controls

#### `/FileSendButton`
**Type**: Toggle Button  
**Description**: Enables/disables sending file playback audio to all users  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Parameter**: `paramSendFileAudio`

#### `/PlaybackSlider`
**Type**: Slider  
**Description**: Controls the file playback level/gain  
**Data Type**: Float  
**Range**: Audio level (0.0 - 2.0, where 1.0 is unity gain)  
**Action**: Sets file playback volume

#### `/FileMonitorSlider`
**Type**: Slider  
**Description**: Controls the file playback monitor level (what you hear)  
**Data Type**: Float  
**Range**: Audio level  
**Action**: Sets file playback monitor volume

### Soundboard Controls

#### `/SoundboardSendButton`
**Type**: Toggle Button  
**Description**: Enables/disables sending soundboard audio to all users  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Parameter**: `paramSendSoundboardAudio`

#### `/SoundboardLevelSlider`
**Type**: Slider  
**Description**: Controls the soundboard output level/gain  
**Data Type**: Float  
**Range**: Audio level  
**Action**: Sets soundboard output volume

#### `/SoundboardMonitorSlider`
**Type**: Slider  
**Description**: Controls the soundboard monitor level (what you hear)  
**Data Type**: Float  
**Range**: Audio level  
**Action**: Sets soundboard monitor volume

### Metronome Advanced Controls

#### `/MetSendButton`
**Type**: Toggle Button  
**Description**: Enables/disables sending metronome to all users  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Parameter**: `paramSendMetAudio`

#### `/MetPanSlider`
**Type**: Slider  
**Description**: Controls the metronome stereo pan position  
**Data Type**: Float  
**Range**: -1.0 (full left) to 1.0 (full right), 0.0 is center  
**Action**: Sets metronome pan position

#### `/MetMonitorSlider`
**Type**: Slider  
**Description**: Controls the metronome monitor level (what you hear)  
**Data Type**: Float  
**Range**: Audio level  
**Action**: Sets metronome monitor volume

### Input Reverb Controls

#### `/InReverbButton`
**Type**: Toggle Button  
**Description**: Opens/closes the input reverb configuration panel  
**Data Type**: Integer (1 to trigger)  
**Action**: Toggles input reverb panel visibility

#### `/InputReverbLevel`
**Type**: Slider  
**Description**: Controls the input reverb level/mix  
**Data Type**: Float  
**Range**: Audio level  
**Parameter**: `paramInputReverbLevel`

#### `/InputReverbSize`
**Type**: Slider  
**Description**: Controls the input reverb room size  
**Data Type**: Float  
**Range**: Room size parameter  
**Parameter**: `paramInputReverbSize`

#### `/InputReverbDamping`
**Type**: Slider  
**Description**: Controls the input reverb damping (high frequency absorption)  
**Data Type**: Float  
**Range**: Damping parameter  
**Parameter**: `paramInputReverbDamping`

#### `/InputReverbPreDelay`
**Type**: Slider  
**Description**: Controls the input reverb pre-delay time  
**Data Type**: Float  
**Range**: Pre-delay time in milliseconds  
**Parameter**: `paramInputReverbPreDelay`

### Monitor Delay Controls

#### `/MonDelayButton`
**Type**: Toggle Button  
**Description**: Toggles monitor delay enabled on all input groups  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Action**: Enables/disables monitor delay for all inputs

### Input Group Pre Level Controls

SonoBus supports OSC control for up to 16 Input Groups. Each Input Group can be controlled independently using numbered OSC addresses.

#### `/InputGroup[1-16]PreLevel`
**Type**: Slider  
**Description**: Controls the pre-fader level (gain) for the specified Input Group  
**Data Type**: Float  
**Range**: Audio level (0.0 - 2.0, where 1.0 is unity gain)  
**Examples**:
- `/InputGroup1PreLevel` - Controls Pre Level for Input Group 1
- `/InputGroup2PreLevel` - Controls Pre Level for Input Group 2
- `/InputGroup16PreLevel` - Controls Pre Level for Input Group 16

**Note**: Input Groups are numbered 1-16. Only Input Groups that exist at runtime will respond to OSC messages. The number and configuration of Input Groups is determined by the user's audio setup.

#### `/InputGroup[1-16]Pan`
**Type**: Slider  
**Description**: Controls the stereo pan position for single-channel Input Groups  
**Data Type**: Float  
**Range**: -1.0 (hard left) to 1.0 (hard right), 0.0 = center  
**Examples**:
- `/InputGroup1Pan` - Controls Pan for Input Group 1
- `/InputGroup2Pan` - Controls Pan for Input Group 2

#### `/InputGroup[1-16]PanLeft`
**Type**: Slider  
**Description**: Controls the left channel pan for dual-channel Input Groups  
**Data Type**: Float  
**Range**: -1.0 (hard left) to 1.0 (hard right), 0.0 = center  
**Note**: Used when Input Group has stereo input channels

#### `/InputGroup[1-16]PanRight`
**Type**: Slider  
**Description**: Controls the right channel pan for dual-channel Input Groups  
**Data Type**: Float  
**Range**: -1.0 (hard left) to 1.0 (hard right), 0.0 = center  
**Note**: Used when Input Group has stereo input channels

#### `/InputGroup[1-16]Monitor`
**Type**: Slider  
**Description**: Controls the monitor level (local monitoring gain) for the specified Input Group  
**Data Type**: Float  
**Range**: Audio level (0.0 - 2.0, where 1.0 is unity gain)  
**Examples**:
- `/InputGroup1Monitor` - Controls Monitor level for Input Group 1
- `/InputGroup2Monitor` - Controls Monitor level for Input Group 2

#### `/InputGroup[1-16]MonDelayEnable`
**Type**: Toggle Button  
**Description**: Enables/disables additional monitoring delay for the specified Input Group  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Note**: Part of the M.FX (Monitoring Effects) controls

#### `/InputGroup[1-16]MonDelayTime`
**Type**: Slider  
**Description**: Sets the monitoring delay time in milliseconds for the specified Input Group  
**Data Type**: Float  
**Range**: 0.0 - 5000.0 ms  
**Note**: Part of the M.FX (Monitoring Effects) controls

#### `/InputGroup[1-16]MonDelayLink`
**Type**: Toggle Button  
**Description**: Links delay time with other inputs (global setting)  
**Data Type**: Integer (0 = unlinked, 1 = linked)  
**Note**: This is a global setting that affects all Input Groups

#### `/InputGroup[1-16]MonReverbSend`
**Type**: Slider  
**Description**: Controls the main reverb send level for the specified Input Group's monitoring  
**Data Type**: Float  
**Range**: Audio level (0.0 - 1.0)  
**Note**: Part of the M.FX (Monitoring Effects) controls

#### `/InputGroup[1-16]Mute`
**Type**: Toggle Button  
**Description**: Mutes/unmutes the specified Input Group  
**Data Type**: Integer (0 = unmuted, 1 = muted)  
**Note**: Stateful control - maintains mute state

#### `/InputGroup[1-16]Solo`
**Type**: Toggle Button  
**Description**: Solos/unsolos the specified Input Group  
**Data Type**: Integer (0 = not soloed, 1 = soloed)  
**Note**: Stateful control - when any Input Group is soloed, only soloed groups are heard

#### `/InputGroup[1-16]InputReverbSend`
**Type**: Slider  
**Description**: Controls the input reverb send level (applied before sending to peers)  
**Data Type**: Float  
**Range**: Audio level (0.0 - 1.0)  
**Note**: Part of the Input FX controls

#### `/InputGroup[1-16]PolarityInvert`
**Type**: Toggle Button  
**Description**: Inverts the polarity/phase of the specified Input Group  
**Data Type**: Integer (0 = normal, 1 = inverted)  
**Note**: Part of the Input FX controls

### Input Group Compressor Controls

#### `/InputGroup[1-16]CompressorEnable`
**Type**: Toggle Button  
**Description**: Enables/disables the compressor effect for the specified Input Group  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Note**: Part of the Input FX controls

#### `/InputGroup[1-16]CompressorThreshold`
**Type**: Slider  
**Description**: Sets the compressor threshold in dB  
**Data Type**: Float  
**Range**: -60.0 to 0.0 dB  

#### `/InputGroup[1-16]CompressorRatio`
**Type**: Slider  
**Description**: Sets the compressor ratio  
**Data Type**: Float  
**Range**: 1.0 to 20.0 (e.g., 4.0 means 4:1 ratio)  

#### `/InputGroup[1-16]CompressorAttack`
**Type**: Slider  
**Description**: Sets the compressor attack time in milliseconds  
**Data Type**: Float  
**Range**: 1.0 to 1000.0 ms  

#### `/InputGroup[1-16]CompressorRelease`
**Type**: Slider  
**Description**: Sets the compressor release time in milliseconds  
**Data Type**: Float  
**Range**: 1.0 to 1000.0 ms  

#### `/InputGroup[1-16]CompressorMakeupGain`
**Type**: Slider  
**Description**: Sets the compressor makeup gain in dB  
**Data Type**: Float  
**Range**: 0.0 to 60.0 dB  

#### `/InputGroup[1-16]CompressorAuto`
**Type**: Toggle Button  
**Description**: Enables/disables automatic makeup gain calculation  
**Data Type**: Integer (0 = manual, 1 = automatic)  

### Input Group Expander (Noise Gate) Controls

#### `/InputGroup[1-16]ExpanderEnable`
**Type**: Toggle Button  
**Description**: Enables/disables the noise gate effect for the specified Input Group  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Note**: Part of the Input FX controls

#### `/InputGroup[1-16]ExpanderNoiseFloor`
**Type**: Slider  
**Description**: Sets the noise gate threshold (noise floor) in dB  
**Data Type**: Float  
**Range**: -96.0 to 0.0 dB  

#### `/InputGroup[1-16]ExpanderRatio`
**Type**: Slider  
**Description**: Sets the expander ratio  
**Data Type**: Float  
**Range**: 1.0 to 20.0  

#### `/InputGroup[1-16]ExpanderAttack`
**Type**: Slider  
**Description**: Sets the expander attack time in milliseconds  
**Data Type**: Float  
**Range**: 1.0 to 1000.0 ms  

#### `/InputGroup[1-16]ExpanderRelease`
**Type**: Slider  
**Description**: Sets the expander release time in milliseconds  
**Data Type**: Float  
**Range**: 1.0 to 1000.0 ms  

### Input Group Parametric EQ Controls

#### `/InputGroup[1-16]EqEnable`
**Type**: Toggle Button  
**Description**: Enables/disables the parametric EQ effect for the specified Input Group  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Note**: Part of the Input FX controls

#### `/InputGroup[1-16]EqLowShelfFreq`
**Type**: Slider  
**Description**: Sets the low shelf filter frequency in Hz  
**Data Type**: Float  
**Range**: 20.0 to 2000.0 Hz  
**Default**: 60.0 Hz

#### `/InputGroup[1-16]EqLowShelfGain`
**Type**: Slider  
**Description**: Sets the low shelf filter gain in dB  
**Data Type**: Float  
**Range**: -24.0 to 24.0 dB  

#### `/InputGroup[1-16]EqPara1Freq`
**Type**: Slider  
**Description**: Sets the parametric band 1 center frequency in Hz  
**Data Type**: Float  
**Range**: 20.0 to 20000.0 Hz  
**Default**: 90.0 Hz

#### `/InputGroup[1-16]EqPara1Gain`
**Type**: Slider  
**Description**: Sets the parametric band 1 gain in dB  
**Data Type**: Float  
**Range**: -24.0 to 24.0 dB  

#### `/InputGroup[1-16]EqPara1Q`
**Type**: Slider  
**Description**: Sets the parametric band 1 Q (bandwidth)  
**Data Type**: Float  
**Range**: 0.1 to 10.0  
**Default**: 1.5

#### `/InputGroup[1-16]EqHighShelfFreq`
**Type**: Slider  
**Description**: Sets the high shelf filter frequency in Hz  
**Data Type**: Float  
**Range**: 500.0 to 16000.0 Hz  
**Default**: 10000.0 Hz

#### `/InputGroup[1-16]EqHighShelfGain`
**Type**: Slider  
**Description**: Sets the high shelf filter gain in dB  
**Data Type**: Float  
**Range**: -24.0 to 24.0 dB  

#### `/InputGroup[1-16]EqPara2Freq`
**Type**: Slider  
**Description**: Sets the parametric band 2 center frequency in Hz  
**Data Type**: Float  
**Range**: 20.0 to 20000.0 Hz  
**Default**: 360.0 Hz

#### `/InputGroup[1-16]EqPara2Gain`
**Type**: Slider  
**Description**: Sets the parametric band 2 gain in dB  
**Data Type**: Float  
**Range**: -24.0 to 24.0 dB  

#### `/InputGroup[1-16]EqPara2Q`
**Type**: Slider  
**Description**: Sets the parametric band 2 Q (bandwidth)  
**Data Type**: Float  
**Range**: 0.1 to 10.0  
**Default**: 4.0

### File Playback Controls

#### `/FilePlaybackPreLevel`
**Type**: Slider  
**Description**: Controls the pre-fader level (gain) for File Playback  
**Data Type**: Float  
**Range**: Audio level (0.0 - 2.0, where 1.0 is unity gain)

## Peer Controls

SonoBus supports OSC control for up to 16 remote peers (connected users). Each peer can be controlled independently using numbered OSC addresses. Peer controls dynamically map to connected peers and will only respond when the specified peer index exists.

**Important Notes:**
- Peers are numbered 1-16 in OSC addresses (e.g., `/Peer1Mute`, `/Peer2Solo`)
- Only peers that are currently connected will respond to OSC messages
- FX controls (Compressor, Expander/Noise Gate, EQ, etc.) operate on the first/main channel group (channel group 0) of each peer
- Peer numbering corresponds to the order peers connect to the session

### Peer Mute and Solo Controls

#### `/Peer[1-16]Mute`
**Type**: Toggle Button  
**Description**: Mutes/unmutes receiving audio from the specified peer  
**Data Type**: Float (0.0 = unmuted/receiving, 1.0 = muted/not receiving)  
**Examples**:
- `/Peer1Mute` - Mutes/unmutes Peer 1
- `/Peer2Mute` - Mutes/unmutes Peer 2
- `/Peer16Mute` - Mutes/unmutes Peer 16

**Note**: Muting a peer prevents their audio from being heard locally. This is equivalent to clicking the MUTE button in the peer's UI panel.

#### `/Peer[1-16]Solo`
**Type**: Toggle Button  
**Description**: Solos/unsolos the specified peer  
**Data Type**: Float (0.0 = not soloed, 1.0 = soloed)  
**Examples**:
- `/Peer1Solo` - Solos/unsolos Peer 1
- `/Peer2Solo` - Solos/unsolos Peer 2

**Note**: When any peer is soloed, only soloed peers are heard. This is equivalent to clicking the SOLO button in the peer's UI panel.

#### `/Peer[1-16]BufferMin`
**Type**: Push Button  
**Description**: Resets the jitter buffer to the minimum for the specified peer  
**Data Type**: Integer (1 to trigger)  
**Examples**:
- `/Peer1BufferMin` - Resets jitter buffer for Peer 1
- `/Peer2BufferMin` - Resets jitter buffer for Peer 2

**Note**: This is a momentary push button with no state. Send value 1 to trigger the action.

#### `/Peer[1-16]ResetDrop`
**Type**: Push Button  
**Description**: Resets packet drop statistics for the specified peer  
**Data Type**: Integer (1 to trigger)  
**Examples**:
- `/Peer1ResetDrop` - Resets drop stats for Peer 1
- `/Peer2ResetDrop` - Resets drop stats for Peer 2

**Note**: This is a momentary push button with no state. Send value 1 to trigger the action. Clears the dropped packet counter for the peer.

#### `/Peer[1-16]Level`
**Type**: Slider  
**Description**: Controls the level/gain for the specified peer  
**Data Type**: Float  
**Range**: 0.0 - 2.0 (0.0 = silence, 1.0 = unity gain, 2.0 = +6dB)  
**Examples**:
- `/Peer1Level` - Controls level for Peer 1
- `/Peer2Level` - Controls level for Peer 2

**Note**: Adjusts the output level/gain for the peer's audio. This is equivalent to moving the level slider in the peer's UI panel.

#### `/Peer[1-16]Pan`
**Type**: Slider  
**Description**: Controls the stereo pan position for the specified peer (channel group 0, channel 0)  
**Data Type**: Float  
**Range**: -1.0 to 1.0 (-1.0 = full left, 0.0 = center, 1.0 = full right)  
**Examples**:
- `/Peer1Pan` - Controls pan for Peer 1
- `/Peer2Pan` - Controls pan for Peer 2

**Note**: Adjusts the stereo pan position for the peer's audio. This is equivalent to moving the pan slider in the peer's UI panel.

#### `/Peer[1-16]RemotePeerUserName`
**Type**: Read-Only Text Field  
**Description**: Provides the username of the connected peer  
**Data Type**: String  
**Direction**: Send only (SonoBus → OSC controller)  
**Examples**:
- `/Peer1RemotePeerUserName` - Username of Peer 1
- `/Peer2RemotePeerUserName` - Username of Peer 2

**Note**: This is a read-only control. SonoBus automatically sends the username when a peer joins the session and clears it to an empty string when the peer leaves. OSC messages sent to this address are ignored.

**Note**: This control adjusts the receive level/gain for audio from the peer. Values above 1.0 provide gain boost.

### Peer Input Effects (FX) Controls

All FX controls operate on the first/main channel group (channel group 0) of each peer. These controls affect the audio received from the peer before it is mixed with other audio.

#### `/Peer[1-16]InputReverbSend`
**Type**: Slider  
**Description**: Controls the input reverb send level for the specified peer  
**Data Type**: Float  
**Range**: 0.0 - 1.0 (0.0 = no reverb, 1.0 = maximum reverb send)  
**Examples**:
- `/Peer1InputReverbSend` - Controls input reverb send for Peer 1
- `/Peer2InputReverbSend` - Controls input reverb send for Peer 2

**Note**: This sends the peer's audio to the input reverb effect before mixing.

#### `/Peer[1-16]PolarityInvert`
**Type**: Toggle Button  
**Description**: Inverts the polarity/phase of audio received from the specified peer  
**Data Type**: Integer (0 = normal polarity, 1 = inverted polarity)  
**Examples**:
- `/Peer1PolarityInvert` - Inverts polarity for Peer 1
- `/Peer2PolarityInvert` - Inverts polarity for Peer 2

**Note**: Polarity inversion can be useful for correcting phase issues between microphones.

### Peer Compressor Controls

#### `/Peer[1-16]CompressorEnable`
**Type**: Toggle Button  
**Description**: Enables/disables the compressor effect for the specified peer  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Examples**:
- `/Peer1CompressorEnable` - Enables/disables compressor for Peer 1
- `/Peer2CompressorEnable` - Enables/disables compressor for Peer 2

#### `/Peer[1-16]CompressorThreshold`
**Type**: Slider  
**Description**: Sets the compressor threshold in dB for the specified peer  
**Data Type**: Float  
**Range**: -60.0 to 0.0 dB  
**Note**: Signals above this level will be compressed

#### `/Peer[1-16]CompressorRatio`
**Type**: Slider  
**Description**: Sets the compressor ratio for the specified peer  
**Data Type**: Float  
**Range**: 1.0 to 20.0 (e.g., 4.0 means 4:1 ratio)  
**Note**: Higher ratios result in more aggressive compression

#### `/Peer[1-16]CompressorAttack`
**Type**: Slider  
**Description**: Sets the compressor attack time in milliseconds for the specified peer  
**Data Type**: Float  
**Range**: 1.0 to 1000.0 ms  
**Note**: How quickly the compressor responds to signals exceeding the threshold

#### `/Peer[1-16]CompressorRelease`
**Type**: Slider  
**Description**: Sets the compressor release time in milliseconds for the specified peer  
**Data Type**: Float  
**Range**: 1.0 to 1000.0 ms  
**Note**: How quickly the compressor recovers after the signal drops below threshold

#### `/Peer[1-16]CompressorMakeupGain`
**Type**: Slider  
**Description**: Sets the compressor makeup gain in dB for the specified peer  
**Data Type**: Float  
**Range**: 0.0 to 60.0 dB  
**Note**: Gain applied after compression to compensate for level reduction

#### `/Peer[1-16]CompressorAuto`
**Type**: Toggle Button  
**Description**: Enables/disables automatic makeup gain calculation for the specified peer  
**Data Type**: Integer (0 = manual makeup gain, 1 = automatic)  
**Note**: When enabled, makeup gain is automatically calculated based on threshold and ratio

### Peer Expander (Noise Gate) Controls

The expander acts as a noise gate, reducing the level of signals below the threshold.

#### `/Peer[1-16]ExpanderEnable`
**Type**: Toggle Button  
**Description**: Enables/disables the noise gate effect for the specified peer  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Examples**:
- `/Peer1ExpanderEnable` - Enables/disables noise gate for Peer 1
- `/Peer2ExpanderEnable` - Enables/disables noise gate for Peer 2

#### `/Peer[1-16]ExpanderNoiseFloor`
**Type**: Slider  
**Description**: Sets the noise gate threshold (noise floor) in dB for the specified peer  
**Data Type**: Float  
**Range**: -96.0 to 0.0 dB  
**Note**: Signals below this level will be attenuated

#### `/Peer[1-16]ExpanderRatio`
**Type**: Slider  
**Description**: Sets the expander ratio for the specified peer  
**Data Type**: Float  
**Range**: 1.0 to 20.0  
**Note**: Higher ratios result in more aggressive gating

#### `/Peer[1-16]ExpanderAttack`
**Type**: Slider  
**Description**: Sets the expander attack time in milliseconds for the specified peer  
**Data Type**: Float  
**Range**: 1.0 to 1000.0 ms  
**Note**: How quickly the gate opens when signal exceeds threshold

#### `/Peer[1-16]ExpanderRelease`
**Type**: Slider  
**Description**: Sets the expander release time in milliseconds for the specified peer  
**Data Type**: Float  
**Range**: 1.0 to 1000.0 ms  
**Note**: How quickly the gate closes when signal drops below threshold

### Peer Parametric EQ Controls

The parametric EQ provides four bands of equalization: low shelf, two parametric bands, and high shelf.

#### `/Peer[1-16]EqEnable`
**Type**: Toggle Button  
**Description**: Enables/disables the parametric EQ effect for the specified peer  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Examples**:
- `/Peer1EqEnable` - Enables/disables EQ for Peer 1
- `/Peer2EqEnable` - Enables/disables EQ for Peer 2

#### `/Peer[1-16]EqLowShelfFreq`
**Type**: Slider  
**Description**: Sets the low shelf filter frequency in Hz for the specified peer  
**Data Type**: Float  
**Range**: 20.0 to 2000.0 Hz  
**Default**: 60.0 Hz  
**Note**: Frequency at which the low shelf filter takes effect

#### `/Peer[1-16]EqLowShelfGain`
**Type**: Slider  
**Description**: Sets the low shelf filter gain in dB for the specified peer  
**Data Type**: Float  
**Range**: -24.0 to 24.0 dB  
**Note**: Boost or cut applied to frequencies below the low shelf frequency

#### `/Peer[1-16]EqPara1Freq`
**Type**: Slider  
**Description**: Sets the parametric band 1 center frequency in Hz for the specified peer  
**Data Type**: Float  
**Range**: 20.0 to 20000.0 Hz  
**Default**: 90.0 Hz  
**Note**: Center frequency of the first parametric band

#### `/Peer[1-16]EqPara1Gain`
**Type**: Slider  
**Description**: Sets the parametric band 1 gain in dB for the specified peer  
**Data Type**: Float  
**Range**: -24.0 to 24.0 dB  
**Note**: Boost or cut applied at the parametric band 1 frequency

#### `/Peer[1-16]EqPara1Q`
**Type**: Slider  
**Description**: Sets the parametric band 1 Q (bandwidth) for the specified peer  
**Data Type**: Float  
**Range**: 0.1 to 10.0  
**Default**: 1.5  
**Note**: Higher Q values result in narrower bandwidth (more selective filtering)

#### `/Peer[1-16]EqHighShelfFreq`
**Type**: Slider  
**Description**: Sets the high shelf filter frequency in Hz for the specified peer  
**Data Type**: Float  
**Range**: 500.0 to 16000.0 Hz  
**Default**: 10000.0 Hz  
**Note**: Frequency at which the high shelf filter takes effect

#### `/Peer[1-16]EqHighShelfGain`
**Type**: Slider  
**Description**: Sets the high shelf filter gain in dB for the specified peer  
**Data Type**: Float  
**Range**: -24.0 to 24.0 dB  
**Note**: Boost or cut applied to frequencies above the high shelf frequency

#### `/Peer[1-16]EqPara2Freq`
**Type**: Slider  
**Description**: Sets the parametric band 2 center frequency in Hz for the specified peer  
**Data Type**: Float  
**Range**: 20.0 to 20000.0 Hz  
**Default**: 360.0 Hz  
**Note**: Center frequency of the second parametric band

#### `/Peer[1-16]EqPara2Gain`
**Type**: Slider  
**Description**: Sets the parametric band 2 gain in dB for the specified peer  
**Data Type**: Float  
**Range**: -24.0 to 24.0 dB  
**Note**: Boost or cut applied at the parametric band 2 frequency

#### `/Peer[1-16]EqPara2Q`
**Type**: Slider  
**Description**: Sets the parametric band 2 Q (bandwidth) for the specified peer  
**Data Type**: Float  
**Range**: 0.1 to 10.0  
**Default**: 4.0  
**Note**: Higher Q values result in narrower bandwidth (more selective filtering)

## Options Tab Controls

### Audio Options

#### `/OptionsDynamicResamplingButton`
**Type**: Toggle Button  
**Description**: Enables/disables drift correction (dynamic resampling)  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Parameter**: `paramDynamicResampling`  
**Note**: Not recommended for most users

#### `/OptionsInputLimiterButton`
**Type**: Toggle Button  
**Description**: Enables/disables the input FX limiter  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Action**: Applies limiter to all input groups

#### `/OptionsDefaultLevelSlider`
**Type**: Slider  
**Description**: Sets the default level for new peer connections  
**Data Type**: Float  
**Range**: Audio level in decibels  
**Parameter**: `paramDefaultPeerLevel`

#### `/OptionsMaxRecvPaddingSlider`
**Type**: Slider  
**Description**: Sets the maximum receive padding in milliseconds  
**Data Type**: Float  
**Range**: 0.0 - 500.0 ms  
**Parameter**: `paramMaxRecvPaddingMs`

### Connection Options

#### `/OptionsAutoReconnectButton`
**Type**: Toggle Button  
**Description**: Enables/disables automatic reconnection to the last group  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Parameter**: `paramAutoReconnectLast`

### Default Format Options

#### `/OptionsAutosizeDefaultChoice`
**Type**: Choice Button  
**Description**: Sets the default auto network buffer setting for new connections  
**Data Type**: Integer (option ID)  
**Parameter**: `paramDefaultAutoNetbuf`  
**Options**:
- 0: Off
- 1: Auto
- 2: Auto Increased

#### `/OptionsFormatChoiceDefaultChoice`
**Type**: Choice Button  
**Description**: Sets the default audio format/quality for new connections  
**Data Type**: Integer (option ID)  
**Parameter**: `paramDefaultSendQual`  
**Options**: Various codec quality settings (PCM, Opus at different bitrates)

#### `/BufferTimeSlider`
**Type**: Slider  
**Description**: Sets the default jitter buffer size in milliseconds  
**Data Type**: Float  
**Range**: Buffer time in milliseconds  
**Parameter**: `paramDefaultNetbufMs`

#### `/OptionsAutoDropThreshSlider`
**Type**: Slider  
**Description**: Controls auto-jitter buffer adjustment sensitivity based on dropout rate  
**Data Type**: Float  
**Range**: 1.0 - 20.0 seconds  
**Note**: Threshold for automatic buffer size increases when dropouts occur

### UI Options

#### `/OptionsDisableShortcutButton`
**Type**: Toggle Button  
**Description**: Enables/disables keyboard shortcuts  
**Data Type**: Integer (0 = shortcuts enabled, 1 = shortcuts disabled)  
**Action**: Toggles keyboard shortcut functionality

#### `/OptionsSliderSnapToMouseButton`
**Type**: Toggle Button  
**Description**: Controls whether sliders snap to clicked position  
**Data Type**: Integer (0 = no snap, 1 = snap to mouse)  
**Action**: Changes slider behavior for all sliders

#### `/OptionsChangeAllFormatButton`
**Type**: Toggle Button  
**Description**: When enabled, changing default codec also changes existing peer connections  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Action**: Controls codec change behavior

### System Options

#### `/OptionsUseSpecificUdpPortButton`
**Type**: Toggle Button  
**Description**: Enables/disables use of a specific UDP port  
**Data Type**: Integer (0 = system-chosen port, 1 = use specific port)  
**Action**: Toggles specific UDP port usage

#### `/OptionsUdpPortEditor`
**Type**: Text Input  
**Description**: Sets the specific UDP port to use (when enabled)  
**Data Type**: Integer (port number)  
**Range**: 1-65535  
**Note**: Only active when UseSpecificUdpPort is enabled

#### `/OptionsOverrideSamplerateButton`
**Type**: Toggle Button  
**Description**: Enables/disables sample rate override  
**Data Type**: Integer (0 = use device rate, 1 = override)  
**Action**: Toggles sample rate override (standalone app only)

#### `/OptionsShouldCheckForUpdateButton`
**Type**: Toggle Button  
**Description**: Enables/disables automatic update checking  
**Data Type**: Integer (0 = no auto-check, 1 = auto-check)  
**Action**: Controls automatic update check behavior (standalone app only)

### OSC Configuration

#### `/OSCTargetIPAddress`
**Type**: Text Input  
**Description**: Sets the IP address for outbound OSC messages  
**Data Type**: String (IP address)  
**Default**: "127.0.0.1"

#### `/OSCTargetPort`
**Type**: Text Input  
**Description**: Sets the port for outbound OSC messages  
**Data Type**: Integer (port number)  
**Range**: 1-65535  
**Default**: 6001

#### `/OSCReceivePort`
**Type**: Text Input  
**Description**: Sets the port for receiving OSC messages  
**Data Type**: Integer (port number)  
**Range**: 1-65535  
**Default**: 6000

## Recording Tab Controls

### Recording Options

#### `/OptionsRecStealth`
**Type**: Toggle Button  
**Description**: Enables/disables stealth recording (no visual indication to others)  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Action**: Sets stealth recording mode

#### `/OptionsMetRecordedButton`
**Type**: Toggle Button  
**Description**: Enables/disables recording the metronome to file  
**Data Type**: Integer (0 = not recorded, 1 = recorded)  
**Parameter**: `paramMetIsRecorded`

#### `/OptionsRecSelfPostFxButton`
**Type**: Toggle Button  
**Description**: Records your audio after FX processing (post-FX)  
**Data Type**: Integer (0 = pre-FX, 1 = post-FX)  
**Action**: Controls recording point for self audio

#### `/OptionsRecSelfSilenceMutedButton`
**Type**: Toggle Button  
**Description**: Silences self recording when muted  
**Data Type**: Integer (0 = record when muted, 1 = silence when muted)  
**Action**: Controls self recording behavior when muted

#### `/OptionsRecMixButton`
**Type**: Toggle Button  
**Description**: Enables/disables recording the output mix  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Action**: Includes/excludes output mix in recording

#### `/OptionsRecSelfButton`
**Type**: Toggle Button  
**Description**: Enables/disables recording your own audio (dry)  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Action**: Includes/excludes your audio in recording

#### `/OptionsRecOthersButton`
**Type**: Toggle Button  
**Description**: Enables/disables recording individual user tracks  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Action**: Includes/excludes individual user tracks in recording

#### `/OptionsRecMixMinusButton`
**Type**: Toggle Button  
**Description**: Enables/disables recording the full mix without yourself  
**Data Type**: Integer (0 = disabled, 1 = enabled)  
**Action**: Includes/excludes the output mix minus your own audio in recording

**Note**: At least one recording option must be enabled. If all are disabled, RecMix is automatically enabled.

## Example OSC Messages

### Using Pure Data or Max/MSP

```
# Mute your audio
/MainMuteButton 1

# Unmute your audio
/MainMuteButton 0

# Set output gain to specific level
/OutGainSlider -6.0

# Enable metronome
/MetEnableButton 1

# Set metronome tempo to 120 BPM
/MetTempoSlider 120.0

# Start recording
/RecordingButton 1

# Push to talk (press)
/MainPushToTalkButton 1

# Push to talk (release)
/MainPushToTalkButton 0

# Reset jitter buffers
/BufferMinButton 1

# Enable stealth recording
/OptionsRecStealth 1

# Set default format to specific codec
/OptionsFormatChoiceDefaultChoice 4

# Mute Peer 1
/Peer1Mute 1.0

# Unmute Peer 1
/Peer1Mute 0.0

# Solo Peer 2
/Peer2Solo 1.0

# Enable compressor for Peer 1
/Peer1CompressorEnable 1

# Set compressor threshold for Peer 1
/Peer1CompressorThreshold -20.0

# Set compressor ratio for Peer 1
/Peer1CompressorRatio 4.0

# Enable noise gate for Peer 2
/Peer2ExpanderEnable 1

# Set noise gate threshold for Peer 2
/Peer2ExpanderNoiseFloor -40.0

# Enable EQ for Peer 1
/Peer1EqEnable 1

# Boost low frequencies for Peer 1
/Peer1EqLowShelfGain 3.0

# Set input reverb send for Peer 3
/Peer3InputReverbSend 0.5

# Invert polarity for Peer 1
/Peer1PolarityInvert 1
```

### Using Python with python-osc

```python
from pythonosc import udp_client

# Create OSC client
client = udp_client.SimpleUDPClient("127.0.0.1", 9001)

# Mute sending
client.send_message("/MainMuteButton", 1)

# Set output level
client.send_message("/OutGainSlider", -3.0)

# Enable metronome
client.send_message("/MetEnableButton", 1)

# Set tempo
client.send_message("/MetTempoSlider", 120.0)

# Start recording
client.send_message("/RecordingButton", 1)

# Mute Peer 1
client.send_message("/Peer1Mute", 1.0)

# Solo Peer 2
client.send_message("/Peer2Solo", 1.0)

# Enable compressor for Peer 1
client.send_message("/Peer1CompressorEnable", 1)
client.send_message("/Peer1CompressorThreshold", -20.0)
client.send_message("/Peer1CompressorRatio", 4.0)

# Enable noise gate for Peer 2
client.send_message("/Peer2ExpanderEnable", 1)
client.send_message("/Peer2ExpanderNoiseFloor", -40.0)

# Apply EQ to Peer 1
client.send_message("/Peer1EqEnable", 1)
client.send_message("/Peer1EqLowShelfGain", 3.0)
client.send_message("/Peer1EqHighShelfGain", -2.0)
```

### Using TouchOSC

Create controls with the following OSC addresses:
- Toggle button with address `/MainMuteButton` sending 0/1
- Fader with address `/OutGainSlider` sending float values
- Button with address `/RecordingButton` sending 0/1
- XY Pad with `/MetTempoSlider` on one axis
- Toggle buttons for `/Peer1Mute`, `/Peer2Mute`, etc. sending 0.0/1.0
- Toggle buttons for `/Peer1Solo`, `/Peer2Solo`, etc. sending 0.0/1.0
- Faders for peer FX controls like `/Peer1CompressorThreshold`, `/Peer1EqLowShelfGain`, etc.

## Receiving OSC Messages from SonoBus

When controls change in SonoBus (either by UI interaction or incoming OSC), the application sends OSC messages with the new values to the configured target IP and port. This allows external controllers to stay synchronized with the application state.

Example listening in Python:
```python
from pythonosc import dispatcher
from pythonosc import osc_server

def handle_mute(unused_addr, value):
    print(f"Mute state changed to: {value}")

def handle_slider(unused_addr, value):
    print(f"Slider value changed to: {value}")

# Create dispatcher
disp = dispatcher.Dispatcher()
disp.map("/MainMuteButton", handle_mute)
disp.map("/OutGainSlider", handle_slider)

# Create server
server = osc_server.ThreadingOSCUDPServer(("127.0.0.1", 9000), disp)
print("Listening for OSC messages...")
server.serve_forever()
```

## Notes

- All OSC communication is done over UDP
- Messages are sent only when OSC is enabled in the Options tab
- Float values are sent as OSC float32
- Integer values are sent as OSC int32
- Changes to parameters via OSC will trigger the same behavior as UI interaction
- Some controls may have range limits enforced by the application

## Troubleshooting

1. **Not receiving messages**: Check that the receive port matches your sender's target port
2. **Not sending messages**: Verify OSC is enabled and target IP/port are correct
3. **Values not applying**: Ensure data types match (float vs int) and values are in valid range
4. **Network issues**: If using remote control, ensure firewall allows UDP traffic on OSC ports

## Related Resources

- [OSC Specification](http://opensoundcontrol.org/spec-1_0)
- [TouchOSC](https://hexler.net/products/touchosc)
- [python-osc](https://github.com/attwad/python-osc)

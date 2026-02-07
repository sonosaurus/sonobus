#pragma once

#include "JuceHeader.h"
#include <functional>
#include <map>

// Global constant for OSC message scaling factor
// Used to scale outbound float values for TouchOSC compatibility
constexpr float OSC_SCALE_FACTOR = 0.5f;

// Inverse scale factor for incoming OSC messages
// Used to compensate for the outbound scaling on specific controls
constexpr float OSC_INVERSE_SCALE_FACTOR = 2.0f;

class OSCManager : private juce::OSCReceiver,
                   private juce::OSCReceiver::ListenerWithOSCAddress<juce::OSCReceiver::MessageLoopCallback>
{
public:
    OSCManager();
    ~OSCManager();

    bool initializeReceiver(int port);
    bool initializeSender(const juce::String& targetIP, int port);
    void disconnectReceiver();
    void disconnectSender();
    void sendMessage(const juce::String& address, const juce::var& value);
    
    // Generic control registration
    using ControlCallback = std::function<void(const juce::OSCMessage&)>;
    void registerControl(const juce::String& address, ControlCallback callback);
    void unregisterControl(const juce::String& address);

private:
    void oscMessageReceived(const juce::OSCMessage& message) override;

    // Helper: addresses that use inverted 0/1 semantics on the wire
    static inline bool isInvertedOSCAddress(const juce::String& address)
    {
        return address == "/MainMuteButton" || address == "/BufferMinButton";
    }

    juce::OSCSender sender;
    bool senderConnected = false;  // Track sender connection state
    
    // Control registry for dynamic OSC message handling
    std::map<juce::String, ControlCallback> controlRegistry;
};

#include "OSCManager.h"

OSCManager::OSCManager()
{
    // OSCManager object created, but not initialized until OSC is enabled
}

OSCManager::~OSCManager()
{
    sender.disconnect();
    disconnect();
}

// Initialize Receiver
bool OSCManager::initializeReceiver(int port)
{
    if (!connect(port))
    {
        juce::Logger::writeToLog("Receiver failed to connect on port " + juce::String(port));
        return false;
    }

    // Legacy listener - keeping for backward compatibility
    addListener(this, "/volumeControl");
    juce::Logger::writeToLog("Receiver connected on port " + juce::String(port));
    return true;
}

// Initialize Sender
bool OSCManager::initializeSender(const juce::String& targetIP, int port)
{
    senderConnected = sender.connect(targetIP, port);
    if (!senderConnected)
    {
        juce::Logger::writeToLog("Sender failed to connect to " + targetIP + ":" + juce::String(port));
    }
    return senderConnected;
}

// Disconnect Receiver
void OSCManager::disconnectReceiver()
{
    disconnect();
    juce::Logger::writeToLog("OSC Receiver disconnected.");
}

// Disconnect Sender
void OSCManager::disconnectSender()
{
    sender.disconnect();
    senderConnected = false;
    juce::Logger::writeToLog("OSC Sender disconnected.");
}

// Send Message
void OSCManager::sendMessage(const juce::String& address, const juce::var& value)
{
    if (senderConnected)
    {
        if (value.isDouble())
        {
            // Apply OSC_SCALE_FACTOR to float/double values
            sender.send(address, static_cast<float>(value) * OSC_SCALE_FACTOR);
        }
        else if (value.isInt())
        {
            sender.send(address, static_cast<int>(value));
        }
        else if (value.isString())
        {
            sender.send(address, value.toString());
        }
        else
        {
            juce::Logger::writeToLog("Unsupported var type in sendMessage for: " + address);
        }
    }
    else
    {
        juce::Logger::writeToLog("OSC Sender is not connected! Cannot send message to: " + address);
    }
}

// Register Control
void OSCManager::registerControl(const juce::String& address, ControlCallback callback)
{
    controlRegistry[address] = callback;
    // Also register as a listener for this OSC address pattern
    addListener(this, address);
    // juce::Logger::writeToLog("Registered control for OSC address: " + address);
}

// Unregister Control
void OSCManager::unregisterControl(const juce::String& address)
{
    auto it = controlRegistry.find(address);
    if (it != controlRegistry.end())
    {
        controlRegistry.erase(it);
        // Note: JUCE doesn't provide a way to remove a specific listener,
        // but the callback will no longer be called since it's removed from the registry
        // juce::Logger::writeToLog("Unregistered control for OSC address: " + address);
    }
}

// Handle Received Messages
void OSCManager::oscMessageReceived(const juce::OSCMessage& message)
{
    juce::String address = message.getAddressPattern().toString();
    // juce::Logger::writeToLog("Incoming OSC message: " + address);
    
    // Check if a callback is registered for this address
    auto it = controlRegistry.find(address);
    if (it != controlRegistry.end())
    {
        // Call the registered callback
        it->second(message);
    }
    else
    {
        juce::Logger::writeToLog("No handler registered for OSC address: " + address);
    }
}

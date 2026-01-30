// ... (file header unchanged)

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

// Helper: whether this address needs inverting of 0/1 values
static bool addressNeedsInvert(const juce::String& address)
{
    return address == "/MainMuteButton" || address == "/MainRecvMuteButton";
}

// Send Message
void OSCManager::sendMessage(const juce::String& address, const juce::var& value)
{
    if (!senderConnected)
    {
        juce::Logger::writeToLog("OSC Sender is not connected! Cannot send message to: " + address);
        return;
    }

    // If this address requires inverted send semantics, flip 0 <-> 1 for numeric values.
    if (addressNeedsInvert(address))
    {
        // integer case
        if (value.isInt())
        {
            int v = static_cast<int>(value);
            int inv = (v == 0) ? 1 : 0;
            sender.send(address, inv);
            return;
        }
        // double/float case (some callers send 0.0/1.0 floats)
        else if (value.isDouble())
        {
            double dv = static_cast<double>(value);
            double invd = (std::abs(dv) < 0.5) ? 1.0 : 0.0; // treat near-zero as 0, otherwise 1
            // Apply existing OSC scale factor for floats
            sender.send(address, static_cast<float>(invd * OSC_SCALE_FACTOR));
            return;
        }
        // string or other types: fall through to normal handling (no invert)
    }

    // Default handling for other addresses or non-invert cases
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
        // If this address uses inverted semantics, translate the incoming 0/1 back to the app's original values:
        // incoming 0 -> treated as 1 internally, incoming 1 -> treated as 0 internally.
        if (addressNeedsInvert(address) && message.size() > 0)
        {
            // Handle integer argument
            if (message[0].isInt32())
            {
                int incoming = message[0].getInt32();
                int translated = (incoming == 0) ? 1 : 0;
                juce::OSCMessage translatedMsg(address, translated);
                it->second(translatedMsg);
                return;
            }
            // Handle float argument (some controllers send 0.0/1.0)
            else if (message[0].isFloat32())
            {
                float incoming = message[0].getFloat32();
                // treat <0.5 as 0, >=0.5 as 1
                int bin = (std::abs(incoming) < 0.5f) ? 0 : 1;
                int translated = (bin == 0) ? 1 : 0;
                juce::OSCMessage translatedMsg(address, static_cast<float>(translated));
                it->second(translatedMsg);
                return;
            }
            else
            {
                // Unsupported type for inversion - fall through to deliver original message
                juce::Logger::writeToLog("OSCManager: received non-numeric argument for inverted address " + address);
            }
        }

        // Default: pass the original message through
        it->second(message);
    }
    else
    {
        // no registered control: optional logging or ignore
        //juce::Logger::writeToLog("Received OSC for unregistered address: " + address);
    }
}

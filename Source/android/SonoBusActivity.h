
#pragma once

#include "JuceHeader.h"

#if JUCE_ANDROID


#include "AppConfig.h"

#include "juce_core/native/juce_BasicNativeHeaders.h"

#include "juce_core/juce_core.h"
#include "juce_core/native/juce_android_JNIHelpers.h"

class SonoBusActivity
{
public:
    SonoBusActivity(jobject javaObject)
    {
        auto* env = getEnv();
        
        javaCounterpartInstance = env->NewWeakGlobalRef(javaObject);
        env->SetLongField (javaObject, SonoBusActivityJavaClass.cppCounterpartInstance,
                           reinterpret_cast<jlong> (this));
        
        // initialise the JUCE message manager!
        MessageManager::getInstance();
        
        DBG("SonoBusActivity C++ constructed");
    }

    ~SonoBusActivity()
    {
        auto* env = getEnv();
        
        {
            LocalRef<jobject> javaThis (env->NewLocalRef (javaCounterpartInstance));
            
            if (javaThis != nullptr)
                env->SetLongField (javaThis.get(), SonoBusActivityJavaClass.cppCounterpartInstance, 0);
        }
        
        env->DeleteWeakGlobalRef(javaCounterpartInstance);
    }
    
    static jobject gAssetManager;
    static String gPendingImportFile;
    
private:

 //==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    FIELD    (cppCounterpartInstance,    "cppCounterpartInstance", "J") \
    CALLBACK (constructNativeClassJni,   "constructNativeClass",   "()V") \
    CALLBACK (destroyNativeClassJni,     "destroyNativeClass",     "()V") \
    
    DECLARE_JNI_CLASS (SonoBusActivityJavaClass, JUCE_ANDROID_ACTIVITY_OVERRIDE_CLASSPATH)
#undef JNI_CLASS_MEMBERS


 static SonoBusActivity* getCppInstance (JNIEnv* env, jobject javaInstance)
    {
        // always call JUCE::initialiseJUCEThread in java callbacks
        return reinterpret_cast<SonoBusActivity*> (env->GetLongField (javaInstance,
                                                                                SonoBusActivityJavaClass.cppCounterpartInstance));
    }
    
    static void JNIEXPORT constructNativeClassJni (JNIEnv* env, jobject javaInstance)
    {
        initialiseJuce_GUI();
        new SonoBusActivity (javaInstance);
    }
    
    static void JNIEXPORT destroyNativeClassJni (JNIEnv* env, jobject javaInstance)
    {
        if (auto* myself = getCppInstance (env, javaInstance))
            delete myself;
    }

 
    //==============================================================================
    jweak javaCounterpartInstance = nullptr;
    
};


namespace juce {

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (setForegroundServiceActive,     "setForegroundServiceActive", "(Z)V")

    DECLARE_JNI_CLASS (SonoBusActivity, JUCE_ANDROID_ACTIVITY_OVERRIDE_CLASSPATH);
#undef JNI_CLASS_MEMBERS


}


#endif

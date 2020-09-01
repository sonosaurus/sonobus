/* ------------------------------------------------------------
author: "JOS, Revised by RM"
name: "zitaRev"
version: "0.0"
Code generated with Faust 2.28.3 (https://faust.grame.fr)
Compilation options: -lang cpp -scal -ftz 0
------------------------------------------------------------ */

#ifndef  __zitaRev_H__
#define  __zitaRev_H__

// Music 256a / CS 476a | fall 2016
// CCRMA, Stanford University
//
// Author: Romain Michon (rmichonATccrmaDOTstanfordDOTedu)
// Description: Simple Faust architecture file to easily integrate a Faust DSP module
// in a JUCE project

// needed by any Faust arch file
/************************** BEGIN misc.h **************************/
/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2017 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef __misc__
#define __misc__

#include <algorithm>
#include <map>
#include <cstdlib>
#include <string.h>
#include <fstream>
#include <string>

/************************** BEGIN meta.h **************************/
/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2017 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef __meta__
#define __meta__

struct Meta
{
    virtual ~Meta() {};
    virtual void declare(const char* key, const char* value) = 0;
    
};

#endif
/**************************  END  meta.h **************************/

using std::max;
using std::min;

struct XXXX_Meta : std::map<const char*, const char*>
{
    void declare(const char* key, const char* value) { (*this)[key] = value; }
};

struct MY_Meta : Meta, std::map<const char*, const char*>
{
    void declare(const char* key, const char* value) { (*this)[key] = value; }
};

static int lsr(int x, int n) { return int(((unsigned int)x) >> n); }

static int int2pow2(int x) { int r = 0; while ((1<<r) < x) r++; return r; }

static long lopt(char* argv[], const char* name, long def)
{
    for (int i = 0; argv[i]; i++) if (!strcmp(argv[i], name)) return std::atoi(argv[i+1]);
    return def;
}

static long lopt1(int argc, char* argv[], const char* longname, const char* shortname, long def)
{
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i-1], shortname) == 0 || strcmp(argv[i-1], longname) == 0) {
            return atoi(argv[i]);
        }
    }
    return def;
}

static const char* lopts(char* argv[], const char* name, const char* def)
{
    for (int i = 0; argv[i]; i++) if (!strcmp(argv[i], name)) return argv[i+1];
    return def;
}

static const char* lopts1(int argc, char* argv[], const char* longname, const char* shortname, const char* def)
{
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i-1], shortname) == 0 || strcmp(argv[i-1], longname) == 0) {
            return argv[i];
        }
    }
    return def;
}

static bool isopt(char* argv[], const char* name)
{
    for (int i = 0; argv[i]; i++) if (!strcmp(argv[i], name)) return true;
    return false;
}

static std::string pathToContent(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ifstream::binary);
    
    file.seekg(0, file.end);
    int size = int(file.tellg());
    file.seekg(0, file.beg);
    
    // And allocate buffer to that a single line can be read...
    char* buffer = new char[size + 1];
    file.read(buffer, size);
    
    // Terminate the string
    buffer[size] = 0;
    std::string result = buffer;
    file.close();
    delete [] buffer;
    return result;
}

#endif

/**************************  END  misc.h **************************/

// allows to control a Faust DSP module in a simple manner by using parameter's path
/************************** BEGIN MapUI.h **************************/
/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2017 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef FAUST_MAPUI_H
#define FAUST_MAPUI_H

#include <vector>
#include <map>
#include <string>

/************************** BEGIN UI.h **************************/
/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2020 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef __UI_H__
#define __UI_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

/*******************************************************************************
 * UI : Faust DSP User Interface
 * User Interface as expected by the buildUserInterface() method of a DSP.
 * This abstract class contains only the method that the Faust compiler can
 * generate to describe a DSP user interface.
 ******************************************************************************/

struct Soundfile;

template <typename REAL>
struct UIReal
{
    UIReal() {}
    virtual ~UIReal() {}
    
    // -- widget's layouts
    
    virtual void openTabBox(const char* label) = 0;
    virtual void openHorizontalBox(const char* label) = 0;
    virtual void openVerticalBox(const char* label) = 0;
    virtual void closeBox() = 0;
    
    // -- active widgets
    
    virtual void addButton(const char* label, REAL* zone) = 0;
    virtual void addCheckButton(const char* label, REAL* zone) = 0;
    virtual void addVerticalSlider(const char* label, REAL* zone, REAL init, REAL min, REAL max, REAL step) = 0;
    virtual void addHorizontalSlider(const char* label, REAL* zone, REAL init, REAL min, REAL max, REAL step) = 0;
    virtual void addNumEntry(const char* label, REAL* zone, REAL init, REAL min, REAL max, REAL step) = 0;
    
    // -- passive widgets
    
    virtual void addHorizontalBargraph(const char* label, REAL* zone, REAL min, REAL max) = 0;
    virtual void addVerticalBargraph(const char* label, REAL* zone, REAL min, REAL max) = 0;
    
    // -- soundfiles
    
    virtual void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) = 0;
    
    // -- metadata declarations
    
    virtual void declare(REAL* zone, const char* key, const char* val) {}
};

struct UI : public UIReal<FAUSTFLOAT>
{
    UI() {}
    virtual ~UI() {}
};

#endif
/**************************  END  UI.h **************************/
/************************** BEGIN PathBuilder.h **************************/
/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2017 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef FAUST_PATHBUILDER_H
#define FAUST_PATHBUILDER_H

#include <vector>
#include <string>
#include <algorithm>

/*******************************************************************************
 * PathBuilder : Faust User Interface
 * Helper class to build complete hierarchical path for UI items.
 ******************************************************************************/

class PathBuilder
{

    protected:
    
        std::vector<std::string> fControlsLevel;
       
    public:
    
        PathBuilder() {}
        virtual ~PathBuilder() {}
    
        std::string buildPath(const std::string& label) 
        {
            std::string res = "/";
            for (size_t i = 0; i < fControlsLevel.size(); i++) {
                res += fControlsLevel[i];
                res += "/";
            }
            res += label;
            std::replace(res.begin(), res.end(), ' ', '_');
            return res;
        }
    
        std::string buildLabel(std::string label)
        {
            std::replace(label.begin(), label.end(), ' ', '_');
            return label;
        }
    
        void pushLabel(const std::string& label) { fControlsLevel.push_back(label); }
        void popLabel() { fControlsLevel.pop_back(); }
    
};

#endif  // FAUST_PATHBUILDER_H
/**************************  END  PathBuilder.h **************************/

/*******************************************************************************
 * MapUI : Faust User Interface
 * This class creates a map of complete hierarchical path and zones for each UI items.
 ******************************************************************************/

class MapUI : public UI, public PathBuilder
{
    
    protected:
    
        // Complete path map
        std::map<std::string, FAUSTFLOAT*> fPathZoneMap;
    
        // Label zone map
        std::map<std::string, FAUSTFLOAT*> fLabelZoneMap;
    
    public:
        
        MapUI() {}
        virtual ~MapUI() {}
        
        // -- widget's layouts
        void openTabBox(const char* label)
        {
            pushLabel(label);
        }
        void openHorizontalBox(const char* label)
        {
            pushLabel(label);
        }
        void openVerticalBox(const char* label)
        {
            pushLabel(label);
        }
        void closeBox()
        {
            popLabel();
        }
        
        // -- active widgets
        void addButton(const char* label, FAUSTFLOAT* zone)
        {
            fPathZoneMap[buildPath(label)] = zone;
            fLabelZoneMap[label] = zone;
        }
        void addCheckButton(const char* label, FAUSTFLOAT* zone)
        {
            fPathZoneMap[buildPath(label)] = zone;
            fLabelZoneMap[label] = zone;
        }
        void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT fmin, FAUSTFLOAT fmax, FAUSTFLOAT step)
        {
            fPathZoneMap[buildPath(label)] = zone;
            fLabelZoneMap[label] = zone;
        }
        void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT fmin, FAUSTFLOAT fmax, FAUSTFLOAT step)
        {
            fPathZoneMap[buildPath(label)] = zone;
            fLabelZoneMap[label] = zone;
        }
        void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT fmin, FAUSTFLOAT fmax, FAUSTFLOAT step)
        {
            fPathZoneMap[buildPath(label)] = zone;
            fLabelZoneMap[label] = zone;
        }
        
        // -- passive widgets
        void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT fmin, FAUSTFLOAT fmax)
        {
            fPathZoneMap[buildPath(label)] = zone;
            fLabelZoneMap[label] = zone;
        }
        void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT fmin, FAUSTFLOAT fmax)
        {
            fPathZoneMap[buildPath(label)] = zone;
            fLabelZoneMap[label] = zone;
        }
    
        // -- soundfiles
        virtual void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) {}
        
        // -- metadata declarations
        virtual void declare(FAUSTFLOAT* zone, const char* key, const char* val)
        {}
        
        // set/get
        void setParamValue(const std::string& path, FAUSTFLOAT value)
        {
            if (fPathZoneMap.find(path) != fPathZoneMap.end()) {
                *fPathZoneMap[path] = value;
            } else if (fLabelZoneMap.find(path) != fLabelZoneMap.end()) {
                *fLabelZoneMap[path] = value;
            }
        }
        
        FAUSTFLOAT getParamValue(const std::string& path)
        {
            if (fPathZoneMap.find(path) != fPathZoneMap.end()) {
                return *fPathZoneMap[path];
            } else if (fLabelZoneMap.find(path) != fLabelZoneMap.end()) {
                return *fLabelZoneMap[path];
            } else {
                return FAUSTFLOAT(0);
            }
        }
    
        // map access 
        std::map<std::string, FAUSTFLOAT*>& getMap() { return fPathZoneMap; }
        
        int getParamsCount() { return int(fPathZoneMap.size()); }
        
        std::string getParamAddress(int index)
        {
            if (index < 0 || index > int(fPathZoneMap.size())) {
                return "";
            } else {
                auto it = fPathZoneMap.begin();
                while (index-- > 0 && it++ != fPathZoneMap.end()) {}
                return it->first;
            }
        }
    
        std::string getParamAddress(FAUSTFLOAT* zone)
        {
            for (auto& it : fPathZoneMap) {
                if (it.second == zone) return it.first;
            }
            return "";
        }
    
        FAUSTFLOAT* getParamZone(const std::string& str)
        {
            if (fPathZoneMap.find(str) != fPathZoneMap.end()) {
                return fPathZoneMap[str];
            }
            if (fLabelZoneMap.find(str) != fLabelZoneMap.end()) {
                return fLabelZoneMap[str];
            }
            return nullptr;
        }
    
        FAUSTFLOAT* getParamZone(int index)
        {
            if (index < 0 || index > int(fPathZoneMap.size())) {
                return nullptr;
            } else {
                auto it = fPathZoneMap.begin();
                while (index-- > 0 && it++ != fPathZoneMap.end()) {}
                return it->second;
            }
        }
    
        static bool endsWith(const std::string& str, const std::string& end)
        {
            size_t l1 = str.length();
            size_t l2 = end.length();
            return (l1 >= l2) && (0 == str.compare(l1 - l2, l2, end));
        }
};


#endif // FAUST_MAPUI_H
/**************************  END  MapUI.h **************************/

// needed by any Faust arch file
/************************** BEGIN faustdsp.h **************************/
/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2017 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef __dsp__
#define __dsp__

#include <string>
#include <vector>

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

struct UI;
struct Meta;

/**
 * DSP memory manager.
 */

struct dsp_memory_manager {
    
    virtual ~dsp_memory_manager() {}
    
    virtual void* allocate(size_t size) = 0;
    virtual void destroy(void* ptr) = 0;
    
};

/**
* Signal processor definition.
*/

class faustdsp {

    public:

        faustdsp() {}
        virtual ~faustdsp() {}

        /* Return instance number of audio inputs */
        virtual int getNumInputs() = 0;
    
        /* Return instance number of audio outputs */
        virtual int getNumOutputs() = 0;
    
        /**
         * Trigger the ui_interface parameter with instance specific calls
         * to 'openTabBox', 'addButton', 'addVerticalSlider'... in order to build the UI.
         *
         * @param ui_interface - the user interface builder
         */
        virtual void buildUserInterface(UI* ui_interface) = 0;
    
        /* Returns the sample rate currently used by the instance */
        virtual int getSampleRate() = 0;
    
        /**
         * Global init, calls the following methods:
         * - static class 'classInit': static tables initialization
         * - 'instanceInit': constants and instance state initialization
         *
         * @param sample_rate - the sampling rate in Hertz
         */
        virtual void init(int sample_rate) = 0;

        /**
         * Init instance state
         *
         * @param sample_rate - the sampling rate in Hertz
         */
        virtual void instanceInit(int sample_rate) = 0;

        /**
         * Init instance constant state
         *
         * @param sample_rate - the sampling rate in Hertz
         */
        virtual void instanceConstants(int sample_rate) = 0;
    
        /* Init default control parameters values */
        virtual void instanceResetUserInterface() = 0;
    
        /* Init instance state (delay lines...) */
        virtual void instanceClear() = 0;
 
        /**
         * Return a clone of the instance.
         *
         * @return a copy of the instance on success, otherwise a null pointer.
         */
        virtual faustdsp* clone() = 0;
    
        /**
         * Trigger the Meta* parameter with instance specific calls to 'declare' (key, value) metadata.
         *
         * @param m - the Meta* meta user
         */
        virtual void metadata(Meta* m) = 0;
    
        /**
         * DSP instance computation, to be called with successive in/out audio buffers.
         *
         * @param count - the number of frames to compute
         * @param inputs - the input audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         * @param outputs - the output audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         *
         */
        virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) = 0;
    
        /**
         * DSP instance computation: alternative method to be used by subclasses.
         *
         * @param date_usec - the timestamp in microsec given by audio driver.
         * @param count - the number of frames to compute
         * @param inputs - the input audio buffers as an array of non-interleaved FAUSTFLOAT samples (either float, double or quad)
         * @param outputs - the output audio buffers as an array of non-interleaved FAUSTFLOAT samples (either float, double or quad)
         *
         */
        virtual void compute(double /*date_usec*/, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) { compute(count, inputs, outputs); }
       
};

/**
 * Generic DSP decorator.
 */

class decorator_dsp : public faustdsp {

    protected:

        faustdsp* fDSP;

    public:

        decorator_dsp(faustdsp* faustdsp = nullptr):fDSP(faustdsp) {}
        virtual ~decorator_dsp() { delete fDSP; }

        virtual int getNumInputs() { return fDSP->getNumInputs(); }
        virtual int getNumOutputs() { return fDSP->getNumOutputs(); }
        virtual void buildUserInterface(UI* ui_interface) { fDSP->buildUserInterface(ui_interface); }
        virtual int getSampleRate() { return fDSP->getSampleRate(); }
        virtual void init(int sample_rate) { fDSP->init(sample_rate); }
        virtual void instanceInit(int sample_rate) { fDSP->instanceInit(sample_rate); }
        virtual void instanceConstants(int sample_rate) { fDSP->instanceConstants(sample_rate); }
        virtual void instanceResetUserInterface() { fDSP->instanceResetUserInterface(); }
        virtual void instanceClear() { fDSP->instanceClear(); }
        virtual decorator_dsp* clone() { return new decorator_dsp(fDSP->clone()); }
        virtual void metadata(Meta* m) { fDSP->metadata(m); }
        // Beware: subclasses usually have to overload the two 'compute' methods
        virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) { fDSP->compute(count, inputs, outputs); }
        virtual void compute(double date_usec, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) { fDSP->compute(date_usec, count, inputs, outputs); }
    
};

/**
 * DSP factory class.
 */

class dsp_factory {
    
    protected:
    
        // So that to force sub-classes to use deleteDSPFactory(dsp_factory* factory);
        virtual ~dsp_factory() {}
    
    public:
    
        virtual std::string getName() = 0;
        virtual std::string getSHAKey() = 0;
        virtual std::string getDSPCode() = 0;
        virtual std::string getCompileOptions() = 0;
        virtual std::vector<std::string> getLibraryList() = 0;
        virtual std::vector<std::string> getIncludePathnames() = 0;
    
        virtual faustdsp* createDSPInstance() = 0;
    
        virtual void setMemoryManager(dsp_memory_manager* manager) = 0;
        virtual dsp_memory_manager* getMemoryManager() = 0;
    
};

/**
 * On Intel set FZ (Flush to Zero) and DAZ (Denormals Are Zero)
 * flags to avoid costly denormals.
 */

#ifdef __SSE__
    #include <xmmintrin.h>
    #ifdef __SSE2__
        #define AVOIDDENORMALS _mm_setcsr(_mm_getcsr() | 0x8040)
    #else
        #define AVOIDDENORMALS _mm_setcsr(_mm_getcsr() | 0x8000)
    #endif
#else
    #define AVOIDDENORMALS
#endif

#endif
/**************************  END  faustdsp.h **************************/

// tags used by the Faust compiler to paste the generated c++ code
#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <math.h>

static float zitaRev_faustpower2_f(float value) {
	return (value * value);
}

#ifndef FAUSTCLASS 
#define FAUSTCLASS zitaRev
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class zitaRev : public faustdsp {
	
 private:
	
	FAUSTFLOAT fVslider0;
	float fRec0[2];
	int IOTA;
	float fVec0[16384];
	FAUSTFLOAT fVslider1;
	float fRec1[2];
	int fSampleRate;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fVslider2;
	FAUSTFLOAT fVslider3;
	float fConst2;
	float fConst3;
	FAUSTFLOAT fVslider4;
	FAUSTFLOAT fVslider5;
	FAUSTFLOAT fVslider6;
	float fConst4;
	FAUSTFLOAT fVslider7;
	float fRec14[2];
	float fRec13[2];
	float fVec1[32768];
	float fConst5;
	int iConst6;
	float fVec2[16384];
	float fConst7;
	FAUSTFLOAT fVslider8;
	float fVec3[2048];
	int iConst8;
	float fRec11[2];
	float fConst9;
	float fConst10;
	float fRec18[2];
	float fRec17[2];
	float fVec4[32768];
	float fConst11;
	int iConst12;
	float fVec5[4096];
	int iConst13;
	float fRec15[2];
	float fConst14;
	float fConst15;
	float fRec22[2];
	float fRec21[2];
	float fVec6[16384];
	float fConst16;
	int iConst17;
	float fVec7[4096];
	int iConst18;
	float fRec19[2];
	float fConst19;
	float fConst20;
	float fRec26[2];
	float fRec25[2];
	float fVec8[32768];
	float fConst21;
	int iConst22;
	float fVec9[4096];
	int iConst23;
	float fRec23[2];
	float fConst24;
	float fConst25;
	float fRec30[2];
	float fRec29[2];
	float fVec10[16384];
	float fConst26;
	int iConst27;
	float fVec11[2048];
	int iConst28;
	float fRec27[2];
	float fConst29;
	float fConst30;
	float fRec34[2];
	float fRec33[2];
	float fVec12[16384];
	float fConst31;
	int iConst32;
	float fVec13[4096];
	int iConst33;
	float fRec31[2];
	float fConst34;
	float fConst35;
	float fRec38[2];
	float fRec37[2];
	float fVec14[16384];
	float fConst36;
	int iConst37;
	float fVec15[4096];
	int iConst38;
	float fRec35[2];
	float fConst39;
	float fConst40;
	float fRec42[2];
	float fRec41[2];
	float fVec16[16384];
	float fConst41;
	int iConst42;
	float fVec17[2048];
	int iConst43;
	float fRec39[2];
	float fRec3[3];
	float fRec4[3];
	float fRec5[3];
	float fRec6[3];
	float fRec7[3];
	float fRec8[3];
	float fRec9[3];
	float fRec10[3];
	FAUSTFLOAT fVslider9;
	FAUSTFLOAT fVslider10;
	float fRec43[3];
	float fRec2[3];
	float fRec45[3];
	float fRec44[3];
	
 public:
	
	void metadata(Meta* m) { 
		m->declare("author", "JOS, Revised by RM");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "0.1");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "0.1");
		m->declare("description", "Example GUI for `zita_rev1_stereo` (mostly following the Linux `zita-rev1` GUI).");
		m->declare("filename", "zitaRev.dsp");
		m->declare("filters.lib/allpass_comb:author", "Julius O. Smith III");
		m->declare("filters.lib/allpass_comb:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/allpass_comb:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/peak_eq_rm:author", "Julius O. Smith III");
		m->declare("filters.lib/peak_eq_rm:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/peak_eq_rm:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "MIT-style STK-4.3 license");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.3");
		m->declare("name", "zitaRev");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "0.1");
		m->declare("reverbs.lib/name", "Faust Reverb Library");
		m->declare("reverbs.lib/version", "0.0");
		m->declare("routes.lib/name", "Faust Signal Routing Library");
		m->declare("routes.lib/version", "0.2");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/version", "0.0");
		m->declare("version", "0.0");
	}

	virtual int getNumInputs() {
		return 2;
	}
	virtual int getNumOutputs() {
		return 2;
	}
	virtual int getInputRate(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 1;
				break;
			}
			case 1: {
				rate = 1;
				break;
			}
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}
	virtual int getOutputRate(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 1;
				break;
			}
			case 1: {
				rate = 1;
				break;
			}
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}
	
	static void classInit(int sample_rate) {
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(192000.0f, std::max<float>(1.0f, float(fSampleRate)));
		fConst1 = (6.28318548f / fConst0);
		fConst2 = std::floor(((0.219990999f * fConst0) + 0.5f));
		fConst3 = ((0.0f - (6.90775537f * fConst2)) / fConst0);
		fConst4 = (3.14159274f / fConst0);
		fConst5 = std::floor(((0.0191229992f * fConst0) + 0.5f));
		iConst6 = int(std::min<float>(16384.0f, std::max<float>(0.0f, (fConst2 - fConst5))));
		fConst7 = (0.00100000005f * fConst0);
		iConst8 = int(std::min<float>(1024.0f, std::max<float>(0.0f, (fConst5 + -1.0f))));
		fConst9 = std::floor(((0.256891012f * fConst0) + 0.5f));
		fConst10 = ((0.0f - (6.90775537f * fConst9)) / fConst0);
		fConst11 = std::floor(((0.0273330007f * fConst0) + 0.5f));
		iConst12 = int(std::min<float>(16384.0f, std::max<float>(0.0f, (fConst9 - fConst11))));
		iConst13 = int(std::min<float>(2048.0f, std::max<float>(0.0f, (fConst11 + -1.0f))));
		fConst14 = std::floor(((0.192303002f * fConst0) + 0.5f));
		fConst15 = ((0.0f - (6.90775537f * fConst14)) / fConst0);
		fConst16 = std::floor(((0.0292910002f * fConst0) + 0.5f));
		iConst17 = int(std::min<float>(8192.0f, std::max<float>(0.0f, (fConst14 - fConst16))));
		iConst18 = int(std::min<float>(2048.0f, std::max<float>(0.0f, (fConst16 + -1.0f))));
		fConst19 = std::floor(((0.210389003f * fConst0) + 0.5f));
		fConst20 = ((0.0f - (6.90775537f * fConst19)) / fConst0);
		fConst21 = std::floor(((0.0244210009f * fConst0) + 0.5f));
		iConst22 = int(std::min<float>(16384.0f, std::max<float>(0.0f, (fConst19 - fConst21))));
		iConst23 = int(std::min<float>(2048.0f, std::max<float>(0.0f, (fConst21 + -1.0f))));
		fConst24 = std::floor(((0.125f * fConst0) + 0.5f));
		fConst25 = ((0.0f - (6.90775537f * fConst24)) / fConst0);
		fConst26 = std::floor(((0.0134579996f * fConst0) + 0.5f));
		iConst27 = int(std::min<float>(8192.0f, std::max<float>(0.0f, (fConst24 - fConst26))));
		iConst28 = int(std::min<float>(1024.0f, std::max<float>(0.0f, (fConst26 + -1.0f))));
		fConst29 = std::floor(((0.127837002f * fConst0) + 0.5f));
		fConst30 = ((0.0f - (6.90775537f * fConst29)) / fConst0);
		fConst31 = std::floor(((0.0316039994f * fConst0) + 0.5f));
		iConst32 = int(std::min<float>(8192.0f, std::max<float>(0.0f, (fConst29 - fConst31))));
		iConst33 = int(std::min<float>(2048.0f, std::max<float>(0.0f, (fConst31 + -1.0f))));
		fConst34 = std::floor(((0.174713001f * fConst0) + 0.5f));
		fConst35 = ((0.0f - (6.90775537f * fConst34)) / fConst0);
		fConst36 = std::floor(((0.0229039993f * fConst0) + 0.5f));
		iConst37 = int(std::min<float>(8192.0f, std::max<float>(0.0f, (fConst34 - fConst36))));
		iConst38 = int(std::min<float>(2048.0f, std::max<float>(0.0f, (fConst36 + -1.0f))));
		fConst39 = std::floor(((0.153128996f * fConst0) + 0.5f));
		fConst40 = ((0.0f - (6.90775537f * fConst39)) / fConst0);
		fConst41 = std::floor(((0.0203460008f * fConst0) + 0.5f));
		iConst42 = int(std::min<float>(8192.0f, std::max<float>(0.0f, (fConst39 - fConst41))));
		iConst43 = int(std::min<float>(1024.0f, std::max<float>(0.0f, (fConst41 + -1.0f))));
	}
	
	virtual void instanceResetUserInterface() {
		fVslider0 = FAUSTFLOAT(-20.0f);
		fVslider1 = FAUSTFLOAT(0.0f);
		fVslider2 = FAUSTFLOAT(1500.0f);
		fVslider3 = FAUSTFLOAT(0.0f);
		fVslider4 = FAUSTFLOAT(2.0f);
		fVslider5 = FAUSTFLOAT(6000.0f);
		fVslider6 = FAUSTFLOAT(3.0f);
		fVslider7 = FAUSTFLOAT(200.0f);
		fVslider8 = FAUSTFLOAT(60.0f);
		fVslider9 = FAUSTFLOAT(315.0f);
		fVslider10 = FAUSTFLOAT(0.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fRec0[l0] = 0.0f;
		}
		IOTA = 0;
		for (int l1 = 0; (l1 < 16384); l1 = (l1 + 1)) {
			fVec0[l1] = 0.0f;
		}
		for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			fRec1[l2] = 0.0f;
		}
		for (int l3 = 0; (l3 < 2); l3 = (l3 + 1)) {
			fRec14[l3] = 0.0f;
		}
		for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			fRec13[l4] = 0.0f;
		}
		for (int l5 = 0; (l5 < 32768); l5 = (l5 + 1)) {
			fVec1[l5] = 0.0f;
		}
		for (int l6 = 0; (l6 < 16384); l6 = (l6 + 1)) {
			fVec2[l6] = 0.0f;
		}
		for (int l7 = 0; (l7 < 2048); l7 = (l7 + 1)) {
			fVec3[l7] = 0.0f;
		}
		for (int l8 = 0; (l8 < 2); l8 = (l8 + 1)) {
			fRec11[l8] = 0.0f;
		}
		for (int l9 = 0; (l9 < 2); l9 = (l9 + 1)) {
			fRec18[l9] = 0.0f;
		}
		for (int l10 = 0; (l10 < 2); l10 = (l10 + 1)) {
			fRec17[l10] = 0.0f;
		}
		for (int l11 = 0; (l11 < 32768); l11 = (l11 + 1)) {
			fVec4[l11] = 0.0f;
		}
		for (int l12 = 0; (l12 < 4096); l12 = (l12 + 1)) {
			fVec5[l12] = 0.0f;
		}
		for (int l13 = 0; (l13 < 2); l13 = (l13 + 1)) {
			fRec15[l13] = 0.0f;
		}
		for (int l14 = 0; (l14 < 2); l14 = (l14 + 1)) {
			fRec22[l14] = 0.0f;
		}
		for (int l15 = 0; (l15 < 2); l15 = (l15 + 1)) {
			fRec21[l15] = 0.0f;
		}
		for (int l16 = 0; (l16 < 16384); l16 = (l16 + 1)) {
			fVec6[l16] = 0.0f;
		}
		for (int l17 = 0; (l17 < 4096); l17 = (l17 + 1)) {
			fVec7[l17] = 0.0f;
		}
		for (int l18 = 0; (l18 < 2); l18 = (l18 + 1)) {
			fRec19[l18] = 0.0f;
		}
		for (int l19 = 0; (l19 < 2); l19 = (l19 + 1)) {
			fRec26[l19] = 0.0f;
		}
		for (int l20 = 0; (l20 < 2); l20 = (l20 + 1)) {
			fRec25[l20] = 0.0f;
		}
		for (int l21 = 0; (l21 < 32768); l21 = (l21 + 1)) {
			fVec8[l21] = 0.0f;
		}
		for (int l22 = 0; (l22 < 4096); l22 = (l22 + 1)) {
			fVec9[l22] = 0.0f;
		}
		for (int l23 = 0; (l23 < 2); l23 = (l23 + 1)) {
			fRec23[l23] = 0.0f;
		}
		for (int l24 = 0; (l24 < 2); l24 = (l24 + 1)) {
			fRec30[l24] = 0.0f;
		}
		for (int l25 = 0; (l25 < 2); l25 = (l25 + 1)) {
			fRec29[l25] = 0.0f;
		}
		for (int l26 = 0; (l26 < 16384); l26 = (l26 + 1)) {
			fVec10[l26] = 0.0f;
		}
		for (int l27 = 0; (l27 < 2048); l27 = (l27 + 1)) {
			fVec11[l27] = 0.0f;
		}
		for (int l28 = 0; (l28 < 2); l28 = (l28 + 1)) {
			fRec27[l28] = 0.0f;
		}
		for (int l29 = 0; (l29 < 2); l29 = (l29 + 1)) {
			fRec34[l29] = 0.0f;
		}
		for (int l30 = 0; (l30 < 2); l30 = (l30 + 1)) {
			fRec33[l30] = 0.0f;
		}
		for (int l31 = 0; (l31 < 16384); l31 = (l31 + 1)) {
			fVec12[l31] = 0.0f;
		}
		for (int l32 = 0; (l32 < 4096); l32 = (l32 + 1)) {
			fVec13[l32] = 0.0f;
		}
		for (int l33 = 0; (l33 < 2); l33 = (l33 + 1)) {
			fRec31[l33] = 0.0f;
		}
		for (int l34 = 0; (l34 < 2); l34 = (l34 + 1)) {
			fRec38[l34] = 0.0f;
		}
		for (int l35 = 0; (l35 < 2); l35 = (l35 + 1)) {
			fRec37[l35] = 0.0f;
		}
		for (int l36 = 0; (l36 < 16384); l36 = (l36 + 1)) {
			fVec14[l36] = 0.0f;
		}
		for (int l37 = 0; (l37 < 4096); l37 = (l37 + 1)) {
			fVec15[l37] = 0.0f;
		}
		for (int l38 = 0; (l38 < 2); l38 = (l38 + 1)) {
			fRec35[l38] = 0.0f;
		}
		for (int l39 = 0; (l39 < 2); l39 = (l39 + 1)) {
			fRec42[l39] = 0.0f;
		}
		for (int l40 = 0; (l40 < 2); l40 = (l40 + 1)) {
			fRec41[l40] = 0.0f;
		}
		for (int l41 = 0; (l41 < 16384); l41 = (l41 + 1)) {
			fVec16[l41] = 0.0f;
		}
		for (int l42 = 0; (l42 < 2048); l42 = (l42 + 1)) {
			fVec17[l42] = 0.0f;
		}
		for (int l43 = 0; (l43 < 2); l43 = (l43 + 1)) {
			fRec39[l43] = 0.0f;
		}
		for (int l44 = 0; (l44 < 3); l44 = (l44 + 1)) {
			fRec3[l44] = 0.0f;
		}
		for (int l45 = 0; (l45 < 3); l45 = (l45 + 1)) {
			fRec4[l45] = 0.0f;
		}
		for (int l46 = 0; (l46 < 3); l46 = (l46 + 1)) {
			fRec5[l46] = 0.0f;
		}
		for (int l47 = 0; (l47 < 3); l47 = (l47 + 1)) {
			fRec6[l47] = 0.0f;
		}
		for (int l48 = 0; (l48 < 3); l48 = (l48 + 1)) {
			fRec7[l48] = 0.0f;
		}
		for (int l49 = 0; (l49 < 3); l49 = (l49 + 1)) {
			fRec8[l49] = 0.0f;
		}
		for (int l50 = 0; (l50 < 3); l50 = (l50 + 1)) {
			fRec9[l50] = 0.0f;
		}
		for (int l51 = 0; (l51 < 3); l51 = (l51 + 1)) {
			fRec10[l51] = 0.0f;
		}
		for (int l52 = 0; (l52 < 3); l52 = (l52 + 1)) {
			fRec43[l52] = 0.0f;
		}
		for (int l53 = 0; (l53 < 3); l53 = (l53 + 1)) {
			fRec2[l53] = 0.0f;
		}
		for (int l54 = 0; (l54 < 3); l54 = (l54 + 1)) {
			fRec45[l54] = 0.0f;
		}
		for (int l55 = 0; (l55 < 3); l55 = (l55 + 1)) {
			fRec44[l55] = 0.0f;
		}
	}
	
	virtual void init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}
	virtual void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}
	
	virtual zitaRev* clone() {
		return new zitaRev();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->declare(0, "0", "");
		ui_interface->declare(0, "tooltip", "~ ZITA REV1 FEEDBACK DELAY NETWORK (FDN) & SCHROEDER  ALLPASS-COMB REVERBERATOR (8x8). See Faust's reverbs.lib for documentation and  references");
		ui_interface->openHorizontalBox("Zita_Rev1");
		ui_interface->declare(0, "1", "");
		ui_interface->openHorizontalBox("Input");
		ui_interface->declare(&fVslider8, "1", "");
		ui_interface->declare(&fVslider8, "style", "knob");
		ui_interface->declare(&fVslider8, "tooltip", "Delay in ms   before reverberation begins");
		ui_interface->declare(&fVslider8, "unit", "ms");
		ui_interface->addVerticalSlider("In Delay", &fVslider8, 60.0f, 20.0f, 100.0f, 1.0f);
		ui_interface->closeBox();
		ui_interface->declare(0, "2", "");
		ui_interface->openHorizontalBox("Decay Times in Bands (see tooltips)");
		ui_interface->declare(&fVslider7, "1", "");
		ui_interface->declare(&fVslider7, "scale", "log");
		ui_interface->declare(&fVslider7, "style", "knob");
		ui_interface->declare(&fVslider7, "tooltip", "Crossover frequency (Hz) separating low and middle frequencies");
		ui_interface->declare(&fVslider7, "unit", "Hz");
		ui_interface->addVerticalSlider("LF X", &fVslider7, 200.0f, 50.0f, 1000.0f, 1.0f);
		ui_interface->declare(&fVslider6, "2", "");
		ui_interface->declare(&fVslider6, "scale", "log");
		ui_interface->declare(&fVslider6, "style", "knob");
		ui_interface->declare(&fVslider6, "tooltip", "T60 = time (in seconds) to decay 60dB in low-frequency band");
		ui_interface->declare(&fVslider6, "unit", "s");
		ui_interface->addVerticalSlider("Low RT60", &fVslider6, 3.0f, 1.0f, 8.0f, 0.100000001f);
		ui_interface->declare(&fVslider4, "3", "");
		ui_interface->declare(&fVslider4, "scale", "log");
		ui_interface->declare(&fVslider4, "style", "knob");
		ui_interface->declare(&fVslider4, "tooltip", "T60 = time (in seconds) to decay 60dB in middle band");
		ui_interface->declare(&fVslider4, "unit", "s");
		ui_interface->addVerticalSlider("Mid RT60", &fVslider4, 2.0f, 1.0f, 8.0f, 0.100000001f);
		ui_interface->declare(&fVslider5, "4", "");
		ui_interface->declare(&fVslider5, "scale", "log");
		ui_interface->declare(&fVslider5, "style", "knob");
		ui_interface->declare(&fVslider5, "tooltip", "Frequency (Hz) at which the high-frequency T60 is half the middle-band's T60");
		ui_interface->declare(&fVslider5, "unit", "Hz");
		ui_interface->addVerticalSlider("HF Damping", &fVslider5, 6000.0f, 1500.0f, 23520.0f, 1.0f);
		ui_interface->closeBox();
		ui_interface->declare(0, "3", "");
		ui_interface->openHorizontalBox("RM Peaking Equalizer 1");
		ui_interface->declare(&fVslider9, "1", "");
		ui_interface->declare(&fVslider9, "scale", "log");
		ui_interface->declare(&fVslider9, "style", "knob");
		ui_interface->declare(&fVslider9, "tooltip", "Center-frequency of second-order Regalia-Mitra peaking equalizer section 1");
		ui_interface->declare(&fVslider9, "unit", "Hz");
		ui_interface->addVerticalSlider("Eq1 Freq", &fVslider9, 315.0f, 40.0f, 2500.0f, 1.0f);
		ui_interface->declare(&fVslider10, "2", "");
		ui_interface->declare(&fVslider10, "style", "knob");
		ui_interface->declare(&fVslider10, "tooltip", "Peak level   in dB of second-order Regalia-Mitra peaking equalizer section 1");
		ui_interface->declare(&fVslider10, "unit", "dB");
		ui_interface->addVerticalSlider("Eq1 Level", &fVslider10, 0.0f, -15.0f, 15.0f, 0.100000001f);
		ui_interface->closeBox();
		ui_interface->declare(0, "4", "");
		ui_interface->openHorizontalBox("RM Peaking Equalizer 2");
		ui_interface->declare(&fVslider2, "1", "");
		ui_interface->declare(&fVslider2, "scale", "log");
		ui_interface->declare(&fVslider2, "style", "knob");
		ui_interface->declare(&fVslider2, "tooltip", "Center-frequency of second-order Regalia-Mitra peaking equalizer section 2");
		ui_interface->declare(&fVslider2, "unit", "Hz");
		ui_interface->addVerticalSlider("Eq2 Freq", &fVslider2, 1500.0f, 160.0f, 10000.0f, 1.0f);
		ui_interface->declare(&fVslider3, "2", "");
		ui_interface->declare(&fVslider3, "style", "knob");
		ui_interface->declare(&fVslider3, "tooltip", "Peak level   in dB of second-order Regalia-Mitra peaking equalizer section 2");
		ui_interface->declare(&fVslider3, "unit", "dB");
		ui_interface->addVerticalSlider("Eq2 Level", &fVslider3, 0.0f, -15.0f, 15.0f, 0.100000001f);
		ui_interface->closeBox();
		ui_interface->declare(0, "5", "");
		ui_interface->openHorizontalBox("Output");
		ui_interface->declare(&fVslider1, "1", "");
		ui_interface->declare(&fVslider1, "style", "knob");
		ui_interface->declare(&fVslider1, "tooltip", "-1 = dry, 1 = wet");
		ui_interface->addVerticalSlider("Dry/Wet Mix", &fVslider1, 0.0f, -1.0f, 1.0f, 0.00999999978f);
		ui_interface->declare(&fVslider0, "2", "");
		ui_interface->declare(&fVslider0, "style", "knob");
		ui_interface->declare(&fVslider0, "tooltip", "Output scale   factor");
		ui_interface->declare(&fVslider0, "unit", "dB");
		ui_interface->addVerticalSlider("Level", &fVslider0, -20.0f, -70.0f, 40.0f, 0.100000001f);
		ui_interface->closeBox();
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = (0.00100000005f * std::pow(10.0f, (0.0500000007f * float(fVslider0))));
		float fSlow1 = (0.00100000005f * float(fVslider1));
		float fSlow2 = float(fVslider2);
		float fSlow3 = std::pow(10.0f, (0.0500000007f * float(fVslider3)));
		float fSlow4 = (fConst1 * (fSlow2 / std::sqrt(std::max<float>(0.0f, fSlow3))));
		float fSlow5 = ((1.0f - fSlow4) / (fSlow4 + 1.0f));
		float fSlow6 = float(fVslider4);
		float fSlow7 = std::exp((fConst3 / fSlow6));
		float fSlow8 = zitaRev_faustpower2_f(fSlow7);
		float fSlow9 = std::cos((fConst1 * float(fVslider5)));
		float fSlow10 = (1.0f - (fSlow8 * fSlow9));
		float fSlow11 = (1.0f - fSlow8);
		float fSlow12 = (fSlow10 / fSlow11);
		float fSlow13 = std::sqrt(std::max<float>(0.0f, ((zitaRev_faustpower2_f(fSlow10) / zitaRev_faustpower2_f(fSlow11)) + -1.0f)));
		float fSlow14 = (fSlow12 - fSlow13);
		float fSlow15 = (fSlow7 * (fSlow13 + (1.0f - fSlow12)));
		float fSlow16 = float(fVslider6);
		float fSlow17 = ((std::exp((fConst3 / fSlow16)) / fSlow7) + -1.0f);
		float fSlow18 = (1.0f / std::tan((fConst4 * float(fVslider7))));
		float fSlow19 = (1.0f / (fSlow18 + 1.0f));
		float fSlow20 = (1.0f - fSlow18);
		int iSlow21 = int(std::min<float>(8192.0f, std::max<float>(0.0f, (fConst7 * float(fVslider8)))));
		float fSlow22 = std::exp((fConst10 / fSlow6));
		float fSlow23 = zitaRev_faustpower2_f(fSlow22);
		float fSlow24 = (1.0f - (fSlow23 * fSlow9));
		float fSlow25 = (1.0f - fSlow23);
		float fSlow26 = (fSlow24 / fSlow25);
		float fSlow27 = std::sqrt(std::max<float>(0.0f, ((zitaRev_faustpower2_f(fSlow24) / zitaRev_faustpower2_f(fSlow25)) + -1.0f)));
		float fSlow28 = (fSlow26 - fSlow27);
		float fSlow29 = (fSlow22 * (fSlow27 + (1.0f - fSlow26)));
		float fSlow30 = ((std::exp((fConst10 / fSlow16)) / fSlow22) + -1.0f);
		float fSlow31 = std::exp((fConst15 / fSlow6));
		float fSlow32 = zitaRev_faustpower2_f(fSlow31);
		float fSlow33 = (1.0f - (fSlow32 * fSlow9));
		float fSlow34 = (1.0f - fSlow32);
		float fSlow35 = (fSlow33 / fSlow34);
		float fSlow36 = std::sqrt(std::max<float>(0.0f, ((zitaRev_faustpower2_f(fSlow33) / zitaRev_faustpower2_f(fSlow34)) + -1.0f)));
		float fSlow37 = (fSlow35 - fSlow36);
		float fSlow38 = (fSlow31 * (fSlow36 + (1.0f - fSlow35)));
		float fSlow39 = ((std::exp((fConst15 / fSlow16)) / fSlow31) + -1.0f);
		float fSlow40 = std::exp((fConst20 / fSlow6));
		float fSlow41 = zitaRev_faustpower2_f(fSlow40);
		float fSlow42 = (1.0f - (fSlow41 * fSlow9));
		float fSlow43 = (1.0f - fSlow41);
		float fSlow44 = (fSlow42 / fSlow43);
		float fSlow45 = std::sqrt(std::max<float>(0.0f, ((zitaRev_faustpower2_f(fSlow42) / zitaRev_faustpower2_f(fSlow43)) + -1.0f)));
		float fSlow46 = (fSlow44 - fSlow45);
		float fSlow47 = (fSlow40 * (fSlow45 + (1.0f - fSlow44)));
		float fSlow48 = ((std::exp((fConst20 / fSlow16)) / fSlow40) + -1.0f);
		float fSlow49 = std::exp((fConst25 / fSlow6));
		float fSlow50 = zitaRev_faustpower2_f(fSlow49);
		float fSlow51 = (1.0f - (fSlow50 * fSlow9));
		float fSlow52 = (1.0f - fSlow50);
		float fSlow53 = (fSlow51 / fSlow52);
		float fSlow54 = std::sqrt(std::max<float>(0.0f, ((zitaRev_faustpower2_f(fSlow51) / zitaRev_faustpower2_f(fSlow52)) + -1.0f)));
		float fSlow55 = (fSlow53 - fSlow54);
		float fSlow56 = (fSlow49 * (fSlow54 + (1.0f - fSlow53)));
		float fSlow57 = ((std::exp((fConst25 / fSlow16)) / fSlow49) + -1.0f);
		float fSlow58 = std::exp((fConst30 / fSlow6));
		float fSlow59 = zitaRev_faustpower2_f(fSlow58);
		float fSlow60 = (1.0f - (fSlow59 * fSlow9));
		float fSlow61 = (1.0f - fSlow59);
		float fSlow62 = (fSlow60 / fSlow61);
		float fSlow63 = std::sqrt(std::max<float>(0.0f, ((zitaRev_faustpower2_f(fSlow60) / zitaRev_faustpower2_f(fSlow61)) + -1.0f)));
		float fSlow64 = (fSlow62 - fSlow63);
		float fSlow65 = (fSlow58 * (fSlow63 + (1.0f - fSlow62)));
		float fSlow66 = ((std::exp((fConst30 / fSlow16)) / fSlow58) + -1.0f);
		float fSlow67 = std::exp((fConst35 / fSlow6));
		float fSlow68 = zitaRev_faustpower2_f(fSlow67);
		float fSlow69 = (1.0f - (fSlow68 * fSlow9));
		float fSlow70 = (1.0f - fSlow68);
		float fSlow71 = (fSlow69 / fSlow70);
		float fSlow72 = std::sqrt(std::max<float>(0.0f, ((zitaRev_faustpower2_f(fSlow69) / zitaRev_faustpower2_f(fSlow70)) + -1.0f)));
		float fSlow73 = (fSlow71 - fSlow72);
		float fSlow74 = (fSlow67 * (fSlow72 + (1.0f - fSlow71)));
		float fSlow75 = ((std::exp((fConst35 / fSlow16)) / fSlow67) + -1.0f);
		float fSlow76 = std::exp((fConst40 / fSlow6));
		float fSlow77 = zitaRev_faustpower2_f(fSlow76);
		float fSlow78 = (1.0f - (fSlow77 * fSlow9));
		float fSlow79 = (1.0f - fSlow77);
		float fSlow80 = (fSlow78 / fSlow79);
		float fSlow81 = std::sqrt(std::max<float>(0.0f, ((zitaRev_faustpower2_f(fSlow78) / zitaRev_faustpower2_f(fSlow79)) + -1.0f)));
		float fSlow82 = (fSlow80 - fSlow81);
		float fSlow83 = (fSlow76 * (fSlow81 + (1.0f - fSlow80)));
		float fSlow84 = ((std::exp((fConst40 / fSlow16)) / fSlow76) + -1.0f);
		float fSlow85 = float(fVslider9);
		float fSlow86 = std::pow(10.0f, (0.0500000007f * float(fVslider10)));
		float fSlow87 = (fConst1 * (fSlow85 / std::sqrt(std::max<float>(0.0f, fSlow86))));
		float fSlow88 = ((1.0f - fSlow87) / (fSlow87 + 1.0f));
		float fSlow89 = (0.0f - (std::cos((fConst1 * fSlow85)) * (fSlow88 + 1.0f)));
		float fSlow90 = (0.0f - (std::cos((fConst1 * fSlow2)) * (fSlow5 + 1.0f)));
		for (int i = 0; (i < count); i = (i + 1)) {
			fRec0[0] = (fSlow0 + (0.999000013f * fRec0[1]));
			float fTemp0 = float(input0[i]);
			fVec0[(IOTA & 16383)] = fTemp0;
			fRec1[0] = (fSlow1 + (0.999000013f * fRec1[1]));
			float fTemp1 = (fRec1[0] + 1.0f);
			float fTemp2 = (1.0f - (0.5f * fTemp1));
			fRec14[0] = (fSlow19 * ((fRec10[1] + fRec10[2]) - (fSlow20 * fRec14[1])));
			fRec13[0] = ((fSlow14 * fRec13[1]) + (fSlow15 * (fRec10[1] + (fSlow17 * fRec14[0]))));
			fVec1[(IOTA & 32767)] = ((0.353553385f * fRec13[0]) + 9.99999968e-21f);
			float fTemp3 = float(input1[i]);
			fVec2[(IOTA & 16383)] = fTemp3;
			float fTemp4 = (0.300000012f * fVec2[((IOTA - iSlow21) & 16383)]);
			float fTemp5 = (((0.600000024f * fRec11[1]) + fVec1[((IOTA - iConst6) & 32767)]) - fTemp4);
			fVec3[(IOTA & 2047)] = fTemp5;
			fRec11[0] = fVec3[((IOTA - iConst8) & 2047)];
			float fRec12 = (0.0f - (0.600000024f * fTemp5));
			fRec18[0] = (0.0f - (fSlow19 * ((fSlow20 * fRec18[1]) - (fRec6[1] + fRec6[2]))));
			fRec17[0] = ((fSlow28 * fRec17[1]) + (fSlow29 * (fRec6[1] + (fSlow30 * fRec18[0]))));
			fVec4[(IOTA & 32767)] = ((0.353553385f * fRec17[0]) + 9.99999968e-21f);
			float fTemp6 = (((0.600000024f * fRec15[1]) + fVec4[((IOTA - iConst12) & 32767)]) - fTemp4);
			fVec5[(IOTA & 4095)] = fTemp6;
			fRec15[0] = fVec5[((IOTA - iConst13) & 4095)];
			float fRec16 = (0.0f - (0.600000024f * fTemp6));
			fRec22[0] = (0.0f - (fSlow19 * ((fSlow20 * fRec22[1]) - (fRec8[1] + fRec8[2]))));
			fRec21[0] = ((fSlow37 * fRec21[1]) + (fSlow38 * (fRec8[1] + (fSlow39 * fRec22[0]))));
			fVec6[(IOTA & 16383)] = ((0.353553385f * fRec21[0]) + 9.99999968e-21f);
			float fTemp7 = (fVec6[((IOTA - iConst17) & 16383)] + (fTemp4 + (0.600000024f * fRec19[1])));
			fVec7[(IOTA & 4095)] = fTemp7;
			fRec19[0] = fVec7[((IOTA - iConst18) & 4095)];
			float fRec20 = (0.0f - (0.600000024f * fTemp7));
			fRec26[0] = (0.0f - (fSlow19 * ((fSlow20 * fRec26[1]) - (fRec4[1] + fRec4[2]))));
			fRec25[0] = ((fSlow46 * fRec25[1]) + (fSlow47 * (fRec4[1] + (fSlow48 * fRec26[0]))));
			fVec8[(IOTA & 32767)] = ((0.353553385f * fRec25[0]) + 9.99999968e-21f);
			float fTemp8 = (fTemp4 + ((0.600000024f * fRec23[1]) + fVec8[((IOTA - iConst22) & 32767)]));
			fVec9[(IOTA & 4095)] = fTemp8;
			fRec23[0] = fVec9[((IOTA - iConst23) & 4095)];
			float fRec24 = (0.0f - (0.600000024f * fTemp8));
			fRec30[0] = (0.0f - (fSlow19 * ((fSlow20 * fRec30[1]) - (fRec9[1] + fRec9[2]))));
			fRec29[0] = ((fSlow55 * fRec29[1]) + (fSlow56 * (fRec9[1] + (fSlow57 * fRec30[0]))));
			fVec10[(IOTA & 16383)] = ((0.353553385f * fRec29[0]) + 9.99999968e-21f);
			float fTemp9 = (0.300000012f * fVec0[((IOTA - iSlow21) & 16383)]);
			float fTemp10 = (fVec10[((IOTA - iConst27) & 16383)] - (fTemp9 + (0.600000024f * fRec27[1])));
			fVec11[(IOTA & 2047)] = fTemp10;
			fRec27[0] = fVec11[((IOTA - iConst28) & 2047)];
			float fRec28 = (0.600000024f * fTemp10);
			fRec34[0] = (0.0f - (fSlow19 * ((fSlow20 * fRec34[1]) - (fRec5[1] + fRec5[2]))));
			fRec33[0] = ((fSlow64 * fRec33[1]) + (fSlow65 * (fRec5[1] + (fSlow66 * fRec34[0]))));
			fVec12[(IOTA & 16383)] = ((0.353553385f * fRec33[0]) + 9.99999968e-21f);
			float fTemp11 = (fVec12[((IOTA - iConst32) & 16383)] - (fTemp9 + (0.600000024f * fRec31[1])));
			fVec13[(IOTA & 4095)] = fTemp11;
			fRec31[0] = fVec13[((IOTA - iConst33) & 4095)];
			float fRec32 = (0.600000024f * fTemp11);
			fRec38[0] = (0.0f - (fSlow19 * ((fSlow20 * fRec38[1]) - (fRec7[1] + fRec7[2]))));
			fRec37[0] = ((fSlow73 * fRec37[1]) + (fSlow74 * (fRec7[1] + (fSlow75 * fRec38[0]))));
			fVec14[(IOTA & 16383)] = ((0.353553385f * fRec37[0]) + 9.99999968e-21f);
			float fTemp12 = ((fTemp9 + fVec14[((IOTA - iConst37) & 16383)]) - (0.600000024f * fRec35[1]));
			fVec15[(IOTA & 4095)] = fTemp12;
			fRec35[0] = fVec15[((IOTA - iConst38) & 4095)];
			float fRec36 = (0.600000024f * fTemp12);
			fRec42[0] = (0.0f - (fSlow19 * ((fSlow20 * fRec42[1]) - (fRec3[1] + fRec3[2]))));
			fRec41[0] = ((fSlow82 * fRec41[1]) + (fSlow83 * (fRec3[1] + (fSlow84 * fRec42[0]))));
			fVec16[(IOTA & 16383)] = ((0.353553385f * fRec41[0]) + 9.99999968e-21f);
			float fTemp13 = ((fVec16[((IOTA - iConst42) & 16383)] + fTemp9) - (0.600000024f * fRec39[1]));
			fVec17[(IOTA & 2047)] = fTemp13;
			fRec39[0] = fVec17[((IOTA - iConst43) & 2047)];
			float fRec40 = (0.600000024f * fTemp13);
			float fTemp14 = (fRec40 + fRec36);
			float fTemp15 = (fRec28 + (fRec32 + fTemp14));
			fRec3[0] = (fRec11[1] + (fRec15[1] + (fRec19[1] + (fRec23[1] + (fRec27[1] + (fRec31[1] + (fRec35[1] + (fRec39[1] + (fRec12 + (fRec16 + (fRec20 + (fRec24 + fTemp15))))))))))));
			fRec4[0] = ((fRec27[1] + (fRec31[1] + (fRec35[1] + (fRec39[1] + fTemp15)))) - (fRec11[1] + (fRec15[1] + (fRec19[1] + (fRec23[1] + (fRec12 + (fRec16 + (fRec24 + fRec20))))))));
			float fTemp16 = (fRec32 + fRec28);
			fRec5[0] = ((fRec19[1] + (fRec23[1] + (fRec35[1] + (fRec39[1] + (fRec20 + (fRec24 + fTemp14)))))) - (fRec11[1] + (fRec15[1] + (fRec27[1] + (fRec31[1] + (fRec12 + (fRec16 + fTemp16)))))));
			fRec6[0] = ((fRec11[1] + (fRec15[1] + (fRec35[1] + (fRec39[1] + (fRec12 + (fRec16 + fTemp14)))))) - (fRec19[1] + (fRec23[1] + (fRec27[1] + (fRec31[1] + (fRec20 + (fRec24 + fTemp16)))))));
			float fTemp17 = (fRec40 + fRec32);
			float fTemp18 = (fRec36 + fRec28);
			fRec7[0] = ((fRec15[1] + (fRec23[1] + (fRec31[1] + (fRec39[1] + (fRec16 + (fRec24 + fTemp17)))))) - (fRec11[1] + (fRec19[1] + (fRec27[1] + (fRec35[1] + (fRec12 + (fRec20 + fTemp18)))))));
			fRec8[0] = ((fRec11[1] + (fRec19[1] + (fRec31[1] + (fRec39[1] + (fRec12 + (fRec20 + fTemp17)))))) - (fRec15[1] + (fRec23[1] + (fRec27[1] + (fRec35[1] + (fRec16 + (fRec24 + fTemp18)))))));
			float fTemp19 = (fRec40 + fRec28);
			float fTemp20 = (fRec36 + fRec32);
			fRec9[0] = ((fRec11[1] + (fRec23[1] + (fRec27[1] + (fRec39[1] + (fRec12 + (fRec24 + fTemp19)))))) - (fRec15[1] + (fRec19[1] + (fRec31[1] + (fRec35[1] + (fRec16 + (fRec20 + fTemp20)))))));
			fRec10[0] = ((fRec15[1] + (fRec19[1] + (fRec27[1] + (fRec39[1] + (fRec16 + (fRec20 + fTemp19)))))) - (fRec11[1] + (fRec23[1] + (fRec31[1] + (fRec35[1] + (fRec12 + (fRec24 + fTemp20)))))));
			float fTemp21 = (0.370000005f * (fRec4[0] + fRec5[0]));
			float fTemp22 = (fSlow89 * fRec43[1]);
			fRec43[0] = (fTemp21 - (fTemp22 + (fSlow88 * fRec43[2])));
			float fTemp23 = ((fSlow88 * fRec43[0]) + (fTemp22 + fRec43[2]));
			float fTemp24 = (0.5f * ((fTemp21 + fTemp23) + (fSlow86 * (fTemp23 - fTemp21))));
			float fTemp25 = (fSlow90 * fRec2[1]);
			fRec2[0] = (fTemp24 - (fTemp25 + (fSlow5 * fRec2[2])));
			float fTemp26 = (fSlow5 * fRec2[0]);
			output0[i] = FAUSTFLOAT((0.5f * (fRec0[0] * ((fTemp0 * fTemp1) + (fTemp2 * ((fTemp26 + (fRec2[2] + (fTemp24 + fTemp25))) + (fSlow3 * ((fTemp26 + (fTemp25 + fRec2[2])) - fTemp24))))))));
			float fTemp27 = (0.370000005f * (fRec4[0] - fRec5[0]));
			float fTemp28 = (fSlow89 * fRec45[1]);
			fRec45[0] = (fTemp27 - (fTemp28 + (fSlow88 * fRec45[2])));
			float fTemp29 = ((fSlow88 * fRec45[0]) + (fTemp28 + fRec45[2]));
			float fTemp30 = (0.5f * ((fTemp27 + fTemp29) + (fSlow86 * (fTemp29 - fTemp27))));
			float fTemp31 = (fSlow90 * fRec44[1]);
			fRec44[0] = (fTemp30 - (fTemp31 + (fSlow5 * fRec44[2])));
			float fTemp32 = (fSlow5 * fRec44[0]);
			output1[i] = FAUSTFLOAT((0.5f * (fRec0[0] * ((fTemp3 * fTemp1) + (fTemp2 * ((fTemp32 + (fRec44[2] + (fTemp30 + fTemp31))) + (fSlow3 * ((fTemp32 + (fTemp31 + fRec44[2])) - fTemp30))))))));
			fRec0[1] = fRec0[0];
			IOTA = (IOTA + 1);
			fRec1[1] = fRec1[0];
			fRec14[1] = fRec14[0];
			fRec13[1] = fRec13[0];
			fRec11[1] = fRec11[0];
			fRec18[1] = fRec18[0];
			fRec17[1] = fRec17[0];
			fRec15[1] = fRec15[0];
			fRec22[1] = fRec22[0];
			fRec21[1] = fRec21[0];
			fRec19[1] = fRec19[0];
			fRec26[1] = fRec26[0];
			fRec25[1] = fRec25[0];
			fRec23[1] = fRec23[0];
			fRec30[1] = fRec30[0];
			fRec29[1] = fRec29[0];
			fRec27[1] = fRec27[0];
			fRec34[1] = fRec34[0];
			fRec33[1] = fRec33[0];
			fRec31[1] = fRec31[0];
			fRec38[1] = fRec38[0];
			fRec37[1] = fRec37[0];
			fRec35[1] = fRec35[0];
			fRec42[1] = fRec42[0];
			fRec41[1] = fRec41[0];
			fRec39[1] = fRec39[0];
			fRec3[2] = fRec3[1];
			fRec3[1] = fRec3[0];
			fRec4[2] = fRec4[1];
			fRec4[1] = fRec4[0];
			fRec5[2] = fRec5[1];
			fRec5[1] = fRec5[0];
			fRec6[2] = fRec6[1];
			fRec6[1] = fRec6[0];
			fRec7[2] = fRec7[1];
			fRec7[1] = fRec7[0];
			fRec8[2] = fRec8[1];
			fRec8[1] = fRec8[0];
			fRec9[2] = fRec9[1];
			fRec9[1] = fRec9[0];
			fRec10[2] = fRec10[1];
			fRec10[1] = fRec10[0];
			fRec43[2] = fRec43[1];
			fRec43[1] = fRec43[0];
			fRec2[2] = fRec2[1];
			fRec2[1] = fRec2[0];
			fRec45[2] = fRec45[1];
			fRec45[1] = fRec45[0];
			fRec44[2] = fRec44[1];
			fRec44[1] = fRec44[0];
		}
	}

};

#endif

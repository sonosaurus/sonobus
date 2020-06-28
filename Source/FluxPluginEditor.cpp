/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "FluxPluginProcessor.h"
#include "FluxPluginEditor.h"

#include "BeatToggleGrid.h"
#include "DebugLogC.h"

#include "PeersContainerView.h"

enum {
    PeriodicUpdateTimerId = 0
};


class FluxAoOAudioProcessorEditor::PatchMatrixView : public Component, public BeatToggleGridDelegate
{
public:
    PatchMatrixView(FluxAoOAudioProcessor& p) : Component(), processor(p) {
        
        grid = std::make_unique<BeatToggleGrid>();
        grid->setDelegate(this);
        
        addAndMakeVisible(grid.get());
        
        updateGrid();
    }
    virtual ~PatchMatrixView() {
    }
    
    bool beatToggleGridPressed(BeatToggleGrid* grid, int index, const MouseEvent & event) override { 
        int count = processor.getNumberRemotePeers();
        if (count == 0) return false;
        
        int src = index/count;
        int dest = index % count;        
        bool currval = processor.getPatchMatrixValue(src, dest);
        valonpress = currval;
        
        processor.setPatchMatrixValue(src, dest, !currval);

        updateGrid();

        return true;
    }
    
    bool beatToggleGridMoved(BeatToggleGrid* grid, int fromIndex, int toIndex, const MouseEvent & event) override {
        int count = processor.getNumberRemotePeers();
        if (count == 0) return false;
        
        int tosrc = toIndex/count;
        int todest = toIndex % count;        
        bool currval = processor.getPatchMatrixValue(tosrc, todest);
        
        processor.setPatchMatrixValue(tosrc, todest, !valonpress);
        
        updateGrid();
        return true;
    }
    
    bool beatToggleGridReleased(BeatToggleGrid* grid, int index, const MouseEvent & event) override {
        return true;
    }
    
    void updateGrid()
    {
        int count = processor.getNumberRemotePeers();
        if ( count*count != grid->getItems()) {
            updateGridLayout();
        }
        
        for (int i=0; i < count; ++i) {
            for (int j=0; j < count; ++j) {
                int item = i*count + j;
                
                grid->setSelected(processor.getPatchMatrixValue(i, j), item);
                if (i == j) {
                    grid->setAccented(processor.getPatchMatrixValue(i, j), item);                    
                }
            }            
        }
        
        grid->refreshGrid(false);
        repaint();
    }
    
    Rectangle<int> getPreferredBounds() const
    {
        int count = processor.getNumberRemotePeers();
        int labwidth = 30;
        int labheight= 18;
        int buttwidth = 60;
        int buttheight = 44;

        Rectangle<int> prefsize;
        prefsize.setX(0);
        prefsize.setY(0);
        prefsize.setWidth(count * buttwidth + labwidth);
        prefsize.setHeight(count * buttheight + labheight);

        return prefsize;
    }

    
    void updateGridLayout() {

        int count = processor.getNumberRemotePeers();
        
        /*
        if (count == 0) {
            grid->setVisible(false);
            return;
        } else {
            grid->setVisible(true);
        }
        */
        
        
        topbox.items.clear();
        topbox.flexDirection = FlexBox::Direction::row;    
        leftbox.items.clear();
        leftbox.flexDirection = FlexBox::Direction::column;    

        while (leftLabels.size() < count) {
            Label * leftlab = new Label("lab", String::formatted("%d", leftLabels.size() + 1));
            leftlab->setJustificationType(Justification::centred);
            leftLabels.add(leftlab);

            Label * toplab = new Label("lab", String::formatted("%d", topLabels.size() + 1));
            toplab->setJustificationType(Justification::centred);
            topLabels.add(toplab);

            addAndMakeVisible( leftLabels.getUnchecked(leftLabels.size()-1));
            addAndMakeVisible( topLabels.getUnchecked(topLabels.size()-1));
        }
        
        for (int i=0; i < leftLabels.size(); ++i) {
            leftLabels.getUnchecked(i)->setVisible(i < count);
            topLabels.getUnchecked(i)->setVisible(i < count);
        }
        
        int labwidth = 30;
        int labheight= 18;
        int buttheight = 36;
        
        topbox.items.add(FlexItem(labwidth, labheight));

        grid->setItems(count * count);
        grid->setSegments(count);
        
        for (int i=0; i < count; ++i) {
            grid->setSegmentSize(count, i);
        }
        
        grid->refreshGrid(true);
        
        for (int i=0; i < count; ++i) {

            for (int j=0; j < count; ++j) {
                int item = i*count + j;
                grid->setLabel(String::formatted("%d>%d", i+1, j+1), item);                
            }
            
            topbox.items.add(FlexItem(20, labheight, *topLabels.getUnchecked(i)).withMargin(2).withFlex(1));
            leftbox.items.add(FlexItem(20, labheight, *leftLabels.getUnchecked(i)).withMargin(2).withFlex(1));            
        }
        

        middlebox.items.clear();
        middlebox.flexDirection = FlexBox::Direction::row;    
        middlebox.items.add(FlexItem(labwidth, labheight, leftbox).withMargin(2).withFlex(0));
        middlebox.items.add(FlexItem(labwidth, buttheight, *grid).withMargin(2).withFlex(1));
        

        
        mainbox.items.clear();
        mainbox.flexDirection = FlexBox::Direction::column;    
        mainbox.items.add(FlexItem(labwidth*2, labheight, topbox).withMargin(2).withFlex(0).withMaxHeight(30));
        mainbox.items.add(FlexItem(labwidth*2, buttheight, middlebox).withMargin(2).withFlex(1));
        
        //grid->refreshGrid(false);
        
        resized();
    }
    
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        //g.fillAll (getLookAndFeel().findColour (ColourScheme::UIColour::widgetBackground));

        //g.setColour (Colours::white);
        //g.setFont (15.0f);
        //g.drawFittedText ("Hello World!", getLocalBounds(), Justification::centred, 1);
    }

    void resized() override
    {
        Component::resized();
        
        //updateGridLayout();
        
        mainbox.performLayout(getLocalBounds());    

    }
    
    std::unique_ptr<BeatToggleGrid> grid;
    OwnedArray<Label> leftLabels;
    OwnedArray<Label> topLabels;

    FlexBox mainbox;
    FlexBox middlebox;
    FlexBox leftbox;
    FlexBox topbox;

    
    bool valonpress = false;
    
    FluxAoOAudioProcessor & processor;
};


static void configLevelSlider(Slider * slider)
{
    //slider->setTextValueSuffix(" dB");
    slider->setRange(0.0, 1.0, 0.0);
    slider->setSkewFactor(0.25);
    slider->setDoubleClickReturnValue(true, 1.0);
    slider->setTextBoxIsEditable(false);
    slider->setSliderSnapsToMousePosition(false);
    slider->setScrollWheelEnabled(false);
    slider->valueFromTextFunction = [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); };
    slider->textFromValueFunction = [](float v) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); };
}


static void configLabel(Label *label, bool val)
{
    if (val) {
        label->setFont(12);
        label->setColour(Label::textColourId, Colour(0x90eeeeee));
        label->setJustificationType(Justification::centred);        
    }
    else {
        label->setFont(14);
        //label->setColour(Label::textColourId, Colour(0xaaeeeeee));
        label->setJustificationType(Justification::centredLeft);
    }
}


//==============================================================================
FluxAoOAudioProcessorEditor::FluxAoOAudioProcessorEditor (FluxAoOAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),  sonoSliderLNF(14)
{
    LookAndFeel::setDefaultLookAndFeel(&sonoLookAndFeel);
    
    sonoLookAndFeel.setUsingNativeAlertWindows(true);

    
    mInGainSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
    mInGainSlider->setName("ingain");
    mInGainSlider->setSliderSnapsToMousePosition(false);
    mInGainSlider->setTextBoxIsEditable(false);
    mInGainSlider->setScrollWheelEnabled(false);

    mDrySlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
    mDrySlider->setName("dry");
    mDrySlider->setSliderSnapsToMousePosition(false);
    mDrySlider->setTextBoxIsEditable(false);
    mDrySlider->setScrollWheelEnabled(false);

    mOutGainSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
    mOutGainSlider->setName("wet");
    mOutGainSlider->setSliderSnapsToMousePosition(false);
    mOutGainSlider->setTextBoxIsEditable(false);
    mOutGainSlider->setScrollWheelEnabled(false);

    //configKnobSlider(mInGainSlider.get());
    //configKnobSlider(mDrySlider.get());
    //configKnobSlider(mWetSlider.get());
    
    //mBufferTimeSlider    = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxRight); 
    //mBufferTimeSlider->setName("time");
    //mBufferTimeSlider->setSliderSnapsToMousePosition(false);
    //mBufferTimeSlider->setChangeNotificationOnlyOnRelease(true);

    mInGainLabel = std::make_unique<Label>(FluxAoOAudioProcessor::paramDry, TRANS("Input Level"));
    configLabel(mInGainLabel.get(), false);

    mDryLabel = std::make_unique<Label>(FluxAoOAudioProcessor::paramDry, TRANS("Input Monitor"));
    configLabel(mDryLabel.get(), false);

    //mWetLabel = std::make_unique<Label>(FluxAoOAudioProcessor::paramWet, TRANS("Wet"));
    //configLabel(mWetLabel.get(), false);

    mOutGainLabel = std::make_unique<Label>("outgain", TRANS("Output Level"));
    configLabel(mOutGainLabel.get(), false);

    
    mStreamingEnabledButton = std::make_unique<ToggleButton>(TRANS("Streaming"));
    
    
    mInGainAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), FluxAoOAudioProcessor::paramInGain, *mInGainSlider);
    mDryAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), FluxAoOAudioProcessor::paramDry, *mDrySlider);
    mWetAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), FluxAoOAudioProcessor::paramWet, *mOutGainSlider);
    //mBufferTimeAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), FluxAoOAudioProcessor::paramBufferTime, *mBufferTimeSlider);
    
    mStreamingEnabledAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), FluxAoOAudioProcessor::paramStreamingEnabled, *mStreamingEnabledButton);

    
    mLocalAddressLabel = std::make_unique<Label>("localaddr", TRANS("--"));
    mLocalAddressLabel->setJustificationType(Justification::centredLeft);
    mLocalAddressStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Local Address:"));
    mLocalAddressStaticLabel->setJustificationType(Justification::centredRight);


    mRemoteAddressStaticLabel = std::make_unique<Label>("remaddrst", TRANS("Host: "));
    mRemoteAddressStaticLabel->setJustificationType(Justification::centredRight);
    
    mAddRemoteHostEditor = std::make_unique<TextEditor>("remaddredit");
    mAddRemoteHostEditor->setFont(Font(16));
    mAddRemoteHostEditor->setText("100.36.128.246:11000"); // 100.36.128.246
    
    //mAddRemotePortEditor = std::make_unique<TextEditor>("remportedit");
    //mAddRemotePortEditor->setFont(Font(16));
    //mAddRemotePortEditor->setText("11000");
    
    
    mConnectButton = std::make_unique<TextButton>("addsource");
    mConnectButton->setButtonText(TRANS("Connect To"));
    mConnectButton->addListener(this);
    

    mPatchbayButton = std::make_unique<TextButton>("patch");
    mPatchbayButton->setButtonText(TRANS("Patchbay"));
    mPatchbayButton->addListener(this);

    auto flags = foleys::LevelMeter::Horizontal|foleys::LevelMeter::Minimal|foleys::LevelMeter::MaxNumber;
    
    inputMeter = std::make_unique<foleys::LevelMeter>(flags);
    inputMeter->setMeterSource (&processor.getInputMeterSource());   
    inputMeter->setLookAndFeel(&inMeterLnf);
    
    outputMeter = std::make_unique<foleys::LevelMeter>(flags);
    outputMeter->setLookAndFeel(&outMeterLnf);
    outputMeter->setMeterSource (&processor.getOutputMeterSource());        
    

    mPeerContainer = std::make_unique<PeersContainerView>(processor);
    
    mPeerViewport = std::make_unique<Viewport>();
    mPeerViewport->setViewedComponent(mPeerContainer.get(), false);
    
    
    addAndMakeVisible(mDrySlider.get());
    addAndMakeVisible(mOutGainSlider.get());
    //addAndMakeVisible(mWetLabel.get());
    addAndMakeVisible(mInGainSlider.get());
//    addAndMakeVisible(mBufferTimeSlider.get());
    addAndMakeVisible(mLocalAddressLabel.get());
    addAndMakeVisible(mLocalAddressStaticLabel.get());
    addAndMakeVisible(mConnectButton.get());
    addAndMakeVisible(mPatchbayButton.get());
    addAndMakeVisible(mAddRemoteHostEditor.get());
    //addAndMakeVisible(mAddRemotePortEditor.get());
    //addAndMakeVisible(mRemoteAddressStaticLabel.get());
    addAndMakeVisible(mInGainLabel.get());
    addAndMakeVisible(mDryLabel.get());
    addAndMakeVisible(mOutGainLabel.get());
    addAndMakeVisible(mStreamingEnabledButton.get());
    addAndMakeVisible(inputMeter.get());
    addAndMakeVisible(outputMeter.get());
    addAndMakeVisible(mPeerViewport.get());
    //addAndMakeVisible(mPeerContainer.get());
    
    updateLayout();
    
    updateState();

    
    //setResizeLimits(400, 300, 2000, 1000);

    if (JUCEApplicationBase::isStandaloneApp()) {
        setResizable(true, false);
    } else  {
        setResizable(true, true);    
    }
    

    
    startTimer(PeriodicUpdateTimerId, 500);
    
   // Make sure that before the constructor has finished, you've set the
   // editor's size to whatever you need it to be.
   setSize (600, 400);
   
}

FluxAoOAudioProcessorEditor::~FluxAoOAudioProcessorEditor()
{
}



void FluxAoOAudioProcessorEditor::configKnobSlider(Slider * slider)
{
    //slider->setVelocityBasedMode(true);
    //slider->setVelocityModeParameters(2.5, 1, 0.05);
    //slider->setTextBoxStyle(Slider::NoTextBox, true, 40, 18);
    slider->setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 60, 14);
    slider->setMouseDragSensitivity(128);
    slider->setScrollWheelEnabled(false);
    //slider->setPopupDisplayEnabled(true, false, this);
    slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
    slider->setLookAndFeel(&sonoSliderLNF);
}

//////////////////



//////////////////////////

void FluxAoOAudioProcessorEditor::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    
}




void FluxAoOAudioProcessorEditor::timerCallback(int timerid)
{
    if (timerid == PeriodicUpdateTimerId) {
        if (mPeerContainer->getPeerViewCount() != processor.getNumberRemotePeers()) {
            mPeerContainer->rebuildPeerViews();
            resized();
            
            if (patchbayCalloutBox) {
                mPatchMatrixView->updateGridLayout();
                mPatchMatrixView->updateGrid();
            }

        }
        else {
            mPeerContainer->updatePeerViews();

            if (patchbayCalloutBox) {
                mPatchMatrixView->updateGrid();
            }
        }
    }
}

void FluxAoOAudioProcessorEditor::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == mConnectButton.get()) {
        String hostport = mAddRemoteHostEditor->getText();        
        //String port = mAddRemotePortEditor->getText();
        // parse it
        StringArray toks = StringArray::fromTokens(hostport, ":", "");
        String host;
        int port = 11000;
        
        if (toks.size() >= 1) {
            host = toks[0].trim();
        }
        if (toks.size() >= 2) {
            port = toks[1].trim().getIntValue();
        }
        
        if (host.isNotEmpty() && port != 0) {
            processor.connectRemotePeer(host, port);
        }
    }
    else if (buttonThatWasClicked == mPatchbayButton.get()) {
        if (!patchbayCalloutBox) {
            showPatchbay(true);
        } else {
            showPatchbay(false);
        }        
    }
    else {
        
       
    }
}

void FluxAoOAudioProcessorEditor::showPatchbay(bool flag)
{
    if (!mPatchMatrixView) {
        mPatchMatrixView = std::make_unique<PatchMatrixView>(processor);
    }
    
    if (flag && patchbayCalloutBox == nullptr) {
        
        Viewport * wrap = new Viewport();
        
        Component* dw = this->findParentComponentOfClass<DocumentWindow>();
        
        if (!dw) {
            dw = this->findParentComponentOfClass<AudioProcessorEditor>();        
        }
        if (!dw) {
            dw = this->findParentComponentOfClass<Component>();        
        }
        if (!dw) {
            dw = this;
        }
        
        // calculate based on how many peers we have
        auto prefbounds = mPatchMatrixView->getPreferredBounds();
        const int defWidth = prefbounds.getWidth(); 
        const int defHeight = prefbounds.getHeight();
        
        wrap->setSize(jmin(defWidth + 8, dw->getWidth() - 20), jmin(defHeight + 8, dw->getHeight() - 24));
        
        //Rectangle<int> setbounds = Rectangle<int>(5, mTitleImage->getBottom() + 2, std::min(100, getLocalBounds().getWidth() - 10), 80);
        
        mPatchMatrixView->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        //wrap->addAndMakeVisible(mSettingsPanel.get());
        wrap->setViewedComponent(mPatchMatrixView.get(), false);
        
        //patchbayBox.performLayout(mPatchMatrixView->getLocalBounds());
        
        mPatchMatrixView->updateGridLayout();
        mPatchMatrixView->updateGrid();
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mPatchbayButton->getScreenBounds());
        DebugLogC("callout bounds: %s", bounds.toString().toRawUTF8());
        patchbayCalloutBox = & CallOutBox::launchAsynchronously (wrap, bounds , dw);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(patchbayCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(patchbayCalloutBox.get())) {
            box->dismiss();
            patchbayCalloutBox = nullptr;
        }
    }
}


void FluxAoOAudioProcessorEditor::sliderValueChanged (Slider* slider)
{

}

void FluxAoOAudioProcessorEditor::updateState()
{
    mLocalAddressLabel->setText(String::formatted("%s : %d", processor.getLocalIPAddress().toString().toRawUTF8(), processor.getUdpLocalPort()), dontSendNotification);
}


void FluxAoOAudioProcessorEditor::parameterChanged (const String& pname, float newValue)
{
    /*
    if (pname == YaleDAudioProcessor::paramLPFilterCutoff) {
        lastLPCutoff = newValue;
        mLPCutoffChanged = true;
        if (MessageManager::getInstance()->isThisTheMessageThread())
        {
            cancelPendingUpdate();
            updateLPFilterCutoff(lastLPCutoff);
        }
        else {
            triggerAsyncUpdate();
        }        
    }
*/
}

void FluxAoOAudioProcessorEditor::handleAsyncUpdate()
{
 
}

//==============================================================================
void FluxAoOAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    //g.setColour (Colours::white);
    //g.setFont (15.0f);
    //g.drawFittedText ("Hello World!", getLocalBounds(), Justification::centred, 1);
}

void FluxAoOAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    Component::resized();
 
    DebugLogC("RESIZED to %d %d", getWidth(), getHeight());
    
    mainBox.performLayout(getLocalBounds());    

    Rectangle<int> peersminbounds = mPeerContainer->getMinimumContentBounds();
    
    mPeerContainer->setBounds(Rectangle<int>(0, 0, std::max(peersminbounds.getWidth(), mPeerViewport->getWidth() - 10), std::max(peersminbounds.getHeight() + 5, mPeerViewport->getHeight())));
    
    
    mInGainLabel->setBounds(mInGainSlider->getBounds().removeFromLeft(mInGainSlider->getWidth()*0.5));
    mDryLabel->setBounds(mDrySlider->getBounds().removeFromLeft(mDrySlider->getWidth()*0.5));
    mOutGainLabel->setBounds(mOutGainSlider->getBounds().removeFromLeft(mOutGainSlider->getWidth()*0.5));
}


void FluxAoOAudioProcessorEditor::updateLayout()
{
    int minKnobWidth = 50;
    int minitemheight = 36;
    int setitemheight = 36;
    int minButtonWidth = 80;
    int sliderheight = 44;
    int meterheight = 32 ;
    
    inGainBox.items.clear();
    inGainBox.flexDirection = FlexBox::Direction::column;
    //inGainBox.items.add(FlexItem(minKnobWidth, 16, *mInGainLabel).withMargin(0));
    inGainBox.items.add(FlexItem(minKnobWidth, minitemheight, *mInGainSlider).withMargin(0).withFlex(1));

    dryBox.items.clear();
    dryBox.flexDirection = FlexBox::Direction::column;
    //dryBox.items.add(FlexItem(minKnobWidth, 16, *mDryLabel).withMargin(0));
    dryBox.items.add(FlexItem(minKnobWidth, minitemheight, *mDrySlider).withMargin(0).withFlex(1));

    //wetBox.items.clear();
    //wetBox.flexDirection = FlexBox::Direction::column;
    //wetBox.items.add(FlexItem(minKnobWidth, 18, *mWetLabel).withMargin(0));
    //wetBox.items.add(FlexItem(minKnobWidth, minitemheight, *mWetSlider).withMargin(0).withFlex(1));

    //bufferTimeBox.items.clear();
    //bufferTimeBox.flexDirection = FlexBox::Direction::row;
    //bufferTimeBox.items.add(FlexItem(80, 18, *mBufferTimeLabel).withMargin(2));
    //bufferTimeBox.items.add(FlexItem(200, minitemheight, *mBufferTimeSlider).withMargin(3).withFlex(1));

    titleBox.items.clear();
    titleBox.flexDirection = FlexBox::Direction::row;
    //titleBox.items.add(FlexItem(90, 20, *mTitleImage).withMargin(1).withMaxWidth(151).withFlex(1));
    titleBox.items.add(FlexItem(6, 20).withMargin(1).withFlex(0));
    titleBox.items.add(FlexItem(6, 20).withMargin(1).withFlex(1));
    titleBox.items.add(FlexItem(120, 18, *mLocalAddressStaticLabel).withMargin(2));
    titleBox.items.add(FlexItem(160, minitemheight, *mLocalAddressLabel).withMargin(2).withFlex(0));
    //titleBox.items.add(FlexItem(3, 20).withMargin(1).withFlex(0.2));
    //titleBox.items.add(FlexItem(40, 20, *mDummyVisual).withMargin(1).withFlex(4).withMaxWidth(200.0f));
    
    paramsBox.items.clear();
    paramsBox.flexDirection = FlexBox::Direction::column;
    paramsBox.items.add(FlexItem(minKnobWidth, minitemheight, inGainBox).withMargin(2).withFlex(0));
    paramsBox.items.add(FlexItem(6, 2).withMargin(1).withFlex(0));
    paramsBox.items.add(FlexItem(100, meterheight, *inputMeter).withMargin(2).withFlex(0));
    paramsBox.items.add(FlexItem(6, 2).withMargin(1).withFlex(0));
    paramsBox.items.add(FlexItem(minKnobWidth, minitemheight, dryBox).withMargin(2).withFlex(0));
    //paramsBox.items.add(FlexItem(minKnobWidth, 60, wetBox).withMargin(0).withFlex(1));


    addressBox.items.clear();
    addressBox.flexDirection = FlexBox::Direction::row;
    //addressBox.items.add(FlexItem(40, 18, *mRemoteAddressStaticLabel).withMargin(2));
    addressBox.items.add(FlexItem(180, minitemheight, *mAddRemoteHostEditor).withMargin(2).withFlex(1));
    //addressBox.items.add(FlexItem(55, minitemheight, *mAddRemotePortEditor).withMargin(2).withFlex(1));

    
    remoteBox.items.clear();
    remoteBox.flexDirection = FlexBox::Direction::column;
    remoteBox.items.add(FlexItem(minButtonWidth, minitemheight, *mConnectButton).withMargin(2).withFlex(0).withMaxWidth(150).withMinWidth(100).withAlignSelf(FlexItem::AlignSelf::center));
    remoteBox.items.add(FlexItem(180, minitemheight, addressBox).withMargin(2).withFlex(0).withMaxWidth(240).withAlignSelf(FlexItem::AlignSelf::center));
    remoteBox.items.add(FlexItem(10, 0).withFlex(1));
    //remoteBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));

    middleBox.items.clear();
    middleBox.flexDirection = FlexBox::Direction::row;
    middleBox.items.add(FlexItem(100, minitemheight*2 + meterheight + 10, paramsBox).withMargin(2).withFlex(3));
    middleBox.items.add(FlexItem(6, 0));
    middleBox.items.add(FlexItem(180, minitemheight*2, remoteBox).withMargin(2).withFlex(1));
    int midboxminheight = (minitemheight*3 + 20);
    
    toolbarBox.items.clear();
    toolbarBox.flexDirection = FlexBox::Direction::row;
    toolbarBox.items.add(FlexItem(100, minitemheight, *mOutGainSlider).withMargin(2).withFlex(1));
    toolbarBox.items.add(FlexItem(100, minitemheight, *outputMeter).withMargin(2).withFlex(1));
    toolbarBox.items.add(FlexItem(6, 6).withMargin(1).withFlex(0.2));
    toolbarBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));
    toolbarBox.items.add(FlexItem(12, 6).withMargin(0).withFlex(0));
    

    //middleBox.items.clear();
    //middleBox.flexDirection = FlexBox::Direction::row;
    //middleBox.items.add(FlexItem(minButtonWidth*2, minitemheight, remoteSourceBox).withMargin(2).withFlex(1));
    //middleBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxWidth(60));
    //middleBox.items.add(FlexItem(minButtonWidth*2, minitemheight, remoteSinkBox).withMargin(2).withFlex(1));

    
    int minheight = 0;
    
    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::column;    
    mainBox.items.add(FlexItem(120, 20, titleBox).withMargin(3).withFlex(1).withMaxHeight(30)); minheight += 22;
    mainBox.items.add(FlexItem(120, midboxminheight, middleBox).withMargin(3).withFlex(0)); minheight += midboxminheight;
    //mainBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxHeight(2.0));
    mainBox.items.add(FlexItem(120, 50, *mPeerViewport).withMargin(3).withFlex(3)); minheight += 50 + 6;
    //mainBox.items.add(FlexItem(120, 50, *mPeerContainer).withMargin(3).withFlex(3)); minheight += 50;
    //mainBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxHeight(2.0));
    mainBox.items.add(FlexItem(100, minitemheight, toolbarBox).withMargin(2).withFlex(0)); minheight += minitemheight + 4;
    mainBox.items.add(FlexItem(10, 4).withFlex(0));
        
    
    //if (getHeight() < minheight) {
    //    setSize(getWidth(), minheight);
    //}
}



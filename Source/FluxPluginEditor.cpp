/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "FluxPluginProcessor.h"
#include "FluxPluginEditor.h"

#include "BeatToggleGrid.h"

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
        
        grid->setItems(count * count);
        grid->setSegments(count);
        
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

        topbox.items.add(FlexItem(labwidth, labheight));
        
        for (int i=0; i < count; ++i) {
            grid->setSegmentSize(count, i);

            for (int j=0; j < count; ++j) {
                int item = i*count + j;
                grid->setLabel(String::formatted("%d>%d", i+1, j+1), item);                
            }
            
            topbox.items.add(FlexItem(20, labheight, *topLabels.getUnchecked(i)).withMargin(2).withFlex(1));
            leftbox.items.add(FlexItem(20, labheight, *leftLabels.getUnchecked(i)).withMargin(2).withFlex(1));            
        }
        

        middlebox.items.clear();
        middlebox.flexDirection = FlexBox::Direction::row;    
        middlebox.items.add(FlexItem(labwidth, 100, leftbox).withMargin(2).withFlex(0));
        middlebox.items.add(FlexItem(labwidth, 100, *grid).withMargin(2).withFlex(1));
        

        
        mainbox.items.clear();
        mainbox.flexDirection = FlexBox::Direction::column;    
        mainbox.items.add(FlexItem(120, labheight, topbox).withMargin(3).withFlex(0).withMaxHeight(30));
        mainbox.items.add(FlexItem(120, 100, middlebox).withMargin(3).withFlex(1));
        
        resized();
    }
    
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

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

    mDrySlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
    mDrySlider->setName("dry");
    mDrySlider->setSliderSnapsToMousePosition(false);
    mDrySlider->setTextBoxIsEditable(false);

    //mWetSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxBelow); 
    //mWetSlider->setName("wet");
    //mWetSlider->setSliderSnapsToMousePosition(false);
    
    //configKnobSlider(mInGainSlider.get());
    //configKnobSlider(mDrySlider.get());
    //configKnobSlider(mWetSlider.get());
    
    //mBufferTimeSlider    = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxRight); 
    //mBufferTimeSlider->setName("time");
    //mBufferTimeSlider->setSliderSnapsToMousePosition(false);
    //mBufferTimeSlider->setChangeNotificationOnlyOnRelease(true);

    mInGainLabel = std::make_unique<Label>(FluxAoOAudioProcessor::paramDry, TRANS("Input Gain"));
    configLabel(mInGainLabel.get(), false);

    mDryLabel = std::make_unique<Label>(FluxAoOAudioProcessor::paramDry, TRANS("Input Monitor"));
    configLabel(mDryLabel.get(), false);

    //mWetLabel = std::make_unique<Label>(FluxAoOAudioProcessor::paramWet, TRANS("Wet"));
    //configLabel(mWetLabel.get(), false);

    //mBufferTimeLabel = std::make_unique<Label>(FluxAoOAudioProcessor::paramBufferTime, TRANS("Buffer Time"));
    //configLabel(mBufferTimeLabel.get(), false);

    
    mStreamingEnabledButton = std::make_unique<ToggleButton>(TRANS("Streaming"));
    
    
    mInGainAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), FluxAoOAudioProcessor::paramInGain, *mInGainSlider);
    mDryAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), FluxAoOAudioProcessor::paramDry, *mDrySlider);
    //mWetAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), FluxAoOAudioProcessor::paramWet, *mWetSlider);
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
    mAddRemoteHostEditor->setText("100.36.128.246"); // 100.36.128.246
    
    mAddRemotePortEditor = std::make_unique<TextEditor>("remportedit");
    mAddRemotePortEditor->setFont(Font(16));
    mAddRemotePortEditor->setText("11000");
    
    
    mAddRemoteSourceButton = std::make_unique<TextButton>("addsource");
    mAddRemoteSourceButton->setButtonText(TRANS("Connect To"));
    mAddRemoteSourceButton->addListener(this);
    
    mRemoveRemoteSourceButton = std::make_unique<TextButton>("delsource");
    mRemoveRemoteSourceButton->setButtonText(TRANS("Uninvite Source"));
    mRemoveRemoteSourceButton->addListener(this);

    
    mAddRemoteSinkButton = std::make_unique<TextButton>("addsink");
    mAddRemoteSinkButton->setButtonText(TRANS("Add Dest"));
    mAddRemoteSinkButton->addListener(this);

    mRemoveRemoteSinkButton = std::make_unique<TextButton>("delsink");
    mRemoveRemoteSinkButton->setButtonText(TRANS("Remove Dest"));
    mRemoveRemoteSinkButton->addListener(this);

    mPatchbayButton = std::make_unique<TextButton>("patch");
    mPatchbayButton->setButtonText(TRANS("Patchbay"));
    mPatchbayButton->addListener(this);

    
    
    addAndMakeVisible(mDrySlider.get());
    //addAndMakeVisible(mWetSlider.get());
    //addAndMakeVisible(mWetLabel.get());
    addAndMakeVisible(mInGainSlider.get());
//    addAndMakeVisible(mBufferTimeSlider.get());
    addAndMakeVisible(mLocalAddressLabel.get());
    addAndMakeVisible(mLocalAddressStaticLabel.get());
    addAndMakeVisible(mAddRemoteSourceButton.get());
    addAndMakeVisible(mPatchbayButton.get());
    //addAndMakeVisible(mRemoveRemoteSourceButton.get());
    //addAndMakeVisible(mAddRemoteSinkButton.get());
    //addAndMakeVisible(mRemoveRemoteSinkButton.get());
    addAndMakeVisible(mAddRemoteHostEditor.get());
    addAndMakeVisible(mAddRemotePortEditor.get());
    addAndMakeVisible(mRemoteAddressStaticLabel.get());
    addAndMakeVisible(mInGainLabel.get());
    addAndMakeVisible(mDryLabel.get());
//    addAndMakeVisible(mBufferTimeLabel.get());
    addAndMakeVisible(mStreamingEnabledButton.get());
    
    updateLayout();
    
    updateState();

    setResizable(true, false);
    
    startTimer(PeriodicUpdateTimerId, 500);
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 400);
}

FluxAoOAudioProcessorEditor::~FluxAoOAudioProcessorEditor()
{
}

FluxAoOAudioProcessorEditor::PeerViewInfo * FluxAoOAudioProcessorEditor::createPeerViewInfo()
{
    /*
    struct FluxAoOAudioProcessorEditor::PeerViewInfo {
        std::unique_ptr<TextButton> activeButton;
        std::unique_ptr<Label>  levelLabel;
        std::unique_ptr<Slider> levelSlider;
        std::unique_ptr<Label>  bufferTimeLabel;
        std::unique_ptr<Slider> bufferTimeSlider;        
    };
*/
    
    PeerViewInfo * pvf = new PeerViewInfo();

    pvf->nameLabel = std::make_unique<Label>("name", "");
    pvf->nameLabel->setJustificationType(Justification::centredLeft);

    
    pvf->sendActiveButton = std::make_unique<ToggleButton>(TRANS("Send"));
    pvf->sendActiveButton->addListener(this);

    pvf->recvActiveButton = std::make_unique<ToggleButton>(TRANS("Recv"));
    pvf->recvActiveButton->addListener(this);

    pvf->statusLabel = std::make_unique<Label>("status", "");
    configLabel(pvf->statusLabel.get(), false);
    pvf->statusLabel->setJustificationType(Justification::centredLeft);

    
    pvf->levelSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    pvf->levelSlider->setName("level");
    pvf->levelSlider->setTextValueSuffix(" dB");
    pvf->levelSlider->setRange(-120.0, 6.0, 1);
    pvf->levelSlider->setSkewFactorFromMidPoint(-24.0);
    pvf->levelSlider->setDoubleClickReturnValue(true, 0.0);
    pvf->levelSlider->setTextBoxIsEditable(false);
    pvf->levelSlider->setSliderSnapsToMousePosition(false);
    pvf->levelSlider->addListener(this);
    //configKnobSlider(pvf->levelSlider.get());
    
    pvf->levelLabel = std::make_unique<Label>("level", TRANS("Level"));
    configLabel(pvf->levelLabel.get(), false);

    pvf->bufferTimeSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    pvf->bufferTimeSlider->setName("buffer");
    pvf->bufferTimeSlider->setRange(0, 1000, 1);
    pvf->bufferTimeSlider->setTextValueSuffix(" ms");
    pvf->bufferTimeSlider->setSkewFactor(0.5);
    pvf->bufferTimeSlider->setDoubleClickReturnValue(true, 20.0);
    pvf->bufferTimeSlider->setTextBoxIsEditable(false);
    pvf->bufferTimeSlider->setSliderSnapsToMousePosition(false);
    pvf->bufferTimeSlider->setChangeNotificationOnlyOnRelease(true);

    pvf->bufferTimeSlider->addListener(this);
    //configKnobSlider(pvf->bufferTimeSlider.get());

    pvf->packetsizeSlider     = std::make_unique<Slider>(Slider::IncDecButtons,  Slider::TextBoxLeft);
    pvf->packetsizeSlider->setName("buffer");
    pvf->packetsizeSlider->setRange(6, 11, 1);
    pvf->packetsizeSlider->textFromValueFunction = [](double value) { return String::formatted("%d", 1 << (int) value); };
    pvf->packetsizeSlider->setTextValueSuffix("");
    pvf->packetsizeSlider->setSkewFactor(1.0);
    pvf->packetsizeSlider->setDoubleClickReturnValue(true, 9.0);
    pvf->packetsizeSlider->setTextBoxIsEditable(false);
    pvf->packetsizeSlider->setSliderSnapsToMousePosition(false);
    pvf->packetsizeSlider->setChangeNotificationOnlyOnRelease(true);

    pvf->packetsizeSlider->addListener(this);

    
    pvf->bufferTimeLabel = std::make_unique<Label>("level", TRANS("Net Buffer"));
    configLabel(pvf->bufferTimeLabel.get(), false);

    pvf->removeButton = std::make_unique<TextButton>("X");
    pvf->removeButton->addListener(this);
    pvf->levelSlider->addListener(this);

    pvf->formatChoiceButton = std::make_unique<SonoChoiceButton>();
    pvf->formatChoiceButton->addChoiceListener(this);
    int numformats = processor.getNumberAudioCodecFormats();
    for (int i=0; i < numformats; ++i) {
        pvf->formatChoiceButton->addItem(processor.getAudioCodeFormatName(i), i+1);
    }


    
    return pvf;
}
    


void FluxAoOAudioProcessorEditor::configKnobSlider(Slider * slider)
{
    //slider->setVelocityBasedMode(true);
    //slider->setVelocityModeParameters(2.5, 1, 0.05);
    //slider->setTextBoxStyle(Slider::NoTextBox, true, 40, 18);
    slider->setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 60, 14);
    slider->setMouseDragSensitivity(128);
    //slider->setPopupDisplayEnabled(true, false, this);
    slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
    slider->setLookAndFeel(&sonoSliderLNF);
}

void FluxAoOAudioProcessorEditor::configLabel(Label *label, bool val)
{
    if (val) {
        label->setFont(12);
        label->setColour(Label::textColourId, Colour(0x90eeeeee));
        label->setJustificationType(Justification::centred);        
    }
    else {
        label->setFont(14);
        //label->setColour(Label::textColourId, Colour(0xaaeeeeee));
        label->setJustificationType(Justification::centred);
    }
}

void FluxAoOAudioProcessorEditor::rebuildPeerViews()
{
    int numpeers = processor.getNumberRemotePeers();
    
    while (mPeerViews.size() < numpeers) {
        mPeerViews.add(createPeerViewInfo());        
    }
    while (mPeerViews.size() > numpeers) {
        mPeerViews.removeLast();
    }
    
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

        addAndMakeVisible(pvf->nameLabel.get());
        addAndMakeVisible(pvf->statusLabel.get());
        addAndMakeVisible(pvf->sendActiveButton.get());
        addAndMakeVisible(pvf->recvActiveButton.get());
        addAndMakeVisible(pvf->bufferTimeLabel.get());
        addAndMakeVisible(pvf->bufferTimeSlider.get());
        addAndMakeVisible(pvf->levelSlider.get());
        addAndMakeVisible(pvf->levelLabel.get());
        addAndMakeVisible(pvf->removeButton.get());
        addAndMakeVisible(pvf->formatChoiceButton.get());
        addAndMakeVisible(pvf->packetsizeSlider.get());
    }
    
    updatePeerViews();
    updateLayout();
    resized();
}

void FluxAoOAudioProcessorEditor::updatePeerViews()
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        String hostname;
        int port = 0;
        processor.getRemotePeerAddressInfo(i, hostname, port);
        bool connected = processor.getRemotePeerConnected(i);

        pvf->nameLabel->setText(String::formatted("%s : %d", hostname.toRawUTF8(), port), dontSendNotification);
        pvf->statusLabel->setText(String::formatted(" %Ld packets", processor.getRemotePeerPacketsReceived(i)), dontSendNotification);
        
        pvf->sendActiveButton->setToggleState(processor.getRemotePeerSendActive(i) && connected, dontSendNotification);
        pvf->recvActiveButton->setToggleState(processor.getRemotePeerRecvActive(i) && connected, dontSendNotification);

        if (!pvf->levelSlider->isMouseOverOrDragging()) {
            pvf->levelSlider->setValue(processor.getRemotePeerLevelDb(i), dontSendNotification);
        }
        if (!pvf->bufferTimeSlider->isMouseOverOrDragging()) {
            pvf->bufferTimeSlider->setValue(processor.getRemotePeerBufferTime(i), dontSendNotification);
        }

        
        int formatindex = processor.getRemotePeerAudioCodecFormat(i);
        pvf->formatChoiceButton->setSelectedItemIndex(formatindex >= 0 ? formatindex : processor.getDefaultAudioCodecFormat());
        
               
        pvf->packetsizeSlider->setValue(findHighestSetBit(processor.getRemotePeerSendPacketsize(i)), dontSendNotification);
        
        //pvf->removeButton->setEnabled(connected);
        pvf->levelSlider->setEnabled(connected);
        pvf->bufferTimeSlider->setEnabled(connected);

        pvf->formatChoiceButton->setAlpha(connected ? 1.0 : 0.8);
        pvf->nameLabel->setAlpha(connected ? 1.0 : 0.8);
        pvf->sendActiveButton->setAlpha(connected ? 1.0 : 0.8);
        pvf->recvActiveButton->setAlpha(connected ? 1.0 : 0.8);
        pvf->levelLabel->setAlpha(connected ? 1.0 : 0.6);
        pvf->statusLabel->setAlpha(connected ? 1.0 : 0.6);
        pvf->levelSlider->setAlpha(connected ? 1.0 : 0.6);
        pvf->bufferTimeLabel->setAlpha(connected ? 1.0 : 0.6);
        pvf->bufferTimeSlider->setAlpha(connected ? 1.0 : 0.6);
    }
}

void FluxAoOAudioProcessorEditor::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        if (pvf->formatChoiceButton.get() == comp) {
            processor.setRemotePeerAudioCodecFormat(i, index);
            break;
        }        
    }
}


void FluxAoOAudioProcessorEditor::timerCallback(int timerid)
{
    if (timerid == PeriodicUpdateTimerId) {
        if (mPeerViews.size() != processor.getNumberRemotePeers()) {
            rebuildPeerViews();

            if (mPatchbayWindow && mPatchbayWindow->isVisible()) {
                mPatchMatrixView->updateGridLayout();
                mPatchMatrixView->updateGrid();
            }

        }
        else {
            updatePeerViews();

            if (mPatchbayWindow && mPatchbayWindow->isVisible()) {
                mPatchMatrixView->updateGrid();
            }
        }
    }
}

void FluxAoOAudioProcessorEditor::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == mAddRemoteSourceButton.get()) {
        String host = mAddRemoteHostEditor->getText();
        String port = mAddRemotePortEditor->getText();
        
        if (host.isNotEmpty() && port.getIntValue() != 0) {
            processor.connectRemotePeer(host, port.getIntValue());
        }
    }
    else if (buttonThatWasClicked == mRemoveRemoteSourceButton.get()) {
        String host = mAddRemoteHostEditor->getText();
        String port = mAddRemotePortEditor->getText();
        
        if (host.isNotEmpty() && port.getIntValue() != 0) {
            processor.unInviteRemoteSource(host, port.getIntValue(), 1);
        }
        
    }
    else if (buttonThatWasClicked == mAddRemoteSinkButton.get()) {
        String host = mAddRemoteHostEditor->getText();
        String port = mAddRemotePortEditor->getText();
        
        if (host.isNotEmpty() && port.getIntValue() != 0) {
            processor.addRemoteSink(host, port.getIntValue(), 1);
        }

    }
    else if (buttonThatWasClicked == mRemoveRemoteSinkButton.get()) {
        String host = mAddRemoteHostEditor->getText();
        String port = mAddRemotePortEditor->getText();
        
        if (host.isNotEmpty() && port.getIntValue() != 0) {
            processor.removeRemoteSink(host, port.getIntValue(), 1);
        }
        
    }
    else if (buttonThatWasClicked == mPatchbayButton.get()) {
        if (!mPatchbayWindow || !mPatchbayWindow->isVisible()) {
            showPatchbay(true);
        } else {
            showPatchbay(false);
        }        
    }
    else {
        
        for (int i=0; i < mPeerViews.size(); ++i) {
            PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
            bool connected = processor.getRemotePeerConnected(i);

            if (pvf->sendActiveButton.get() == buttonThatWasClicked) {
                if (!connected) {
                    String hostname;
                    int port = 0;
                    processor.getRemotePeerAddressInfo(i, hostname, port);
                    processor.connectRemotePeer(hostname, port);
                } else {
                    processor.setRemotePeerSendActive(i, pvf->sendActiveButton->getToggleState());             
                }
                break;
            }
            else if (pvf->recvActiveButton.get() == buttonThatWasClicked) {
                if (!connected) {
                    String hostname;
                    int port = 0;
                    processor.getRemotePeerAddressInfo(i, hostname, port);
                    processor.connectRemotePeer(hostname, port);
                } else {
                    processor.setRemotePeerRecvActive(i, pvf->recvActiveButton->getToggleState());             
                }
                break;
            }
            else if (pvf->removeButton.get() == buttonThatWasClicked) {
                if (!connected) {
                    // remove it
                    processor.removeRemotePeer(i);
                }
                else {
                    processor.disconnectRemotePeer(i);                    
                }
                break;
            }
        }

    }
}

void FluxAoOAudioProcessorEditor::showPatchbay(bool flag)
{
    if (flag) {
        
        if (!mPatchMatrixView) {
            mPatchMatrixView = std::make_unique<PatchMatrixView>(processor);
            DialogWindow::LaunchOptions options;
            options.dialogBackgroundColour = Colours::black;
            options.dialogTitle = TRANS("Patchbay");
            options.content.setNonOwned(mPatchMatrixView.get());
            options.content->setSize(200, 200); 
            mPatchbayWindow.reset(options.create());
            //mPatchbayWindow->setSize(200, 200); 
        }

        mPatchMatrixView->updateGridLayout();
        mPatchMatrixView->updateGrid();
        mPatchbayWindow->setVisible(true);        
        //mPatchbayWindow->setSize(mPatchbayButton->getWidth(), mPatchbayButton->getHeight());
    }
    else {
        if (mPatchbayWindow) {
            mPatchbayWindow->setVisible(false);
        }
    }
    
}


void FluxAoOAudioProcessorEditor::sliderValueChanged (Slider* slider)
{
   for (int i=0; i < mPeerViews.size(); ++i) {
       PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
       if (pvf->levelSlider.get() == slider) {
           processor.setRemotePeerLevelDb(i, pvf->levelSlider->getValue());   
       }
       else if (pvf->bufferTimeSlider.get() == slider) {
           processor.setRemotePeerBufferTime(i, pvf->bufferTimeSlider->getValue());   
       }
       else if (pvf->packetsizeSlider.get() == slider) {
           processor.setRemotePeerSendPacketsize(i, 1 << (int)pvf->packetsizeSlider->getValue());   
       }

   }

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
    
    mainBox.performLayout(getLocalBounds());    

}


void FluxAoOAudioProcessorEditor::updateLayout()
{
    int minKnobWidth = 50;
    int minitemheight = 36;
    int setitemheight = 36;
    int minButtonWidth = 80;

    inGainBox.items.clear();
    inGainBox.flexDirection = FlexBox::Direction::column;
    inGainBox.items.add(FlexItem(minKnobWidth, 20, *mInGainLabel).withMargin(0));
    inGainBox.items.add(FlexItem(minKnobWidth, minitemheight, *mInGainSlider).withMargin(0).withFlex(1));

    dryBox.items.clear();
    dryBox.flexDirection = FlexBox::Direction::column;
    dryBox.items.add(FlexItem(minKnobWidth, 20, *mDryLabel).withMargin(0));
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
    //titleBox.items.add(FlexItem(100, 20, *mStreamingEnabledButton).withMargin(3).withFlex(0));
    titleBox.items.add(FlexItem(6, 20).withMargin(1).withFlex(1));
    titleBox.items.add(FlexItem(120, 18, *mLocalAddressStaticLabel).withMargin(2));
    titleBox.items.add(FlexItem(160, minitemheight, *mLocalAddressLabel).withMargin(2).withFlex(1));
    //titleBox.items.add(FlexItem(3, 20).withMargin(1).withFlex(0.2));
    //titleBox.items.add(FlexItem(40, 20, *mDummyVisual).withMargin(1).withFlex(4).withMaxWidth(200.0f));
    
    paramsBox.items.clear();
    paramsBox.flexDirection = FlexBox::Direction::row;
    paramsBox.items.add(FlexItem(minKnobWidth, 55, inGainBox).withMargin(2).withFlex(1));
    paramsBox.items.add(FlexItem(8, 6).withMargin(1).withFlex(0));
    paramsBox.items.add(FlexItem(minKnobWidth, 55, dryBox).withMargin(2).withFlex(1));
    //paramsBox.items.add(FlexItem(minKnobWidth, 60, wetBox).withMargin(0).withFlex(1));


    remoteBox.items.clear();
    remoteBox.flexDirection = FlexBox::Direction::row;
    remoteBox.items.add(FlexItem(minButtonWidth, minitemheight, *mAddRemoteSourceButton).withMargin(2).withFlex(1).withMaxWidth(150));
    remoteBox.items.add(FlexItem(80, 18, *mRemoteAddressStaticLabel).withMargin(2));
    remoteBox.items.add(FlexItem(100, minitemheight, *mAddRemoteHostEditor).withMargin(2).withFlex(1));
    remoteBox.items.add(FlexItem(40, minitemheight, *mAddRemotePortEditor).withMargin(2).withFlex(1));
    remoteBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));

    //remoteSourceBox.items.clear();
    //remoteSourceBox.flexDirection = FlexBox::Direction::row;
    //remoteSourceBox.items.add(FlexItem(minButtonWidth, minitemheight, *mAddRemoteSourceButton).withMargin(2).withFlex(1).withMaxWidth(150));
    //remoteSourceBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRemoveRemoteSourceButton).withMargin(2).withFlex(1));

    //remoteSinkBox.items.clear();
    //remoteSinkBox.flexDirection = FlexBox::Direction::row;
    //remoteSinkBox.items.add(FlexItem(8, 6).withMargin(1).withFlex(1));
    //remoteSinkBox.items.add(FlexItem(minButtonWidth, minitemheight, *mAddRemoteSinkButton).withMargin(2).withFlex(1).withMaxWidth(90));
    //remoteSinkBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRemoveRemoteSinkButton).withMargin(2).withFlex(1).withMaxWidth(90));

    peersBox.items.clear();
    peersBox.flexDirection = FlexBox::Direction::column;

    int peersheight = 0;
    
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        const int ph = minitemheight*2 + 4;
        const int pw = 200;


        pvf->activebox.items.clear();
        pvf->activebox.flexDirection = FlexBox::Direction::row;
        pvf->activebox.items.add(FlexItem(80, minitemheight, *pvf->sendActiveButton).withMargin(2).withFlex(1));
        pvf->activebox.items.add(FlexItem(80, minitemheight, *pvf->recvActiveButton).withMargin(2).withFlex(1));

        pvf->namebox.items.clear();
        pvf->namebox.flexDirection = FlexBox::Direction::row;
        pvf->namebox.items.add(FlexItem(180, minitemheight, *pvf->nameLabel).withMargin(2).withFlex(0));
        pvf->namebox.items.add(FlexItem(8, 18).withMargin(0));
        pvf->namebox.items.add(FlexItem(100, minitemheight, *pvf->formatChoiceButton).withMargin(2).withFlex(0));
        pvf->namebox.items.add(FlexItem(160, minitemheight, pvf->activebox).withMargin(2).withFlex(0));
        pvf->namebox.items.add(FlexItem(10, 18).withMargin(0));
        pvf->namebox.items.add(FlexItem(120, minitemheight, *pvf->statusLabel).withMargin(2).withFlex(0));
        pvf->namebox.items.add(FlexItem(5, 18).withMargin(0).withFlex(3.0));

        
        FlexBox & box = pvf->controlsbox;
        box.items.clear();
        box.flexDirection = FlexBox::Direction::row;

        box.items.add(FlexItem(50, 18, *pvf->levelLabel).withMargin(0));
        box.items.add(FlexItem(100, minitemheight, *pvf->levelSlider).withMargin(2).withFlex(2));
        box.items.add(FlexItem(60, 18, *pvf->bufferTimeLabel).withMargin(0));
        box.items.add(FlexItem(100, minitemheight, *pvf->bufferTimeSlider).withMargin(2).withFlex(1));
        box.items.add(FlexItem(100, minitemheight, *pvf->packetsizeSlider).withMargin(2).withFlex(0));
        box.items.add(FlexItem(32, minitemheight, *pvf->removeButton).withMargin(2).withFlex(0));

        
        pvf->mainbox.items.clear();
        pvf->mainbox.flexDirection = FlexBox::Direction::column;
        pvf->mainbox.items.add(FlexItem(160, minitemheight, pvf->namebox).withMargin(2).withFlex(0));
        pvf->mainbox.items.add(FlexItem(160, minitemheight, pvf->controlsbox).withMargin(2).withFlex(0));

        
        peersBox.items.add(FlexItem(pw, ph, pvf->mainbox).withMargin(3).withFlex(0));
        peersheight += ph + 6;
    }


    //middleBox.items.clear();
    //middleBox.flexDirection = FlexBox::Direction::row;
    //middleBox.items.add(FlexItem(minButtonWidth*2, minitemheight, remoteSourceBox).withMargin(2).withFlex(1));
    //middleBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxWidth(60));
    //middleBox.items.add(FlexItem(minButtonWidth*2, minitemheight, remoteSinkBox).withMargin(2).withFlex(1));

    
    int minheight = 0;
    
    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::column;    
    mainBox.items.add(FlexItem(120, 20, titleBox).withMargin(3).withFlex(1).withMaxHeight(30)); minheight += 22;
    mainBox.items.add(FlexItem(120, 65, paramsBox).withMargin(3).withFlex(0).withMaxHeight(120)); minheight += 66;
    //mainBox.items.add(FlexItem(100, minitemheight, bufferTimeBox).withMargin(3).withFlex(0)); minheight += minitemheight + 6;
    mainBox.items.add(FlexItem(120, minitemheight, remoteBox).withMargin(3).withFlex(0)); minheight += 46;
    //mainBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxHeight(2.0));
    //mainBox.items.add(FlexItem(minButtonWidth*4, minitemheight, middleBox).withMargin(3).withFlex(1).withMaxHeight(52)); minheight += minitemheight + 6;
    mainBox.items.add(FlexItem(120, peersheight, peersBox).withMargin(3).withFlex(1)); minheight += peersheight + 6;
    mainBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxHeight(2.0));
    mainBox.items.add(FlexItem(100, 3).withMargin(0).withFlex(0.1)); minheight += 3;
        
    setResizeLimits(300, minheight, 1000, 1000);
    
    if (getHeight() < minheight) {
        setSize(getWidth(), minheight);
    }
}



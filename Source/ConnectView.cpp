// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell

#include "ConnectView.h"

#include "RandomSentenceGenerator.h"

using namespace SonoAudio;

class SonobusConnectTabbedComponent : public TabbedComponent
{
public:
    SonobusConnectTabbedComponent(TabbedButtonBar::Orientation orientation, ConnectView & editor_) : TabbedComponent(orientation), editor(editor_) {

    }

    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName) override {

        editor.connectTabChanged(newCurrentTabIndex);
    }

protected:
    ConnectView & editor;

};


enum {
    nameTextColourId = 0x1002830,
    selectedColourId = 0x1002840,
    separatorColourId = 0x1002850,
};


ConnectView::ConnectView(SonobusAudioProcessor& proc, AooServerConnectionInfo & info)
: Component(), processor(proc), currConnectionInfo(info),
recentsListModel(this),
recentsGroupFont (17.0, Font::bold), recentsNameFont(15, Font::plain), recentsInfoFont(13, Font::plain),
publicGroupsListModel(this)
{
    setColour (nameTextColourId, Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.9f));
    setColour (selectedColourId, Colour::fromFloatRGBA(0.0f, 0.4f, 0.8f, 0.5f));
    setColour (separatorColourId, Colour::fromFloatRGBA(0.3f, 0.3f, 0.3f, 0.3f));

    mConnectTab = std::make_unique<SonobusConnectTabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop, *this);
    mConnectTab->setOutline(0);
    mConnectTab->setTabBarDepth(36);
    mConnectTab->getTabbedButtonBar().setMinimumTabScaleFactor(0.1f);
    mConnectTab->getTabbedButtonBar().setColour(TabbedButtonBar::frontTextColourId, Colour::fromFloatRGBA(0.4, 0.8, 1.0, 1.0));
    mConnectTab->getTabbedButtonBar().setColour(TabbedButtonBar::frontOutlineColourId, Colour::fromFloatRGBA(0.4, 0.8, 1.0, 0.5));

    mDirectConnectContainer = std::make_unique<Component>();
    mServerConnectContainer = std::make_unique<Component>();
    mPublicServerConnectContainer = std::make_unique<Component>();
    mServerConnectViewport = std::make_unique<Viewport>();
    mPublicServerConnectViewport = std::make_unique<Viewport>();
    mRecentsContainer = std::make_unique<Component>();

    mServerConnectViewport->setViewedComponent(mServerConnectContainer.get());



    mRecentsGroup = std::make_unique<GroupComponent>("", TRANS("RECENTS"));
    mRecentsGroup->setColour(GroupComponent::textColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.8));
    mRecentsGroup->setColour(GroupComponent::outlineColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.1));
    mRecentsGroup->setTextLabelPosition(Justification::centred);

    mConnectTab->addTab(TRANS("RECENTS"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mRecentsContainer.get(), false);
    mConnectTab->addTab(TRANS("PRIVATE GROUP"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mServerConnectViewport.get(), false);
    mConnectTab->addTab(TRANS("PUBLIC GROUPS"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mPublicServerConnectContainer.get(), false);
    //mConnectTab->addTab(TRANS("DIRECT"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mDirectConnectContainer.get(), false);




    mLocalAddressLabel = std::make_unique<TextEditor>("localaddr");
    mLocalAddressLabel->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
    mLocalAddressLabel->setTitle(TRANS("Local Address:Port"));
    mLocalAddressLabel->setReadOnly(true);
    mLocalAddressLabel->setWantsKeyboardFocus(true);
    //mLocalAddressLabel->setJustificationType(Justification::centredLeft);
    mLocalAddressStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Local Address:"));
    mLocalAddressStaticLabel->setJustificationType(Justification::centredRight);


    mRemoteAddressStaticLabel = std::make_unique<Label>("remaddrst", TRANS("Host: "));
    mRemoteAddressStaticLabel->setJustificationType(Justification::centredRight);

    mDirectConnectDescriptionLabel = std::make_unique<Label>("dirconndesc", TRANS("Connect directly to other instances of SonoBus on your local network with the local address that they advertise. This is experimental, using a private group is recommended instead, and works fine on local networks."));
    mDirectConnectDescriptionLabel->setJustificationType(Justification::topLeft);

    mAddRemoteHostEditor = std::make_unique<TextEditor>("remaddredit");
    mAddRemoteHostEditor->setTitle(TRANS("Remote Host:Port"));
    mAddRemoteHostEditor->setFont(Font(16));
    mAddRemoteHostEditor->setText("", false); // 100.36.128.246:11000
    mAddRemoteHostEditor->setTextToShowWhenEmpty(TRANS("IPaddress:port"), Colour(0x44ffffff));


    mConnectTitle = std::make_unique<Label>("conntime", TRANS("Connect"));
    mConnectTitle->setJustificationType(Justification::centred);
    mConnectTitle->setFont(Font(20, Font::bold));
    mConnectTitle->setColour(Label::textColourId, Colour(0x66ffffff));

    mConnectComponentBg = std::make_unique<DrawableRectangle>();
    mConnectComponentBg->setFill (Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0));

    mConnectCloseButton = std::make_unique<SonoDrawableButton>("x", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> ximg(Drawable::createFromImageData(BinaryData::x_icon_svg, BinaryData::x_icon_svgSize));
    mConnectCloseButton->setTitle(TRANS("Close"));
    mConnectCloseButton->setImages(ximg.get());
    mConnectCloseButton->addListener(this);
    mConnectCloseButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);

    mConnectMenuButton = std::make_unique<SonoDrawableButton>("menu", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> dotimg(Drawable::createFromImageData(BinaryData::dots_icon_png, BinaryData::dots_icon_pngSize));
    mConnectMenuButton->setTitle(TRANS("Menu"));
    mConnectMenuButton->setImages(dotimg.get());
    mConnectMenuButton->addListener(this);
    mConnectMenuButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mConnectMenuButton->setAlpha(0.7f);

    mDirectConnectButton = std::make_unique<TextButton>("directconnect");
    mDirectConnectButton->setButtonText(TRANS("Direct Connect"));
    mDirectConnectButton->addListener(this);
    mDirectConnectButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.6));
    mDirectConnectButton->setWantsKeyboardFocus(true);


    mServerConnectButton = std::make_unique<TextButton>("serverconnect");
    mServerConnectButton->setButtonText(TRANS("Connect to Group"));
    mServerConnectButton->addListener(this);
    mServerConnectButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.6));
    mServerConnectButton->setWantsKeyboardFocus(true);

    mServerHostEditor = std::make_unique<TextEditor>("srvaddredit");
    mServerHostEditor->setTitle(TRANS("Connection Server"));
    mServerHostEditor->setFont(Font(14));
    configEditor(mServerHostEditor.get());

    mServerUsernameEditor = std::make_unique<TextEditor>("srvaddredit");
    mServerUsernameEditor->setTitle(TRANS("Your Displayed Name:"));
    mServerUsernameEditor->setFont(Font(16));
    mServerUsernameEditor->setText(processor.getCurrentUsername(), false);
    configEditor(mServerUsernameEditor.get());

    mServerUserPasswordEditor = std::make_unique<TextEditor>("userpass"); // 0x25cf
    mServerUserPasswordEditor->setFont(Font(14));
    mServerUserPasswordEditor->setTextToShowWhenEmpty(TRANS("optional"), Colour(0x44ffffff));
    configEditor(mServerUserPasswordEditor.get());


    mServerUserStaticLabel = std::make_unique<Label>("serveruserst", TRANS("Your Displayed Name:"));
    configServerLabel(mServerUserStaticLabel.get());
    mServerUserStaticLabel->setMinimumHorizontalScale(0.8);

    mServerUserPassStaticLabel = std::make_unique<Label>("serveruserpassst", TRANS("Password:"));
    configServerLabel(mServerUserPassStaticLabel.get());

    mServerGroupStaticLabel = std::make_unique<Label>("servergroupst", TRANS("Group Name:"));
    configServerLabel(mServerGroupStaticLabel.get());
    mServerGroupStaticLabel->setMinimumHorizontalScale(0.8);

    mServerGroupPassStaticLabel = std::make_unique<Label>("servergrouppassst", TRANS("Password:"));
    configServerLabel(mServerGroupPassStaticLabel.get());

    mServerHostStaticLabel = std::make_unique<Label>("serverhostst", TRANS("Connection Server:"));
    configServerLabel(mServerHostStaticLabel.get());


    mServerGroupEditor = std::make_unique<TextEditor>("groupedit");
    mServerGroupEditor->setTitle(TRANS("Group Name:"));
    mServerGroupEditor->setFont(Font(16));
    mServerGroupEditor->setText(!currConnectionInfo.groupIsPublic ? currConnectionInfo.groupName : "", false);
    configEditor(mServerGroupEditor.get());

    mServerGroupPasswordEditor = std::make_unique<TextEditor>("grouppass"); // 0x25cf
    mServerGroupPasswordEditor->setTitle(TRANS("Optional Group Password"));
    mServerGroupPasswordEditor->setFont(Font(14));
    mServerGroupPasswordEditor->setTextToShowWhenEmpty(TRANS("optional"), Colour(0x44ffffff));
    mServerGroupPasswordEditor->setText(currConnectionInfo.groupPassword, false);
    configEditor(mServerGroupPasswordEditor.get());

    mServerGroupRandomButton = std::make_unique<SonoDrawableButton>("randgroup", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> randimg(Drawable::createFromImageData(BinaryData::dice_icon_128_png, BinaryData::dice_icon_128_pngSize));
    mServerGroupRandomButton->setTitle(TRANS("Randomize Group Name"));
    mServerGroupRandomButton->setImages(randimg.get());
    mServerGroupRandomButton->addListener(this);
    mServerGroupRandomButton->setTooltip(TRANS("Generate a random group name"));

    mServerCopyButton = std::make_unique<SonoDrawableButton>("copy", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> copyimg(Drawable::createFromImageData(BinaryData::copy_icon_svg, BinaryData::copy_icon_svgSize));
    mServerCopyButton->setTitle(TRANS("Copy Share Link"));
    mServerCopyButton->setImages(copyimg.get());
    mServerCopyButton->addListener(this);
    mServerCopyButton->setTooltip(TRANS("Copy connection information to the clipboard to share"));

    mServerPasteButton = std::make_unique<SonoDrawableButton>("paste", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> pasteimg(Drawable::createFromImageData(BinaryData::paste_icon_svg, BinaryData::paste_icon_svgSize));
    mServerPasteButton->setTitle(TRANS("Paste Share Link"));
    mServerPasteButton->setImages(pasteimg.get());
    mServerPasteButton->addListener(this);
    mServerPasteButton->setTooltip(TRANS("Paste connection information from the clipboard"));

    mServerShareButton = std::make_unique<SonoDrawableButton>("share", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> shareimg(Drawable::createFromImageData(BinaryData::copy_icon_svg, BinaryData::copy_icon_svgSize));
    mServerShareButton->setImages(shareimg.get());
    mServerShareButton->addListener(this);


    mServerStatusLabel = std::make_unique<Label>("servstat", "");
    mServerStatusLabel->setJustificationType(Justification::centred);
    mServerStatusLabel->setFont(16);
    mServerStatusLabel->setColour(Label::textColourId, Colour(0x99ffaaaa));
    mServerStatusLabel->setMinimumHorizontalScale(0.75);

    mServerInfoLabel = std::make_unique<Label>("servinfo", "");
    mServerInfoLabel->setJustificationType(Justification::centred);
    mServerInfoLabel->setFont(16);
    mServerInfoLabel->setColour(Label::textColourId, Colour(0x99dddddd));
    mServerInfoLabel->setMinimumHorizontalScale(0.85);

    String servaudioinfo = TRANS("The connection server is only used to help users find each other, no audio passes through it. All audio is sent directly between users (peer to peer).");
    mServerAudioInfoLabel = std::make_unique<Label>("servaudioinfo", servaudioinfo);
    mServerAudioInfoLabel->setJustificationType(Justification::centredTop);
    mServerAudioInfoLabel->setFont(14);
    mServerAudioInfoLabel->setColour(Label::textColourId, Colour(0x99aaaaaa));
    mServerAudioInfoLabel->setMinimumHorizontalScale(0.75);

    mMainStatusLabel = std::make_unique<Label>("mainstat", "");
    mMainStatusLabel->setJustificationType(Justification::centredRight);
    mMainStatusLabel->setFont(13);
    mMainStatusLabel->setColour(Label::textColourId, Colour(0x66ffffff));


    mClearRecentsButton = std::make_unique<SonoTextButton>("clearrecent");
    mClearRecentsButton->setButtonText(TRANS("Clear All"));
    mClearRecentsButton->addListener(this);
    mClearRecentsButton->setTextJustification(Justification::centred);

    mRecentsListBox = std::make_unique<ListBox>("recentslist");
    mRecentsListBox->setColour (ListBox::outlineColourId, Colour::fromFloatRGBA(0.7, 0.7, 0.7, 0.0));
    mRecentsListBox->setColour (ListBox::backgroundColourId, Colour::fromFloatRGBA(0.1, 0.12, 0.1, 0.0f));
    mRecentsListBox->setColour (ListBox::textColourId, Colours::whitesmoke.withAlpha(0.8f));
    mRecentsListBox->setTitle(TRANS("Recents List"));
    mRecentsListBox->setOutlineThickness (1);
#if JUCE_IOS || JUCE_ANDROID
    mRecentsListBox->getViewport()->setScrollOnDragEnabled(true);
#endif
    mRecentsListBox->getViewport()->setScrollBarsShown(true, false);
    mRecentsListBox->setMultipleSelectionEnabled (false);
    mRecentsListBox->setRowHeight(50);
    mRecentsListBox->setModel (&recentsListModel);
    mRecentsListBox->setRowSelectedOnMouseDown(true);
    mRecentsListBox->setRowClickedOnMouseDown(false);


    mPublicGroupsListBox = std::make_unique<ListBox>("publicgroupslist");
    mPublicGroupsListBox->setColour (ListBox::outlineColourId, Colour::fromFloatRGBA(0.7, 0.7, 0.7, 0.0));
    mPublicGroupsListBox->setColour (ListBox::backgroundColourId, Colour::fromFloatRGBA(0.1, 0.12, 0.1, 0.0f));
    mPublicGroupsListBox->setColour (ListBox::textColourId, Colours::whitesmoke.withAlpha(0.8f));
    mPublicGroupsListBox->setTitle(TRANS("Public Groups List"));
    mPublicGroupsListBox->setOutlineThickness (1);
#if JUCE_IOS || JUCE_ANDROID
    mPublicGroupsListBox->getViewport()->setScrollOnDragEnabled(true);
#endif
    mPublicGroupsListBox->getViewport()->setScrollBarsShown(true, false);
    mPublicGroupsListBox->setMultipleSelectionEnabled (false);
    mPublicGroupsListBox->setRowHeight(38);
    mPublicGroupsListBox->setModel (&publicGroupsListModel);
    mPublicGroupsListBox->setRowSelectedOnMouseDown(true);
    mPublicGroupsListBox->setRowClickedOnMouseDown(false);


    mPublicServerHostEditor = std::make_unique<TextEditor>("pubsrvaddredit");
    mPublicServerHostEditor->setTitle(TRANS("Connection Server:"));
    mPublicServerHostEditor->setFont(Font(14));
    configEditor(mPublicServerHostEditor.get());
    mPublicServerHostEditor->setTooltip(servaudioinfo);

    mPublicServerHostStaticLabel = std::make_unique<Label>("pubaddrst", TRANS("Connection Server:"));
    configServerLabel(mPublicServerHostStaticLabel.get());

    mPublicServerUserStaticLabel = std::make_unique<Label>("pubuserst", TRANS("Your Displayed Name:"));
    configServerLabel(mPublicServerUserStaticLabel.get());
    mPublicServerUserStaticLabel->setMinimumHorizontalScale(0.8);

    mPublicServerStatusInfoLabel = std::make_unique<Label>("pubsrvinfo", "");
    configServerLabel(mPublicServerStatusInfoLabel.get());
    mPublicServerStatusInfoLabel->setJustificationType(Justification::centredLeft);

    mPublicServerUsernameEditor = std::make_unique<TextEditor>("pubsrvaddredit");
    mPublicServerUsernameEditor->setTitle(TRANS("Your Displayed Name:"));
    mPublicServerUsernameEditor->setFont(Font(16));
    mPublicServerUsernameEditor->setText(processor.getCurrentUsername(), false);
    configEditor(mPublicServerUsernameEditor.get());

    mPublicGroupComponent = std::make_unique<GroupComponent>("", TRANS("Active Public Groups"));
    mPublicGroupComponent->setColour(GroupComponent::textColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.8));
    mPublicGroupComponent->setColour(GroupComponent::outlineColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.1));
    mPublicGroupComponent->setTextLabelPosition(Justification::centred);

    mPublicServerInfoStaticLabel = std::make_unique<Label>("pubinfost", TRANS("Select existing group below OR "));
    configServerLabel(mPublicServerInfoStaticLabel.get());
    mPublicServerInfoStaticLabel->setFont(16);
    mPublicServerInfoStaticLabel->setMinimumHorizontalScale(0.8);

    mPublicServerAddGroupButton = std::make_unique<TextButton>("addgroup");
    mPublicServerAddGroupButton->setButtonText(TRANS("Create Group..."));
    mPublicServerAddGroupButton->addListener(this);
    mPublicServerAddGroupButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.6));
    mPublicServerAddGroupButton->setWantsKeyboardFocus(true);

    mPublicServerGroupEditor = std::make_unique<TextEditor>("pubgroupedit");
    mPublicServerGroupEditor->setTitle(TRANS("Public Group Name"));
    mPublicServerGroupEditor->setFont(Font(16));
    mPublicServerGroupEditor->setText(currConnectionInfo.groupIsPublic ? currConnectionInfo.groupName : "", false);
    configEditor(mPublicServerGroupEditor.get());
    mPublicServerGroupEditor->setTextToShowWhenEmpty(TRANS("enter group name"), Colour(0x44ffffff));
    mPublicServerGroupEditor->setTooltip(TRANS("Choose a descriptive group name that includes geographic information and genre"));


    // parenting
    mDirectConnectContainer->addAndMakeVisible(mDirectConnectButton.get());
    mDirectConnectContainer->addAndMakeVisible(mAddRemoteHostEditor.get());
    mDirectConnectContainer->addAndMakeVisible(mRemoteAddressStaticLabel.get());
    mDirectConnectContainer->addAndMakeVisible(mDirectConnectDescriptionLabel.get());
    mDirectConnectContainer->addAndMakeVisible(mLocalAddressLabel.get());
    mDirectConnectContainer->addAndMakeVisible(mLocalAddressStaticLabel.get());

    mServerConnectContainer->addAndMakeVisible(mServerConnectButton.get());
    mServerConnectContainer->addAndMakeVisible(mServerHostEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerUsernameEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerUserStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupRandomButton.get());
#if ! (JUCE_IOS || JUCE_ANDROID)
    mServerConnectContainer->addAndMakeVisible(mServerPasteButton.get());
    mServerConnectContainer->addAndMakeVisible(mServerCopyButton.get());
#else
    mServerConnectContainer->addAndMakeVisible(mServerShareButton.get());
#endif
    mServerConnectContainer->addAndMakeVisible(mServerGroupStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerHostStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupPassStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupPasswordEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerStatusLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerInfoLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerAudioInfoLabel.get());

    mRecentsContainer->addAndMakeVisible(mRecentsListBox.get());
    mRecentsContainer->addAndMakeVisible(mClearRecentsButton.get());

    mPublicServerConnectContainer->addAndMakeVisible(mPublicGroupComponent.get());
    mPublicGroupComponent->addAndMakeVisible(mPublicGroupsListBox.get());

    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerHostEditor.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerHostStaticLabel.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerUserStaticLabel.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerUsernameEditor.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerAddGroupButton.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerInfoStaticLabel.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerStatusInfoLabel.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerGroupEditor.get());


    addAndMakeVisible(mConnectComponentBg.get());
    addAndMakeVisible(mConnectTab.get());
    addAndMakeVisible(mConnectTitle.get());
    addAndMakeVisible(mConnectCloseButton.get());
    addAndMakeVisible(mConnectMenuButton.get());


    std::istringstream gramstream(std::string(BinaryData::wordmaker_g, BinaryData::wordmaker_gSize));
    mRandomSentence = std::make_unique<RandomSentenceGenerator>(gramstream);
    mRandomSentence->capEveryWord = true;

    setFocusContainerType(FocusContainerType::keyboardFocusContainer);
    mConnectTab->setFocusContainerType(FocusContainerType::none);
    mConnectTab->getTabbedButtonBar().setFocusContainerType(FocusContainerType::none);
    mConnectTab->getTabbedButtonBar().setWantsKeyboardFocus(true);
    mConnectTab->setWantsKeyboardFocus(true);
    for (int i=0; i < mConnectTab->getTabbedButtonBar().getNumTabs(); ++i) {
        mConnectTab->getTabbedButtonBar().getTabButton(i)->setWantsKeyboardFocus(true);
    }


    updateLayout();
}

ConnectView::~ConnectView() {}

juce::Rectangle<int> ConnectView::getMinimumContentBounds() const {
    int defWidth = 200;
    int defHeight = 100;
    return Rectangle<int>(0,0,defWidth,defHeight);
}

void ConnectView::grabInitialFocus()
{
    if (auto * butt = mConnectTab->getTabbedButtonBar().getTabButton(mConnectTab->getCurrentTabIndex())) {
        butt->setWantsKeyboardFocus(true);
        butt->grabKeyboardFocus();
    }
}

void ConnectView::escapePressed()
{
    if (!mServerGroupEditor->hasKeyboardFocus(false)
        && !mServerHostEditor->hasKeyboardFocus(false)
        && !mServerUsernameEditor->hasKeyboardFocus(false)
        && !mServerUserPasswordEditor->hasKeyboardFocus(false)
        && !mServerGroupPasswordEditor->hasKeyboardFocus(false)
        && !mPublicServerHostEditor->hasKeyboardFocus(false)
        && !mPublicServerGroupEditor->hasKeyboardFocus(false)
        && !mPublicServerUsernameEditor->hasKeyboardFocus(false)
        )
    {
        // close us down
        setVisible(false);
    }
}



void ConnectView::timerCallback(int timerid)
{

}

void ConnectView::configServerLabel(Label *label)
{
    label->setFont(14);
    label->setColour(Label::textColourId, Colour(0x90eeeeee));
    label->setJustificationType(Justification::centredRight);
}

void ConnectView::configEditor(TextEditor *editor, bool passwd)
{
    editor->addListener(this);
    if (passwd)  {
        editor->setIndents(8, 6);
    } else {
        editor->setIndents(8, 8);
    }
}

void ConnectView::updateState()
{
    String locstr;
    locstr << processor.getLocalIPAddress().toString() << ":" << processor.getUdpLocalPort();
    mLocalAddressLabel->setText(locstr, dontSendNotification);

    resetPrivateGroupLabels();
    updateServerFieldsFromConnectionInfo();

    updateRecents();

    if (firstTimeConnectShow) {
        if (mConnectTab->getNumTabs() > 2) {
            if (recentsListModel.getNumRows() > 0) {
                // show recents tab first
                mConnectTab->setCurrentTabIndex(0);
            } else {
                mConnectTab->setCurrentTabIndex(1);
            }
        }
        firstTimeConnectShow = false;
    }

    if (mConnectTab->getCurrentContentComponent() == mPublicServerConnectContainer.get()) {
        publicGroupLogin();
    }
}


void ConnectView::updateLayout()
{
    int minKnobWidth = 50;
    int minSliderWidth = 50;
    int minPannerWidth = 40;
    int maxPannerWidth = 100;
    int minitemheight = 36;
    int knobitemheight = 80;
    int minpassheight = 30;
    int setitemheight = 36;
    int minButtonWidth = 90;
    int sliderheight = 44;
    int inmeterwidth = 22 ;
    int outmeterwidth = 22 ;
    int servLabelWidth = 72;
    int iconheight = 24;
    int iconwidth = iconheight;
    int knoblabelheight = 18;
    int panbuttwidth = 26;

#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 44;
    knobitemheight = 90;
    minpassheight = 38;
    panbuttwidth = 32;
#endif

    localAddressBox.items.clear();
    localAddressBox.flexDirection = FlexBox::Direction::row;
    localAddressBox.items.add(FlexItem(60, 18, *mLocalAddressStaticLabel).withMargin(2).withFlex(1).withMaxWidth(110));
    localAddressBox.items.add(FlexItem(100, minitemheight, *mLocalAddressLabel).withMargin(2).withFlex(1).withMaxWidth(160));


    addressBox.items.clear();
    addressBox.flexDirection = FlexBox::Direction::row;
    addressBox.items.add(FlexItem(servLabelWidth, 18, *mRemoteAddressStaticLabel).withMargin(2).withFlex(0.25));
    addressBox.items.add(FlexItem(172, minitemheight, *mAddRemoteHostEditor).withMargin(2).withFlex(1));


    remoteBox.items.clear();
    remoteBox.flexDirection = FlexBox::Direction::column;
    remoteBox.items.add(FlexItem(5, 8).withFlex(0));
    remoteBox.items.add(FlexItem(180, minitemheight, addressBox).withMargin(2).withFlex(0));
    remoteBox.items.add(FlexItem(minButtonWidth, minitemheight, *mDirectConnectButton).withMargin(8).withFlex(0).withMinWidth(100));
    remoteBox.items.add(FlexItem(180, 2*minitemheight, *mDirectConnectDescriptionLabel).withMargin(2).withFlex(1).withMaxHeight(150));
    remoteBox.items.add(FlexItem(60, minitemheight, localAddressBox).withMargin(2).withFlex(0));
    remoteBox.items.add(FlexItem(10, 0).withFlex(1));

    servAddressBox.items.clear();
    servAddressBox.flexDirection = FlexBox::Direction::row;
    servAddressBox.items.add(FlexItem(servLabelWidth, minitemheight, *mServerHostStaticLabel).withMargin(2).withFlex(1));
    servAddressBox.items.add(FlexItem(172, minpassheight, *mServerHostEditor).withMargin(2).withFlex(1));

    servUserBox.items.clear();
    servUserBox.flexDirection = FlexBox::Direction::row;
    servUserBox.items.add(FlexItem(servLabelWidth, minitemheight, *mServerUserStaticLabel).withMargin(2).withFlex(0.25));
    servUserBox.items.add(FlexItem(120, minitemheight, *mServerUsernameEditor).withMargin(2).withFlex(1));

    servUserPassBox.items.clear();
    servUserPassBox.flexDirection = FlexBox::Direction::row;
    servUserPassBox.items.add(FlexItem(servLabelWidth, minpassheight, *mServerUserPassStaticLabel).withMargin(2).withFlex(1));
    servUserPassBox.items.add(FlexItem(90, minpassheight, *mServerUserPasswordEditor).withMargin(2).withFlex(1));

    servGroupBox.items.clear();
    servGroupBox.flexDirection = FlexBox::Direction::row;
    servGroupBox.items.add(FlexItem(servLabelWidth, minitemheight, *mServerGroupStaticLabel).withMargin(2).withFlex(0.25));
    servGroupBox.items.add(FlexItem(120, minitemheight, *mServerGroupEditor).withMargin(2).withFlex(1));
    servGroupBox.items.add(FlexItem(minPannerWidth, minitemheight, *mServerGroupRandomButton).withMargin(2).withFlex(0));

    servGroupPassBox.items.clear();
    servGroupPassBox.flexDirection = FlexBox::Direction::row;
    servGroupPassBox.items.add(FlexItem(servLabelWidth, minpassheight, *mServerGroupPassStaticLabel).withMargin(2).withFlex(1));
    servGroupPassBox.items.add(FlexItem(90, minpassheight, *mServerGroupPasswordEditor).withMargin(2).withFlex(1));

    servStatusBox.items.clear();
    servStatusBox.flexDirection = FlexBox::Direction::row;
#if !(JUCE_IOS || JUCE_ANDROID)
    servStatusBox.items.add(FlexItem(minPannerWidth, minitemheight, *mServerPasteButton).withMargin(2).withFlex(0).withMaxHeight(minitemheight));
#endif
    servStatusBox.items.add(FlexItem(servLabelWidth, minpassheight, *mServerStatusLabel).withMargin(2).withFlex(1));
#if !(JUCE_IOS || JUCE_ANDROID)
    servStatusBox.items.add(FlexItem(minPannerWidth, minitemheight, *mServerCopyButton).withMargin(2).withFlex(0).withMaxHeight(minitemheight));
#else
    servStatusBox.items.add(FlexItem(minPannerWidth, minitemheight, *mServerShareButton).withMargin(2).withFlex(0).withMaxHeight(minitemheight));
#endif

    servButtonBox.items.clear();
    servButtonBox.flexDirection = FlexBox::Direction::row;
    servButtonBox.items.add(FlexItem(5, 3).withFlex(0.1));
    servButtonBox.items.add(FlexItem(minButtonWidth, minitemheight, *mServerConnectButton).withMargin(2).withFlex(1).withMaxWidth(300));
    servButtonBox.items.add(FlexItem(5, 3).withFlex(0.1));



    int maxservboxwidth = 400;

    serverBox.items.clear();
    serverBox.flexDirection = FlexBox::Direction::column;
    serverBox.items.add(FlexItem(5, 3).withFlex(0));
    serverBox.items.add(FlexItem(80, minpassheight, servStatusBox).withMargin(2).withFlex(1).withMaxWidth(maxservboxwidth).withMaxHeight(60));
    serverBox.items.add(FlexItem(5, 4).withFlex(0));
    serverBox.items.add(FlexItem(180, minitemheight, servGroupBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(180, minpassheight, servGroupPassBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(5, 6).withFlex(0));
    serverBox.items.add(FlexItem(180, minitemheight, servUserBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(5, 6).withFlex(1).withMaxHeight(14));
    serverBox.items.add(FlexItem(minButtonWidth, minitemheight, servButtonBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(5, 10).withFlex(1));
    serverBox.items.add(FlexItem(100, minitemheight, servAddressBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(80, minpassheight, *mServerAudioInfoLabel).withMargin(2).withFlex(1).withMaxWidth(maxservboxwidth).withMaxHeight(60));
    serverBox.items.add(FlexItem(5, 8).withFlex(0));

    minHeight = 4*minitemheight + 3*minpassheight + 58;

    // public groups stuff

    int staticlabelmaxw = 180;

    publicServAddressBox.items.clear();
    publicServAddressBox.flexDirection = FlexBox::Direction::row;
    publicServAddressBox.items.add(FlexItem(servLabelWidth, minitemheight, *mPublicServerHostStaticLabel).withMargin(2).withFlex(1).withMaxWidth(staticlabelmaxw));
    publicServAddressBox.items.add(FlexItem(150, minpassheight, *mPublicServerHostEditor).withMargin(2).withFlex(1).withMaxWidth(220));
    publicServAddressBox.items.add(FlexItem(servLabelWidth, minitemheight, *mPublicServerStatusInfoLabel).withMargin(2).withFlex(0.75));

    publicServUserBox.items.clear();
    publicServUserBox.flexDirection = FlexBox::Direction::row;
    publicServUserBox.items.add(FlexItem(servLabelWidth, minitemheight, *mPublicServerUserStaticLabel).withMargin(2).withFlex(1).withMaxWidth(staticlabelmaxw));
    publicServUserBox.items.add(FlexItem(172, minitemheight, *mPublicServerUsernameEditor).withMargin(2).withFlex(1));

    publicAddGroupBox.items.clear();
    publicAddGroupBox.flexDirection = FlexBox::Direction::row;
    publicAddGroupBox.items.add(FlexItem(100, minitemheight, *mPublicServerInfoStaticLabel).withMargin(2).withFlex(0.5));
    publicAddGroupBox.items.add(FlexItem(100, minitemheight, *mPublicServerAddGroupButton).withMargin(2).withFlex(1).withMaxWidth(180));
    //publicAddGroupBox.items.add(FlexItem(4, 8).withFlex(0.25));
    publicAddGroupBox.items.add(FlexItem(100, minitemheight, *mPublicServerGroupEditor).withMargin(2).withFlex(1));


    int maxpubservboxwidth = 600;

    publicGroupsBox.items.clear();
    publicGroupsBox.flexDirection = FlexBox::Direction::column;
    publicGroupsBox.items.add(FlexItem(5, 3).withFlex(0));
    publicGroupsBox.items.add(FlexItem(100, minitemheight, publicServAddressBox).withMargin(2).withFlex(0));
    publicGroupsBox.items.add(FlexItem(5, 4).withFlex(0));
    publicGroupsBox.items.add(FlexItem(180, minitemheight, publicServUserBox).withMargin(2).withFlex(0).withMaxWidth(maxpubservboxwidth));
    publicGroupsBox.items.add(FlexItem(5, 4).withFlex(0));
    publicGroupsBox.items.add(FlexItem(180, minitemheight, publicAddGroupBox).withMargin(2).withFlex(0).withMaxWidth(maxpubservboxwidth));
    publicGroupsBox.items.add(FlexItem(5, 7).withFlex(0));
    publicGroupsBox.items.add(FlexItem(100, minitemheight, *mPublicGroupComponent).withMargin(2).withFlex(1));

    // recents
    clearRecentsBox.items.clear();
    clearRecentsBox.flexDirection = FlexBox::Direction::row;
    clearRecentsBox.items.add(FlexItem(10, 5).withMargin(0).withFlex(1));
    clearRecentsBox.items.add(FlexItem(minButtonWidth, minitemheight, *mClearRecentsButton).withMargin(0).withFlex(0));
    clearRecentsBox.items.add(FlexItem(10, 5).withMargin(0).withFlex(1));

    recentsBox.items.clear();
    recentsBox.flexDirection = FlexBox::Direction::column;
    recentsBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRecentsListBox).withMargin(6).withFlex(1));
    recentsBox.items.add(FlexItem(minButtonWidth, minitemheight, clearRecentsBox).withMargin(1).withFlex(0));
    recentsBox.items.add(FlexItem(10, 5).withMargin(0).withFlex(0));

    // main layout
    connectTitleBox.items.clear();
    connectTitleBox.flexDirection = FlexBox::Direction::row;
    connectTitleBox.items.add(FlexItem(50, minitemheight, *mConnectCloseButton).withMargin(2));
    connectTitleBox.items.add(FlexItem(80, minitemheight, *mConnectTitle).withMargin(2).withFlex(1));
    connectTitleBox.items.add(FlexItem(50, minitemheight, *mConnectMenuButton).withMargin(2));

    connectHorizBox.items.clear();
    connectHorizBox.flexDirection = FlexBox::Direction::row;
    connectHorizBox.items.add(FlexItem(100, 100, *mConnectTab).withMargin(3).withFlex(1));
    if (mConnectTab->getNumTabs() < 3) {
        connectHorizBox.items.add(FlexItem(335, 100, *mRecentsGroup).withMargin(3).withFlex(0));
    }

    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::column;
    mainBox.items.add(FlexItem(100, minitemheight, connectTitleBox).withMargin(3).withFlex(0));
    mainBox.items.add(FlexItem(100, 100, connectHorizBox).withMargin(3).withFlex(1));


}

void ConnectView::resized()  {



    mConnectComponentBg->setRectangle (getLocalBounds().toFloat());

    if (getWidth() > 700) {
        if (mConnectTab->getNumTabs() > 2) {
            // move recents to main connect component, out of tab
            int adjcurrtab = jmax(0, mConnectTab->getCurrentTabIndex() - 1);

            mConnectTab->removeTab(0);
            mRecentsGroup->addAndMakeVisible(mRecentsContainer.get());
            addAndMakeVisible(mRecentsGroup.get());

            mConnectTab->setCurrentTabIndex(adjcurrtab);
            updateLayout();
        }
    } else {
        if (mConnectTab->getNumTabs() < 3) {
            int tabsel = mConnectTab->getCurrentTabIndex();
            mRecentsGroup->removeChildComponent(mRecentsContainer.get());
            mRecentsGroup->setVisible(false);

            mConnectTab->addTab(TRANS("RECENTS"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mRecentsContainer.get(), false);
            mConnectTab->moveTab(2, 0);
            mConnectTab->setCurrentTabIndex(tabsel + 1);
            updateLayout();
        }
    }


    mainBox.performLayout(getLocalBounds().reduced(2, 2));

    mServerConnectContainer->setBounds(0,0,
                                       mServerConnectViewport->getWidth() - (mServerConnectViewport->getHeight() < minHeight ? mServerConnectViewport->getScrollBarThickness() : 0 ),
                                       jmax(minHeight, mServerConnectViewport->getHeight()));

    //remoteBox.performLayout(mDirectConnectContainer->getLocalBounds().withSizeKeepingCentre(jmin(400, mDirectConnectContainer->getWidth()), mDirectConnectContainer->getHeight()));
    serverBox.performLayout(mServerConnectContainer->getLocalBounds().withSizeKeepingCentre(jmin(400, mServerConnectContainer->getWidth()), mServerConnectContainer->getHeight()));

    //mPublicServerConnectContainer->setBounds(0,0,
    //                                   mPublicServerConnectViewport->getWidth() - (mPublicServerConnectViewport->getHeight() < minServerConnectHeight ? mPublicServerConnectViewport->getScrollBarThickness() : 0 ),
    //                                   jmax(minServerConnectHeight, mPublicServerConnectViewport->getHeight()));



    publicGroupsBox.performLayout(mPublicServerConnectContainer->getLocalBounds());
    //publicGroupsBox.performLayout(mPublicServerConnectContainer->getLocalBounds().withSizeKeepingCentre(jmin(400, mPublicServerConnectContainer->getWidth()), mPublicServerConnectContainer->getHeight()));

    mPublicGroupsListBox->setBounds(mPublicGroupComponent->getLocalBounds().reduced(4).withTrimmedTop(10));


    if (mConnectTab->getNumTabs() < 3) {
        mRecentsContainer->setBounds(mRecentsGroup->getLocalBounds().reduced(4).withTrimmedTop(10));
    }
    recentsBox.performLayout(mRecentsContainer->getLocalBounds());

    mServerInfoLabel->setBounds(mServerStatusLabel->getBounds());

    if (isVisible()) {
        mRecentsListBox->updateContent();
    }
}

void ConnectView::groupJoinFailed()
{
    mServerGroupPasswordEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
    mServerGroupPasswordEditor->repaint();
}


void ConnectView::showActiveGroupTab()
{

    int adjindex = mConnectTab->getCurrentTabIndex() + (mConnectTab->getNumTabs() > 2 ? 0 : 1);
    if (adjindex != 1 && adjindex != 2) {
        if (currConnectionInfo.groupIsPublic) {
            showPublicGroupTab();
        } else {
            showPrivateGroupTab();
        }
    }
}

void ConnectView::showPrivateGroupTab()
{
    mConnectTab->setCurrentTabIndex(mConnectTab->getNumTabs() > 2 ? 1 : 0);
}

void ConnectView::showPublicGroupTab()
{
    mConnectTab->setCurrentTabIndex(mConnectTab->getNumTabs() > 2 ? 2 : 1);
}

bool ConnectView::getServerGroupAndPasswordText(String & retgroup, String & retpass) const
{
    if (mConnectTab->getCurrentContentComponent() == mServerConnectViewport.get()) {
        retgroup = mServerGroupEditor->getText();
        retpass = mServerGroupPasswordEditor->getText();
        return true;
    }
    return false;
}


void ConnectView::connectTabChanged (int newCurrentTabIndex)
{
    // normaliza index to have recents as 0
    int adjindex = mConnectTab->getNumTabs() < 3 ? newCurrentTabIndex + 1 : newCurrentTabIndex;

    // public groups
    if (adjindex == 2) {
        publicGroupLogin();
    }
    else if (adjindex == 1) {
        // private groups
        resetPrivateGroupLabels();
    }
}

void ConnectView::publicGroupLogin()
{
    String hostport = mPublicServerHostEditor->getText();
    DBG("Public host enter pressed");
    // parse it
    StringArray toks = StringArray::fromTokens(hostport, ":", "");
    String host = "aoo.sonobus.net";
    int port = DEFAULT_SERVER_PORT;

    if (toks.size() >= 1) {
        host = toks[0].trim();
    }
    if (toks.size() >= 2) {
        port = toks[1].trim().getIntValue();
    }

    AooServerConnectionInfo info;
    info.userName = mPublicServerUsernameEditor->getText();
    info.serverHost = host;
    info.serverPort = port;

    bool connchanged = (info.serverHost != currConnectionInfo.serverHost
                        || info.serverPort != currConnectionInfo.serverPort
                        || info.userName != currConnectionInfo.userName);

    if (connchanged
        || !processor.getWatchPublicGroups()
        || !processor.isConnectedToServer()
        ) {

        if (connchanged && processor.isConnectedToServer()) {
            processor.disconnectFromServer();
        }
        else if (!processor.getWatchPublicGroups() && processor.isConnectedToServer()) {
            processor.setWatchPublicGroups(true);
        }

        if (!processor.isConnectedToServer()) {

            Timer::callAfterDelay(100, [this,info] {
                connectWithInfo(info, true);
                updatePublicGroups();
            });
        }
    }
}

bool ConnectView::copyInfoToClipboard(bool singleURL, String * retmessage)
{
    String message = TRANS("Share this link with others to connect with SonoBus:") + " \n";

    String hostport = mServerHostEditor->getText();
    if (hostport.isEmpty()) {
        hostport = "aoo.sonobus.net";
    }

    String groupName;
    String groupPassword;

    if (!currConnected) {
        if (mConnectTab->getCurrentContentComponent() == mServerConnectViewport.get()) {
            groupName = mServerGroupEditor->getText();
            groupPassword = mServerGroupPasswordEditor->getText();
        }
    } else {
        groupName = currConnectionInfo.groupName;
        groupPassword = currConnectionInfo.groupPassword;
    }

    String urlstr1;
    urlstr1 << String("sonobus://") << hostport << String("/");
    URL url(urlstr1);
    URL url2("http://go.sonobus.net/sblaunch");

    if (url.isWellFormed() && groupName.isNotEmpty()) {

        url2 = url2.withParameter("s", hostport);

        url = url.withParameter("g", groupName);
        url2 = url2.withParameter("g", groupName);

        if (groupPassword.isNotEmpty()) {
            url = url.withParameter("p", groupPassword);
            url2 = url2.withParameter("p", groupPassword);
        }

        if (currConnected && currConnectionInfo.groupIsPublic) {
            url = url.withParameter("public", "1");
            url2 = url2.withParameter("public", "1");
        }

        //message += url.toString(true);
        //message += "\n\n";

        //message += TRANS("Or share this link:") + "\n";
        message += url2.toString(true);
        message += "\n";

        if (singleURL) {
            message = url2.toString(true);
        }
        SystemClipboard::copyTextToClipboard(message);

        if (retmessage) {
            *retmessage = message;
        }

        return true;
    }
    return false;
}


void ConnectView::textEditorReturnKeyPressed (TextEditor& ed)
{
    DBG("Return pressed");

    if (&ed == mPublicServerHostEditor.get() || &ed == mPublicServerUsernameEditor.get()) {
        publicGroupLogin();
    }
    else if (&ed == mPublicServerGroupEditor.get()) {
        buttonClicked(mPublicServerAddGroupButton.get());
    }


    if (isVisible() && mServerConnectButton->isShowing()) {
        //mServerConnectButton->setWantsKeyboardFocus(true);
        mServerConnectButton->grabKeyboardFocus();
        //mServerConnectButton->setWantsKeyboardFocus(false);
    }
    else if (isVisible() && mPublicServerAddGroupButton->isShowing()) {
        //mServerConnectButton->setWantsKeyboardFocus(true);
        mPublicServerAddGroupButton->grabKeyboardFocus();
        //mServerConnectButton->setWantsKeyboardFocus(false);
    }
    //else if (isVisible() && mDirectConnectButton->isShowing()) {
        //mServerConnectButton->setWantsKeyboardFocus(true);
    //    mDirectConnectButton->grabKeyboardFocus();
        //mServerConnectButton->setWantsKeyboardFocus(false);
    //}
}

void ConnectView::textEditorEscapeKeyPressed (TextEditor& ed)
{
    DBG("escape pressed");
    if (isVisible()) {
        //mServerConnectButton->setWantsKeyboardFocus(true);
        mServerConnectButton->grabKeyboardFocus();
        //mServerConnectButton->setWantsKeyboardFocus(false);
    }
    //grabKeyboardFocus();
}

void ConnectView::textEditorTextChanged (TextEditor& ed)
{
    if (&ed == mPublicServerUsernameEditor.get() || &ed == mServerUsernameEditor.get()) {
        // try to set the current username, it will fail if we are connected, no big deal
        processor.setCurrentUsername(ed.getText());
    }
}


void ConnectView::textEditorFocusLost (TextEditor& ed)
{
    // only one we care about live is udp port
    if (&ed == mPublicServerHostEditor.get() || &ed == mPublicServerUsernameEditor.get()) {
        publicGroupLogin();
    }

}

void ConnectView::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == mDirectConnectButton.get()) {
        String hostport = mAddRemoteHostEditor->getText();
        //String port = mAddRemotePortEditor->getText();
        // parse it
        StringArray toks = StringArray::fromTokens(hostport, ":/ ", "");
        String host;
        int port = 11000;

        if (toks.size() >= 1) {
            host = toks[0].trim();
        }
        if (toks.size() >= 2) {
            port = toks[1].trim().getIntValue();
        }

        if (host.isNotEmpty() && port != 0) {
            if (processor.connectRemotePeer(host, port, "", "", processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainRecvMute)->getValue() == 0)) {
                setVisible(false);
                if (auto * callout = dynamic_cast<CallOutBox*>(directConnectCalloutBox.get())) {
                    callout->dismiss();
                    directConnectCalloutBox = nullptr;
                }
            }
        }

    }
    else if (buttonThatWasClicked == mServerConnectButton.get()) {
        bool wasconnected = false;
        if (processor.isConnectedToServer()) {

            //mConnectionTimeLabel->setText(TRANS("Total: ") + SonoUtility::durationToString(processor.getElapsedConnectedTime(), true), dontSendNotification);

            processor.disconnectFromServer();
            //updateState();
            wasconnected = true;

        }

        String hostport = mServerHostEditor->getText();

        // parse it
        StringArray toks = StringArray::fromTokens(hostport, ":", "");
        String host = "aoo.sonobus.net";
        int port = DEFAULT_SERVER_PORT;

        if (toks.size() >= 1) {
            host = toks[0].trim();
        }
        if (toks.size() >= 2) {
            port = toks[1].trim().getIntValue();
        }

        AooServerConnectionInfo info;
        info.userName = mServerUsernameEditor->getText();
        info.groupName = mServerGroupEditor->getText();
        info.groupPassword = mServerGroupPasswordEditor->getText();
        info.groupIsPublic = false;
        info.serverHost = host;
        info.serverPort = port;

        connectWithInfo(info);

        listeners.call(&ConnectView::Listener::connectionsChanged, this);

        //mConnectionTimeLabel->setText("", dontSendNotification);

    }
    else if (buttonThatWasClicked == mPublicServerAddGroupButton.get()) {

        String hostport = mPublicServerHostEditor->getText();

        // parse it
        StringArray toks = StringArray::fromTokens(hostport, ":", "");
        String host = "aoo.sonobus.net";
        int port = DEFAULT_SERVER_PORT;

        if (toks.size() >= 1) {
            host = toks[0].trim();
        }
        if (toks.size() >= 2) {
            port = toks[1].trim().getIntValue();
        }

        AooServerConnectionInfo info;
        info.userName = mPublicServerUsernameEditor->getText();
        info.groupName = mPublicServerGroupEditor->getText();
        info.groupPassword = "";
        info.groupIsPublic = true;
        info.serverHost = host;
        info.serverPort = port;

        connectWithInfo(info);

        listeners.call(&ConnectView::Listener::connectionsChanged, this);

        //mConnectionTimeLabel->setText("", dontSendNotification);

    }
    else if (buttonThatWasClicked == mServerGroupRandomButton.get()) {
        // randomize group name
        String rgroup = mRandomSentence->randomSentence();
        mServerGroupEditor->setText(rgroup, dontSendNotification);
    }
    else if (buttonThatWasClicked == mServerPasteButton.get()) {
        if (attemptToPasteConnectionFromClipboard()) {
            updateServerFieldsFromConnectionInfo();
            updateServerStatusLabel(TRANS("Filled in Group information from clipboard! Press 'Connect to Group' to join..."), false);
        }
    }
    else if (buttonThatWasClicked == mServerCopyButton.get()) {
        if (copyInfoToClipboard()) {
            showPopTip(TRANS("Copied connection info to clipboard for you to share with others"), 3000, mServerCopyButton.get());
        }
    }
    else if (buttonThatWasClicked == mServerShareButton.get()) {
        String message;
        bool singleurl = false;
#if JUCE_IOS || JUCE_ANDROID
        singleurl = true;
#endif
        if (copyInfoToClipboard(singleurl, &message)) {
            URL url(message);
            if (url.isWellFormed()) {
                Array<URL> urlarray;
                urlarray.add(url);
                ContentSharer::getInstance()->shareFiles(urlarray, [](bool result, const String& msg){ DBG("url share returned " << (int)result << " : " <<  msg); });
            } else {
                ContentSharer::getInstance()->shareText(message, [](bool result, const String& msg){ DBG("share returned " << (int)result << " : " << msg); });
            }
        }

        //copyInfoToClipboard();
        //showPopTip(TRANS("Copied connection info to clipboard for you to share with others"), 3000, mServerCopyButton.get());
    }
    else if (buttonThatWasClicked == mConnectCloseButton.get()) {
        setVisible(false);

        processor.setWatchPublicGroups(false);

        updateState();
    }
    else if (buttonThatWasClicked == mConnectMenuButton.get()) {

        showAdvancedMenu();
    }
    else if (buttonThatWasClicked == mClearRecentsButton.get()) {
        processor.clearRecentServerConnectionInfos();
        updateRecents();
    }

}

void ConnectView::showAdvancedMenu()
{
    // jlc
    Array<GenericItemChooserItem> items;
    items.add(GenericItemChooserItem(TRANS("Connect to Raw Address...")));

    Component* dw = mConnectMenuButton->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw) dw = mConnectMenuButton->findParentComponentOfClass<Component>();
    Rectangle<int> bounds =  dw->getLocalArea(nullptr, mConnectMenuButton->getScreenBounds());

    SafePointer<ConnectView> safeThis(this);

    auto callback = [safeThis,dw,bounds](GenericItemChooser* chooser,int index) mutable {
        if (!safeThis) return;
        auto wrap = std::make_unique<Viewport>();

        int defWidth = 320;
#if JUCE_IOS || JUCE_ANDROID
        int defHeight = 300;
#else
        int defHeight = 250;
#endif

        int extrawidth = 0;
        if (defHeight > dw->getHeight() - 24) {
            extrawidth = wrap->getScrollBarThickness() + 1;
        }

        wrap->setSize(jmin(defWidth + extrawidth, dw->getWidth() - 10), jmin(defHeight, dw->getHeight() - 24));

        safeThis->mDirectConnectContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));

        wrap->setViewedComponent(safeThis->mDirectConnectContainer.get(), false);
        safeThis->mDirectConnectContainer->setVisible(true);

        safeThis->remoteBox.performLayout(safeThis->mDirectConnectContainer->getLocalBounds().withSizeKeepingCentre(jmin(400, safeThis->mDirectConnectContainer->getWidth()), safeThis->mDirectConnectContainer->getHeight()));

        // show direct connect container
        safeThis->directConnectCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(safeThis->directConnectCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }

    };

    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, -1, dw ? dw->getHeight()-30 : 0);

}

void ConnectView::connectWithInfo(const AooServerConnectionInfo & info, bool allowEmptyGroup)
{
    currConnectionInfo = info;

    if (currConnectionInfo.groupName.isEmpty() && !allowEmptyGroup) {
        if (info.groupIsPublic) {
            mPublicServerStatusInfoLabel->setText(TRANS("You need to specify a group name!"), dontSendNotification);
            mPublicServerGroupEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mPublicServerGroupEditor->repaint();
            mPublicServerStatusInfoLabel->setVisible(true);
        }
        else {
            mServerStatusLabel->setText(TRANS("You need to specify a group name!"), dontSendNotification);
            mServerGroupEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mServerGroupEditor->repaint();
            mServerInfoLabel->setVisible(false);
            mServerStatusLabel->setVisible(true);
        }
        return;
    }
    else {
        mServerGroupEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mServerGroupEditor->repaint();
        mPublicServerGroupEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mPublicServerGroupEditor->repaint();
    }

    if (currConnectionInfo.userName.isEmpty()) {
        String mesg = TRANS("You need to specify a user name!");

        if (info.groupIsPublic) {
            mPublicServerStatusInfoLabel->setText(mesg, dontSendNotification);
            mPublicServerUsernameEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mPublicServerUsernameEditor->repaint();
        } else {
            mServerStatusLabel->setText(mesg, dontSendNotification);
            mServerUsernameEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mServerUsernameEditor->repaint();
        }

        mServerInfoLabel->setVisible(false);
        mServerStatusLabel->setVisible(true);
        return;
    }
    else {
        mServerUsernameEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mServerUsernameEditor->repaint();
        mPublicServerUsernameEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mPublicServerUsernameEditor->repaint();
    }

    //mServerGroupPasswordEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
    mServerGroupPasswordEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
    mServerGroupPasswordEditor->repaint();

    if (currConnectionInfo.serverHost.isNotEmpty() && currConnectionInfo.serverPort != 0)
    {
        processor.disconnectFromServer();

        Timer::callAfterDelay(100, [this] {
            processor.connectToServer(currConnectionInfo.serverHost, currConnectionInfo.serverPort, currConnectionInfo.userName, currConnectionInfo.userPassword);
           // updateState();
            listeners.call(&ConnectView::Listener::connectionsChanged, this);

        });

        mServerHostEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mServerHostEditor->repaint();

        mPublicServerHostEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mPublicServerHostEditor->repaint();
    }
    else {
        String mesg = TRANS("Server address is invalid!");
        if (info.groupIsPublic) {
            mPublicServerStatusInfoLabel->setText(mesg, dontSendNotification);
            mPublicServerHostEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mPublicServerHostEditor->repaint();
        } else {
            mServerStatusLabel->setText(mesg, dontSendNotification);
            mServerHostEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mServerHostEditor->repaint();
        }

        mServerInfoLabel->setVisible(false);
        mServerStatusLabel->setVisible(true);
    }
}

void ConnectView::updateRecents()
{
    recentsListModel.updateState();
    mRecentsListBox->updateContent();
    mRecentsListBox->deselectAllRows();
}

void ConnectView::updatePublicGroups()
{
    publicGroupsListModel.updateState();
    mPublicGroupsListBox->updateContent();
    mPublicGroupsListBox->repaint();
    mPublicGroupsListBox->deselectAllRows();
}

void ConnectView::resetPrivateGroupLabels()
{
    if (!mServerInfoLabel) return;

    mServerInfoLabel->setText(TRANS("All who join the same Group will be able to connect with each other."), dontSendNotification);
    mServerInfoLabel->setVisible(true);
    mServerStatusLabel->setVisible(false);
}


bool ConnectView::attemptToPasteConnectionFromClipboard()
{
    auto clip = SystemClipboard::getTextFromClipboard();

    if (clip.isNotEmpty()) {
        // look for sonobus URL anywhere in it
        String urlpart = clip.fromFirstOccurrenceOf("sonobus://", true, true);
        if (urlpart.isNotEmpty()) {
            // find the end (whitespace) and strip it out
            urlpart = urlpart.upToFirstOccurrenceOf("\n", false, true).trim();
            urlpart = urlpart.upToFirstOccurrenceOf(" ", false, true).trim();
            URL url(urlpart);

            if (url.isWellFormed()) {
                DBG("Got good sonobus URL: " << urlpart);

                // clear clipboard
                SystemClipboard::copyTextToClipboard("");

                return handleSonobusURL(url);
            }
        }
        else {
            // look for go.sonobus.net/sblaunch  url
            urlpart = clip.fromFirstOccurrenceOf("http://go.sonobus.net/sblaunch?", true, false);
            if (urlpart.isEmpty()) urlpart = clip.fromFirstOccurrenceOf("https://go.sonobus.net/sblaunch?", true, false);

            if (urlpart.isNotEmpty()) {
                urlpart = urlpart.upToFirstOccurrenceOf("\n", false, true).trim();
                urlpart = urlpart.upToFirstOccurrenceOf(" ", false, true).trim();
                URL url(urlpart);

                if (url.isWellFormed()) {
                    DBG("Got good http sonobus URL: " << urlpart);

                    SystemClipboard::copyTextToClipboard("");

                    return handleSonobusURL(url);
                }
            }
        }
    }

    return false;
}

bool ConnectView::handleSonobusURL(const URL & url)
{
    // look for either  http://go.sonobus.net/sblaunch?  style url
    // or sonobus://host:port/? style

    auto & pnames = url.getParameterNames();
    auto & pvals = url.getParameterValues();

    int ind;
    if ((ind = pnames.indexOf("g", true)) >= 0) {
        currConnectionInfo.groupName = pvals[ind];

        if ((ind = pnames.indexOf("p", true)) >= 0) {
            currConnectionInfo.groupPassword = pvals[ind];
        } else {
            currConnectionInfo.groupPassword = "";
        }

        if ((ind = pnames.indexOf("public", true)) >= 0) {
            currConnectionInfo.groupIsPublic = pvals[ind].getIntValue() > 0;
        } else {
            currConnectionInfo.groupIsPublic = false;
        }

    }

    if (url.getScheme() == "sonobus") {
        // use domain part as host:port
        String hostpart = url.getDomain();
        currConnectionInfo.serverHost =  hostpart.upToFirstOccurrenceOf(":", false, true);
        int port = url.getPort();
        if (port > 0) {
            currConnectionInfo.serverPort = port;
        } else {
            currConnectionInfo.serverPort = DEFAULT_SERVER_PORT;
        }
    }
    else {
        if ((ind = pnames.indexOf("s", true)) >= 0) {
            String hostpart = pvals[ind];
            currConnectionInfo.serverHost =  hostpart.upToFirstOccurrenceOf(":", false, true);
            String portpart = hostpart.fromFirstOccurrenceOf(":", false, false);
            int port = portpart.getIntValue();
            if (port > 0) {
                currConnectionInfo.serverPort = port;
            } else {
                currConnectionInfo.serverPort = DEFAULT_SERVER_PORT;
            }
        }
    }

    return true;
}


void ConnectView::updateServerStatusLabel(const String & mesg, bool mainonly)
{
    const double fadeAfterSec = 5.0;
    //mMainStatusLabel->setText(mesg, dontSendNotification);
    //Desktop::getInstance().getAnimator().fadeIn(mMainStatusLabel.get(), 200);
    //serverStatusFadeTimestamp = Time::getMillisecondCounterHiRes() * 1e-3 + fadeAfterSec;

    if (!mainonly) {
        mServerStatusLabel->setText(mesg, dontSendNotification);
        mPublicServerStatusInfoLabel->setText(mesg, dontSendNotification);
        mServerInfoLabel->setVisible(false);
        mServerStatusLabel->setVisible(true);
        mPublicServerStatusInfoLabel->setVisible(true);
    }
}


void ConnectView::updateServerFieldsFromConnectionInfo()
{
    if (currConnectionInfo.serverPort == DEFAULT_SERVER_PORT) {
        mServerHostEditor->setText( currConnectionInfo.serverHost, false);
        mPublicServerHostEditor->setText( currConnectionInfo.serverHost, false);
    } else {
        String hostport;
        hostport << currConnectionInfo.serverHost << ":" << currConnectionInfo.serverPort;
        mServerHostEditor->setText( hostport, false);
        mPublicServerHostEditor->setText( hostport, false);
    }
    mServerUsernameEditor->setText(currConnectionInfo.userName, false);
    mPublicServerUsernameEditor->setText(currConnectionInfo.userName, false);
    if (currConnectionInfo.groupIsPublic) {
        mPublicServerGroupEditor->setText(currConnectionInfo.groupName, false);
    } else if (currConnectionInfo.groupName.isNotEmpty()){
        mServerGroupEditor->setText(currConnectionInfo.groupName, false);
    }
    mServerGroupPasswordEditor->setText(currConnectionInfo.groupPassword, false);
}

void ConnectView::showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth)
{
    popTip.reset(new BubbleMessageComponent());
    popTip->setAllowedPlacement(BubbleComponent::above);
    
    if (target) {
        if (auto * parent = target->findParentComponentOfClass<AudioProcessorEditor>()) {
            parent->addChildComponent (popTip.get());
        } else {
            addChildComponent(popTip.get());            
        }
    }
    else {
        addChildComponent(popTip.get());
    }
    
    AttributedString text(message);
    text.setJustification (Justification::centred);
    text.setColour (findColour (TextButton::textColourOffId));
    text.setFont(Font(12));
    if (target) {
        popTip->showAt(target, text, timeoutMs);
    }
    else {
        Rectangle<int> topbox(getWidth()/2 - maxwidth/2, 0, maxwidth, 2);
        popTip->showAt(topbox, text, timeoutMs);
    }
    popTip->toFront(false);
}

void ConnectView::paint(Graphics & g)
{
    /*
    //g.fillAll (Colours::black);
    Rectangle<int> bounds = getLocalBounds();

    bounds.reduce(1, 1);
    bounds.removeFromLeft(3);
    
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(outlineColor);
    g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 0.5f);
*/
}


#pragma RecentsListModel


ConnectView::RecentsListModel::RecentsListModel(ConnectView * parent_) : parent(parent_)
{
    groupImage = ImageCache::getFromMemory(BinaryData::people_png, BinaryData::people_pngSize);
    personImage = ImageCache::getFromMemory(BinaryData::person_png, BinaryData::person_pngSize);
    removeImage = Drawable::createFromImageData(BinaryData::x_icon_svg, BinaryData::x_icon_svgSize);
}


void ConnectView::RecentsListModel::updateState()
{
    parent->processor.getRecentServerConnectionInfos(recents);

}

int ConnectView::RecentsListModel::getNumRows()
{
    return recents.size();
}

String ConnectView::RecentsListModel::getNameForRow (int rowNumber)
{
    if (rowNumber < recents.size()) {
        return recents.getReference(rowNumber).groupName;
    }
    return ListBoxModel::getNameForRow(rowNumber);
}


void ConnectView::RecentsListModel::paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= recents.size()) return;

    if (rowIsSelected) {
        g.setColour (parent->findColour(selectedColourId));
        g.fillRect(Rectangle<int>(0,0,width,height));
    }

    g.setColour(parent->findColour(separatorColourId));
    g.drawLine(0, height-1, width, height);


    g.setColour (parent->findColour(nameTextColourId));
    g.setFont (parent->recentsGroupFont);

    AooServerConnectionInfo & info = recents.getReference(rowNumber);

    float xratio = 0.5;
    int removewidth = jmin(36, height - 6);
    float yratio = 0.6;
    float adjwidth = width - removewidth;

    // DebugLogC("Paint %s", text.toRawUTF8());
    float iconsize = height*yratio;
    float groupheight = height*yratio;
    g.drawImageWithin(groupImage, 0, 0, iconsize, iconsize, RectanglePlacement::fillDestination);
    g.drawFittedText (info.groupName, iconsize + 4, 0, adjwidth*xratio - 8 - iconsize, groupheight, Justification::centredLeft, true);

    g.setFont (parent->recentsNameFont);
    g.setColour (parent->findColour(nameTextColourId).withAlpha(0.8f));
    g.drawImageWithin(personImage, adjwidth*xratio, 0, iconsize, iconsize, RectanglePlacement::fillDestination);
    g.drawFittedText (info.userName, adjwidth*xratio + iconsize, 0, adjwidth*(1.0f - xratio) - 4 - iconsize, groupheight, Justification::centredLeft, true);

    String infostr;

    if (info.groupIsPublic) {
        infostr += TRANS("PUBLIC") + " ";
    }

    if (info.groupPassword.isNotEmpty()) {
        infostr += TRANS("password protected,") + " ";
    }

    infostr += TRANS("on") + " " + Time(info.timestamp).toString(true, true, false) + " " ;

    if (info.serverHost != "aoo.sonobus.net") {
        infostr += TRANS("to") + " " +  info.serverHost;
    }

    g.setColour (parent->findColour(nameTextColourId).withAlpha(0.5f));
    g.setFont (parent->recentsInfoFont);

    g.drawFittedText (infostr, 14, height * yratio, adjwidth - 24, height * (1.0f - yratio), Justification::centredTop, true);

    removeImage->drawWithin(g, Rectangle<float>(adjwidth + removewidth*0.25*yratio, height*0.5 - removewidth*0.5*yratio, removewidth*yratio, removewidth*yratio), RectanglePlacement::fillDestination, 0.9);

    removeButtonX = adjwidth;
    cachedWidth = width;
}

void ConnectView::RecentsListModel::listBoxItemClicked (int rowNumber, const MouseEvent& e)
{
    // use this
    DBG("Clicked " << rowNumber << "  x: " << e.getPosition().x << "  width: " << cachedWidth);

    if (e.getPosition().x > removeButtonX) {
        parent->processor.removeRecentServerConnectionInfo(rowNumber);
        parent->updateRecents();
    }
    else {
        parent->connectWithInfo(recents.getReference(rowNumber));
    }
}

void ConnectView::RecentsListModel::selectedRowsChanged(int rowNumber)
{

}

void ConnectView::RecentsListModel::deleteKeyPressed (int rowNumber)
{
    DBG("delete key pressed");
    if (rowNumber < recents.size()) {
        parent->processor.removeRecentServerConnectionInfo(rowNumber);
        parent->updateRecents();
    }
}

void ConnectView::RecentsListModel::returnKeyPressed (int rowNumber)
{
    DBG("return key pressed: " << rowNumber);

    if (rowNumber < recents.size()) {
        parent->connectWithInfo(recents.getReference(rowNumber));
    }
}

#pragma PublicGroupsListModel

ConnectView::PublicGroupsListModel::PublicGroupsListModel(ConnectView * parent_) : parent(parent_)
{
    groupImage = ImageCache::getFromMemory(BinaryData::people_png, BinaryData::people_pngSize);
    personImage = ImageCache::getFromMemory(BinaryData::person_png, BinaryData::person_pngSize);
}


void ConnectView::PublicGroupsListModel::updateState()
{
    groups.clear();
    parent->processor.getPublicGroupInfos(groups);
}

int ConnectView::PublicGroupsListModel::getNumRows()
{
    return groups.size();
}

void ConnectView::PublicGroupsListModel::paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= groups.size()) return;

    AooPublicGroupInfo & info = groups.getReference(rowNumber);

    bool iscurr = parent->processor.isConnectedToServer() && info.groupName == parent->processor.getCurrentJoinedGroup();

    if (rowIsSelected || iscurr) {
        g.setColour (parent->findColour(selectedColourId));
        g.fillRect(Rectangle<int>(0,0,width,height));
    }

    g.setColour(parent->findColour(separatorColourId));
    g.drawLine(0, height-1, width, height);


    g.setColour (parent->findColour(nameTextColourId));
    g.setFont (parent->recentsGroupFont);


    float xratio = 0.7;
    int removewidth = 0 ; // jmin(36, height - 6);
    float yratio = 1.0; // 0.6
    float adjwidth = width - removewidth;

    // DebugLogC("Paint %s", text.toRawUTF8());
    float iconsize = height*yratio;
    float groupheight = height*yratio;
    g.drawImageWithin(groupImage, 0, 0, iconsize, iconsize, RectanglePlacement::fillDestination);
    g.drawFittedText (info.groupName, iconsize + 4, 0, adjwidth*xratio - 8 - iconsize, groupheight, Justification::centredLeft, true);

    g.setFont (parent->recentsNameFont);
    g.setColour (parent->findColour(nameTextColourId).withAlpha(0.8f));
    g.drawImageWithin(personImage, adjwidth*xratio, 0, iconsize, iconsize, RectanglePlacement::fillDestination);
    String usertext;
    usertext << info.activeCount << (info.activeCount > 1 ? TRANS(" active users") : TRANS(" active user"));
    g.drawFittedText (usertext, adjwidth*xratio + iconsize, 0, adjwidth*(1.0f - xratio) - 4 - iconsize, groupheight, Justification::centredLeft, true);


    //g.setColour (parent->findColour(nameTextColourId).withAlpha(0.5f));
    //g.setFont (parent->recentsInfoFont);
    //g.drawFittedText (infostr, 14, height * yratio, adjwidth - 24, height * (1.0f - yratio), Justification::centredTop, true);

    cachedWidth = width;
}

String ConnectView::PublicGroupsListModel::getNameForRow (int rowNumber)
{
    if (rowNumber < groups.size()) {
        return groups.getReference(rowNumber).groupName;
    }
    return ListBoxModel::getNameForRow(rowNumber);
}

void ConnectView::PublicGroupsListModel::returnKeyPressed (int rowNumber)
{
    DBG("return key pressed: " << rowNumber);

    groupSelected(rowNumber);
}

void ConnectView::PublicGroupsListModel::listBoxItemClicked (int rowNumber, const MouseEvent& e)
{
    // use this
    DBG("Clicked " << rowNumber << "  x: " << e.getPosition().x << "  width: " << cachedWidth);

    groupSelected(rowNumber);

}

void ConnectView::PublicGroupsListModel::groupSelected(int rowNumber)
{
    if (rowNumber >= groups.size() || rowNumber < 0) {
        DBG("Clicked out of bounds row!");
        return;
    }

    auto & ginfo = groups.getReference(rowNumber);

    if (parent->processor.isConnectedToServer() && ginfo.groupName == parent->processor.getCurrentJoinedGroup()) {
        DBG("Already joined this group!");
        return;
    }

    if (parent->processor.isConnectedToServer()) {
        parent->currConnectionInfo.groupName = ginfo.groupName;
        parent->currConnectionInfo.groupPassword.clear();
        parent->currConnectionInfo.groupIsPublic = true;
        parent->currConnectionInfo.timestamp = Time::getCurrentTime().toMilliseconds();
        parent->processor.addRecentServerConnectionInfo(parent->currConnectionInfo);

        bool isPublic = true;

        parent->processor.leaveServerGroup(parent->processor.getCurrentJoinedGroup());

        parent->processor.joinServerGroup(parent->currConnectionInfo.groupName, parent->currConnectionInfo.groupPassword, isPublic);

        parent->processor.setWatchPublicGroups(false);
    }
    else {

        AooServerConnectionInfo cinfo;
        cinfo.userName = parent->mPublicServerUsernameEditor->getText();
        cinfo.groupName = ginfo.groupName;
        cinfo.groupIsPublic = true;
        cinfo.serverHost = parent->currConnectionInfo.serverHost;
        cinfo.serverPort = parent->currConnectionInfo.serverPort;

        parent->connectWithInfo(cinfo);
    }
}


void ConnectView::PublicGroupsListModel::selectedRowsChanged(int rowNumber)
{

}

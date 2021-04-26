// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include "JuceHeader.h"

#include "SonobusPluginProcessor.h"
#include "SonoLookAndFeel.h"
#include "SonoChoiceButton.h"
#include "SonoDrawableButton.h"
#include "GenericItemChooser.h"

class RandomSentenceGenerator;


class ConnectView :
public Component,
public Button::Listener,
public SonoChoiceButton::Listener,
public GenericItemChooser::Listener,
public TextEditor::Listener,
public MultiTimer
{
public:
    ConnectView(SonobusAudioProcessor& proc, AooServerConnectionInfo & info);
    virtual ~ConnectView();


    class Listener {
    public:
        virtual ~Listener() {}
        virtual void connectionsChanged(ConnectView *comp) {}
    };

    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }

    void timerCallback(int timerid) override;

    void buttonClicked (Button* buttonThatWasClicked) override;

    void textEditorReturnKeyPressed (TextEditor&) override;
    void textEditorEscapeKeyPressed (TextEditor&) override;
    void textEditorTextChanged (TextEditor&) override;
    void textEditorFocusLost (TextEditor&) override;

    juce::Rectangle<int> getMinimumContentBounds() const;


    void updateState();
    void updateLayout();

    void paint(Graphics & g) override;
    void resized() override;

    void showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth=100);

    void connectTabChanged (int newCurrentTabIndex);

    void connectWithInfo(const AooServerConnectionInfo & info, bool allowEmptyGroup = false);

    bool handleSonobusURL(const URL & url);

    bool attemptToPasteConnectionFromClipboard();
    bool copyInfoToClipboard(bool singleURL=false, String * retmessage = nullptr);

    void showActiveGroupTab();
    void showPrivateGroupTab();
    void showPublicGroupTab();

    String getServerHostText() const { return mServerHostEditor->getText(); }

    bool getServerGroupAndPasswordText(String & retgroup, String & retpass) const;

    void updateRecents();

    void updateServerStatusLabel(const String & mesg, bool mainonly);
    void updateServerFieldsFromConnectionInfo();

    void updatePublicGroups();
    void resetPrivateGroupLabels();
    void groupJoinFailed();

protected:

    void configEditor(TextEditor *editor, bool passwd = false);
    void configServerLabel(Label *label);

    void publicGroupLogin();
    void showAdvancedMenu();


    SonobusAudioProcessor& processor;

    ListenerList<Listener> listeners;

    AooServerConnectionInfo & currConnectionInfo;
    bool currConnected = false;

    bool firstTimeConnectShow = true;

    std::unique_ptr<RandomSentenceGenerator> mRandomSentence;


    std::unique_ptr<Label> mLocalAddressStaticLabel;
    std::unique_ptr<TextEditor> mLocalAddressLabel;

    std::unique_ptr<TextButton> mDirectConnectButton;

    std::unique_ptr<Label> mRemoteAddressStaticLabel;
    std::unique_ptr<TextEditor> mAddRemoteHostEditor;
    std::unique_ptr<Label> mDirectConnectDescriptionLabel;
    std::unique_ptr<TextButton> mServerConnectButton;
    std::unique_ptr<Label> mServerHostStaticLabel;
    std::unique_ptr<TextEditor> mServerHostEditor;

    std::unique_ptr<Label> mPublicServerHostStaticLabel;
    std::unique_ptr<TextEditor> mPublicServerHostEditor;
    std::unique_ptr<TextEditor> mPublicServerUsernameEditor;
    std::unique_ptr<Label> mPublicServerStatusInfoLabel;
    std::unique_ptr<Label> mPublicServerUserStaticLabel;
    std::unique_ptr<GroupComponent> mPublicGroupComponent;
    std::unique_ptr<Label> mPublicServerInfoStaticLabel;
    std::unique_ptr<TextButton> mPublicServerAddGroupButton;
    std::unique_ptr<TextEditor> mPublicServerGroupEditor;


    std::unique_ptr<Label> mServerUserStaticLabel;
    std::unique_ptr<TextEditor> mServerUsernameEditor;
    std::unique_ptr<Label> mServerUserPassStaticLabel;
    std::unique_ptr<TextEditor> mServerUserPasswordEditor;

    std::unique_ptr<Label> mServerGroupStaticLabel;
    std::unique_ptr<TextEditor> mServerGroupEditor;
    std::unique_ptr<Label> mServerGroupPassStaticLabel;
    std::unique_ptr<TextEditor> mServerGroupPasswordEditor;

    std::unique_ptr<SonoDrawableButton> mServerGroupRandomButton;
    std::unique_ptr<SonoDrawableButton> mServerPasteButton;
    std::unique_ptr<SonoDrawableButton> mServerCopyButton;
    std::unique_ptr<SonoDrawableButton> mServerShareButton;
    std::unique_ptr<Label> mServerAudioInfoLabel;


    std::unique_ptr<Label> mServerStatusLabel;
    std::unique_ptr<Label> mServerInfoLabel;
    std::unique_ptr<Label> mMainStatusLabel;

    std::unique_ptr<TabbedComponent> mConnectTab;
    std::unique_ptr<Component> mDirectConnectContainer;
    std::unique_ptr<Viewport> mServerConnectViewport;
    std::unique_ptr<Component> mServerConnectContainer;
    std::unique_ptr<Viewport> mPublicServerConnectViewport;
    std::unique_ptr<Component> mPublicServerConnectContainer;
    std::unique_ptr<Component> mRecentsContainer;
    std::unique_ptr<GroupComponent> mRecentsGroup;

    std::unique_ptr<DrawableRectangle> mConnectComponentBg;
    std::unique_ptr<Label> mConnectTitle;
    std::unique_ptr<SonoDrawableButton> mConnectCloseButton;
    std::unique_ptr<SonoDrawableButton> mConnectMenuButton;

    std::unique_ptr<TabbedComponent> mSettingsTab;

    WeakReference<Component> directConnectCalloutBox;


    class RecentsListModel : public ListBoxModel
    {
    public:
        RecentsListModel(ConnectView * parent_);
        int getNumRows() override;
        void     paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked (int rowNumber, const MouseEvent& e) override;
        void selectedRowsChanged(int lastRowSelected) override;

        void updateState();

    protected:
        ConnectView * parent;

        Image groupImage;
        Image personImage;
        std::unique_ptr<Drawable> removeImage;

        int cachedWidth = 0;
        int removeButtonX = 0;

        Array<AooServerConnectionInfo> recents;
    };
    RecentsListModel recentsListModel;
    Font recentsGroupFont;
    Font recentsNameFont;
    Font recentsInfoFont;

    std::unique_ptr<ListBox> mRecentsListBox;
    std::unique_ptr<SonoTextButton> mClearRecentsButton;

    // public groups stuff
    class PublicGroupsListModel : public ListBoxModel
    {
    public:
        PublicGroupsListModel(ConnectView * parent_);
        int getNumRows() override;
        void paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked (int rowNumber, const MouseEvent& e) override;
        void selectedRowsChanged(int lastRowSelected) override;

        void updateState();

    protected:
        ConnectView * parent;

        Image groupImage;
        Image personImage;

        int cachedWidth = 0;

        Array<AooPublicGroupInfo> groups;
    };
    PublicGroupsListModel publicGroupsListModel;

    std::unique_ptr<ListBox> mPublicGroupsListBox;


    // layout boxes
    FlexBox mainBox;

    FlexBox serverBox;
    FlexBox servStatusBox;
    FlexBox servGroupBox;
    FlexBox servGroupPassBox;
    FlexBox servUserBox;
    FlexBox servUserPassBox;
    //FlexBox inputBox;
    FlexBox servAddressBox;
    FlexBox servButtonBox;
    FlexBox localAddressBox;
    FlexBox addressBox;
    FlexBox remoteBox;

    FlexBox publicGroupsBox;
    FlexBox publicServAddressBox;
    FlexBox publicServUserBox;
    FlexBox publicAddGroupBox;

    FlexBox connectMainBox;
    FlexBox connectHorizBox;
    FlexBox connectTitleBox;

    FlexBox connectBox;

    FlexBox recentsBox;
    FlexBox clearRecentsBox;

    int minHeight;
    

    // keep this down here, so it gets destroyed early
    std::unique_ptr<BubbleMessageComponent> popTip;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectView)

};

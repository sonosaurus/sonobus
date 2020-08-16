
#pragma once

#include "JuceHeader.h"

#include "SonoUtility.h"
#include "SonobusTypes.h"

//==============================================================================
class WaveformTransportComponent  : public Component,
                           public ChangeListener,
                           public FileDragAndDropTarget,
                           public ChangeBroadcaster,
                           private ScrollBar::Listener,
                           private Timer
{
public:
    WaveformTransportComponent (AudioFormatManager& formatManager,
                                AudioTransportSource& source,
                                ApplicationCommandManager & cmdman)
                        //        Slider& slider);
        : transportSource (source),
           commandManager (cmdman),
          //zoomSlider (slider),
          thumbnail (512, formatManager, thumbnailCache)
    {
        posLabel.setFont(14);
        posLabel.setColour(Label::textColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.8).withAlpha(0.8f));
        posLabel.setJustificationType(Justification::bottomLeft);

        totLabel.setFont(14);
        totLabel.setColour(Label::textColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.8).withAlpha(0.8f));
        totLabel.setJustificationType(Justification::bottomRight);

        nameLabel.setFont(12);
        nameLabel.setColour(Label::textColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.8).withAlpha(0.8f));
        nameLabel.setJustificationType(Justification::centredTop);
        nameLabel.setMinimumHorizontalScale(0.5f);
        
        wavecolor = Colour::fromFloatRGBA(0.2, 0.5, 0.7, 1.0);
        thumbnail.addChangeListener (this);

        addAndMakeVisible (scrollbar);
        scrollbar.setRangeLimits (visibleRange);
        scrollbar.setAutoHide (false);
        scrollbar.addListener (this);
        scrollbar.setAlpha (0.6f);
        scrollbar.addMouseListener(this, false);
        
        currentPositionMarker.setFill (Colours::white.withAlpha (0.85f));
        addAndMakeVisible (currentPositionMarker);

        selcolor = Colour::fromFloatRGBA(0.6, 0.6, 0.6, 0.3);
        loopcolor = Colour::fromFloatRGBA(0.7, 0.2, 0.5, 0.35);
        
        currentLoopRect.setFill (selcolor);
        currentLoopRect.setCornerSize(Point<float>(4.0f,4.0f));
        addAndMakeVisible (currentLoopRect);

        addAndMakeVisible(posLabel);
        addAndMakeVisible(totLabel);
        addAndMakeVisible(nameLabel);
        
        transportSource.addChangeListener(this);
        
        updateLoopPosition();
    }

    ~WaveformTransportComponent()
    {
        transportSource.removeChangeListener(this);
        scrollbar.removeListener (this);
        thumbnail.removeChangeListener (this);
    }

    void setURL (const URL& url)
    {
        InputSource* inputSource = nullptr;

       #if ! JUCE_IOS
        if (url.isLocalFile())
        {
            inputSource = new FileInputSource (url.getLocalFile());
        }
        else
       #endif
        {
            if (inputSource == nullptr)
                inputSource = new URLInputSource (url);
        }

        if (inputSource != nullptr)
        {
            thumbnail.setSource (inputSource);

            Range<double> newRange (0.0, thumbnail.getTotalLength());
            scrollbar.setRangeLimits (newRange);
            setRange (newRange);

            
            currentPositionMarker.setVisible(true);

            currentLoopRect.setVisible(transportSource.isLooping());

            double lensec = transportSource.getLengthInSeconds();
            totLabel.setText(SonoUtility::durationToString(lensec, true), dontSendNotification);

            selRangeStart = 0;
            selRangeEnd = lensec;
            setLoopFromSelection();
            
            String fname = url.getFileName().isNotEmpty() ? url.getLocalFile().getFileNameWithoutExtension() : "";
            
            nameLabel.setText(fname, dontSendNotification);
            
            updatePositionLabels();
            
            //startTimerHz (20);
        } else {
            currentPositionMarker.setVisible(false);
            currentLoopRect.setVisible(false);
            stopTimer();
        }
    }

    URL getLastDroppedFile() const noexcept { return lastFileDropped; }

    void setZoomFactor (double amount, double zoomAtXRatio = 0.5f)
    {
        zoomFactor = amount;
        //DebugLogC("Zoomfact: %g", zoomFactor);
        if (thumbnail.getTotalLength() > 0)
        {
            auto newScale = jmax (0.001, thumbnail.getTotalLength() * (1.0 - jlimit (0.0, 0.99, amount)));
            auto timeAtXratio = xToTime (zoomAtXRatio * getWidth());

            setRange ({ timeAtXratio - newScale * zoomAtXRatio, timeAtXratio + newScale * (1.0 - zoomAtXRatio) });
        }
    }

    void setRange (Range<double> newRange)
    {
        visibleRange = newRange;
        scrollbar.setCurrentRange (visibleRange);
        scrollbar.setVisible(zoomFactor > 0.0);
            
        updateCursorPosition();
        updateLoopPosition();
        repaint();
    }

    void setFollowsTransport (bool shouldFollow)
    {
        isFollowingTransport = shouldFollow;
    }

    void updateState ()
    {
        updateLoopPosition();
        updateCursorPosition();
    }
    
    void updateSelectionFromLoop() {
        int64 lstart,llen, tot;
        tot = transportSource.getTotalLength();
        double totsec = transportSource.getLengthInSeconds();
        transportSource.getLoopRange(lstart, llen);
        
        if (totsec > 0) {
            double startpos =  totsec * (lstart / (double) tot);
            double endpos = startpos +  totsec * (llen / (double)tot);
            
            selRangeStart = startpos;
            selRangeEnd = endpos;
        }
        
        updateLoopPosition();
    }
    
    void setLoopFromSelection() {
        setLoopFromTimeRange(selRangeStart, selRangeEnd);
        updateLoopPosition();
    }
    
    void getLoopRangeSec(double & start, double & length) {
        start = selRangeStart;
        length = selRangeEnd - selRangeStart;
    }
    
    void paint (Graphics& g) override
    {
        g.setColour(Colours::black);
        //g.fillAll (Colours::black);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
        
        
        g.setColour (wavecolor);

        if (thumbnail.getTotalLength() > 0.0)
        {
            auto thumbArea = getLocalBounds();

            if (zoomFactor > 0.0) {
            //    thumbArea.removeFromBottom (scrollbar.getHeight() + 4);
            }

            thumbnail.drawChannels (g, thumbArea.reduced (2),
                                    visibleRange.getStart(), visibleRange.getEnd(), 1.0f);
        }
        else
        {
            g.setFont (14.0f);
            g.drawFittedText ("(No audio file selected)", getLocalBounds(), Justification::centred, 2);
        }
    }

    void resized() override
    {
        scrollbar.setBounds (getLocalBounds().removeFromBottom (14).reduced (2));
        updateLoopPosition();

        int labheight = getHeight();
        
        nameLabel.setBounds(4, 2, getWidth() - 8, labheight - 4);
        totLabel.setBounds(0.5 * getWidth() + 4, 2, 0.5*getWidth() - 8, labheight - 4);
        posLabel.setBounds(4, 2, 0.5*getWidth() - 8, labheight - 4);
    }

    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        if (source == &transportSource) {
            if (transportSource.isPlaying()) {
                startTimerHz(20);
                
                locatedOutsideDuringPlayback = false;

                double currPos = transportSource.getCurrentPosition();

                if (currPos >= selRangeStart && currPos < selRangeEnd) {
                    startedWithinSelection = true;
                } else {
                    startedWithinSelection = false;
                }
                                
            } else {
                stopTimer();
                setLoopFromTimeRange(selRangeStart, selRangeEnd);
                if (locatedOutsideDuringPlayback) {
                    transportSource.setPosition(selRangeStart);
                    updateState();
                }
                locatedOutsideDuringPlayback = false;
            }
        }

        // this method is called by the thumbnail when it has changed, so we should repaint it..
        repaint();
    }

    bool isInterestedInFileDrag (const StringArray& /*files*/) override
    {
        return false; // handled in the main editor now
    }

    void filesDropped (const StringArray& files, int /*x*/, int /*y*/) override
    {
        lastFileDropped = URL (File (files[0]));
        sendChangeMessage();
    }

    void mouseDown (const MouseEvent& e) override
    {
        if (e.eventComponent != this) return;

        if ((wasPlayingOnDown = transportSource.isPlaying())) {
         //   transportSource.stop();
        }

        dragActive = false;
        startDragX = e.x;
        startDragY = lastDragY = e.y;

        if (true || transportSource.isLooping())
        {
            
#if JUCE_IOS
            const int touchthresh = 50;
#else
            const int touchthresh = 25;
#endif
            int sdelta = abs(currentLoopRect.getRectangle().getTopLeft().getX() - e.x);
            int edelta = abs(currentLoopRect.getRectangle().getTopRight().getX() - e.x);
#if 0
            if (sdelta < edelta) {
                draggingLoopEdge = 1;
            } else {
                draggingLoopEdge = 2;
            }
#else
            resetSelOnDrag = false;
            
            if (sdelta < touchthresh) {
                draggingLoopEdge = 1;
            }
            else if (edelta < touchthresh) {
                draggingLoopEdge = 2;
            }
            else {
                // reset and start dragging fresh from here
                draggingLoopEdge = 2;
                resetSelOnDrag = true;
            }


#endif
        } else {
            draggingLoopEdge = 0;
        }

        mouseDrag (e);
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (e.eventComponent != this) return;

        if (!dragActive) {
            if (abs(startDragX - e.x) > 3) {
                dragActive = true;
                
                if (resetSelOnDrag) {
                    selRangeStart = selRangeEnd = xToTime(startDragX);
                    locatedOutsideDuringPlayback = true;
                }
                
                touchZooming = false;
            }
            else if (abs(startDragY - e.y) > 5) {
                dragActive = true;
                
                startDragY = lastDragY = e.y;

                touchZooming = true;
            }
        }

        if (!dragActive) {
            return;
        }
        
        if (wasPlayingOnDown && transportSource.isPlaying() && transportSource.isLooping()) {
            transportSource.stop();
        }
        
        /*
        if (canMoveTransport() && !transportSource.isLooping()) {
            transportSource.setPosition (jlimit (0.0, transportSource.getLengthInSeconds(), xToTime ((float) e.x)));
            if (!transportSource.isPlaying()) {
                updateCursorPosition();
            }
        }
         */
        
        if (touchZooming) {
            // vertical drag adjusts zoom
            float deltay = (lastDragY - e.y) * 0.015f;
            //DBG("DeltaY is " << deltay);

            if (deltay != 0.0f) {
                double newfact = jlimit(0.0, 1.0, zoomFactor - deltay);
                double xratio = e.x / (double)getWidth();
                setZoomFactor(newfact, xratio);
                //zoomSlider.setValue (zoomSlider.getValue() - wheel.deltaY);
            }

            repaint();
        }
        else if (draggingLoopEdge > 0)
        {
            double startpos = selRangeStart;
            double endpos = selRangeEnd;
            double totsecs = transportSource.getLengthInSeconds();
            
            double xtime = xToTime((float) e.x);
            
            if (draggingLoopEdge == 1) {
                // drag loop start
                startpos = jlimit(0.0, totsecs, xtime);

                if (endpos < startpos) {
                    // swap it
                    double tmp = startpos;
                    startpos = endpos;
                    endpos = tmp;
                    draggingLoopEdge = 2;
                }

            } else if (draggingLoopEdge == 2) {
                // drag loop end
                endpos = jlimit(0.0, totsecs, xtime);
                
                if (endpos < startpos) {
                    // swap it
                    double tmp = startpos;
                    startpos = endpos;
                    endpos = tmp;
                    draggingLoopEdge = 1;
                }
            }

            selRangeStart = startpos;
            selRangeEnd = endpos;
            
            if (!transportSource.isPlaying()) {
                setLoopFromTimeRange(selRangeStart, selRangeEnd);
            }
            
            updateLoopPosition();
        }
        
        if (canMoveTransport() && !transportSource.isPlaying()) {
            transportSource.setPosition (jlimit (0.0, transportSource.getLengthInSeconds(), selRangeStart));
            if (!transportSource.isPlaying()) {
                updateCursorPosition();
            }
        }
        
        updatePositionLabels();
        
        lastDragY = e.y;
    }

    void setLoopFromTimeRange(double startpos, double endpos)
    {
        double totsecs = transportSource.getLengthInSeconds();
        int64 totsamps = transportSource.getTotalLength();

        int64 lstart = (int64) (totsamps * startpos / totsecs);
        int64 llength = (int64) (totsamps * (endpos - startpos) / totsecs);
        llength = jlimit( jmin((int64)2048, totsamps), totsamps, llength);
        
        if (lstart + llength > totsamps) {
            lstart = totsamps - llength;
        }
        
        transportSource.setLoopRange(lstart, llength);
    }
    
    void mouseUp (const MouseEvent& ev) override
    {
        if (ev.eventComponent != this) return;

        if (!dragActive && canMoveTransport()) {
            
            // if not looping and clicked outside the current selection, clear it (set it to full file range)
            double xtime = xToTime((float) ev.x);
            if (!loopingState && (xtime < selRangeStart || xtime > selRangeEnd)) {
                selRangeStart = 0.0;
                selRangeEnd = transportSource.getLengthInSeconds();

                //if (!transportSource.isPlaying()) {
                setLoopFromTimeRange(selRangeStart, selRangeEnd);
                //}
                
                updateLoopPosition();
            }
            else if (ev.mods.isCommandDown()) {
                if (ev.getNumberOfClicks() > 1) {
                    // zoom full out
                    zoomFactor = 0.0;
                    setRange ({ 0.0, transportSource.getLengthInSeconds() });
                }
                else if (transportSource.getLengthInSeconds() > 0 && (selRangeEnd - selRangeStart) < transportSource.getLengthInSeconds())
                {
                    // zoom to selection
                    zoomFactor = 1.0 - (selRangeEnd - selRangeStart) / transportSource.getLengthInSeconds();
                    setRange ({ selRangeStart, selRangeEnd });
                }                
            }
            
            transportSource.setPosition (jlimit (0.0, transportSource.getLengthInSeconds(), xToTime ((float) ev.x)));
            if (!transportSource.isPlaying()) {
                updateCursorPosition();
            }
        }
        
        if (wasPlayingOnDown || ev.getNumberOfClicks() > 1) {
            if (ev.getNumberOfClicks() > 1 && transportSource.isPlaying()) {
                transportSource.stop();                
            } else {
                transportSource.start();
            }
        }
        //transportSource.start();
    }

    void mouseWheelMove (const MouseEvent& ev, const MouseWheelDetails& wheel) override
    {
        
        if (thumbnail.getTotalLength() > 0.0)
        {
            bool wheelzooms = ev.mods.isAltDown();
            bool wheelscrolls = !wheelzooms && zoomFactor > 0.0;
            
            float scrolldeltax = wheel.deltaX != 0.0f ? wheel.deltaX : wheelscrolls ? wheel.deltaY : 0.0f;
            
            if (scrolldeltax != 0.0f) {
                auto newStart = visibleRange.getStart() - 2.0f*scrolldeltax * (visibleRange.getLength()) / 5.0;
                newStart = jlimit (0.0, jmax (0.0, thumbnail.getTotalLength() - (visibleRange.getLength())), newStart);
                
                if (canMoveTransport())
                    setRange ({ newStart, newStart + visibleRange.getLength() });
            }

           // DBG("DeltaY is " << wheel.deltaY);
            
            
            if (wheelzooms && wheel.deltaY != 0.0f) {
                double newfact = jlimit(0.0, 1.0, zoomFactor + wheel.deltaY);
                double xratio = ev.x / (double)getWidth();
                setZoomFactor(newfact, xratio);
                //zoomSlider.setValue (zoomSlider.getValue() - wheel.deltaY);
            }

            repaint();
        }
    }


private:
    AudioTransportSource& transportSource;
    ApplicationCommandManager& commandManager;
    //Slider& zoomSlider;
    ScrollBar scrollbar  { false };

    Label posLabel;
    Label totLabel;
    Label nameLabel;
    
    AudioThumbnailCache thumbnailCache  { 5 };
    AudioThumbnail thumbnail;
    Range<double> visibleRange;
    double zoomFactor = 0;
    bool isFollowingTransport = false;
    bool wasPlayingOnDown = false;
    bool touchZooming = false;
    int draggingLoopEdge = 0; // 1 is start, 2 is end
    int startDragX = -1;
    int startDragY = -1;
    int lastDragY = 0;
    bool dragActive = false;
    bool startedWithinSelection = false;
    bool locatedOutsideDuringPlayback = false;
    bool loopingState = false;
    bool resetSelOnDrag = false;
    Colour wavecolor;
    Colour selcolor;
    Colour loopcolor;

    int32 lastPosUpdateStamp = 0;
    URL lastFileDropped;

    // used for selection and loop range (seconds)
    double selRangeStart = 0.0;
    double selRangeEnd = 0.0;
    
    DrawableRectangle currentPositionMarker;
    DrawableRectangle currentLoopRect;

    float timeToX (const double time) const
    {
        if (visibleRange.getLength() <= 0)
            return 0;

        return getWidth() * (float) ((time - visibleRange.getStart()) / visibleRange.getLength());
    }

    double xToTime (const float x) const
    {
        return (x / getWidth()) * (visibleRange.getLength()) + visibleRange.getStart();
    }

    bool canMoveTransport() const noexcept
    {
        return ! (isFollowingTransport && transportSource.isPlaying());
    }

    void scrollBarMoved (ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        if (scrollBarThatHasMoved == &scrollbar)
            if (! (isFollowingTransport && transportSource.isPlaying()))
                setRange (visibleRange.movedToStartAt (newRangeStart));
    }

    void showPopupMenu (Rectangle<int> bounds) {
        auto menu = PopupMenu();
        menu.addCommandItem(&commandManager, SonobusCommands::TrimSelectionToNewFile);
        menu.addCommandItem(&commandManager, SonobusCommands::ShareFile);
        menu.addCommandItem(&commandManager, SonobusCommands::CloseFile);

        menu.showAt(bounds);
    }
    
    void timerCallback() override
    {
        double currpos = transportSource.getCurrentPosition();
        
        if (canMoveTransport()) {
            updateCursorPosition();
        }
        else {
            setRange (visibleRange.movedToStartAt (currpos - (visibleRange.getLength() / 2.0)));
        }
        
        auto nowtime = Time::getMillisecondCounter();
        
        if (nowtime > lastPosUpdateStamp + 1000) {
            
            updatePositionLabels();
            
            lastPosUpdateStamp = nowtime;
        }
        
        if (transportSource.isPlaying() && !loopingState && !locatedOutsideDuringPlayback && selRangeEnd > 0 && currpos > selRangeEnd) {
            // stop transport, end of selection reached
            transportSource.stop();
            transportSource.setPosition(selRangeStart);
            updateCursorPosition();
            updatePositionLabels();
        }
        
    }

    void updatePositionLabels()
    {
        double pos = transportSource.getCurrentPosition();
        posLabel.setText(SonoUtility::durationToString(pos, true), dontSendNotification);

    }
    
    void updateCursorPosition()
    {
        //currentPositionMarker.setVisible (transportSource.isPlaying() || isMouseButtonDown());

        currentPositionMarker.setRectangle (Rectangle<float> (timeToX (transportSource.getCurrentPosition()) - 0.75f, 0,
                                                              1.5f, (float) (getHeight() - (zoomFactor > 0 ? 0 /*scrollbar.getHeight() */ : 0))));

    }

    void updateLoopPosition()
    {
        //currentPositionMarker.setVisible (transportSource.isPlaying() || isMouseButtonDown());

        //int64 lstart,llen, tot;
        //tot = transportSource.getTotalLength();
        double totsec = transportSource.getLengthInSeconds();
        //transportSource.getLoopRange(lstart, llen);
        
        if (totsec > 0) {
            //double startpos =  totsec * (lstart / (double) tot);
            //double endpos = startpos +  totsec * (llen / (double)tot);
            
            float xpos = timeToX(selRangeStart);
            currentLoopRect.setRectangle (Rectangle<float> (xpos , 1.0f,
                                                            timeToX(selRangeEnd) - xpos, (float) (getHeight() - (zoomFactor > 0 ? 0 /*scrollbar.getHeight()*/ : 0)) - 2.0f));

            //selRangeStart = startpos;
            //selRangeEnd = endpos;
        }
                

        if (loopingState != transportSource.isLooping()) {
            loopingState = transportSource.isLooping();
            if (loopingState) {
                currentLoopRect.setFill (loopcolor);
            }
            else {
                currentLoopRect.setFill (selcolor);
            }
        }

        bool selvisible = (selRangeEnd - selRangeStart) > 0 && ((selRangeEnd - selRangeStart) < transportSource.getLengthInSeconds() || loopingState);
        
        currentLoopRect.setVisible(selvisible);

    }

};



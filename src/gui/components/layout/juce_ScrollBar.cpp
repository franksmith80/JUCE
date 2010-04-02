/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-9 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#include "../../../core/juce_StandardHeader.h"

BEGIN_JUCE_NAMESPACE

#include "juce_ScrollBar.h"
#include "../lookandfeel/juce_LookAndFeel.h"
#include "../buttons/juce_Button.h"


//==============================================================================
class ScrollbarButton  : public Button
{
public:
    int direction;

    ScrollbarButton (const int direction_, ScrollBar& owner_)
        : Button (String::empty),
          direction (direction_),
          owner (owner_)
    {
        setWantsKeyboardFocus (false);
    }

    ~ScrollbarButton()
    {
    }

    void paintButton (Graphics& g, bool isMouseOver, bool isMouseDown)
    {
        getLookAndFeel()
            .drawScrollbarButton (g, owner,
                                  getWidth(), getHeight(),
                                  direction,
                                  owner.isVertical(),
                                  isMouseOver, isMouseDown);
    }

    void clicked()
    {
        owner.moveScrollbarInSteps ((direction == 1 || direction == 2) ? 1 : -1);
    }


    juce_UseDebuggingNewOperator

private:
    ScrollBar& owner;

    ScrollbarButton (const ScrollbarButton&);
    ScrollbarButton& operator= (const ScrollbarButton&);
};


//==============================================================================
ScrollBar::ScrollBar (const bool vertical_,
                      const bool buttonsAreVisible)
    : totalRange (0.0, 1.0),
      visibleRange (0.0, 0.1),
      singleStepSize (0.1),
      thumbAreaStart (0),
      thumbAreaSize (0),
      thumbStart (0),
      thumbSize (0),
      initialDelayInMillisecs (100),
      repeatDelayInMillisecs (50),
      minimumDelayInMillisecs (10),
      vertical (vertical_),
      isDraggingThumb (false),
      alwaysVisible (false),
      upButton (0),
      downButton (0)
{
    setButtonVisibility (buttonsAreVisible);

    setRepaintsOnMouseActivity (true);
    setFocusContainer (true);
}

ScrollBar::~ScrollBar()
{
    deleteAllChildren();
}

//==============================================================================
void ScrollBar::setRangeLimits (const Range<double>& newRangeLimit)
{
    if (totalRange != newRangeLimit)
    {
        totalRange = newRangeLimit;
        setCurrentRange (visibleRange);
        updateThumbPosition();
    }
}

void ScrollBar::setRangeLimits (const double newMinimum, const double newMaximum)
{
    jassert (newMaximum >= newMinimum); // these can't be the wrong way round!
    setRangeLimits (Range<double> (newMinimum, newMaximum));
}

void ScrollBar::setCurrentRange (const Range<double>& newRange)
{
    const Range<double> constrainedRange (totalRange.constrainRange (newRange));

    if (visibleRange != constrainedRange)
    {
        visibleRange = constrainedRange;

        updateThumbPosition();
        triggerAsyncUpdate();
    }
}

void ScrollBar::setCurrentRange (const double newStart, const double newSize)
{
    setCurrentRange (Range<double> (newStart, newStart + newSize));
}

void ScrollBar::setCurrentRangeStart (const double newStart)
{
    setCurrentRange (visibleRange.movedToStartAt (newStart));
}

void ScrollBar::setSingleStepSize (const double newSingleStepSize)
{
    singleStepSize = newSingleStepSize;
}

void ScrollBar::moveScrollbarInSteps (const int howManySteps)
{
    setCurrentRange (visibleRange + howManySteps * singleStepSize);
}

void ScrollBar::moveScrollbarInPages (const int howManyPages)
{
    setCurrentRange (visibleRange + howManyPages * visibleRange.getLength());
}

void ScrollBar::scrollToTop()
{
    setCurrentRange (visibleRange.movedToStartAt (getMinimumRangeLimit()));
}

void ScrollBar::scrollToBottom()
{
    setCurrentRange (visibleRange.movedToEndAt (getMaximumRangeLimit()));
}

void ScrollBar::setButtonRepeatSpeed (const int initialDelayInMillisecs_,
                                      const int repeatDelayInMillisecs_,
                                      const int minimumDelayInMillisecs_)
{
    initialDelayInMillisecs = initialDelayInMillisecs_;
    repeatDelayInMillisecs = repeatDelayInMillisecs_;
    minimumDelayInMillisecs = minimumDelayInMillisecs_;

    if (upButton != 0)
    {
        upButton->setRepeatSpeed (initialDelayInMillisecs,  repeatDelayInMillisecs,  minimumDelayInMillisecs);
        downButton->setRepeatSpeed (initialDelayInMillisecs,  repeatDelayInMillisecs,  minimumDelayInMillisecs);
    }
}

//==============================================================================
void ScrollBar::addListener (ScrollBarListener* const listener)
{
    listeners.add (listener);
}

void ScrollBar::removeListener (ScrollBarListener* const listener)
{
    listeners.remove (listener);
}

void ScrollBar::handleAsyncUpdate()
{
    double start = visibleRange.getStart(); // (need to use a temp variable for VC7 compatibility)
    listeners.call (&ScrollBarListener::scrollBarMoved, this, start);
}

//==============================================================================
void ScrollBar::updateThumbPosition()
{
    int newThumbSize = roundToInt (totalRange.getLength() > 0 ? (visibleRange.getLength() * thumbAreaSize) / totalRange.getLength()
                                                              : thumbAreaSize);

    if (newThumbSize < getLookAndFeel().getMinimumScrollbarThumbSize (*this))
        newThumbSize = jmin (getLookAndFeel().getMinimumScrollbarThumbSize (*this), thumbAreaSize - 1);

    if (newThumbSize > thumbAreaSize)
        newThumbSize = thumbAreaSize;

    int newThumbStart = thumbAreaStart;

    if (totalRange.getLength() > visibleRange.getLength())
        newThumbStart += roundToInt (((visibleRange.getStart() - totalRange.getStart()) * (thumbAreaSize - newThumbSize))
                                         / (totalRange.getLength() - visibleRange.getLength()));

    setVisible (alwaysVisible || (totalRange.getLength() > visibleRange.getLength() && visibleRange.getLength() > 0.0));

    if (thumbStart != newThumbStart  || thumbSize != newThumbSize)
    {
        const int repaintStart = jmin (thumbStart, newThumbStart) - 4;
        const int repaintSize = jmax (thumbStart + thumbSize, newThumbStart + newThumbSize) + 8 - repaintStart;

        if (vertical)
            repaint (0, repaintStart, getWidth(), repaintSize);
        else
            repaint (repaintStart, 0, repaintSize, getHeight());

        thumbStart = newThumbStart;
        thumbSize = newThumbSize;
    }
}

void ScrollBar::setOrientation (const bool shouldBeVertical)
{
    if (vertical != shouldBeVertical)
    {
        vertical = shouldBeVertical;

        if (upButton != 0)
        {
            ((ScrollbarButton*) upButton)->direction    = (vertical) ? 0 : 3;
            ((ScrollbarButton*) downButton)->direction  = (vertical) ? 2 : 1;
        }

        updateThumbPosition();
    }
}

void ScrollBar::setButtonVisibility (const bool buttonsAreVisible)
{
    deleteAndZero (upButton);
    deleteAndZero (downButton);

    if (buttonsAreVisible)
    {
        addAndMakeVisible (upButton   = new ScrollbarButton ((vertical) ? 0 : 3, *this));
        addAndMakeVisible (downButton = new ScrollbarButton ((vertical) ? 2 : 1, *this));

        setButtonRepeatSpeed (initialDelayInMillisecs, repeatDelayInMillisecs, minimumDelayInMillisecs);
    }

    updateThumbPosition();
}

void ScrollBar::setAutoHide (const bool shouldHideWhenFullRange)
{
    alwaysVisible = ! shouldHideWhenFullRange;
    updateThumbPosition();
}

//==============================================================================
void ScrollBar::paint (Graphics& g)
{
    if (thumbAreaSize > 0)
    {
        LookAndFeel& lf = getLookAndFeel();

        const int thumb = (thumbAreaSize > lf.getMinimumScrollbarThumbSize (*this))
                                ? thumbSize : 0;

        if (vertical)
        {
            lf.drawScrollbar (g, *this,
                              0, thumbAreaStart,
                              getWidth(), thumbAreaSize,
                              vertical,
                              thumbStart, thumb,
                              isMouseOver(), isMouseButtonDown());
        }
        else
        {
            lf.drawScrollbar (g, *this,
                              thumbAreaStart, 0,
                              thumbAreaSize, getHeight(),
                              vertical,
                              thumbStart, thumb,
                              isMouseOver(), isMouseButtonDown());
        }
    }
}

void ScrollBar::lookAndFeelChanged()
{
    setComponentEffect (getLookAndFeel().getScrollbarEffect());
}

void ScrollBar::resized()
{
    const int length = ((vertical) ? getHeight() : getWidth());

    const int buttonSize = (upButton != 0) ? jmin (getLookAndFeel().getScrollbarButtonSize (*this), (length >> 1))
                                           : 0;

    if (length < 32 + getLookAndFeel().getMinimumScrollbarThumbSize (*this))
    {
        thumbAreaStart = length >> 1;
        thumbAreaSize = 0;
    }
    else
    {
        thumbAreaStart = buttonSize;
        thumbAreaSize = length - (buttonSize << 1);
    }

    if (upButton != 0)
    {
        if (vertical)
        {
            upButton->setBounds (0, 0, getWidth(), buttonSize);
            downButton->setBounds (0, thumbAreaStart + thumbAreaSize, getWidth(), buttonSize);
        }
        else
        {
            upButton->setBounds (0, 0, buttonSize, getHeight());
            downButton->setBounds (thumbAreaStart + thumbAreaSize, 0, buttonSize, getHeight());
        }
    }

    updateThumbPosition();
}

void ScrollBar::mouseDown (const MouseEvent& e)
{
    isDraggingThumb = false;
    lastMousePos = vertical ? e.y : e.x;
    dragStartMousePos = lastMousePos;
    dragStartRange = visibleRange.getStart();

    if (dragStartMousePos < thumbStart)
    {
        moveScrollbarInPages (-1);
        startTimer (400);
    }
    else if (dragStartMousePos >= thumbStart + thumbSize)
    {
        moveScrollbarInPages (1);
        startTimer (400);
    }
    else
    {
        isDraggingThumb = (thumbAreaSize > getLookAndFeel().getMinimumScrollbarThumbSize (*this))
                            && (thumbAreaSize > thumbSize);
    }
}

void ScrollBar::mouseDrag (const MouseEvent& e)
{
    if (isDraggingThumb)
    {
        const int deltaPixels = ((vertical) ? e.y : e.x) - dragStartMousePos;

        setCurrentRangeStart (dragStartRange
                                + deltaPixels * (totalRange.getLength() - visibleRange.getLength())
                                    / (thumbAreaSize - thumbSize));
    }
    else
    {
        lastMousePos = (vertical) ? e.y : e.x;
    }
}

void ScrollBar::mouseUp (const MouseEvent&)
{
    isDraggingThumb = false;
    stopTimer();
    repaint();
}

void ScrollBar::mouseWheelMove (const MouseEvent&,
                                float wheelIncrementX,
                                float wheelIncrementY)
{
    float increment = vertical ? wheelIncrementY : wheelIncrementX;

    if (increment < 0)
        increment = jmin (increment * 10.0f, -1.0f);
    else if (increment > 0)
        increment = jmax (increment * 10.0f, 1.0f);

    setCurrentRange (visibleRange - singleStepSize * increment);
}

void ScrollBar::timerCallback()
{
    if (isMouseButtonDown())
    {
        startTimer (40);

        if (lastMousePos < thumbStart)
            setCurrentRange (visibleRange - visibleRange.getLength());
        else if (lastMousePos > thumbStart + thumbSize)
            setCurrentRangeStart (visibleRange.getEnd());
    }
    else
    {
        stopTimer();
    }
}

bool ScrollBar::keyPressed (const KeyPress& key)
{
    if (! isVisible())
        return false;

    if (key.isKeyCode (KeyPress::upKey) || key.isKeyCode (KeyPress::leftKey))
        moveScrollbarInSteps (-1);
    else if (key.isKeyCode (KeyPress::downKey) || key.isKeyCode (KeyPress::rightKey))
        moveScrollbarInSteps (1);
    else if (key.isKeyCode (KeyPress::pageUpKey))
        moveScrollbarInPages (-1);
    else if (key.isKeyCode (KeyPress::pageDownKey))
        moveScrollbarInPages (1);
    else if (key.isKeyCode (KeyPress::homeKey))
        scrollToTop();
    else if (key.isKeyCode (KeyPress::endKey))
        scrollToBottom();
    else
        return false;

    return true;
}

END_JUCE_NAMESPACE

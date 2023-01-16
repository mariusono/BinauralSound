/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BinauralSoundAudioProcessorEditor::BinauralSoundAudioProcessorEditor (BinauralSoundAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
    
    
    addAndMakeVisible(gAzimuth_Slider);
    gAzimuth_Slider.setTextValueSuffix(" [deg]");
    gAzimuth_Slider.addListener(this);
    gAzimuth_Slider.setRange(-89.0,89.0);
    gAzimuth_Slider.setValue(0.0);
    addAndMakeVisible(gAzimuth_Label);
    gAzimuth_Label.setText("Azimuth", juce::dontSendNotification);
    gAzimuth_Label.attachToComponent(&gAzimuth_Slider, true);

    gAzimuth_SliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts,"AZIMUTH",gAzimuth_Slider);
    
    addAndMakeVisible(gElevation_Slider);
    gElevation_Slider.setTextValueSuffix(" [deg]");
    gElevation_Slider.addListener(this);
    gElevation_Slider.setRange(-180.0,180.0);
    gElevation_Slider.setValue(0.0);
    addAndMakeVisible(gElevation_Label);
    gElevation_Label.setText("Elevation", juce::dontSendNotification);
    gElevation_Label.attachToComponent(&gElevation_Slider, true);

    gElevation_SliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts,"ELEVATION",gElevation_Slider);
    
    addAndMakeVisible(gVolume_Slider);
    gVolume_Slider.setTextValueSuffix(" [dB]");
    gVolume_Slider.addListener(this);
    gVolume_Slider.setRange(-20.0,20.0);
    gVolume_Slider.setValue(0.0);
    addAndMakeVisible(gVolume_Label);
    gVolume_Label.setText("Volume", juce::dontSendNotification);
    gVolume_Label.attachToComponent(&gVolume_Slider, true);

    gVolume_SliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts,"VOLUME",gVolume_Slider);
}

BinauralSoundAudioProcessorEditor::~BinauralSoundAudioProcessorEditor()
{
}

//==============================================================================
void BinauralSoundAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

//    g.setColour (juce::Colours::white);
//    g.setFont (15.0f);
//    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void BinauralSoundAudioProcessorEditor::resized()
{
    auto sliderLeft = 120;

    gAzimuth_Slider.setBounds(sliderLeft, 20, getWidth() - sliderLeft - 10, 20);
    gElevation_Slider.setBounds(sliderLeft, 80, getWidth() - sliderLeft - 10, 20);
    gVolume_Slider.setBounds(sliderLeft, 80+60, getWidth() - sliderLeft - 10, 20);
}


void BinauralSoundAudioProcessorEditor::sliderValueChanged(Slider* slider)
{

    if (slider == &gAzimuth_Slider)
    {
        audioProcessor.set_gAzimuth_param(gAzimuth_Slider.getValue(),audioProcessor.gAzimuth_param_prev);
//        DBG(String(gFB_Slider.getValue()));
    }
    else if (slider == &gElevation_Slider)
    {
        audioProcessor.set_gElevation_param(gElevation_Slider.getValue(),audioProcessor.gElevation_param_prev);
    }
    else if (slider == &gVolume_Slider)
    {
        audioProcessor.set_gVolume_param(gVolume_Slider.getValue());
    }
}


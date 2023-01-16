/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class BinauralSoundAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    BinauralSoundAudioProcessor();
    ~BinauralSoundAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    
    
    //==============================================================================
    // FOR PARAMETERS !
    juce::AudioProcessorValueTreeState apvts;
    
//    void set_gAzimuth_param(float val) { gAzimuth_param = val; }
    void set_gAzimuth_param(float val, float &gAzimuth_param_prev)
    {
        gAzimuth_param = (1-0.8)*val + 0.8*gAzimuth_param_prev;
        gAzimuth_param_prev = gAzimuth_param;
    }
//    void set_gElevation_param(float val) { gElevation_param = val; }
    void set_gElevation_param(float val, float &gElevation_param_prev) // I guess this could just be done in PluginEditor..
    {
        gElevation_param = (1-0.8)*val + 0.8*gElevation_param_prev;
        gElevation_param_prev = gElevation_param;
    }
    
    void set_gVolume_param(float val) { gVolume_param = val; }

    
    // Parameters updated in PluginEditor -> need to be public.
    float gAzimuth_param_prev;
    float gElevation_param_prev;
    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BinauralSoundAudioProcessor)
    
    //==============================================================================
    // BUFFER STUFF
    int BUFFER_SIZE = 16384; // size of delay buffers
    
    int gInitLatency = 16; // initial latency to account for negative delays.

    std::vector<std::vector<float>> gDelayBuffer; // delay buffer from input
    std::vector<int> gWritePointer; // write pointer for delay buffer from input
    std::vector<int> gReadPointer; // read pointer for delay buffer from input
    
    std::vector<std::vector<float>> gDelayBuffer_head_shaddow; // delay buffer loaded from head shadow model
    std::vector<int> gWritePointer_head_shadow; // write pointer for delay buffer loaded from head shadow model
    std::vector<int> gReadPointer_head_shadow; // read pointer for delay buffer loaded from head shadow model
    
    
    //==============================================================================
    // HEAD MODEL STUFF
    float a = 8.75/100; // radius of head
    float c = 343; // speed of sound
    
    float alpha_min = 0.1; // head shadow filter param
    float theta_min = 150; // head shadow filter param
    float theta_min_rad; // head shadow filter param

    
    //==============================================================================
    // AUDIO PARAMS
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        
        params.push_back(std::make_unique<AudioParameterFloat>("AZIMUTH","Azimuth",-89.0,89.0f,0.0f));
        params.push_back(std::make_unique<AudioParameterFloat>("ELEVATION","Elevation",-180.0f,180.0f,0.0f));
        params.push_back(std::make_unique<AudioParameterFloat>("VOLUME","Volume",-20.0f,20.0f,0.0f)); // in dB

        return { params.begin(), params.end()};
    }
    
    float gAzimuth_param;
    float gElevation_param;
    float gVolume_param;


    float thetaLeft;
    float thetaRight;

    float gSampleRate;
    float T;
    
    
    // Outputs
    std::vector<float> outVal_room, outVal, outVal_prev, outVal_head_shadow, outVal_head_shadow_prev, outVal_post_pinnae;
    
};

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BinauralSoundAudioProcessor::BinauralSoundAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParameters())
#endif
{
}

BinauralSoundAudioProcessor::~BinauralSoundAudioProcessor()
{
}

//==============================================================================
const juce::String BinauralSoundAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BinauralSoundAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BinauralSoundAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BinauralSoundAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BinauralSoundAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BinauralSoundAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BinauralSoundAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BinauralSoundAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String BinauralSoundAudioProcessor::getProgramName (int index)
{
    return {};
}

void BinauralSoundAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void BinauralSoundAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

    // Print sample rate -- for checking purposes
    gSampleRate = sampleRate;
    T = 1/gSampleRate;
    Logger::getCurrentLogger()->outputDebugString("Sample rate is " + String(sampleRate) + ".");
    
    // Read parameters from sliders
    auto* selection_gAzimuth = dynamic_cast<AudioParameterFloat*> (apvts.getParameter ("AZIMUTH"));
    gAzimuth_param = selection_gAzimuth->get();
    gAzimuth_param = 0.0; // overwriting.. just in case
    
    auto* selection_gElevation = dynamic_cast<AudioParameterFloat*> (apvts.getParameter ("ELEVATION"));
    gElevation_param = selection_gElevation->get();
    gElevation_param = 0.0; // overwriting.. just in case
    
    
    gVolume_param = 0.0;
    
    // Resizing buffers and preallocating read and write pointers
    gDelayBuffer.resize(2); // 2 channels
    for (int i = 0; i < 2; ++i)
        gDelayBuffer[i].resize(BUFFER_SIZE,0);
    
    gDelayBuffer_head_shaddow.resize(2); // 2 channels
    for (int i = 0; i < 2; ++i)
        gDelayBuffer_head_shaddow[i].resize(BUFFER_SIZE,0);
    
    gInitLatency = 16;
    
    gWritePointer.resize(2,gInitLatency);
    gReadPointer.resize(2,0);
    
    gWritePointer_head_shadow.resize(2,gInitLatency);
    gReadPointer_head_shadow.resize(2,0);
    
    
    // OTHER
    theta_min_rad = theta_min*float_Pi/180.0;
    
    outVal_room.resize(2,0);
    outVal.resize(2,0);
    outVal_prev.resize(2,0);
    outVal_head_shadow.resize(2,0);
    outVal_head_shadow_prev.resize(2,0);
    outVal_post_pinnae.resize(2,0);
    
}

void BinauralSoundAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BinauralSoundAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void BinauralSoundAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    float* const outputL = buffer.getWritePointer(0);
    float* const outputR = buffer.getWritePointer(1);

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    
    //Logger::getCurrentLogger()->outputDebugString("gAzimuth_param is " + String(gAzimuth_param) + ".");
    //Logger::getCurrentLogger()->outputDebugString("gElevation_param is " + String(gElevation_param) + ".");

//    for (int channel = 0; channel < totalNumInputChannels; ++channel)
//    for (int channel = 0; channel < 2; ++channel) // Hardcoded to 2 channels
    for (int channel = 1; channel > -1; --channel) // doing this because im overwriting the first channel after going through the first iteration of the loop !
    {
//        auto* input = buffer.getWritePointer (channel);
        auto* input = buffer.getWritePointer (0); // HARDCODED : always take the left channel.. to avoidn stereo problems

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Current input sample
            float in = input[i];

            // Update parameters for sound source position
            thetaLeft = 90.0 + gAzimuth_param;
            thetaRight = 90.0 - gAzimuth_param;
            
            float theta;
            if (channel == 0)
                theta = thetaLeft;
            else if (channel == 1)
                theta = thetaRight;
            
            float theta_rad =  theta*float_Pi/180;
            
            float delta_T;
            if (0<=abs(theta_rad) && abs(theta_rad)<float_Pi/2)
                delta_T = (-a/c)*cos(theta_rad);
            else if (float_Pi/2 <= abs(theta_rad) && abs(theta_rad) < float_Pi)
                delta_T = (a/c)*(abs(theta_rad)-float_Pi/2);
            
            
            // Room model
            float Kr = 1;
            float dB_difference = 15; // add a slider for this !


            float response_db_Kr = 20 * log10(Kr);
            float Ke_db = response_db_Kr - dB_difference;

            float Ke_ampl = pow(10,(Ke_db/20));

            float tau_Ke = 15*0.001;
            float tau_Ke_samples = floorf(tau_Ke*gSampleRate);
            float tau_Ke_samples_frac = tau_Ke*gSampleRate - tau_Ke_samples;


            // Populate buffer
            gDelayBuffer[channel][gWritePointer[channel]] = in;

            // Convert delay to samples
            float delSamples = delta_T * gSampleRate;
            float delSamples_floor = floorf(delSamples);
            float frac_part = delSamples - delSamples_floor;
            
            // Read from delay line
            int outPointer = (gReadPointer[channel] - 1 - static_cast<int>(delSamples_floor) + BUFFER_SIZE) % BUFFER_SIZE;
            int outPointer_frac = (gReadPointer[channel] - static_cast<int>(delSamples_floor) + BUFFER_SIZE) % BUFFER_SIZE;
            
            outVal[channel] = frac_part*gDelayBuffer[channel][outPointer] + (1-frac_part)*gDelayBuffer[channel][outPointer_frac];
            

            int outPointer_room = (gReadPointer[channel] - 1 - static_cast<int>(tau_Ke_samples) + BUFFER_SIZE) % BUFFER_SIZE;
            int outPointer_room_frac = (gReadPointer[channel] - static_cast<int>(tau_Ke_samples) + BUFFER_SIZE) % BUFFER_SIZE;

            outVal_room[channel] = Ke_ampl * (tau_Ke_samples_frac*gDelayBuffer[channel][outPointer_room] + (1-tau_Ke_samples_frac)*gDelayBuffer[channel][outPointer_room_frac]);
            

            // HEAD SHADOW FILTER
            float alpha = (1+alpha_min/2) + (1-alpha_min/2)*cos(theta_rad/theta_min_rad * float_Pi);
            float beta = 2*c/a;

            outVal_head_shadow[channel] = (2*alpha + T*beta)/(2+T*beta) * outVal[channel] + (-2*alpha + T*beta)/(2+T*beta) * outVal_prev[channel] - (-2 + T*beta)/(2+T*beta) * outVal_head_shadow_prev[channel];

            outVal_prev[channel] = outVal[channel];
            outVal_head_shadow_prev[channel] = outVal_head_shadow[channel];

            // PINNA MODEL
            std::vector<float> rho_k = {0.5,-1,0.5,-0.25,0.25};
            std::vector<float> Ak = {1,5,5,5,5};
            std::vector<float> Bk = {2,4,7,11,13};
            
            std::vector<float> Dk1 = {1,0.5,0.5,0.5,0.5};
            //std::vector<float>  Dk2 = {0.85,0.35,0.35,0.35,0.35}; // alternative scaling factors. see paper Duda and Brown "A Structural Model for Binaural Sound Synthesis"

            std::vector<float> tau;
            std::vector<float> tau_samples;
            std::vector<float> tau_samples_frac_part;
            tau.resize(5,0.0);
            tau_samples.resize(5,0.0);
            tau_samples_frac_part.resize(5,0.0);
            for (int iEvent = 0; iEvent < 5; iEvent++)
            {
                tau[iEvent] = Ak[iEvent]*cos(theta_rad/2)*sin(Dk1[iEvent]*(float_Pi/2-gElevation_param*float_Pi/180))+Bk[iEvent];
                tau_samples[iEvent] = floorf(tau[iEvent]);
                tau_samples_frac_part[iEvent] = tau[iEvent] - tau_samples[iEvent];
            }
            
            
            // Write to delayLine
            gDelayBuffer_head_shaddow[channel][gWritePointer_head_shadow[channel]] = outVal_head_shadow[channel];
            
            int outPointer_1 = (gReadPointer_head_shadow[channel] - 1 - static_cast<int>(tau_samples[0]) + BUFFER_SIZE) % BUFFER_SIZE;
            int outPointer_1_frac = (gReadPointer_head_shadow[channel] - static_cast<int>(tau_samples[0]) + BUFFER_SIZE) % BUFFER_SIZE;

            int outPointer_2 = (gReadPointer_head_shadow[channel] - 1 - static_cast<int>(tau_samples[1]) + BUFFER_SIZE) % BUFFER_SIZE;
            int outPointer_2_frac = (gReadPointer_head_shadow[channel] - static_cast<int>(tau_samples[1]) + BUFFER_SIZE) % BUFFER_SIZE;
            
            int outPointer_3 = (gReadPointer_head_shadow[channel] - 1 - static_cast<int>(tau_samples[2]) + BUFFER_SIZE) % BUFFER_SIZE;
            int outPointer_3_frac = (gReadPointer_head_shadow[channel] - static_cast<int>(tau_samples[2]) + BUFFER_SIZE) % BUFFER_SIZE;
            
            int outPointer_4 = (gReadPointer_head_shadow[channel] - 1 - static_cast<int>(tau_samples[3]) + BUFFER_SIZE) % BUFFER_SIZE;
            int outPointer_4_frac = (gReadPointer_head_shadow[channel] - static_cast<int>(tau_samples[3]) + BUFFER_SIZE) % BUFFER_SIZE;
            
            int outPointer_5 = (gReadPointer_head_shadow[channel] - 1 - static_cast<int>(tau_samples[4]) + BUFFER_SIZE) % BUFFER_SIZE;
            int outPointer_5_frac = (gReadPointer_head_shadow[channel] - static_cast<int>(tau_samples[4]) + BUFFER_SIZE) % BUFFER_SIZE;
            
            
            outVal_post_pinnae[channel] =
            rho_k[0] * (tau_samples_frac_part[0]*gDelayBuffer_head_shaddow[channel][outPointer_1] + (1-tau_samples_frac_part[0])*gDelayBuffer_head_shaddow[channel][outPointer_1_frac])
            + rho_k[1] * (tau_samples_frac_part[1]*gDelayBuffer_head_shaddow[channel][outPointer_2] + (1-tau_samples_frac_part[1])*gDelayBuffer_head_shaddow[channel][outPointer_2_frac])
            + rho_k[2] * (tau_samples_frac_part[2]*gDelayBuffer_head_shaddow[channel][outPointer_3] + (1-tau_samples_frac_part[2])*gDelayBuffer_head_shaddow[channel][outPointer_3_frac])
            + rho_k[3] * (tau_samples_frac_part[3]*gDelayBuffer_head_shaddow[channel][outPointer_4] + (1-tau_samples_frac_part[3])*gDelayBuffer_head_shaddow[channel][outPointer_4_frac])
            + rho_k[4] * (tau_samples_frac_part[4]*gDelayBuffer_head_shaddow[channel][outPointer_5] + (1-tau_samples_frac_part[4])*gDelayBuffer_head_shaddow[channel][outPointer_5_frac]);
                            
            // update gWritePointer
            gWritePointer[channel] = gWritePointer[channel] + 1;
            if (gWritePointer[channel] >= BUFFER_SIZE)
                gWritePointer[channel] = 0;

            // update gReadPointer
            gReadPointer[channel] = gReadPointer[channel] + 1;
            if (gReadPointer[channel] >= BUFFER_SIZE)
                gReadPointer[channel] = 0;
            
            // update gWritePointer_head_shadow
            gWritePointer_head_shadow[channel] = gWritePointer_head_shadow[channel] + 1;
            if (gWritePointer_head_shadow[channel] >= BUFFER_SIZE)
                gWritePointer_head_shadow[channel] = 0;

            // update gReadPointer_head_shadow
            gReadPointer_head_shadow[channel] = gReadPointer_head_shadow[channel] + 1;
            if (gReadPointer_head_shadow[channel] >= BUFFER_SIZE)
                gReadPointer_head_shadow[channel] = 0;
            
            if (abs((outVal_post_pinnae[channel])) > 1)
            {
                Logger::getCurrentLogger()->outputDebugString("Output is too loud!");
            }
            
            if (channel == 0)
            {
                outputL[i] = (outVal_post_pinnae[channel] + outVal_room[channel]) * powf(10,(gVolume_param/20));
//                outputL[i] = (outVal_post_pinnae[channel]) * 1.0;
//                outputL[i] = (outVal_head_shadow[channel]) * 1.0;
            }
            else if (channel == 1)
            {
                outputR[i] = (outVal_post_pinnae[channel] + outVal_room[channel]) * powf(10,(gVolume_param/20));
//                outputR[i] = (outVal_post_pinnae[channel]) * 1.0;
//                outputR[i] = (outVal_head_shadow[channel]) * 1.0;
            }

        }
        // ..do something to the data...
    }
}

//==============================================================================
bool BinauralSoundAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BinauralSoundAudioProcessor::createEditor()
{
    return new BinauralSoundAudioProcessorEditor (*this);
}

//==============================================================================
void BinauralSoundAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void BinauralSoundAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BinauralSoundAudioProcessor();
}

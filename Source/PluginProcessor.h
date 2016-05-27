/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "WDL/WDL/convoengine.h"    // Tale edition of WDL for threaded conv
#include "r8brain-free-src/CDSPResampler.h"


//==============================================================================
/**
*/
class BalanceFlipsideAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    BalanceFlipsideAudioProcessor();
    ~BalanceFlipsideAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool setPreferredBusArrangement (bool isInput, int bus, const AudioChannelSet& preferredSet) override;
   #endif

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Returns destination length
    inline int ResampleLength(int src_len, double src_srate, double dest_srate) const
    {
        return int(dest_srate / src_srate * (double)src_len + 0.5);
    }

    template <class I, class O> void Resample(const I* src, int src_len, double src_srate, O* dest, int dest_len, double dest_srate);

private:
    double sampleRate;
    double impulseSampleRate;

    WDL_ImpulseBuffer wdlImpulse;
    WDL_ConvolutionEngine_Thread wdlEngine;

    static const int blockLength = 64;

    r8b::CDSPResampler24* r8bResampler;                                         // remove pointer?

    ScopedPointer<AudioSampleBuffer> impulseJuceAudioSampleBuffer;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BalanceFlipsideAudioProcessor)
};


#endif  // PLUGINPROCESSOR_H_INCLUDED

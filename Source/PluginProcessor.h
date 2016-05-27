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

private:
    double playbackSampleRate {0.0};
    double impulseSampleRate {0.0};

    WDL_ImpulseBuffer wdlImpulseL;
    WDL_ImpulseBuffer wdlImpulseR;
    WDL_ConvolutionEngine_Thread wdlEngineL;
    WDL_ConvolutionEngine_Thread wdlEngineR;

    static const int r8bBlockLength {64};

    ScopedPointer<r8b::CDSPResampler24> r8bResampler {nullptr};

    ScopedPointer<AudioSampleBuffer> impulseJuceAudioSampleBufferL {nullptr};
    ScopedPointer<AudioSampleBuffer> impulseJuceAudioSampleBufferR {nullptr};

    // Return resampling dest length
    inline int resampleLength(int src_len, double src_srate, double dest_srate) const
    {
        return int(dest_srate / src_srate * (double)src_len + 0.5);
    }

    template <class I, class O>
    void resample (const I* src, int src_len, double src_srate, O* dest, int dest_len, double dest_srate);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BalanceFlipsideAudioProcessor)
};


#endif  // PLUGINPROCESSOR_H_INCLUDED

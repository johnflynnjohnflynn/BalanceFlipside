/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
BalanceFlipsideAudioProcessor::BalanceFlipsideAudioProcessor()
{
    // Put WAV impulses into Juce AudioFormatReaders
    WavAudioFormat wav;
    MemoryInputStream* misL {new MemoryInputStream {BinaryData::flipsidetsL_wav, BinaryData::flipsidetsL_wavSize, false}};
    MemoryInputStream* misR {new MemoryInputStream {BinaryData::flipsidetsR_wav, BinaryData::flipsidetsR_wavSize, false}};
    ScopedPointer<AudioFormatReader> audioReaderL {wav.createReaderFor (misL, true)};
    ScopedPointer<AudioFormatReader> audioReaderR {wav.createReaderFor (misR, true)};

    // Put AudioFormatReader into our IR AudioSampleBuffers                                         // 4 samples? interp?
    impulseJuceAudioSampleBufferL = new AudioSampleBuffer (audioReaderL->numChannels, audioReaderL->lengthInSamples + 4);
    impulseJuceAudioSampleBufferR = new AudioSampleBuffer (audioReaderR->numChannels, audioReaderR->lengthInSamples + 4);
    audioReaderL->read (impulseJuceAudioSampleBufferL, 0, audioReaderL->lengthInSamples + 4, 0, true, true);
    audioReaderR->read (impulseJuceAudioSampleBufferR, 0, audioReaderR->lengthInSamples + 4, 0, true, true);

    jassert (audioReaderL->sampleRate == audioReaderR->sampleRate); // sanity
    impulseSampleRate = audioReaderR->sampleRate;

    wdlEngine.EnableThread(true);
}

BalanceFlipsideAudioProcessor::~BalanceFlipsideAudioProcessor()
{
}

//==============================================================================
const String BalanceFlipsideAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BalanceFlipsideAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BalanceFlipsideAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

double BalanceFlipsideAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BalanceFlipsideAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BalanceFlipsideAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BalanceFlipsideAudioProcessor::setCurrentProgram (int index)
{
}

const String BalanceFlipsideAudioProcessor::getProgramName (int index)
{
    return String();
}

void BalanceFlipsideAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void BalanceFlipsideAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Detect a change in sample rate.
    if (playbackSampleRate != sampleRate)
    {
        playbackSampleRate = sampleRate;

        wdlImpulse.SetNumChannels(2);

        if (r8bResampler) delete r8bResampler;                                                              // is this safe?
        r8bResampler = new r8b::CDSPResampler24 {impulseSampleRate, playbackSampleRate, r8bBlockLength};

        // Resample the impulse response.
        jassert (impulseJuceAudioSampleBufferL->getNumSamples() == impulseJuceAudioSampleBufferR->getNumSamples());
        int len = resampleLength (impulseJuceAudioSampleBufferL->getNumSamples(), impulseSampleRate, playbackSampleRate);
        wdlImpulse.SetLength (len);
        if (len)
        {
            resample (impulseJuceAudioSampleBufferL->getReadPointer(0),
                      impulseJuceAudioSampleBufferL->getNumSamples(),
                      impulseSampleRate,
                      wdlImpulse.impulses[0].Get(),
                      len,
                      playbackSampleRate);
            resample (impulseJuceAudioSampleBufferR->getReadPointer(0),
                      impulseJuceAudioSampleBufferR->getNumSamples(),
                      impulseSampleRate,
                      wdlImpulse.impulses[1].Get(),
                      len,
                      playbackSampleRate);
        }

        // Tie the impulse response to the convolution engine.
        wdlEngine.SetImpulse(&wdlImpulse);
    }

    printf("Latency %i Samples\n", wdlEngine.GetLatency());
}

void BalanceFlipsideAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BalanceFlipsideAudioProcessor::setPreferredBusArrangement (bool isInput, int bus, const AudioChannelSet& preferredSet)
{
    // Reject any bus arrangements that are not compatible with your plugin

    const int numChannels = preferredSet.size();

   #if JucePlugin_IsMidiEffect
    if (numChannels != 0)
        return false;
   #elif JucePlugin_IsSynth
    if (isInput || (numChannels != 1 && numChannels != 2))
        return false;
   #else
    if (numChannels != 1 && numChannels != 2)
        return false;

    if (! AudioProcessor::setPreferredBusArrangement (! isInput, bus, preferredSet))
        return false;
   #endif

    return AudioProcessor::setPreferredBusArrangement (isInput, bus, preferredSet);
}
#endif

void BalanceFlipsideAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    /*for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }*/

    // Send input samples to the convolution engine.
    wdlEngine.Add (buffer.getArrayOfWritePointers(), buffer.getNumSamples(), 2);

    const float* inL = buffer.getReadPointer(0);
    const float* inR = buffer.getReadPointer(1);
    float *outL = buffer.getWritePointer(0);
    float *outR = buffer.getWritePointer(1);

    // find number available samples for the engine
    int numAvail = std::min (wdlEngine.Avail(buffer.getNumSamples()), buffer.getNumSamples());

    // If not enough samples are available yet, then only output the dry
    // signal.
    for (int i = 0; i < buffer.getNumSamples() - numAvail; ++i)             // why -numAvail??
    {
        *outL++ = *inL++;
        *outR++ = *inR++;
    }

    // Output samples from the convolution engine.
    if (numAvail > 0)
    {
        // Apply the convolution
        WDL_FFT_REAL* convoL = wdlEngine.Get()[0];
        WDL_FFT_REAL* convoR = wdlEngine.Get()[1];
        for (int i = 0; i < numAvail; ++i)
        {
            *outL++ = *convoL++;                         // convoluted only
            *outR++ = *convoR++;
        }

        // Remove the sample block from the convolution engine's buffer.
        wdlEngine.Advance(numAvail);
    }
}

//==============================================================================
bool BalanceFlipsideAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* BalanceFlipsideAudioProcessor::createEditor()
{
    return new BalanceFlipsideAudioProcessorEditor (*this);
}

//==============================================================================
void BalanceFlipsideAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void BalanceFlipsideAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BalanceFlipsideAudioProcessor();
}

//==============================================================================
template <class I, class O>
void BalanceFlipsideAudioProcessor::resample(const I* src, int src_len, double src_srate, O* dest, int dest_len, double dest_srate)
{
    if (dest_len == src_len)
    {
        // Copy
        for (int i = 0; i < dest_len; ++i) *dest++ = (O)*src++;
        return;
    }

    // Resample using r8brain-free-src.
    double scale = src_srate / dest_srate;
    while (dest_len > 0)
    {
        double buf[r8bBlockLength], *p = buf;
        int n = r8bBlockLength;
        if (n > src_len) n = src_len;
        for (int i = 0; i < n; ++i) *p++ = (double)*src++;
        if (n < r8bBlockLength) memset(p, 0, (r8bBlockLength - n) * sizeof(double));
        src_len -= n;
        
        n = r8bResampler->process(buf, r8bBlockLength, p);
        if (n > dest_len) n = dest_len;
        for (int i = 0; i < n; ++i) *dest++ = (O)(scale * *p++);
        dest_len -= n;
    }
    
    r8bResampler->clear();
}

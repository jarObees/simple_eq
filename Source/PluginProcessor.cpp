/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Simple_eqAudioProcessor::Simple_eqAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

Simple_eqAudioProcessor::~Simple_eqAudioProcessor()
{
}

//==============================================================================
const juce::String Simple_eqAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Simple_eqAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Simple_eqAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Simple_eqAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Simple_eqAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Simple_eqAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Simple_eqAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Simple_eqAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Simple_eqAudioProcessor::getProgramName (int index)
{
    return {};
}

void Simple_eqAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Simple_eqAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Struct that contains: samples per block, number of channels, and the sample rate.
    // We need this data to "prepare" our filters.
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);
}

void Simple_eqAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Simple_eqAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void Simple_eqAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(buffer); // Simply wrapping the buffer data in a more usable class. 

    auto leftBlock = block.getSingleChannelBlock(0); // Extracting the left channel of the previous AudioBlock.
    auto rightBlock = block.getSingleChannelBlock(1); // Right channel.

    // Part of the processing context. It is 'Replacing' since input/output buffers are the same; they are being
    // They are processed in place. Otherwise, if we were sending the buffers to another place, we would use ::ProcessContextNonReplacing
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    // Run dat shit
    leftChain.process(leftContext);
    rightChain.process(rightContext);

}

//=============================================================================
bool Simple_eqAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Simple_eqAudioProcessor::createEditor()
{
    //return new Simple_eqAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void Simple_eqAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Simple_eqAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}
juce::AudioProcessorValueTreeState::ParameterLayout Simple_eqAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("Low Cut Freq", "Low Cut Freq", juce::NormalisableRange<float>(20.f, 2000.f, 1.f, 1.f), 20.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("High Cut Freq", "High Cut Freq", juce::NormalisableRange<float>(20.f, 2000.f, 1.f, 1.f), 20000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("Peak", "Peak", juce::NormalisableRange<float>(20.f, 2000.f, 1.f, 1.f), 750.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("PeakGain", "PeakGain", juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("PeakQuality", "Peak Quality", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f));

    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }
    layout.add(std::make_unique <juce::AudioParameterChoice>("Lowcut Slope", "Lowcut Slope", stringArray, 0));
    layout.add(std::make_unique <juce::AudioParameterChoice>("HighCut Slope", "Highcut Slope", stringArray, 0));
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Simple_eqAudioProcessor();
}

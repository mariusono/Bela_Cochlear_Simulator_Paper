// Bela libraries
#include <Bela.h>
#include <I2c_Codec.h>
#include <Midi.h>

#include <libraries/Scope/Scope.h>
#include <libraries/Gui/Gui.h>
#include "MonoFilePlayer.h"

// Standard libraries
#include <cmath>
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <random>

// Butterworth/Biquad stuff
#include "Butterworth.h"
#include "BiquadFilter.h"
#include "BiquadChain.h"

// Miscellaneous functions
#include "miscFunctions.h"


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------

int useMIDI = 1;
int useSAMPLE = 0;
int useFixedGains = 1;

// Override the default sample rate

//float gSAMPLE_RATE = 22050; // Can run at 22050 Hz also.. 
float gSAMPLE_RATE = 16000;
static int frameCount = 0;
static int frameCountClip = 0;

// Browser-based GUI to adjust parameters
Gui gui;

// Initialize a scope for debugging
Scope gScope;

// Initialize MIDI library
Midi midi;

// Create an instance of MonoFilePlayer
MonoFilePlayer gPlayer_in_sample;

// Chosen sample file (already resampled at 16000 Hz)
//std::string gFilenameSample = "Kapsalon_sampled_at_16000_hz.wav";
//std::string gFilenameSample = "swedish_whisper_sampled_at_16000_hz.wav";

//std::string gFilenameSample = "Giovanni_original.wav";
std::string gFilenameSample = "Giovanni_remixed.wav";
//std::string gFilenameSample = "Giovanni_reduced_PCA_8.wav";
 

// Number of channels, etc.
int noChannels = 8;
int noActiveElectrodes = 6;
int filterOrder = 6;
int low_filterOrder = 4;

// Bandpass frequency range
float fLow = 200.0f;
float fHigh = 7000.0f;

// Keep track of which channels are currently active (for GUI)
std::vector<float> bufferActiveElements(noChannels, 0.0f);
std::vector<float> bufferClipChan(noChannels, 0.0f);
float clipMain = 0.0f;

// Storage vectors for filter outputs
std::vector<float> out_mat; 
std::vector<float> out_mat_rect; 
std::vector<float> out_mat_low; 

// PSHC
//std::vector<std::vector<float>> pshc_wavetable_vals;
std::vector<std::vector<std::vector<float>>> pshc_wavetable_vals;
std::vector<float> pshc_vals;
std::vector<int> pshc_tableIndices;
std::vector<int> pshc_selectedInstances;
int samplesWavetable;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<int> dist(0, 49);

int selInstance = 0;
	
// Misc
int BUFFER_SIZE = 2048;
std::vector<std::vector<float>> gOutputBuffer_eachChannel;
std::vector<std::vector<float>> gOutputBuffer_eachChannel_after_bandpass; // buffer containing the 
std::vector<std::vector<float>> gOutputBuffer_PSHC_eachChannel;
std::vector<float> RMS_values_envelopes;
std::vector<float> RMS_values_synthesis;
std::vector<float> RMS_values_after_bandpass;
std::vector<float> outSynthesis;
std::vector<float> hannWin;
std::vector<float> outRingBuffer;
std::vector<std::vector<float>> outSynthesis_per_channel;

std::vector<float> gInputGain_pointer;
std::vector<float> midiPotNumbers;
std::vector<float> midiLEDNumbers;
std::vector<float> midiLEDNumbersClipChan;
float midiLEDNumberClipMain;

int gOutputWinWritePointer;
int readIndexOutRingBuffer;
float windowTime;
int windowSize;
int hopSize;
int hopCounter;
int outRingBufferSize;
int writeIndexOutRingBuffer;
float inVal;

float amplitude = 1.0f;


// Frequency bands
std::vector<double> hz_points;
std::vector<double> freq_bandpass_low;
std::vector<double> freq_bandpass_high;
std::vector<double> centerFreqs;

// Lowpass filter
std::vector<double> lowPass_filt_freq_cutoff;
std::vector<std::vector<Biquad>> coeffs_lowPass_filt;
Butterworth butterworth_lowpass;
std::vector<double> overallGain_lowPass_filt;


// Bandpass filters
std::vector<std::vector<Biquad>> coeffs_vec;
Butterworth butterworth;
std::vector<double> overallGain_vec;

// Biquad chains - bandpass for analysis
std::vector<std::unique_ptr<BiquadFilter>> biquadsPointer_bandPass;
std::vector<std::vector<std::unique_ptr<BiquadFilter>>> biquadsPointer_vec_bandPass;
BiquadChain biquadChain_bandPass(biquadsPointer_bandPass, filterOrder);
std::vector<BiquadChain> biquadChain_vec_bandPass(1, biquadChain_bandPass);

// Biquad chains - lowpass for analysis
std::vector<std::unique_ptr<BiquadFilter>> biquadsPointer_lowPass;
std::vector<std::vector<std::unique_ptr<BiquadFilter>>> biquadsPointer_vec_lowPass;
BiquadChain biquadChain_lowPass(biquadsPointer_lowPass, low_filterOrder / 2);
std::vector<BiquadChain> biquadChain_vec_lowPass(1, biquadChain_lowPass);

// Biquad chains - bandpass for resynthesis
std::vector<std::unique_ptr<BiquadFilter>> biquadsPointer_bandPass_post_resynthesis;
std::vector<std::vector<std::unique_ptr<BiquadFilter>>> biquadsPointer_vec_bandPass_post_resynthesis;
BiquadChain biquadChain_bandPass_post_resynthesis(biquadsPointer_bandPass_post_resynthesis, filterOrder);
std::vector<BiquadChain> biquadChain_vec_bandPass_post_resynthesis(1, biquadChain_bandPass_post_resynthesis);

// -----------------------------------------------------------------------------
// setup()
// -----------------------------------------------------------------------------

bool setup(BelaContext *context, void *userData)
{
    char cwd[1024];
    if(getcwd(cwd, sizeof(cwd)) != NULL) {
        std::cout << "Current working dir: " << cwd << std::endl;
    } else {
        std::cerr << "Error getting current working directory." << std::endl;
    }
		    
		    
    inVal = 0;

    // Setup PSHC
    float f0_PSHC = 1.0f;
    samplesWavetable = static_cast<int>(gSAMPLE_RATE / f0_PSHC);

    windowSize = 128; 
    hopCounter = windowSize / 2;
    hopSize = windowSize / 2;
    outRingBufferSize = windowSize * 4;

    writeIndexOutRingBuffer = 0;
    readIndexOutRingBuffer = 0;

    //lowPass_filt_freq_cutoff = 200;
    lowPass_filt_freq_cutoff = {39., 53., 75., 106., 151., 200., 200., 200}; // HARDCODED , comes from python script: make_PSHC_thru_gammatone_filters_save_binary.py

    // Set up the GUI
    gui.setup(context->projectName);

    // Reserve space in the GUI buffers
    gui.setBuffer('f', noChannels); // index 0: input gains
    gui.setBuffer('f', noChannels); // index 1: output gains
    gui.setBuffer('f', 1);         // index 2: amplitude slider

    // Load the sample file
    if(!gPlayer_in_sample.setup(gFilenameSample)) {
        rt_printf("Error loading audio file '%s'\n", gFilenameSample.c_str());
        return false;
    }
    
    // READ THE MIDI 
    midi.readFrom(0);           // Try port 0 first
    midi.writeTo(0);   // send outgoing MIDI back to MidiMix
    midi.enableParser(true);
    rt_printf("MIDI parser enabled.\n");
    rt_printf("Setup complete. Pot #16 -> frequency, pot #17 -> amplitude.\n");
    
    midiPotNumbers = {19, 23, 27, 31, 49, 53, 57, 61}; // Hardcoded for first row of Akai Midimix and first 4 pots of second row
    midiLEDNumbers = {1,4,7,10,13,16,19,22}; // Hardcoded for first row of Akai Midimix and first 4 pots of second row
   
	midiLEDNumbersClipChan = {3,6,9,12,15,18,21,24};
	midiLEDNumberClipMain = 26;
	
	midi.writeNoteOn(0, midiLEDNumberClipMain, 0);


	for (int iChannel = 0; iChannel<noChannels; iChannel++)
	{
    	midi.writeNoteOn(0, midiLEDNumbers[iChannel], 0);
		midi.writeNoteOn(0, midiLEDNumbersClipChan[iChannel], 0);
	}

    // Compute band edges and center frequencies
    BinFrequencies freqs = getBinCenterFrequencies(fLow, fHigh, noChannels);
    hz_points = freqs.edges;
    centerFreqs = freqs.centers;

    freq_bandpass_low  = slice(hz_points, 0, (int)hz_points.size() - 1);
    freq_bandpass_high = slice(hz_points, 1, (int)hz_points.size());

    rt_printf("freq_bandpass_low size %d\n", (int)freq_bandpass_low.size());
    for(int i = 0; i < (int)hz_points.size() - 1; i++)
        rt_printf("freq_bandpass %f,%f\n", freq_bandpass_low[i], freq_bandpass_high[i]);

    for(int i = 0; i < (int)centerFreqs.size(); i++)
        rt_printf("centerFreqs %f\n", centerFreqs[i]);

    // Create bandpass Butterworth coefficients for each channel
    for(int i = 0; i < noChannels; i++)
    {
        std::vector<Biquad> biquad_coeff;
        coeffs_vec.push_back(biquad_coeff);
        overallGain_vec.push_back(0.0);

        butterworth.bandPass(gSAMPLE_RATE,
                             freq_bandpass_high[i],
                             freq_bandpass_low[i],
                             filterOrder,
                             coeffs_vec[i],
                             overallGain_vec[i]);
    }

    // Create bandpass BiquadChains
    for(int j = 0; j < noChannels; j++)
    {
        std::vector<std::unique_ptr<BiquadFilter>> chainVec;
        std::vector<std::unique_ptr<BiquadFilter>> chainVec_post_resynthesis;
        for(int i = 0; i < filterOrder; i++)
        {
            chainVec.push_back(std::make_unique<BiquadFilter>(coeffs_vec[j][i].b0,
                                                              coeffs_vec[j][i].b1,
                                                              coeffs_vec[j][i].b2,
                                                              1.0,
                                                              coeffs_vec[j][i].a1 * (-1),
                                                              coeffs_vec[j][i].a2 * (-1)));
                                                              
            chainVec_post_resynthesis.push_back(std::make_unique<BiquadFilter>(coeffs_vec[j][i].b0,
                                                              coeffs_vec[j][i].b1,
                                                              coeffs_vec[j][i].b2,
                                                              1.0,
                                                              coeffs_vec[j][i].a1 * (-1),
                                                              coeffs_vec[j][i].a2 * (-1)));
        }
        biquadsPointer_vec_bandPass.push_back(std::move(chainVec));
        biquadsPointer_vec_bandPass_post_resynthesis.push_back(std::move(chainVec_post_resynthesis));
    }

    biquadChain_vec_bandPass.clear();
    biquadChain_vec_bandPass_post_resynthesis.clear();
    for(int j = 0; j < noChannels; j++)
    {
        BiquadChain bc(biquadsPointer_vec_bandPass[j], filterOrder);
        biquadChain_vec_bandPass.push_back(bc);
        
        BiquadChain bc_post_resynthesis(biquadsPointer_vec_bandPass_post_resynthesis[j], filterOrder);
        biquadChain_vec_bandPass_post_resynthesis.push_back(bc_post_resynthesis);
    }
    rt_printf("Number of bandpass BiquadChains: %d\n", (int)biquadChain_vec_bandPass.size());
    rt_printf("Number of bandpass BiquadChains post resynthesis: %d\n", (int)biquadChain_vec_bandPass_post_resynthesis.size());



    // Create lowpass Butterworth coefficients for each channel
    for(int i = 0; i < noChannels; i++)
    {
        std::vector<Biquad> biquad_coeff;
        coeffs_lowPass_filt.push_back(biquad_coeff);
        overallGain_lowPass_filt.push_back(0.0);

        butterworth.loPass(gSAMPLE_RATE,
                             lowPass_filt_freq_cutoff[i],
                             0,
                             low_filterOrder,
                             coeffs_lowPass_filt[i],
                             overallGain_lowPass_filt[i]);
    }


    // Create lowpass BiquadChains
    for(int j = 0; j < noChannels; j++)
    {
        std::vector<std::unique_ptr<BiquadFilter>> chainVec;
        for(int i = 0; i < (low_filterOrder / 2); i++)
        {
            chainVec.push_back(std::make_unique<BiquadFilter>(coeffs_lowPass_filt[j][i].b0,
                                                              coeffs_lowPass_filt[j][i].b1,
                                                              coeffs_lowPass_filt[j][i].b2,
                                                              1.0,
                                                              coeffs_lowPass_filt[j][i].a1 * (-1),
                                                              coeffs_lowPass_filt[j][i].a2 * (-1)));
        }
        biquadsPointer_vec_lowPass.push_back(std::move(chainVec));
    }

    biquadChain_vec_lowPass.clear();
    for(int j = 0; j < noChannels; j++)
    {
        BiquadChain bc(biquadsPointer_vec_lowPass[j], low_filterOrder / 2);
        biquadChain_vec_lowPass.push_back(bc);
    }

    rt_printf("Number of lowpass BiquadChains: %d\n", (int)biquadChain_vec_lowPass.size());

    // Preallocate main working vectors
    out_mat.resize(noChannels, 0);
    out_mat_rect.resize(noChannels, 0);
    out_mat_low.resize(noChannels, 0);
    pshc_vals.resize(noChannels, 0);

    gOutputBuffer_eachChannel.clear();
    gOutputBuffer_eachChannel.resize(noChannels, std::vector<float>(BUFFER_SIZE, 0.0f));

    gOutputBuffer_eachChannel_after_bandpass.clear();
    gOutputBuffer_eachChannel_after_bandpass.resize(noChannels, std::vector<float>(BUFFER_SIZE, 0.0f));

    gOutputBuffer_PSHC_eachChannel.clear();
    gOutputBuffer_PSHC_eachChannel.resize(noChannels, std::vector<float>(BUFFER_SIZE, 0.0f));

    RMS_values_envelopes.resize(noChannels, 0.0f);
    RMS_values_synthesis.resize(noChannels, 0.0f);
    RMS_values_after_bandpass.resize(noChannels, 0.0f);
    outSynthesis.resize(windowSize, 0.0f);
    outRingBuffer.resize(outRingBufferSize, 0.0f);
    
    outSynthesis_per_channel.clear();
    outSynthesis_per_channel.resize(noChannels, std::vector<float>(windowSize, 0.0f));

	gInputGain_pointer.resize(noChannels,1.0f);

    // Hann window
    hannWin = hannWindow(windowSize);


    
    pshc_wavetable_vals.clear();
	pshc_wavetable_vals.resize(noChannels);
	int numInstances = 50;
	
	// For each channel, create `numInstances` vectors,
	// each holding `samplesWavetable` floats.
	for(int ch = 0; ch < noChannels; ch++)
	{
	    pshc_wavetable_vals[ch].resize(numInstances);
	    for(int inst = 0; inst < numInstances; inst++)
	    {
	        pshc_wavetable_vals[ch][inst].resize(samplesWavetable, 0.0f);
	    }
	}
	
	// For each instance:
	for(int iInstance = 0; iInstance < numInstances; iInstance++)
	{
	    // For each channel:
	    for(int i = 0; i < noChannels; i++)
	    {
    
	        // Build the filename
	        std::string folderPath = "PSHC_bin_files/";
	        std::string filename = folderPath +
	                               "normalized_slice_ch" + std::to_string(i) +
	                               "_inst_" + std::to_string(iInstance) + ".bin";
	
	        std::ifstream file(filename, std::ios::binary);
	        if(!file)
	        {
	            std::cerr << "Error opening file: " << filename << "\n";
	            continue;
	        }
	
	        // Read binary floats into pshc_wavetable_vals[i][iInstance]
	        file.read(
	            reinterpret_cast<char*>(pshc_wavetable_vals[i][iInstance].data()),
	            pshc_wavetable_vals[i][iInstance].size() * sizeof(float)
	        );
	
	        if(!file)
	            std::cerr << "Error reading file: " << filename << "\n";
	    }
	}		

    

    // Print first few elements of channel 4 (arbitrary example)
    rt_printf("First 10 elements of channel 4's PSHC wavetable instance 1:\n");
    for(int i = 0; i < 10 && i < (int)pshc_wavetable_vals[4][0].size(); i++)
        rt_printf("pshc_wavetable_vals[4][0][i]: %f\n",pshc_wavetable_vals[4][0][i]);
        
    rt_printf("First 10 elements of channel 4's PSHC wavetable instance 2:\n");
    for(int i = 0; i < 10 && i < (int)pshc_wavetable_vals[4][1].size(); i++)
        rt_printf("pshc_wavetable_vals[4][1][i]: %f\n",pshc_wavetable_vals[4][1][i]);
        


    // Initialize PSHC table indices
    pshc_tableIndices.clear();
    pshc_tableIndices.resize(noChannels, 0);
    
    pshc_selectedInstances.clear();
    pshc_selectedInstances.resize(noChannels, 0);

    // Initialise the Bela oscilloscope
    gScope.setup(1, gSAMPLE_RATE);
    gScope.setup(2, gSAMPLE_RATE);

    rt_printf("Setup completed.\n");
    return true;
}


// -----------------------------------------------------------------------------
// render()
// -----------------------------------------------------------------------------

void render(BelaContext *context, void *userData)
{

    // Retrieve buffers from GUI
    DataBuffer& buffer_inputGain  = gui.getDataBuffer(0);
    DataBuffer& amplitude_data    = gui.getDataBuffer(1);

    float* inputGain_pointer  = buffer_inputGain.getAsFloat();
    float* amplitude_pointer  = amplitude_data.getAsFloat();

    // Send bufferActiveElements to the GUI so we can visualize active electrodes
    gui.sendBuffer(0, bufferActiveElements);
    
    
    // Process any MIDI messages 
    if (useMIDI == 1)
    {
    	while(midi.getParser()->numAvailableMessages() > 0) 
    	{
	        MidiChannelMessage message = midi.getParser()->getNextChannelMessage();
	        if(message.getType() == kmmControlChange) {
	            int potNum = message.getDataByte(0);
	            int potVal = message.getDataByte(1);
	            
	            for(int iChannel = 0; iChannel < noChannels; iChannel++)
	            {
		            if(potNum == midiPotNumbers[iChannel]) {
		                // Map 0..127 to 110..880 for frequency
		                float normVal = (float)potVal / 127.0f;
		                float valAmp_db = linearMapping(-40.0f, 6.0f, 0.0f, 1.0f, normVal);
						float valAmp_lin = db_to_amp(valAmp_db);
				        if(valAmp_lin <= 0.012f)
					        valAmp_lin = 0.0f;
		                
		                gInputGain_pointer[iChannel] = valAmp_lin;
		                
		                //rt_printf("MIDI: Input gain at pot %d changed to %f\n", potNum, valAmp_db);
		                rt_printf("MIDI: Input gain at pot %d changed to %f\n", potNum, gInputGain_pointer[iChannel]);
		            }
	            }
	            if(potNum == 62) {
					float normVal = (float)potVal / 127.0f;
					float valAmp_db = linearMapping(-40.0f, 9.0f, 0.0f, 1.0f, normVal);
					amplitude = db_to_amp(valAmp_db);
			        if(amplitude <= 0.012f)
				        amplitude = 0.0f;
				        
	                rt_printf("MIDI: Input gain at pot %d changed to %f\n", potNum, valAmp_db);
	                rt_printf("MIDI: Input gain at pot %d changed to %f\n", potNum, amplitude);
	            }
	        }
		}
    }
    

    

	// Important part ! changes the sample rate of the program. 
	// context->audioSampleRate will still show as 44100 but the actual sample rate is gSAMPLE_RATE. 
	// Did some tests to check this. u need this version if bela tho: better-i2c-rebased-2022 from https://github.com/BelaPlatform/Bela 
    if(context->audioFramesElapsed == 0)
    {
        extern AudioCodec* gAudioCodec;
        // Override sample rate in the codec
        ((I2c_Codec*)gAudioCodec)->setAudioSamplingRate(gSAMPLE_RATE);
        rt_printf("Context's default sample rate: %f (overridden to %f)\n",
                  context->audioSampleRate, gSAMPLE_RATE);
    }

    // Convert amplitude from dB to linear
    if (useMIDI != 1)
    {
	    float amplitudeDB = *amplitude_pointer;
	    amplitude   = powf(10.0, amplitudeDB / 20.0f);
        if(amplitude <= 0.012f)
	        amplitude = 0.0f;
    }
    
    if (useFixedGains == 1)
    {
    	if (useMIDI != 1)
    	{
	    	amplitude = 1.0;
    	}
        for(int iChannel = 0; iChannel < noChannels; iChannel++)
        {
            gInputGain_pointer[iChannel] = 1.0f;
        }
    }


    for(unsigned int n = 0; n < context->audioFrames; n++)
    {
    	if (useSAMPLE == 1)
    	{
    		inVal = gPlayer_in_sample.process(); // Get input sample from file
    	}
    	else
    	{
    		inVal = audioRead(context, n, 0);  // Read mono input from audio in . channel 0..
    	}

        // Process each channel
        for(int iChannel = 0; iChannel < noChannels; iChannel++)
        {
            // Bandpass
            if (useMIDI == 1) 
            {
	            out_mat[iChannel] = biquadChain_vec_bandPass[iChannel].processSample(inVal) * overallGain_vec[iChannel]
					                * gInputGain_pointer[iChannel];
            }
            else
            {
	            out_mat[iChannel] = biquadChain_vec_bandPass[iChannel].processSample(inVal) * overallGain_vec[iChannel]
					                * inputGain_pointer[iChannel];
            }


            gOutputBuffer_eachChannel_after_bandpass[iChannel][gOutputWinWritePointer] = out_mat[iChannel];

            // Rectify
            //// Full-wave rectification (absolute)
			//out_mat_rect[iChannel] = std::abs(out_mat[iChannel]);

			// Half-wave rectification (positive)
			if(out_mat[iChannel] < 0.0f)
			{
			    out_mat_rect[iChannel] = 0.0f;
			}
			else
			{
			    out_mat_rect[iChannel] = out_mat[iChannel];
			}

            // Lowpass
            out_mat_low[iChannel] =
                biquadChain_vec_lowPass[iChannel].processSample(out_mat_rect[iChannel])
                * overallGain_lowPass_filt[iChannel];

            gOutputBuffer_eachChannel[iChannel][gOutputWinWritePointer] = out_mat_low[iChannel];

            // PSHC lookup
            pshc_vals[iChannel] = pshc_wavetable_vals[iChannel][pshc_selectedInstances[iChannel]][pshc_tableIndices[iChannel]];
            pshc_tableIndices[iChannel]++;
            if(pshc_tableIndices[iChannel] >= samplesWavetable)
            {
            	pshc_selectedInstances[iChannel] = dist(gen); // When finishing the wavetable select another instance. 
            	//pshc_selectedInstances[iChannel] = 0; // just use the first instance of PSHCs always..  
                pshc_tableIndices[iChannel] = 0;
            }
                
            gOutputBuffer_PSHC_eachChannel[iChannel][gOutputWinWritePointer] = pshc_vals[iChannel];
        }

        // Windowing/hop logic
        if(hopCounter == hopSize)
        {
            std::fill(outSynthesis.begin(), outSynthesis.end(), 0.0f);

            for(int iChannel = 0; iChannel < noChannels; iChannel++)
			{
	            std::fill(outSynthesis_per_channel[iChannel].begin(), outSynthesis_per_channel[iChannel].end(), 0.0f);
	            
			    int indexStart = (gOutputWinWritePointer - windowSize + BUFFER_SIZE) % BUFFER_SIZE;

			    for(int iWin = 0; iWin < windowSize; iWin++)
			    {				
			        int idx = (indexStart + iWin) % BUFFER_SIZE;
					outSynthesis_per_channel[iChannel][iWin] = biquadChain_vec_bandPass_post_resynthesis[iChannel].processSample
																(gOutputBuffer_eachChannel[iChannel][idx] * gOutputBuffer_PSHC_eachChannel[iChannel][idx]) 
																* overallGain_vec[iChannel];
			    }
			}
            
            // Determine which channels are “active” (RMS of the envelopes of each bandpass channel)
            for(int iChannel = 0; iChannel < noChannels; iChannel++)
			{
			    int indexStart = (gOutputWinWritePointer - windowSize + BUFFER_SIZE) % BUFFER_SIZE;
			
			    float sumSq = 0.0f;
			    for(int iWin = 0; iWin < windowSize; iWin++)
			    {
			        int idx = (indexStart + iWin) % BUFFER_SIZE;
			      
			        float val = gOutputBuffer_eachChannel[iChannel][idx];  // don't think i need outputGain anymore.. 
			        sumSq += val * val;  // Sum of squares
			    }
			    float rms = sqrt(sumSq / windowSize);  // RMS value
			    RMS_values_envelopes[iChannel] = rms;
			}
			
            for(int iChannel = 0; iChannel < noChannels; iChannel++)
			{
			    float sumSq = 0.0f;
			    for(int iWin = 0; iWin < windowSize; iWin++)
			    {
			        float val = outSynthesis_per_channel[iChannel][iWin];   
			        sumSq += val * val;  // Sum of squares
			    }
			    float rms = sqrt(sumSq / windowSize);  // RMS value
			    RMS_values_synthesis[iChannel] = rms;
			}
			
			// Compute RMS values of dry signal after bandpasses.
            for(int iChannel = 0; iChannel < noChannels; iChannel++)
			{
			    int indexStart = (gOutputWinWritePointer - windowSize + BUFFER_SIZE) % BUFFER_SIZE;
			
			    float sumSq = 0.0f;
			    for(int iWin = 0; iWin < windowSize; iWin++)
			    {
			        int idx = (indexStart + iWin) % BUFFER_SIZE;
			        float val = gOutputBuffer_eachChannel_after_bandpass[iChannel][idx];
			        sumSq += val * val;  // Sum of squares
			    }
			    float rms = sqrt(sumSq / windowSize);  // RMS value
			    RMS_values_after_bandpass[iChannel] = rms;
			}

            // Sort to find the channels with the highest peaks
            auto vals = argsort(RMS_values_envelopes);
            std::vector<int> vals_int(vals.begin(), vals.end());

            // Mark the active channels in bufferActiveElements
            for(int iSel = 0; iSel < noChannels; iSel++)
            {
            	bufferActiveElements[vals_int[iSel]] = 0;
                if(iSel >= (noChannels - noActiveElectrodes))
                {
				    if(gInputGain_pointer[vals_int[iSel]] > 0.012f)
				    {
				        // Turn LED off
                        bufferActiveElements[vals_int[iSel]] = 1;
				    }
                }
            }
            
            //if (frameCount == 0)
            //{
            //	rt_printf("context->audioFrames is %d\n",context->audioFrames);
            //}

            // Construct outSynthesis
            for(int iSyn = 0; iSyn < windowSize; iSyn++)
            {
                for(int iSel = noChannels - noActiveElectrodes; iSel < noChannels; iSel++)
                {
                    int selCh = vals_int[iSel];
                    float equalizationFactor = (RMS_values_synthesis[selCh] > 1e-6f) ? (RMS_values_after_bandpass[selCh] / RMS_values_synthesis[selCh]) : 0.0f;
                    if (abs(outSynthesis_per_channel[selCh][iSyn] * equalizationFactor * amplitude) > 1.0f/2.0f)
                    {
                    	bufferClipChan[selCh] = 1;
                    	rt_printf("Cliping at channel %d\n",selCh);
                    }
					outSynthesis[iSyn] += outSynthesis_per_channel[selCh][iSyn] * equalizationFactor;
                }
                outSynthesis[iSyn] *= hannWin[iSyn]; // apply windowing
            }

            // Overlap-add
            for(int iSyn = 0; iSyn < windowSize; iSyn++)
            {
                int pos = (writeIndexOutRingBuffer + iSyn) % outRingBufferSize;
                outRingBuffer[pos] += outSynthesis[iSyn];
            }

            writeIndexOutRingBuffer = (writeIndexOutRingBuffer + hopSize) % outRingBufferSize;

            
            hopCounter = 0;
        }
        hopCounter++;
        


        // Read final output and reset that sample in ring buffer
        float outFinal = amplitude * outRingBuffer[readIndexOutRingBuffer];
        if (abs(outFinal) > 1.0)
        {
        	clipMain = 1;
			rt_printf("Cliping global out!\n");
        }
        outRingBuffer[readIndexOutRingBuffer] = 0.0f; // set sample to 0 after reading
        
        readIndexOutRingBuffer = (readIndexOutRingBuffer + 1) % outRingBufferSize; 

        gOutputWinWritePointer++;
        if(gOutputWinWritePointer >= BUFFER_SIZE)
            gOutputWinWritePointer = 0;

        // Write the output
        for(unsigned int channel = 0; channel < context->audioOutChannels; channel++)
            audioWrite(context, n, channel, outFinal);

		if (useMIDI == 1)
		{
			
		    frameCount += 1; 
		    frameCountClip += 1; 
		    
		    if(frameCountClip >= gSAMPLE_RATE / 4.0f) {
		        frameCountClip = 0;
	
		        if (clipMain == 1)
		        {
			        midi.writeNoteOn(0, midiLEDNumberClipMain, 127); 
			        clipMain = 0;
		        }
		        else
		        {
			        midi.writeNoteOn(0, midiLEDNumberClipMain, 0); 
				}
		        for(int i = 0; i < noChannels; i++)
		        {
					if(bufferClipChan[i] == 1)
					{
			            midi.writeNoteOn(0, midiLEDNumbersClipChan[i], 127); 
			            bufferClipChan[i] = 0;
					}
			        else
			        {
			            midi.writeNoteOn(0, midiLEDNumbersClipChan[i], 0);   	
			        }
		        }
		    }
	
	
		    if(frameCount >= gSAMPLE_RATE / 60.0f) {
		        frameCount = 0;

		        // Now do the LED update
		        for(int i = 0; i < noChannels; i++)
		        {
		            // If bufferActiveElements[i] is 1, turn LED on; otherwise off
		            if(bufferActiveElements[i] == 1)
		                midi.writeNoteOn(0, midiLEDNumbers[i], 127); 
		            else
		                midi.writeNoteOn(0, midiLEDNumbers[i], 0);   
		        }
		    }
		    
		}
		
        // Log to oscilloscope (optional but can help debug)
        gScope.log(outFinal);
        gScope.log(outFinal, inVal);
    }
}


// -----------------------------------------------------------------------------
// cleanup()
// -----------------------------------------------------------------------------

void cleanup(BelaContext *context, void *userData)
{
    // Nothing special to do here
}

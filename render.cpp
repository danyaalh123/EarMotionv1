#include <Bela.h>
#include <libraries/Fft/Fft.h>
#include <vector>
#include <cstdlib> // For system()
#include <string>  // For string manipulation

// FFT-related variables
Fft gFft;                                 // FFT processing object
const int gFftSize = 8192;                // FFT size in samples
std::vector<float> gInputBuffer(gFftSize, 0.0f); // Input buffer
int gInputBufferPointer = 0;              // Current position in buffer

float frequencyBin;                      // Frequency per bin
int alphaBandStartIndex, alphaBandEndIndex; // Indices for alpha band
int betaBandStartIndex, betaBandEndIndex;   // Indices for beta band

float accumulatedAlphaPower = 0, accumulatedBetaPower = 0;
int framesSinceLastSecond = 0;

// Setup function for initializing FFT and precomputing constants
bool setup(BelaContext *context, void *userData)
{
    gFft.setup(gFftSize);
    frequencyBin = context->audioSampleRate / gFftSize;

    // Calculate indices for alpha (8-13 Hz) and beta (13-30 Hz) bands
    alphaBandStartIndex = 8 / frequencyBin;
    alphaBandEndIndex = 13 / frequencyBin;
    betaBandStartIndex = 13 / frequencyBin;
    betaBandEndIndex = 30 / frequencyBin;

    return true;
}

// Function to process FFT on a buffer
void process_fft(BelaContext *context, std::vector<float> const& buffer)
{
    gFft.fft(buffer); // Perform FFT

    float alphaPower = 0, betaPower = 0;

    // Process only relevant frequency bands
    for(int i = alphaBandStartIndex; i <= betaBandEndIndex; i++) {
        float magnitude = gFft.fda(i);
        float power = magnitude * magnitude;

        if(i <= alphaBandEndIndex) { // Alpha band
            alphaPower += power;
        } else { // Beta band
            betaPower += power;
        }
    }

    accumulatedAlphaPower += alphaPower;
    accumulatedBetaPower += betaPower;
}

// Main rendering loop
void render(BelaContext *context, void *userData)
{
    for(unsigned int n = 0; n < context->audioFrames; n++) {
        float input = analogRead(context, n, 0); // Read audio input
        gInputBuffer[gInputBufferPointer] = input;
        gInputBufferPointer++;

        if(gInputBufferPointer >= gFftSize) {
            gInputBufferPointer = 0;
            process_fft(context, gInputBuffer);
        }

        framesSinceLastSecond++;
        if(framesSinceLastSecond >= context->audioSampleRate) {
            float averageAlphaBetaRatio = accumulatedAlphaPower / accumulatedBetaPower;
            // rt_printf("Average Alpha/Beta Ratio: %f\n", averageAlphaBetaRatio);
            // rt_printf(averageAlphaBetaRatio > 6 ? "User Calm\n" : "User Excited\n");
            // // // Create the CURL command with the random number
            std::string command = "curl -X POST -d \"data=" + std::to_string(int((averageAlphaBetaRatio))) + "\" https://www.earmotion.co.uk/eeg-data -k";
            
            // // Execute the CURL command
            system(command.c_str());


            framesSinceLastSecond = 0;
            accumulatedAlphaPower = 0;
            accumulatedBetaPower = 0;
        }
    }
}

// Cleanup function
void cleanup(BelaContext *context, void *userData)
{
    // Add any necessary cleanup code
}

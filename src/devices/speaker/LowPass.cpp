#include <cmath>
#include <vector>

#include "LowPass.hpp"


// Constructor - initialize the filter with default values
LowPassFilter::LowPassFilter() : x1(0), x2(0), y1(0), y2(0) {
    setCoefficients(1000, 44100); // Default: 1kHz cutoff at 44.1kHz sample rate
}

// Calculate filter coefficients based on cutoff frequency and sample rate
void LowPassFilter::setCoefficients(double cutoffFreq, double sampleRate) {
    // Prevent issues with extreme values
    cutoffFreq = std::fmax(10.0, std::fmin(cutoffFreq, sampleRate * 0.49));
    
    // Precompute values for efficiency
    double omega = 2.0 * M_PI * cutoffFreq / sampleRate;
    double cosOmega = std::cos(omega);
    double alpha = std::sin(omega) / (2.0 * 0.7071); // Q factor = 0.7071 (Butterworth)
    
    // Calculate filter coefficients
    b0 = (1.0 - cosOmega) / 2.0;
    b1 = 1.0 - cosOmega;
    b2 = (1.0 - cosOmega) / 2.0;
    a0 = 1.0 + alpha;
    a1 = -2.0 * cosOmega;
    a2 = 1.0 - alpha;
    
    // Normalize coefficients
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
    b0 /= a0;
    a0 = 1.0;
}

// Process a single sample
/* double LowPassFilter::process(double input) {
    // Calculate output
    double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    
    // Update state variables
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output;
} */

// Process a block of samples
std::vector<double> LowPassFilter::processBlock(const std::vector<double>& inputBlock) {
    std::vector<double> outputBlock(inputBlock.size());
    for (size_t i = 0; i < inputBlock.size(); i++) {
        outputBlock[i] = process(inputBlock[i]);
    }
    return outputBlock;
}

// Reset filter state
void LowPassFilter::reset() {
    x1 = x2 = y1 = y2 = 0.0;
}


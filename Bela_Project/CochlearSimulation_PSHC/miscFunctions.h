
#pragma once

#include <numeric> // for iota
#include <algorithm> // for sort.. 
#include <vector> // for mel filters! 
#include <cmath>// for mel filters!  
#include <random>
#include <iostream>

// MISC FUNCTIONS

// Structure to hold both centers and edges
struct BinFrequencies {
    std::vector<double> centers; // size = numBins
    std::vector<double> edges;   // size = numBins + 1
};

// Greenwood functions
double inverseGreenwood(double f) {
    const double A = 165.4;
    const double a = 2.1;
    const double K = 0.88;
    return std::log10((f / A) + K) / a;
}

double greenwood(double x) {
    const double A = 165.4;
    const double a = 2.1;
    const double K = 0.88;
    return A * (std::pow(10.0, a * x) - K);
}

// Convert a normalized value x ([0,1]) back to frequency [Hz] using Greenwood mapping
double freqFromNormGreenwood(double x, double f_low, double f_high) {
    double inv_low = inverseGreenwood(f_low);
    double inv_high = inverseGreenwood(f_high);
    double x_new = x * (inv_high - inv_low) + inv_low;
    return greenwood(x_new);
}

// Modified function that returns both centers and edges.
BinFrequencies getBinCenterFrequencies(double f_low, double f_high, int numBins)
{
    BinFrequencies result;
    // Compute edges: equally spaced in the normalized Greenwood [0,1] domain.
    result.edges.resize(numBins + 1);
    for (int i = 0; i <= numBins; i++) {
        double x_edge = double(i) / numBins;
        result.edges[i] = freqFromNormGreenwood(x_edge, f_low, f_high);
    }
    
    // Compute centers: geometric mean of adjacent edges.
    result.centers.resize(numBins);
    for (int i = 0; i < numBins; i++) {
        double f_edge_low = result.edges[i];
        double f_edge_high = result.edges[i + 1];
        result.centers[i] = std::sqrt(f_edge_low * f_edge_high);
    }
    
    return result;
}


//------------------------------------------------------------
std::vector<std::string> splitString(const std::string& str)
{
    std::vector<std::string> tokens;
 
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, '\n')) {
        tokens.push_back(token);
    }
 
    return tokens;
}

template<typename T>
T mag2db(T inVal)
{
	auto outVal = 20 * log10(inVal);
    return outVal;
}

template<typename T>
T db2mag(T inVal)
{
	auto outVal = pow(10.0, inVal / 20);	
    return outVal;
}

template<typename T>
std::vector<T> slice(std::vector<T> const &v, int m, int n)
{
    auto first = v.cbegin() + m;
    auto last = v.cbegin() + n + 1;
 
    std::vector<T> vec(first, last);
    return vec;
}

template<typename T>
std::vector<size_t> argsort(const std::vector<T> &array) {
    std::vector<size_t> indices(array.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(),
              [&array](int left, int right) -> bool {
                  // sort indices according to corresponding array element
                  return array[left] < array[right];
              });

    return indices;
}

template<typename T>
std::vector<double> linspace(T start_in, T end_in, int num_in)
{

  std::vector<double> linspaced;

  double start = static_cast<double>(start_in);
  double end = static_cast<double>(end_in);
  double num = static_cast<double>(num_in);

  if (num == 0) { return linspaced; }
  if (num == 1)
    {
      linspaced.push_back(start);
      return linspaced;
    }

  double delta = (end - start) / (num - 1);

  for(int i=0; i < num-1; ++i)
    {
      linspaced.push_back(start + delta * i);
    }
  linspaced.push_back(end); // I want to ensure that start and end
                            // are exactly the same as the input
  return linspaced;
}

std::vector<double> bandpassIntervals_melScale(int N_channel, float minVal, float maxVal){ // use 250 and 10000
    float low_freq_mel = (2595.0f * log10(1.0f + (minVal) / 700.0f));  // Convert Hz to Mel
    float high_freq_mel = (2595.0f * log10(1.0f + (maxVal) / 700.0f));  // Convert Hz to Mel

    // Equally spaced in Mel scale
    std::vector<double> mel_points = linspace(low_freq_mel, high_freq_mel, N_channel + 1);
    
    std::vector<double> hz_points(N_channel + 1,0);
    for (int i = 0; i<mel_points.size();i++)
    {
        hz_points[i] = round((700.0f * (powf(10,(mel_points[i] / 2595.0f)) - 1.0f)));  // Mel to Hz
    }
    return hz_points;
}


double hz_to_mel(double hz) {
    // Convert a value in Hertz to Mels
    return 2595 * std::log10(1 + hz / 700);
}

double mel_to_hz(double mel) {
    // Convert a value in Mels to Hertz
    return 700 * (std::pow(10, mel / 2595) - 1);
}

std::vector<std::vector<double>> mel_filter_bank(int num_filters, int num_fft, double sample_rate) {
    // Assume hz_to_mel and mel_to_hz functions are defined elsewhere

    double mel_min = hz_to_mel(0);
    double mel_max = hz_to_mel(sample_rate / 2);

    // Compute Mel points linearly spaced in Mel scale
    std::vector<double> mels(num_filters + 2);
    double mel_step = (mel_max - mel_min) / (num_filters + 1);
    for (int i = 0; i < num_filters + 2; ++i) {
        mels[i] = mel_min + mel_step * i;
    }

    std::vector<double> hertz(num_filters + 2);
    for (int i = 0; i < num_filters + 2; ++i) {
        hertz[i] = mel_to_hz(mels[i]);
    }

    // The points in FFT bins
    std::vector<int> bins(num_filters + 2);
    for (int i = 0; i < num_filters + 2; ++i) {
        bins[i] = std::floor((num_fft + 1) * hertz[i] / sample_rate);
    }

    // Create filter bank
    std::vector<std::vector<double>> fbank(num_filters, std::vector<double>(num_fft / 2 + 1, 0));
    for (int i = 1; i <= num_filters; ++i) {
        int left = bins[i - 1];
        int center = bins[i];
        int right = bins[i + 1];

        for (int j = left; j < center; ++j) {
            fbank[i - 1][j] = (j - bins[i - 1]) / (double)(bins[i] - bins[i - 1]);
        }
        for (int j = center; j < right; ++j) {
            fbank[i - 1][j] = (bins[i + 1] - j) / (double)(bins[i + 1] - bins[i]);
        }
    }

    return fbank;
}

std::vector<float> hannWindow(int size) {
	float PI = std::acos(-1.0);
	
    std::vector<float> window(size);

    for (int i = 0; i < size; ++i) {
        window[i] = 0.5 * (1 - std::cos(2 * PI * i / (size - 1)));
    }

    return window;
}

double amp_to_db(double amp, double ref = 1.0) {
    const double min_amp = 1e-10;
    return 20.0 * std::log10(std::max(amp, min_amp) / ref);
}

double db_to_amp(double db, double ref = 1.0) {
    return ref * std::pow(10.0, db / 20.0);
}

// Exponential Mapping Function
double exponentialMapping(double rangeOut_bottom, double rangeOut_top,
                          double rangeIn_bottom, double rangeIn_top,
                          double fac, double val) {
    // Map value to [0, 1]
    double valueMapped = (val - rangeIn_bottom) / (rangeIn_top - rangeIn_bottom);
    
    // Apply exponential curve
    double mapToExp = (std::exp(valueMapped * fac) - 1.0) / (std::exp(fac) - 1.0);
    
    // Map back to output range
    double newValue = rangeOut_bottom + (rangeOut_top - rangeOut_bottom) * mapToExp;
    return newValue;
}

// Linear Mapping Function
double linearMapping(double rangeOut_bottom, double rangeOut_top,
                     double rangeIn_bottom, double rangeIn_top,
                     double value) {
    // Linear mapping from input range to output range
    double newValue = rangeOut_bottom + (rangeOut_top - rangeOut_bottom) * (value - rangeIn_bottom) / (rangeIn_top - rangeIn_bottom);
    return newValue;
}

//------------------------------------------------------------
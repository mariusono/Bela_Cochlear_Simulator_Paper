import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import convolve
from gammatone.filters import make_erb_filters, erb_filterbank
import sounddevice as sd  

import os


script_dir = os.path.dirname(os.path.abspath(__file__))
savePath = os.path.join(script_dir, '..', 'PSHC_Gammatone_Carriers')
savePath = os.path.abspath(savePath)

if not os.path.exists(savePath):
    os.makedirs(savePath)

# Greenwood constants
A = 165.4
a = 2.1
K = 0.88

def GenPSHC_vectorized(f0, M, N, k, dur, fs):
    """
    Generate a pulse-spreading harmonic complex (PSHC).
    
    f0  : Fundamental frequency [Hz]
    M   : Lowest harmonic index
    N   : Highest harmonic index
    k   : Subcomplex order
    dur : Duration [s]
    fs  : Sampling rate [Hz]
    
    Returns:
      out : PSHC signal
      t   : time vector
    """
    t = np.linspace(0, dur, int(fs * dur), endpoint=False)
    out = np.zeros_like(t)

    HarmRank = np.arange(M, N + 1)
    j = HarmRank % k  # subcomplex index in [0..k-1]

    r = np.random.permutation(k)  # random perm of [0..k-1]
    u = np.random.rand(k)         # random offsets in [0..1)

    for idx, h in enumerate(HarmRank):
        j_idx = j[idx]
        phi = 2 * np.pi * ((h / (k**2)) * r[j_idx] + u[j_idx])
        delta = 2 * np.pi * f0 * h / fs
        phases = phi + delta * np.arange(len(t))
        out += np.sin(phases)

    return out, t

# Standard Greenwood functions:
def inverseGreenwood(f):
    """Convert frequency (Hz) to Greenwood coordinate."""
    return np.log10((f / A) + K) / a

def greenwood(x):
    """Convert Greenwood coordinate to frequency (Hz)."""
    return A * (10**(a * x) - K)


def normGreenwood(f):
    """Return normalized Greenwood value for frequency f in [f_low, f_high]."""
    return (inverseGreenwood(f) - inv_low) / (inv_high - inv_low)

def freqFromNormGreenwood(x):
    """Convert a normalized Greenwood value x (in [0,1]) back to frequency (Hz)
    by first un-normalizing and then applying the Greenwood function."""
    x_new = x * (inv_high - inv_low) + inv_low
    return greenwood(x_new)




fs  = 16000     # Sampling rate in Hz

# frequency range for CI simulation:
f_low = 200.0    # Hz maps to normalized 0
f_high = 7000.0  # Hz maps to normalized 1

num_bins = 8

num_realizations = 50

# normalized Greenwood endpoints:
inv_low = inverseGreenwood(f_low)
inv_high = inverseGreenwood(f_high)


# Split the normalized Greenwood range [0, 1] into 6 bins.
edges = np.linspace(0, 1, num_bins + 1)

# Compute each bin's edge frequency and then the center frequency
f_centers = []
f_edge_low_list = []
f_edge_high_list = []
for i in range(num_bins):
    f_edge_low = freqFromNormGreenwood(edges[i])
    f_edge_high = freqFromNormGreenwood(edges[i + 1])
    # geometric mean for the center frequency:
    center = np.sqrt(f_edge_low * f_edge_high)
    f_edge_low_list.append(f_edge_low)
    f_edge_high_list.append(f_edge_high)
    f_centers.append(center)

print("bin center frequencies:", f_centers)



for iRealization in range(0,num_realizations):
    
    pps_array = np.zeros(num_bins)
    for channel in range(0,num_bins):
        
        fcenter = f_centers[channel]
        
        filt = make_erb_filters(fs, fcenter)
        
        fkHz = fcenter/1000

        pps = 37 + 151 * fkHz + 0.17 * (fkHz**2)
        pps_array[channel] = pps
        k = int(np.round(np.sqrt(pps)))
            
        f0  = 1.0      
        
        M = int(np.floor(f_edge_low_list[channel] / f0))
        N = int(np.ceil(f_edge_high_list[channel] / f0))
        
        
        dur = 3/f0       # Duration in seconds
        
        # Generate the PSHC and time vector
        out, t = GenPSHC_vectorized(f0, M, N, k, dur, fs)
        
        segment = out[int(fs/f0):int(fs/f0*2)]
        
        # Apply the filterbank:
        gtone_output = erb_filterbank(out, filt)[0]
        
        max_val = np.max(gtone_output[int(fs/f0):int(fs/f0*2)])
        min_val = np.min(gtone_output[int(fs/f0):int(fs/f0*2)])
        fdiff = 1.0 / (max_val - min_val + 1e-12)
        normalized = 2 * (gtone_output - min_val) * fdiff - 1
                
        
        
        slice_data = normalized[int(fs/f0):int(fs/f0*2)]
        
        slice_data_32 = slice_data.astype(np.float32)
        slice_data_32.tofile(os.path.join(savePath, f"normalized_slice_ch{channel}_inst_{iRealization}.bin"))
        
        print(f"Saved normalized slice to 'normalized_slice_ch{channel}_inst_{iRealization}.bin' (channel {channel}, realization {iRealization})")
        

low_pass_cutoffs = pps_array/2
low_pass_cutoffs = np.round(low_pass_cutoffs)
low_pass_cutoffs[low_pass_cutoffs>200] = 200
print(f"low_pass cutoffs : {low_pass_cutoffs}")




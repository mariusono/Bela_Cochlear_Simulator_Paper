#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Mar  2 01:14:16 2025

@author: mariusonofrei
"""
import os
import numpy as np
import librosa
import soundfile as sf
from sklearn.decomposition import PCA
import matplotlib.pyplot as plt
import scipy.signal as sp
from matplotlib.colors import LinearSegmentedColormap

def pca_numpy(X, n_components):
    mean = np.mean(X, axis=0)
    X_centered = X - mean
    U, s, Vt = np.linalg.svd(X_centered, full_matrices=False)
    components = Vt[:n_components, :]
    X_reduced = np.dot(X_centered, components.T)
    n_samples = X.shape[0]
    explained_variance = (s**2) / (n_samples - 1)
    explained_variance = explained_variance[:n_components]
    total_variance = np.sum(np.var(X, axis=0))
    explained_variance_ratio = explained_variance / total_variance
    X_reconstructed = np.dot(X_reduced, components) + mean
    return {
        'X_reduced': X_reduced,
        'components': components,
        'mean': mean,
        'explained_variance': explained_variance,
        'explained_variance_ratio': explained_variance_ratio,
        'X_reconstructed': X_reconstructed
    }

def compute_rms(signal):
    return np.sqrt(np.mean(signal**2))

def compute_rms_db(signal, eps=1e-10):
    rms_val = compute_rms(signal)
    return 20 * np.log10(rms_val + eps)


script_dir = os.path.dirname(os.path.abspath(__file__))



# composer = 'Alberto'
# composer = 'Giorgio' 
composer = 'Giovanni'

# audio_path = os.path.join(script_dir, '..', 'Composers', 'Alberto_Novello', 'Alberto_Cosmic_Debris_Excerpt_Remix_Scaled.wav')
# audio_path = os.path.abspath(audio_path)

# audio_path = os.path.join(script_dir, '..', 'Composers', 'Giorgio_Pacorig', 'giorgio_where_is_my_mind_remixed_scaled.wav')
# audio_path = os.path.abspath(audio_path)

audio_path = os.path.join(script_dir, '..', 'Composers', 'Giovanni_Maier', 'Giovanni_remix_scaled.wav')
audio_path = os.path.abspath(audio_path)


y, sr = librosa.load(audio_path, sr=None)


target_sr = 16000
y = librosa.resample(y, orig_sr=sr, target_sr=target_sr)
sr = target_sr

y_scaled = y / np.max(np.abs(y))
y_remixed = y_scaled

# audio_path = os.path.join(script_dir, '..', 'Composers', 'Alberto_Novello', 'Alberto_Cosmic_Debris_Excerpt_Orig_Scaled.wav')
# audio_path = os.path.abspath(audio_path)

# audio_path = os.path.join(script_dir, '..', 'Composers', 'Giorgio_Pacorig', 'giorgio_where_is_my_mind_original_scaled.wav')
# audio_path = os.path.abspath(audio_path)

audio_path = os.path.join(script_dir, '..', 'Composers', 'Giovanni_Maier', 'Giovanni_original_scaled.wav')
audio_path = os.path.abspath(audio_path)

y, sr = librosa.load(audio_path, sr=None)


target_sr = 22050

y = librosa.resample(y, orig_sr=sr, target_sr=target_sr)
sr = target_sr

n_fft = 2048
hop_length = 512
frame_duration = hop_length / sr
L_time = int(round(0.3 / frame_duration))
freq_resolution = sr / n_fft
L_freq = int(round(200 / freq_resolution))

print("Time filter length (frames):", L_time)
print("Frequency filter length (bins):", L_freq)

harmonic, percussive = librosa.effects.hpss(y, kernel_size=(L_time, L_freq), n_fft=n_fft, hop_length=hop_length)


# plt.figure()
# plt.plot(y)
# plt.plot(harmonic)
# plt.plot(percussive)
# plt.show()

pca_sr = 16000
harmonic = librosa.resample(harmonic, orig_sr=sr, target_sr=pca_sr)
percussive = librosa.resample(percussive, orig_sr=sr, target_sr=pca_sr)
y = librosa.resample(y, orig_sr=sr, target_sr=pca_sr)
sr = pca_sr

# plt.figure()
# plt.plot(y - (harmonic + percussive))
# plt.show()

pre_emphasis_coeff = 0.95
y_pre = sp.lfilter([1, -pre_emphasis_coeff], [1], harmonic)


fmin = 55
n_octaves = 7
bins_per_octave = 24
n_bins = n_octaves * bins_per_octave
hop_length = int(0.002 * sr)


C = librosa.cqt(y_pre, sr=sr, hop_length=hop_length, fmin=fmin, n_bins=n_bins, bins_per_octave=bins_per_octave, window='hann')
magnitude = np.abs(C)
phase = np.angle(C)

n_frames = C.shape[1]
times = librosa.frames_to_time(np.arange(n_frames), sr=sr, hop_length=hop_length)
freqs = librosa.cqt_frequencies(n_bins, fmin=fmin, bins_per_octave=bins_per_octave)

block_duration = 0.2
frames_per_block = int(np.round(block_duration / (hop_length / sr)))
print(f"Frames per block: {frames_per_block}")

B = frames_per_block
step = B // 2
L = 8

n_frames_total = C.shape[1]
block_samples = np.arange(0, 100)
block_len = hop_length * B
max_offset = (n_frames_total - B) * hop_length + block_len

y_accum = np.zeros(max_offset)
window_sum = np.zeros(max_offset)

count = 1
pca_variance_ratio = []
for start in range(0, n_frames_total - B + 1, step):
    print(count)
    count += 1
    end = start + B
    block = C[:, start:end]
    mag_block = np.abs(block)
    phase_block = np.angle(block)
    X_block = mag_block.T
    pca_result = pca_numpy(X_block, n_components=L)
    X_reduced = pca_result['X_reduced']
    pca_variance_ratio.append(pca_result['explained_variance_ratio'])
    basis_vectors = pca_result['components']
    X_reconstructed = pca_result['X_reconstructed']
    mag_reconstructed = X_reconstructed.T
    block_reconstructed = mag_reconstructed * np.exp(1j * phase_block)
    y_block = librosa.icqt(block_reconstructed, sr=sr, hop_length=hop_length, fmin=fmin, bins_per_octave=bins_per_octave, window='hann', length=block_len)
    time_window = np.hanning(block_len)
    y_block_windowed = y_block * time_window
    offset = start * hop_length
    y_accum[offset:offset + block_len] += y_block_windowed
    window_sum[offset:offset + block_len] += time_window
    
epsilon = 1e-10
harmonic_reconstructed = y_accum / (window_sum + epsilon)

harmonic_reconstructed_pre_emphasis_filter = harmonic_reconstructed

harmonic_reconstructed = sp.lfilter([1], [1, -pre_emphasis_coeff], harmonic_reconstructed)
rms_harmonic_db = compute_rms_db(harmonic)
rms_harmonic_reconstructed_db = compute_rms_db(harmonic_reconstructed)
dB_diff = rms_harmonic_db - rms_harmonic_reconstructed_db
scaling_factor = 10 ** (dB_diff / 20)
harmonic_reconstructed_scaled = harmonic_reconstructed * scaling_factor
rms_harmonic_reconstructed_scaled_db = compute_rms_db(harmonic_reconstructed_scaled)

# plt.figure()
# plt.plot(harmonic)
# plt.plot(harmonic_reconstructed_pre_emphasis_filter)
# plt.show()


perc_factor = 1
comb_final = perc_factor * percussive + harmonic_reconstructed_scaled[:len(percussive)]
comb_final = comb_final / np.max(np.abs(comb_final))
comb_final_scaled = comb_final / np.max(np.abs(comb_final))

y_scaled = y / np.max(np.abs(y))

# plt.figure()
# plt.plot(y_scaled)
# plt.plot(y_remixed)
# plt.plot(comb_final_scaled)
# plt.show()

# C_orig = librosa.cqt(y_scaled, sr=sr, hop_length=hop_length, fmin=fmin, n_bins=n_bins, bins_per_octave=bins_per_octave, window='hann')
# magnitude_orig = np.abs(C_orig)
# n_frames_orig = C_orig.shape[1]
# times_orig = librosa.frames_to_time(np.arange(n_frames_orig), sr=sr, hop_length=hop_length)
# freqs = librosa.cqt_frequencies(n_bins, fmin=fmin, bins_per_octave=bins_per_octave)

# C_rec = librosa.cqt(comb_final_scaled, sr=sr, hop_length=hop_length, fmin=fmin, n_bins=n_bins, bins_per_octave=bins_per_octave, window='hann')
# magnitude_rec = np.abs(C_rec)
# n_frames_rec = C_rec.shape[1]
# times_rec = librosa.frames_to_time(np.arange(n_frames_rec), sr=sr, hop_length=hop_length)

# C_remixed = librosa.cqt(y_remixed, sr=sr, hop_length=hop_length, fmin=fmin, n_bins=n_bins, bins_per_octave=bins_per_octave, window='hann')
# magnitude_remixed = np.abs(C_remixed)
# n_frames_remixed = C_remixed.shape[1]
# times_remixed = librosa.frames_to_time(np.arange(n_frames_remixed), sr=sr, hop_length=hop_length)

# custom_cmap = LinearSegmentedColormap.from_list('white_yellow_red_black', ['white', 'yellow', 'red', 'black'])
# vmin = -40
# vmax = 0

# fig, axs = plt.subplots(1, 3, figsize=(12, 4))
# img0 = librosa.display.specshow(librosa.amplitude_to_db(magnitude_orig, ref=np.max),
#                                 sr=sr,
#                                 hop_length=hop_length,
#                                 x_axis='time',
#                                 y_axis='cqt_hz',
#                                 x_coords=times_orig,
#                                 y_coords=freqs,
#                                 ax=axs[0],
#                                 vmin=vmin,
#                                 vmax=vmax,
#                                 cmap=custom_cmap)
# axs[0].set_title("ORIGINAL CQT Magnitude (dB)", fontsize=12)
# img1 = librosa.display.specshow(librosa.amplitude_to_db(magnitude_rec, ref=np.max),
#                                 sr=sr,
#                                 hop_length=hop_length,
#                                 x_axis='time',
#                                 y_axis='cqt_hz',
#                                 x_coords=times_rec,
#                                 y_coords=freqs,
#                                 ax=axs[1],
#                                 vmin=vmin,
#                                 vmax=vmax,
#                                 cmap=custom_cmap)
# axs[1].set_title("CI PREPROCESSED CQT Magnitude (dB)", fontsize=12)
# img2 = librosa.display.specshow(librosa.amplitude_to_db(magnitude_remixed, ref=np.max),
#                                 sr=sr,
#                                 hop_length=hop_length,
#                                 x_axis='time',
#                                 y_axis='cqt_hz',
#                                 x_coords=times_remixed,
#                                 y_coords=freqs,
#                                 ax=axs[2],
#                                 vmin=vmin,
#                                 vmax=vmax,
#                                 cmap=custom_cmap)
# axs[2].set_title("CI REMIXED CQT Magnitude (dB)", fontsize=12)

# cbar_ax = fig.add_axes([0.93, 0.15, 0.02, 0.7])
# cbar = fig.colorbar(img0, cax=cbar_ax, format='%+2.0f dB')
# cbar.set_label('Magnitude (dB)', fontsize=12)
# cbar.ax.tick_params(labelsize=10)
# plt.subplots_adjust(right=0.90)
# plt.tight_layout(rect=[0, 0, 0.92, 1])

# plt.savefig(composer + f'_original_vs_reduced_PCA_{L}_vs_remixed.pdf')
# plt.show()



audio_path_comb_final_scaled = os.path.join(script_dir, '..', 'Generated_Audio_Files', composer + f'_reduced_PCA_{L}.wav')
audio_path_y_scaled = os.path.join(script_dir, '..', 'Generated_Audio_Files', composer + '_original.wav')
audio_path_y_remixed_scaled = os.path.join(script_dir, '..', 'Generated_Audio_Files', composer + '_remixed.wav')

sf.write(audio_path_comb_final_scaled, comb_final_scaled, sr, subtype='FLOAT')
sf.write(audio_path_y_scaled, y_scaled, sr, subtype='FLOAT')
sf.write(audio_path_y_remixed_scaled, y_remixed, sr, subtype='FLOAT')

print("audio saved")

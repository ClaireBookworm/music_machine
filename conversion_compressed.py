import wave
import struct
import sys
import os
import numpy as np
from scipy import signal

def resample_audio(samples, original_rate, target_rate, n_channels):
    """Resample audio data to target sample rate."""
    samples_array = np.array(samples, dtype=np.int16)
    
    if n_channels == 2:
        # Reshape stereo: [L, R, L, R, ...] -> [[L, L, ...], [R, R, ...]]
        left = samples_array[::2]
        right = samples_array[1::2]
        
        # Resample each channel
        num_samples = len(left)
        new_num_samples = int(num_samples * target_rate / original_rate)
        
        left_resampled = signal.resample(left, new_num_samples).astype(np.int16)
        right_resampled = signal.resample(right, new_num_samples).astype(np.int16)
        
        # Interleave back: [L, R, L, R, ...]
        resampled = np.empty(len(left_resampled) * 2, dtype=np.int16)
        resampled[::2] = left_resampled
        resampled[1::2] = right_resampled
        
        return resampled.tolist(), target_rate, 2
    else:
        # Mono: simple resample
        num_samples = len(samples_array)
        new_num_samples = int(num_samples * target_rate / original_rate)
        resampled = signal.resample(samples_array, new_num_samples).astype(np.int16)
        return resampled.tolist(), target_rate, 1

def convert_to_mono(samples):
    """Convert stereo samples to mono by averaging channels."""
    samples_array = np.array(samples, dtype=np.int32)
    mono = ((samples_array[::2] + samples_array[1::2]) // 2).astype(np.int16)
    return mono.tolist()

def estimate_header_size(num_samples, var_name):
    """Estimate the size of the generated header file in bytes."""
    # Header overhead: includes, defines, comments, etc.
    overhead = 500
    
    # Array declaration and formatting
    # Each sample: " 12345," = ~7 bytes average
    array_data = num_samples * 7
    
    # Define statements
    defines = len(f'#define {var_name.upper()}_LENGTH {num_samples}\n') + \
              len(f'#define {var_name.upper()}_RATE 22050\n') + \
              len(f'#define {var_name.upper()}_CHANNELS 1\n')
    
    return overhead + array_data + defines

def convert_wav_to_header(wav_filename, max_size_mb=2.0, target_rate=22050, force_mono=False, max_duration_sec=None):
    """
    Convert a WAV file to a C header file with size constraints.
    
    Args:
        wav_filename: Input WAV file
        max_size_mb: Maximum output file size in MB (default 2.0)
        target_rate: Target sample rate in Hz (default 22050)
        force_mono: If True, convert stereo to mono (saves 50% space)
        max_duration_sec: Maximum duration in seconds (None = no limit)
    """
    max_size_bytes = max_size_mb * 1024 * 1024
    
    base_name = os.path.splitext(os.path.basename(wav_filename))[0]
    var_name = base_name.replace('-', '_').replace(' ', '_').lower()
    header_filename = f'{base_name}.h'
    
    print(f'Processing: {wav_filename}')
    
    with wave.open(wav_filename, 'rb') as wav:
        n_channels = wav.getnchannels()
        sample_width = wav.getsampwidth()
        framerate = wav.getframerate()
        n_frames = wav.getnframes()
        
        print(f'  Original: {n_frames} frames, {n_channels} ch, {framerate} Hz')
        
        if sample_width != 2:
            raise ValueError(f'Unsupported sample width: {sample_width} bytes. Only 16-bit (2 bytes) is supported.')
        
        # Read all frames
        frames = wav.readframes(n_frames)
        samples = struct.unpack(f'{len(frames)//2}h', frames)
        
        # Truncate to max duration if specified
        if max_duration_sec is not None:
            max_samples = int(max_duration_sec * framerate * n_channels)
            if len(samples) > max_samples:
                original_duration = len(samples) / (framerate * n_channels)
                samples = samples[:max_samples]
                print(f'  Truncated from {original_duration:.2f}s to {max_duration_sec}s ({len(samples)} samples)')
        
        # Convert to mono if requested or if too large
        if force_mono and n_channels == 2:
            print(f'  Converting stereo to mono...')
            samples = convert_to_mono(samples)
            n_channels = 1
            print(f'  After mono conversion: {len(samples)} samples')
        
        # Resample if needed
        if framerate != target_rate:
            print(f'  Resampling from {framerate} Hz to {target_rate} Hz...')
            samples, framerate, n_channels = resample_audio(samples, framerate, target_rate, n_channels)
            print(f'  After resampling: {len(samples)} samples')
        
        # Check size and reduce sample rate if needed
        estimated_size = estimate_header_size(len(samples), var_name)
        current_rate = framerate
        
        while estimated_size > max_size_bytes and current_rate > 8000:
            # Reduce sample rate by 10%
            current_rate = int(current_rate * 0.9)
            print(f'  File too large ({estimated_size/1024/1024:.2f} MB), reducing rate to {current_rate} Hz...')
            samples, framerate, n_channels = resample_audio(samples, framerate, current_rate, n_channels)
            estimated_size = estimate_header_size(len(samples), var_name)
        
        if estimated_size > max_size_bytes:
            # Last resort: convert to mono if stereo
            if n_channels == 2:
                print(f'  Still too large, converting to mono...')
                samples = convert_to_mono(samples)
                n_channels = 1
                estimated_size = estimate_header_size(len(samples), var_name)
        
        # Generate header guard name
        guard_name = var_name.upper() + '_H'
        
        # Write to header file
        with open(header_filename, 'w') as f:
            f.write(f'#ifndef {guard_name}\n')
            f.write(f'#define {guard_name}\n\n')
            
            f.write(f'// {var_name}: {len(samples)} samples, {n_channels} ch, {framerate} Hz\n')
            f.write(f'const int16_t {var_name}_data[] PROGMEM = {{\n')
            
            for i, s in enumerate(samples):
                f.write(f'{s:6d},')
                if (i + 1) % 10 == 0:
                    f.write('\n')
                else:
                    f.write(' ')
            
            if len(samples) % 10 != 0:
                f.write('\n')
            
            f.write('};\n\n')
            f.write(f'#define {var_name.upper()}_LENGTH {len(samples)}\n')
            f.write(f'#define {var_name.upper()}_RATE {framerate}\n')
            f.write(f'#define {var_name.upper()}_CHANNELS {n_channels}\n\n')
            f.write(f'#endif // {guard_name}\n')
        
        # Check actual file size
        actual_size = os.path.getsize(header_filename)
        size_mb = actual_size / 1024 / 1024
        
        print(f'✓ Converted {wav_filename} -> {header_filename}')
        print(f'  Final: {len(samples)} samples, {n_channels} ch, {framerate} Hz')
        print(f'  File size: {size_mb:.2f} MB ({actual_size:,} bytes)')
        
        if actual_size > max_size_bytes:
            print(f'  ⚠ WARNING: File size ({size_mb:.2f} MB) exceeds limit ({max_size_mb} MB)')
        else:
            print(f'  ✓ File size OK (under {max_size_mb} MB limit)')
        
        return header_filename, actual_size

if __name__ == '__main__':
    max_size = 2.0  # MB
    target_rate = 22050  # Hz
    force_mono = False  # Set to True to force mono conversion
    max_duration = 4.0  # seconds - truncate audio to max 4 seconds
    
    if len(sys.argv) > 1:
        wav_files = sys.argv[1:]
    else:
        wav_files = [
            'Seventies-funk-drum-loop-109-BPM.wav',
            'Square-synth-keys-loop-112-bpm.wav',
            'Funk-groove-loop.wav',
            'Groove-loop-126-bpm.wav'
        ]
    
    total_size = 0
    for wav_file in wav_files:
        if os.path.exists(wav_file):
            _, size = convert_wav_to_header(wav_file, max_size_mb=max_size, 
                                           target_rate=target_rate, force_mono=force_mono,
                                           max_duration_sec=max_duration)
            total_size += size
            print()
        else:
            print(f'⚠ File not found: {wav_file}\n')
    
    print(f'Total size of all headers: {total_size/1024/1024:.2f} MB')
    if total_size > max_size * 1024 * 1024:
        print(f'⚠ WARNING: Total size exceeds {max_size} MB limit!')
        print(f'  Consider using force_mono=True or lower target_rate')
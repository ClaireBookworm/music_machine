import wave
import sys
import os
import glob
import numpy as np
from scipy import signal

def compress_wav(input_file, output_file=None, max_size_mb=0.23):
    """Compress WAV file to max_size_mb with minimal quality loss."""
    if output_file is None:
        base = input_file.rsplit('.', 1)[0]
        output_file = f'{base}_compressed.wav'
    
    max_bytes = int(max_size_mb * 1024 * 1024)
    
    with wave.open(input_file, 'rb') as wav_in:
        params = wav_in.getparams()
        framerate, n_channels, sampwidth = params.framerate, params.nchannels, params.sampwidth
        
        if sampwidth != 2:
            raise ValueError("Only 16-bit WAV files supported")
        
        frames = wav_in.readframes(params.nframes)
        samples = np.frombuffer(frames, dtype=np.int16)
        
        # Convert stereo to mono (saves 50%)
        if n_channels == 2:
            samples = ((samples[::2].astype(np.int32) + samples[1::2]) // 2).astype(np.int16)
            n_channels = 1
            print(f"Converted to mono")
        
        # Calculate target sample rate to fit size
        target_rate = framerate
        max_samples = max_bytes // 2  # 16-bit = 2 bytes per sample
        
        if len(samples) > max_samples:
            target_rate = int(framerate * max_samples / len(samples))
            print(f"Target rate: {target_rate} Hz (from {framerate} Hz)")
        
        # Resample if needed
        if target_rate < framerate:
            num_samples = len(samples)
            new_num = int(num_samples * target_rate / framerate)
            samples = signal.resample(samples, new_num).astype(np.int16)
            framerate = target_rate
            print(f"Resampled to {framerate} Hz")
        
        # Truncate if still too large
        if len(samples) * 2 > max_bytes:
            samples = samples[:max_bytes // 2]
            print(f"Truncated to {len(samples) / framerate:.2f} seconds")
        
        # Write output
        with wave.open(output_file, 'wb') as wav_out:
            wav_out.setnchannels(n_channels)
            wav_out.setsampwidth(2)
            wav_out.setframerate(framerate)
            wav_out.writeframes(samples.tobytes())
        
        size_mb = os.path.getsize(output_file) / 1024 / 1024
        duration = len(samples) / framerate
        print(f"✓ {input_file} -> {output_file}")
        print(f"  Size: {size_mb:.2f} MB | Duration: {duration:.2f}s | Rate: {framerate} Hz | Channels: {n_channels}")
        
        return output_file

if __name__ == '__main__':
    if len(sys.argv) >= 2:
        # Process specific file(s) provided as arguments
        for wav_file in sys.argv[1:]:
            if os.path.exists(wav_file):
                compress_wav(wav_file)
            else:
                print(f"⚠ File not found: {wav_file}")
    else:
        # Default: process all .wav files in multi_sample/data/
        data_dir = "multi_sample/data"
        if not os.path.exists(data_dir):
            print(f"⚠ Directory not found: {data_dir}")
            print("Usage: python compress_wav.py [input.wav ...]")
            sys.exit(1)
        
        wav_files = glob.glob(os.path.join(data_dir, "*.wav"))
        if not wav_files:
            print(f"⚠ No .wav files found in {data_dir}")
            sys.exit(1)
        
        print(f"Processing {len(wav_files)} file(s) in {data_dir}...\n")
        for wav_file in wav_files:
            compress_wav(wav_file)
            print()

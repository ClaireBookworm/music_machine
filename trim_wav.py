import wave
import sys

def trim_wav(input_file, output_file=None, max_seconds=4.0):
    """Trim WAV file to max_seconds duration."""
    if output_file is None:
        base = input_file.rsplit('.', 1)[0]
        output_file = f'{base}_trimmed.wav'
    
    with wave.open(input_file, 'rb') as wav_in:
        params = wav_in.getparams()
        framerate = params.framerate
        n_channels = params.nchannels
        max_frames = int(max_seconds * framerate)
        
        frames = wav_in.readframes(max_frames)
        
        with wave.open(output_file, 'wb') as wav_out:
            wav_out.setparams(params)
            wav_out.writeframes(frames)
    
    duration = len(frames) / (framerate * n_channels * params.sampwidth)
    print(f'Trimmed {input_file} -> {output_file} ({duration:.2f}s)')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: python trim_wav.py <input.wav> [output.wav]')
        sys.exit(1)
    
    trim_wav(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None)


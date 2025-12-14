import wave
import struct
import sys
import os

# def convert_wav_to_header(wav_filename):
#     base_name = os.path.splitext(os.path.basename(wav_filename))[0]
#     # sanitize for C variable names
#     var_name = base_name.replace('-', '_').replace(' ', '_').lower()
#     header_filename = f'{base_name}.h'

#     with wave.open(wav_filename, 'rb') as wav:
#         n_channels = wav.getnchannels()
#         sample_width = wav.getsampwidth()
#         framerate = wav.getframerate()
#         n_frames = wav.getnframes()

#         if sample_width != 2:
#             raise ValueError(f'Unsupported sample width')

#         frames = wav.readframes(n_frames)
#         samples = struct.unpack(f'{len(frames)//2}h', frames)

#         guard_name = var_name.upper() + '_H'

#         with open(header_filename, 'w') as f:
#             f.write(f'#ifndef {guard_name}\n')
#             f.write(f'#define {guard_name}\n\n')

#             f.write(f'// {var_name}: {n_frames} frames, {n_channels} ch, {framerate} Hz\n')
#             f.write(f'const int16_t {var_name}_data[] PROGMEM = {{\n')  # UNIQUE NAME

#             for i, s in enumerate(samples):
#                 f.write(f'{s:6d},')
#                 if (i + 1) % 10 == 0:
#                     f.write('\n')
#                 else:
#                     f.write(' ')

#             if len(samples) % 10 != 0:
#                 f.write('\n')

#             f.write('};\n\n')
#             f.write(f'#define {var_name.upper()}_LENGTH {len(samples)}\n')  # UNIQUE
#             f.write(f'#define {var_name.upper()}_RATE {framerate}\n')  # UNIQUE
#             f.write(f'#define {var_name.upper()}_CHANNELS {n_channels}\n\n')  # UNIQUE
#             f.write(f'#endif // {guard_name}\n')

#         print(f'Converted {wav_filename} -> {header_filename}')
#         print(f'  Variable: {var_name}_data[{len(samples)}]')

def convert_wav_to_header(wav_filename):
    """Convert a WAV file to a C header file with PROGMEM array."""

    # Generate output filename
    base_name = os.path.splitext(os.path.basename(wav_filename))[0]
    header_filename = f'{base_name}.h'

    with wave.open(wav_filename, 'rb') as wav:
        # Get WAV file properties
        n_channels = wav.getnchannels()
        sample_width = wav.getsampwidth()
        framerate = wav.getframerate()
        n_frames = wav.getnframes()

        # Verify it's 16-bit mono or stereo
        if sample_width != 2:
            raise ValueError(f'Unsupported sample width: {sample_width} bytes. Only 16-bit (2 bytes) is supported.')

        # Read all frames
        frames = wav.readframes(n_frames)

        # Unpack as 16-bit signed integers
        samples = struct.unpack(f'{len(frames)//2}h', frames)

        # Generate header guard name
        guard_name = base_name.upper().replace('-', '_').replace(' ', '_') + '_H'

        # Write to header file
        with open(header_filename, 'w') as f:
            # Write header guard
            f.write(f'#ifndef {guard_name}\n')
            f.write(f'#define {guard_name}\n\n')

            # Write array declaration
            f.write(f'// Audio data: {n_frames} frames, {n_channels} channel(s), {framerate} Hz\n')
            f.write('const int16_t audio_data[] PROGMEM = {\n')

            # Write samples with formatting
            for i, s in enumerate(samples):
                f.write(f'{s:6d},')
                if (i + 1) % 10 == 0:
                    f.write('\n')
                else:
                    f.write(' ')

            # Ensure newline if last line wasn't complete
            if len(samples) % 10 != 0:
                f.write('\n')

            f.write('};\n\n')
            f.write(f'#define AUDIO_DATA_LENGTH {len(samples)}\n')
            f.write(f'#define AUDIO_SAMPLE_RATE {framerate}\n')
            f.write(f'#define AUDIO_N_CHANNELS {n_channels}\n\n')
            f.write(f'#endif // {guard_name}\n')

        print(f'Successfully converted {wav_filename} to {header_filename}')
        print(f'  Samples: {len(samples)}, Channels: {n_channels}, Sample rate: {framerate} Hz')



if __name__ == '__main__':
    if len(sys.argv) > 1:
        wav_file = sys.argv[1]
    else:
        # wav_file = 'Groove-loop-126-bpm.wav'
        # wav_file = 'Funk-groove-loop.wav'
        # wav_file = 'Seventies-funk-drum-loop-109-BPM.wav'
        wav_file = 'Seventies-funk-drum-loop-109-BPM.wav'

    convert_wav_to_header(wav_file)
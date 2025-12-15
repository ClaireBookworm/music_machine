import zipfile
import os
import shutil
import glob

def extract_and_convert_zips(zip_dir="soundeffects", wav_output_dir="soundeffects_wavs", header_output_dir="soundeffects_headers"):
    """Extract WAV files from zip files and convert them to header files."""
    
    # Create output directories
    os.makedirs(wav_output_dir, exist_ok=True)
    os.makedirs(header_output_dir, exist_ok=True)
    
    zip_files = glob.glob(os.path.join(zip_dir, "*.zip"))
    print(f"Found {len(zip_files)} zip files\n")
    
    wav_files = []
    
    # Extract WAV files from each zip
    for zip_path in zip_files:
        print(f"Processing: {os.path.basename(zip_path)}")
        try:
            with zipfile.ZipFile(zip_path, 'r') as zip_ref:
                # Extract all files to temp location
                temp_dir = zip_path.replace('.zip', '_temp')
                zip_ref.extractall(temp_dir)
                
                # Find WAV files in extracted folder
                wavs = glob.glob(os.path.join(temp_dir, "**/*.wav"), recursive=True)
                
                if wavs:
                    # Copy first WAV found to output directory
                    wav_file = wavs[0]
                    base_name = os.path.basename(wav_file)
                    dest_path = os.path.join(wav_output_dir, base_name)
                    shutil.copy2(wav_file, dest_path)
                    wav_files.append(dest_path)
                    print(f"  ✓ Extracted: {base_name}")
                else:
                    print(f"  ⚠ No WAV file found in {os.path.basename(zip_path)}")
                
                # Clean up temp directory
                shutil.rmtree(temp_dir, ignore_errors=True)
        except Exception as e:
            print(f"  ✗ Error processing {os.path.basename(zip_path)}: {e}")
        print()
    
    print(f"Extracted {len(wav_files)} WAV files to {wav_output_dir}\n")
    
    # Convert WAV files to headers
    if wav_files:
        print("Converting WAV files to header files...\n")
        # Import conversion function
        import sys
        sys.path.insert(0, os.getcwd())
        from conversion_compressed import convert_wav_to_header
        
        # Change to header output directory so headers are saved there
        original_cwd = os.getcwd()
        os.chdir(header_output_dir)
        
        try:
            total_size = 0
            for wav_file in wav_files:
                wav_path = os.path.join(original_cwd, wav_file)
                if os.path.exists(wav_path):
                    _, size = convert_wav_to_header(
                        wav_path, 
                        max_size_mb=0.23,
                        target_rate=22050,
                        force_mono=True,
                        max_duration_sec=4.0
                    )
                    total_size += size
                    print()
        finally:
            os.chdir(original_cwd)
        
        print(f"\n✓ Conversion complete!")
        print(f"  Headers saved to: {header_output_dir}")
        print(f"  Total size: {total_size/1024/1024:.2f} MB")
    else:
        print("No WAV files to convert")

if __name__ == '__main__':
    extract_and_convert_zips()


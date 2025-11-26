import matplotlib.pyplot as plt
from scipy.io import wavfile
from scipy import signal
import numpy as np
import os
import wave
import argparse  # Import the argparse library

def create_mock_wav_file(filename, duration=3, sample_rate=8000, frequency=440):
    """
    Creates a sample WAV file for demonstration if one doesn't exist.
    """
    print(f"Creating a sample '{filename}' for demonstration...")
    num_samples = int(duration * sample_rate)
    t = np.linspace(0., duration, num_samples)
    amplitude = np.iinfo(np.int16).max * 0.5
    data = amplitude * np.sin(2. * np.pi * frequency * t)
    
    with wave.open(filename, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        wf.writeframes(data.astype(np.int16).tobytes())
    print("Sample file created.")

def plot_spectrogram(wav_filename):
    """
    Reads a WAV file and plots its spectrogram.
    """
    if not os.path.exists(wav_filename):
        print(f"Error: File '{wav_filename}' not found.")
        # Ask the user if they want to create a mock file instead
        choice = input("Would you like to create a mock file for demonstration? (y/n): ").lower()
        if choice == 'y':
            create_mock_wav_file(wav_filename)
        else:
            print("Aborting.")
            return

    try:
        sample_rate, samples = wavfile.read(wav_filename)
        print(f"Successfully read '{wav_filename}'")
        print(f"Sample Rate: {sample_rate} Hz")
        print(f"Number of samples: {len(samples)}")
        
        if samples.ndim > 1:
            print("Audio is stereo, converting to mono for spectrogram.")
            samples = samples.mean(axis=1)

    except Exception as e:
        print(f"Error reading WAV file: {e}")
        return

    frequencies, times, spectrogram_data = signal.spectrogram(samples, sample_rate, nperseg=256)

    plt.figure(figsize=(10, 5))
    plt.pcolormesh(times, frequencies, 10 * np.log10(spectrogram_data + 1e-10), shading='gouraud')
    
    plt.ylabel('Frequency [Hz]')
    plt.xlabel('Time [s]')
    plt.title(f'Spectrogram of {wav_filename}')
    plt.ylim(0, sample_rate / 2)
    
    cbar = plt.colorbar()
    cbar.set_label('Intensity [dB]')
    
    print("Displaying spectrogram...")
    plt.show()


if __name__ == '__main__':
    # --- Argument Parsing Logic ---
    # 1. Create the parser
    parser = argparse.ArgumentParser(
        description="Generate and display a spectrogram for a given .wav file."
    )

    # 2. Add the arguments
    parser.add_argument(
        '-f', '--filename',
        type=str,
        default='recording.wav',  # Default value if no argument is provided
        help='The path to the .wav file to analyze. Defaults to "recording.wav".'
    )
    
    # 3. Parse the arguments from the command line
    args = parser.parse_args()

    # 4. Use the parsed argument
    # The filename is now accessed via args.filename
    plot_spectrogram(args.filename)
import os
import time

import matplotlib.pyplot as plt
import numpy as np
import serial
from scipy.interpolate import interp1d

from roll import MidiFile

PITCH_PWM_DIFF_THRESHOLD = 100
SERIAL_PORT = '/dev/cu.usbserial-120'

NOTE_TO_PWM = {
    48: 1173,    # C3 -> A3
    50: 1208,    # D3 -> B3
    52: 1236,    # E3 -> C4#
    53: 1244,    # F3 -> D4
    55: 1264,    # G3 -> E4
    57: 1294,    # A3 -> F4#z
    58: 1302,
    59: 1319,    # B3 -> G4#
    60: 1348,    # C4 -> A4
}

# Configure the serial port
ser = serial.Serial(
    baudrate=1200,        # Set the correct baud rate for your device
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1             # Optional: timeout in seconds
)

prev_pitch_pwm = None


def uart_get():
    ret_str = ''
    while not ret_str.endswith('<end>'):
        try:
            ret_str += ser.read(1).decode('utf-8', errors='replace')
        except UnicodeDecodeError:
            ret_str += '<?>'  # Replace invalid bytes with a placeholder
    return ret_str


def uart_send(data: str, debug=False):
    if not debug:
        ser.write(data.encode('utf-8'))

    print("\033[2m UART sent:", data, "\033[0m")

    response = '\033[2mUART DEBUG RESPONSE\033[0m'
    if not debug:
        response = uart_get()
        print("\033[2m UART received:", response, "\033[0m")

    return response


def uart_send_and_wait(data: str, debug=False):
    response = uart_send(data, debug=debug)
    while '<ready>' not in response:
        response = uart_get()
        print("\033[2m UART received:", response, "\033[0m")

    return response


def set_pitch_pwm(new_pwm=None, debug=False):
    global prev_pitch_pwm

    while True:
        try:
            if new_pwm is None:
                new_pwm = int(input("Enter new pulse width(us): "))

            if prev_pitch_pwm is None or abs(prev_pitch_pwm - new_pwm) > PITCH_PWM_DIFF_THRESHOLD:
                confirm = input(
                    f"\033[93mNew pitch PWM varies from previous by more than {PITCH_PWM_DIFF_THRESHOLD}us\033[0m (\033[91m{prev_pitch_pwm}\033[0m -> \033[91m{new_pwm}\033[0m)\nConfirm? (y/n): ") or 'y'
                if confirm.lower() == 'n':
                    raise KeyboardInterrupt("PWM setting interrupted by user")
                if confirm.lower() != 'y':
                    continue

            prev_pitch_pwm = new_pwm
            uart_send('pitch set pulse width us ' + str(new_pwm) + '\r', debug=debug)
            return new_pwm

        except KeyboardInterrupt as e:
            raise e


def tune_result(data_csv: str = 'pwm_freq_data.csv', save_file: str | None = None):
    # Read data from csv file
    with open(data_csv, 'r', encoding='utf-8') as f:
        # Parse each line into tuples of (pwm, freq)
        lines = []
        current_line = []
        prev_pwm = float('inf')

        for line in f:
            pwm, freq = map(float, line.strip().split(','))

            # Start a new line when PWM decreases
            if pwm < prev_pwm and current_line:
                lines.append(current_line)
                current_line = []

            current_line.append((pwm, freq))
            prev_pwm = pwm

        # Add the last line if it's not empty
        if current_line:
            lines.append(current_line)

    # Create figure with 2 subplots side by side
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(20, 6))

    # Plot the original results
    for i, line in enumerate(lines, 1):
        if line:
            pwms, freqs = zip(*line)
            ax1.plot(pwms, freqs, 'o-', label=f'Group {i}')

    ax1.set_xlabel('PWM (us)')
    ax1.set_ylabel('Frequency (Hz)')
    ax1.set_title('PWM vs Frequency Measurements')
    ax1.legend()
    ax1.grid(True)

    # Plot absolute frequency differences between groups
    for i in range(1, len(lines)):
        current_line = lines[i]
        prev_line = lines[i-1]

        if current_line and prev_line:
            # Get PWM values from both lines
            current_pwms, current_freqs = zip(*current_line)
            prev_pwms, prev_freqs = zip(*prev_line)

            # Create interpolation functions for both lines
            current_interp = interp1d(current_pwms, current_freqs, bounds_error=False,
                                      fill_value='extrapolate')
            prev_interp = interp1d(prev_pwms, prev_freqs, bounds_error=False,
                                   fill_value='extrapolate')

            # Create x points spanning both PWM ranges
            min_pwm = min(min(current_pwms), min(prev_pwms))
            max_pwm = max(max(current_pwms), max(prev_pwms))
            pwm_points = np.linspace(min_pwm, max_pwm, 100)

            # Calculate interpolated values and differences
            current_interp_freqs = current_interp(pwm_points)
            prev_interp_freqs = prev_interp(pwm_points)
            freq_diffs = prev_interp_freqs - current_interp_freqs

            # Plot the difference between interpolated lines
            ax2.plot(pwm_points, freq_diffs, '-', label=f'Group {i} - Group {i+1}')

    ax2.set_xlabel('PWM (us)')
    ax2.set_ylabel('Absolute Frequency Difference (Hz)')
    ax2.set_title('Absolute Frequency Differences Between Adjacent Groups')
    ax2.legend()
    ax2.grid(True)

    plt.tight_layout()

    if save_file:
        plt.savefig(save_file)
    plt.show()

    return lines


def tune(debug=False):
    # Clear csv file first
    with open('pwm_freq_data.csv', 'w', encoding='utf-8') as f:
        f.write('')

    pwm = None

    try:
        tune_mode = int(input("Enter mode: 1)Manual 2)Auto: "))
        if tune_mode == 1:   # tune manually
            print('\n')
            pwm = set_pitch_pwm(debug=debug)
            time.sleep(1)

            while True:
                try:
                    freq = int(input("Enter frequency(Hz): "))
                except KeyboardInterrupt:
                    break

                # Save to file after each measurement
                with open('pwm_freq_data.csv', 'a', encoding='utf-8') as f:
                    f.write(f"{pwm},{freq}\n")

                print('\n')
                pwm = set_pitch_pwm(debug=debug)

        elif tune_mode == 2:  # tune automatically
            print("Tuning automatically...")
            print("Tuning finished")
        else:
            print("Invalid mode")
            return
    except KeyboardInterrupt:
        pass
    finally:
        tune_result(save_file='pwm_freq_data.png')
        print("\n\033[91mExit tuning...\033[0m")


def select_midi_file():
    # Get all .mid files from midi directory
    try:
        midi_files = [f for f in os.listdir('midi') if f.endswith('.mid')]
    except FileNotFoundError:
        print("'midi' directory not found")
        return None

    if not midi_files:
        print("No MIDI files found in 'midi' directory")
        return None

    # Print numbered list of files
    print("\nAvailable MIDI files:")
    for i, file in enumerate(midi_files, 1):
        print(f"{i}) {file}")

    # Get user selection
    while True:
        try:
            selection = int(input("\nSelect a song number: "))
            if 1 <= selection <= len(midi_files):
                return os.path.join('midi', midi_files[selection-1])
            print("Invalid selection. Please try again.")
        except ValueError:
            print("Please enter a valid number")
        except KeyboardInterrupt:
            print("\nSelection cancelled")
            return None


def test_midi(debug=False):
    for note, pwm in NOTE_TO_PWM.items():
        current_pwm = pwm
        while True:
            uart_send('play 1\r', debug=debug)
            uart_send(f'play {current_pwm},500\r', debug=debug)
            uart_send('play start\r', debug=debug)
            result = input("Enter result(+/-/y): ")
            if not result:
                continue
            if result.startswith('+'):
                current_pwm += int(result[1:])
            elif result.startswith('-'):
                current_pwm -= int(result[1:])
            elif result == 'y':
                print(f"note, PWM: {note}, {current_pwm}")
                break
            else:
                print("Invalid result")


def play_midi(debug=False):
    data = []
    notes = []
    delays = []
    sum = 0
    flag = False
    midi_file = MidiFile(select_midi_file())
    # midi_file.draw_roll()
    # input("Press Enter to continue...")
    try:
        for i, track in enumerate(midi_file.tracks):
            for message in track:
                if message.type in ['note_on', 'note_off']:
                    print(message)
                    sum += message.time

                    if message.type == 'note_on':
                        flag = not flag
                        if not flag:
                            continue
                        notes.append(message.note)
                        delays.append(int(sum*0.5))
                        sum = 0

    except Exception as e:
        print("Error:", e)

    delays = delays[1:] + [delays[0]]
    data = [f'{NOTE_TO_PWM[note]},{delay}' for note, delay in zip(notes, delays)]

    idx = 0
    BATCH_SIZE = 3
    uart_send(f'play {len(data)}\r', debug=debug)
    while idx + BATCH_SIZE < len(data):
        batch = data[idx:idx+BATCH_SIZE]
        uart_send('play ' + ' '.join(batch) + '\r', debug=debug)
        # input("Press Enter to continue...")
        idx += BATCH_SIZE

    batch = data[idx:]
    uart_send('play ' + ' '.join(batch) + '\r', debug=debug)
    response = uart_send('play start\r', debug=debug)
    while '<done>' not in response:
        response = uart_get()
        print("\033[2m UART received:", response, "\033[0m")


def pick_mode(debug=False):
    try:
        while True:
            option = int(input("Enter option: 1)Set degree delta 2)Set base degree 3) Pick: "))
            if option == 1:
                degree_delta = int(input("Enter degree delta: "))
                uart_send('pick set degree delta ' + str(degree_delta) + '\r', debug=debug)
            elif option == 2:
                base_degree = int(input("Enter base degree: "))
                uart_send('pick set base degree ' + str(base_degree) + '\r', debug=debug)
            elif option == 3:
                uart_send('pick\r', debug=debug)
            else:
                print("Invalid option")
    except KeyboardInterrupt:
        print("\n\033[91mExit pick mode...\033[0m")


def command_mode(debug=False):
    try:
        while True:
            command = input("Enter command: ")
            uart_send(command + '\r', debug=debug)
    except KeyboardInterrupt:
        print("\n\033[91mExit command mode...\033[0m")


if __name__ == "__main__":
    debug_enable = False
    try:
        # Open the serial port if it's not already open
        if not ser.is_open:
            ser.port = SERIAL_PORT
            ser.open()
        print("Connected to serial port " + ser.port)
    except serial.serialutil.SerialException as e:
        debug_enable = True
        print("Unable to open serial port, entering debug mode")

    try:
        while True:
            if debug_enable:
                print("\n[Debug mode]")
                mode = int(
                    input(
                        "Enter mode: 1)Tune 2)Play 3)Tune Result 4)Exit Debug 5)Pick 6)Test MIDI 7)Reset 8)Status: "))
            else:
                mode = int(
                    input(
                        "Enter mode: 1)Tune 2)Play 3)Tune Result 4)Debug 5)Pick 6)Test MIDI 7)Reset 8)Status: "))

            if mode == 1:
                tune(debug=debug_enable)
            elif mode == 2:
                play_midi(debug=debug_enable)
            elif mode == 3:
                tune_result(save_file='pwm_freq_data.png')
            elif mode == 4:
                if not ser.is_open:
                    print("\033[91mSerial port is not open, unable to exit debug mode\033[0m")
                else:
                    debug_enable = not debug_enable
            elif mode == 5:
                pick_mode(debug=debug_enable)
            elif mode == 6:
                test_midi(debug=debug_enable)
            elif mode == 7:
                uart_send('reset\r', debug=debug_enable)
            elif mode == 8:
                uart_send('status\r', debug=debug_enable)
            else:
                print("Invalid mode")
                continue
    except KeyboardInterrupt:
        print("\n\033[91mExiting...\033[0m")

    ser.close()

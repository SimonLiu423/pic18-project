import time

import matplotlib.pyplot as plt
import numpy as np
import serial
from mido import MidiFile
from scipy.interpolate import interp1d

PITCH_PWM_DIFF_THRESHOLD = 100
SERIAL_PORT = '/dev/cu.usbserial-120'

NOTE_TO_PWM = {
    45: 500,
    46: 550,
    47: 600,
    48: 700,
    49: 735,
    50: 770,
    51: 785,
    52: 795,
    53: 850,
    54: 867,
    55: 885,
    56: 892,
    57: 910,
    58: 923,
    59: 937,
    60: 940,
    61: 943,
    62: 650,
    63: 700,
    64: 750,
    65: 800,
}

# Configure the serial port
ser = serial.Serial(
    baudrate=1200,        # Set the correct baud rate for your device
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1             # Optional: timeout in seconds
)

# Load your MIDI file
midi_file = MidiFile('birthday.mid')

prev_pitch_pwm = None


def uart_get():
    ret_str = ''
    while ser.in_waiting or not ret_str.endswith('<end>'):
        try:
            ret_str += ser.read(1).decode('utf-8', errors='replace')
        except UnicodeDecodeError:
            ret_str += '<?>'  # Replace invalid bytes with a placeholder
    ret_str = ret_str.replace('<end>', '')
    return ret_str


def uart_send(data: str, debug=False):
    if not debug:
        ser.write(data.encode('utf-8'))

    print("\033[2m UART sent:", data, "\033[0m")

    # if not debug:
    print("\033[2m UART received:", uart_get(), "\033[0m")


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


def play_midi(debug=False):
    data = []
    sum = 0
    flag = False
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
                        data.append(f'{NOTE_TO_PWM[message.note]},{sum}')
                        sum = 0

                        # uart_send('pitch ' + str(message.note) + '\r', debug=debug)

                        # uart_send(f'pick {message.time*10}\r', debug=debug)

                    # notes.append({
                    #     "Track": i,
                    #     "Type": message.type,
                    #     "Note": message.note,
                    #     "Velocity": message.velocity,
                    #     "Time": message.time
                    # })

        # for note in notes:
        #     print(note)
            # while True:
            #     data_to_send = str(num) + '\r'
            #     ser.write(data_to_send.encode('utf-8'))  # Send data
            #     print("Data sent:", data_to_send)
            #     num += 1
            #     time.sleep(0.5)

            # Optional: Wait and read response
            # time.sleep(1)  # Wait for response (if needed)
            # if ser.in_waiting:
            # response = ser.read(ser.in_waiting).decode('utf-8')  # Read available data
            # print("Received:", response)

    except Exception as e:
        print("Error:", e)

    seg_idx = 0
    step = 3
    while seg_idx + step < len(data):
        uart_send('play ' + ' '.join(data[seg_idx:seg_idx+step]) + '<done>\r', debug=debug)
        # time.sleep(0.005)
        seg_idx += step
    uart_send('play ' + ' '.join(data[seg_idx:]) + '<done>\r', debug=debug)


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
                mode = int(input("Enter mode: 1)Tune 2)Play 3)Tune Result 4)Exit Debug 5)Pick: "))
            else:
                mode = int(input("Enter mode: 1)Tune 2)Play 3)Tune Result 4)Debug 5)Pick: "))

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
            else:
                print("Invalid mode")
                continue
    except KeyboardInterrupt:
        print("\n\033[91mExiting...\033[0m")

    ser.close()

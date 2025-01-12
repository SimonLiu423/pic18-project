import time

import pandas as pd
import serial
from mido import MidiFile

# Configure the serial port
ser = serial.Serial(
    port='/dev/cu.usbserial-1320',  # Replace with your UART device path
    baudrate=1200,        # Set the correct baud rate for your device
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1             # Optional: timeout in seconds
)
# Load your MIDI file
midi_file = MidiFile('twinkle_twinkle.mid')

# Open the serial port if it's not already open
if not ser.is_open:
    ser.open()

try:
    for i, track in enumerate(midi_file.tracks):
        for message in track:
            # if message.type in ['note_on', 'note_off']:
            if message.type == 'note_on':
                data_to_send = str(message.note) + '\r'

                time.sleep(message.time * 0.001)

                ser.write(data_to_send.encode('utf-8'))  # Send data
                print("Data sent:", data_to_send)

                # notes.append({
                # "Track": i,
                # "Type": message.type,
                # "Note": message.note,
                # "Velocity": message.velocity,
                # "Time": message.time
                # })
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

finally:
    ser.close()

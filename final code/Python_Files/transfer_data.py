#Hasan Asim 400512182

import serial
import math

# Set up serial connection (adjust COM port if needed)
s = serial.Serial('COM3', 115200)

s.open
s.reset_output_buffer()
s.reset_input_buffer()

# Open file to save point cloud data
f = open("point_array.xyz", "w") 

step = 0                   # Tracks steps of the motor (0 to 512)
x = 0                      # Starting x-coordinate (in mm)
increment = 100            # Distance to move forward per rotation (in mm)
num_inc = int(input("Enter how many full scans to perform: "))
count = 0                  # Scan counter

while(count < num_inc):
    raw = s.readline()
    data = raw.decode("utf-8")         # Convert byte stream to string
    data = data[0:-2]                  # Remove newline and carriage return

    if data.isdigit():                 # Ensure received string is numeric
        angle = (step / 512) * 2 * math.pi  # Convert step count to angle (radians)
        r = int(data)                      # Distance from ToF sensor
        y = r * math.cos(angle)           # Compute y-coordinate
        z = r * math.sin(angle)           # Compute z-coordinate
        print(y)
        print(z)
        f.write(f'{x} {y} {z}\n')         # Save point to file
        step += 32                        # Move to next angle step

    # After a full 360° sweep, update x-position and scan count
    if step == 512:
        step = 0
        x += increment
        count += 1

    print(data)

# Close file after scan complete
f.close()

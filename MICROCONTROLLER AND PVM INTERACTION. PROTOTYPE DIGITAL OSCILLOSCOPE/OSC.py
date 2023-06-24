import serial.tools.list_ports
import matplotlib.pyplot as plt
import numpy as np
import keyboard
import time
import msvcrt

pause_flag = 1

start_time = 0
new_time = 1
plt.ion()
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)
data = np.array([])
x = np.array([])

ser = serial.Serial(port='COM4',
					baudrate=19200,
					parity=serial.PARITY_ODD,
					stopbits=serial.STOPBITS_ONE,
					bytesize=serial.EIGHTBITS,
					timeout=100000
				   )


while True:
	if keyboard.is_pressed("q"):
		print('exiting program...')
		time.sleep(1)
		ser.close()
		exit(0)

	if keyboard.is_pressed("p") and pause_flag == 0:
		pause_flag = 1
		ser.write("p".encode())
		print('sending: p')
		ser.write("\n".encode())
		time.sleep(1)

	if keyboard.is_pressed("v") and pause_flag == 1:
		pause_flag = 0
		print('sending: s')
		ser.write("s".encode())
		ser.write("\n".encode())
		time.sleep(1)

	if keyboard.is_pressed("o") and pause_flag == 1:
		string = int(input("Enter MC to PC frequency: "))
		char = chr(string)
		new_string = "o" + char
		print('sending: ', new_string)
		ser.write(new_string.encode())
		ser.write("\n".encode())
		time.sleep(1)

	if keyboard.is_pressed("d") and pause_flag == 1:
		string = int(input("Enter sampling frequency: "))
		char = chr(string)
		new_string = "d" + char
		print('sending: ', new_string)
		ser.write(new_string.encode())
		ser.write("\n".encode())
		time.sleep(1)

	if pause_flag == 0:
		a = ser.read()
		b = ser.read()
		c = ser.read()
		dot = ser.read()

		voltage = int(a) + float((int(b) * 10 + int(c)) / 100) 
		print('Current voltage: ', voltage)

		x = np.append(x, start_time)
		start_time += new_time
		data = np.append(data, voltage)
		ax.clear()
		data = data[-20:]
		x = x[-20:]
		ax.set_facecolor('#99ff99')
		ax.set_ylim(0, 5)
		ax.plot(x, data, color='#ff0000')
		plt.grid(linestyle='--', color='black')
		plt.draw()
		plt.pause(0.1)
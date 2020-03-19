#start this program first, then run the time verification program in atmel
import serial
import time
numloops = 1000 #must match Atmel program 

#ser.close()
ser = serial.Serial('COM3', 9600)
print(ser)

start = ser.read(size = 1) #block until something is received
#print(start)
now = time.time()
print(now)
stop = ser.read(size=1)
#print(stop)
time_elapsed = time.time()-now

print(Total time elapsed)
print(time_elapsed)
print(Time per loop)
print(time_elapsedfloat(numloops))


ser.reset_input_buffer()
ser.close()
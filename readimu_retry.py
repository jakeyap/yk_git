#!/usr/bin/python
# ADC REFERENCE GOES FROM 0-4095
#4095 is 3V, 0 is 0V.
# each increment is 0.729mV


import time, sys
import serial
import lib.motetalk as motetalk
from lib.motetalk import cmd, packetify, setpwm

ser = serial.Serial("COM6",115200)
height_data = open('height_data.csv', 'w')
motor_data  = open('motor_data.csv', 'w')

average_value = 0

reference = 670	#about 75cm off the ground
error_term = 0
old_error_term = 0
measurement = 0
motor_output = 0
period = 120
frequency = 100/12

const_proportional = 100
const_integral = 0
const_differential = 0

integral = 0
differential = 0
proportional = 0

def measureheight(number):
  a = number / 100
  b = (number - (a*100))/10
  c = number - (a*100) - (b*10)
  sys.stderr.write("a b c are " + str(a) + " " + str(b) + " " + str(c))
  height_data.write(number)  
  
  return (a, b, c)

#test split number
def splitnumber(number):
  a = number / 100
  if (a>=3):
    sys.stderr.write("sent to motor: 255\t\t")
    return (2, 5, 5)
  else:
    b = (number - (a*100))/10
    c = number - (a*100) - (b*10)
    sys.stderr.write("sent to motor:" + str(a) + str(b) + str(c) +"\t\t")
    return (a, b, c)
#end of test split number  

def write_to_serial((a, b, c)):
  #ser.write(1)
  #ser.write(0)
  #ser.write(0)
  # ser.write(a&0x000000ff)
  # ser.write(b&0x000000ff)
  # numberofbytes = ser.write(c&0x000000ff)
  ser.write(a)
  ser.write(b)
  ser.write(c)

def combine_number((a, b, c)):
  return a*100+b*10+c

def startup(m):
  m.sendbase(cmd.radio(20))

  m.sendheli(cmd.flags(cmd.ledmode_cnt + cmd.tick))
  m.sendheli(cmd.mode(cmd.mode_imu_loop))

  m.sendbase(cmd.flags(cmd.ledmode_cnt + cmd.notick))
  m.sendbase(cmd.mode(cmd.mode_sniff))

def end(m):
  m.sendbase(cmd.mode(cmd.mode_spin))
  m.end()

num_good = 0
num_bad = 0
num_skip = 0

done = 0
n = 0
oldarr = []

openwsnheader =    "n sx sy z1 z3 ta mx my mz lx ly lz ti gx gy gz chk lqi rssi"
openwsnfmtstr = '!xxH H  H  H  H  H  h  h  h  H  H  H  H  h  h  h  H   B   B'

frequency_divider = 0	#helps to slow the adc inputs to basestation

#YK's PID controller here
def pid():
  global error_term, average_value, measurement, integral, const_integral, period, differential, const_differential, old_error_term, proportional, const_proportional, motor_output
  
  error_term = reference - average_value
  integral = integral + error_term * const_integral * period
  differential = const_differential * (error_term - old_error_term) / period
  proportional = error_term * const_proportional
  old_error = error_term
  motor_output = integral + differential + proportional
  if motor_output>255:
    motor_output = 255
  elif motor_output<190:
    motor_output = 190
#YK's PID controller ends

m = motetalk.motetalk(openwsnfmtstr, openwsnheader, "COM10")
startup(m)
#yk = open("yk.txt", "w")
#ser.write("p")
sys.stderr.write( "Sniffing...\n")
print "ts " + openwsnheader
# put the stuff here
#try:
  #(arr, t, crc) = m.nextline()
  #(arr, t, crc) = m.nextline()
  #(arr, t, crc) = m.nextline()
#except: 
  #pass
oldvalue = (0, 0, 0)    #stores the value to send if there is an error in transmission
tempvalue = (0, 0, 0)	#stores the value to send to motor right after pid calculation

while not done:
  try:
    (arr, t, crc) = m.nextline()

    if (arr == False):
      done = 1
      sys.stderr.write("\n** Error ** ")
      sys.stderr.write(repr(t) + "\n\n")
    elif arr:
      if crc:
        # if (frequency_divider ==0):
            # sys.stderr.write("bad:\n")
            # write_to_serial(oldvalue)
            # height_data.write("xxx")
        sys.stderr.flush()
        num_bad = num_bad + 1
      else:
        if oldarr:
          dn = ((arr[0] - n + 32768) % 65536) - 32768
          if dn == 1: 
            #sys.stderr.write(".")
            pass
          else:
            #sys.stderr.write("!%d!" % (dn - 1))
            num_skip = num_skip + dn - 1
          #sys.stderr.flush()
          pass

        #print repr(t), " ".join(map(repr,arr))
        
        if (frequency_divider == 0):
           measurement = arr[6]
           average_value = (average_value + measurement)/40
           pid()
           height_data.write(str(measurement) + "\n")
           motor_data.write(str(motor_output) + "\n")
           tempvalue = splitnumber(motor_output)
           write_to_serial(tempvalue)
           oldvalue = tempvalue
           sys.stderr.write("Value of readings: ")
           print arr[6]
           sys.stderr.write("\n")
           average_value = 0
        else:
           average_value = average_value + arr[6]
           
        n = arr[0]
        oldarr = arr
        num_good = num_good + 1
    else: #else from the elif arr
      # if (frequency_divider ==0):
        # write_to_serial(oldvalue)
        # height_data.write("xxx")
      sys.stderr.write("x")
      sys.stderr.flush()
      num_bad = num_bad + 1
	  
	  
    if frequency_divider >=39:
       frequency_divider = 0
    else:
       frequency_divider = frequency_divider + 1
  except TypeError:
    # if (frequency_divider==0):
       # write_to_serial(oldvalue)
       # height_data.write("xxx")
       # motor_data.write(oldvalue)
    frequency_divider = frequency_divider + 1
    sys.stderr.write("\nNOOB... \n")
    sys.stderr.write(repr(sys.exc_info()))
    done = 0
    
  except:
    sys.stderr.write(repr(sys.exc_info()))
    sys.stderr.write("\nQuitting... \n")
    #ser.write("o")
    done = 1

end(m)
sys.stderr.write( "%d Successful packets recieved\n" % num_good)
sys.stderr.write( "%d packet errors\n" % num_bad)
sys.stderr.write( "%d skipped packets\n" % num_skip)
if (num_skip + num_good):
  sys.stderr.write( "%4.2f%% packet drop rate\n" % (num_skip * 100.0 / (num_skip + num_good)))


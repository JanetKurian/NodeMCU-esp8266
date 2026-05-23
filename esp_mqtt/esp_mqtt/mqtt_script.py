# # #Import("env")

# # import serial
# # import time

# # print("Opening UART...")

# # ser = serial.Serial("COM12", 115200, timeout=1)

# # time.sleep(2)

# # ser.write(b"AT\r\n")

# # time.sleep(1)

# # response = ser.read_all()

# # print(response.decode())

# # ser.close()

# import serial
# import time

# print("Opening UART...")

# ser = serial.Serial("COM13", 115200, timeout=1)

# time.sleep(2)

# # Clear garbage boot data
# ser.reset_input_buffer()

# ser.write(b"AT\r\n")

# time.sleep(2)

# # response = ser.read_all()

# # #print(response.decode(errors="ignore"))
# # print(response.decode("utf-8", errors="ignore"))
# # ser.close()
# response = ser.read_all()

# print("Raw Response:")
# print(response)

# print("Decoded Response:")
# print(response.decode("utf-8", errors="ignore"))

# ser.close()

import serial
import time

ser = serial.Serial("COM13", 115200, timeout=2)

time.sleep(3)

def SendAT(cmd):

    print(f"\nTX --> {cmd}")

    ser.write((cmd + "\r\n").encode())

    time.sleep(3)

    response = ser.read_all()

    print("RX <--")
    print(response.decode(errors="ignore"))

SendAT("AT")

SendAT('AT+CWJAP="moto g85 5G_4516", password = "Moto1234x"')

SendAT('AT+MQTTCONN="broker.hivemq.com",1883')

SendAT('AT+MQTTPUB="test/topic","Hello from ESP8266"')

ser.close()
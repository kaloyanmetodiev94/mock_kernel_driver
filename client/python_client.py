#!/usr/bin/python3

import socket
import sys 
import time

class PythonClient():
	def __init__(self,socket_name):
		self.socket_name=socket_name
		self.socket_name+=(108-len(self.socket_name))*"\0"
		self.data_buffer=bytearray(b'\x00'*9)
		self.receive_buffer_size=1024

		self.connect()

	def connect(self):
		self.client=socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
		self.client.connect(self.socket_name)
	
	def disconnect(self):
		self.client.close()

	def communicate(self):
		print("Sending request...")
		if(self.client.send(self.data_buffer)>0):
			print("Request is OKAY..")
		print("Receiving response...")
		print(self.client.recv(self.receive_buffer_size).decode())

	def numbers_prompt(self):
		numbers = ['','']
		for i in range(len(numbers)):
			while not self.check_int(numbers[i]):
				numbers[i]=input("Enter operand "+str(i+1)+":")
		return list(map(lambda x: int(x),numbers)) #map the list to integers 

	def run(self):
		val='0'
		while self.handle_logic(val):
			val=input()
			

	def handle_logic(self,val):
		if self.check_int(val):
			val=int(val)
			if val==5:
				self.disconnect()
				return False
			elif val==0:
				self.data_buffer[0]=val
				self.communicate()
				return True
			elif 0<val<5:
				self.data_buffer[0]=val
				numbers=self.numbers_prompt()
				self.data_buffer[1:5]=numbers[0].to_bytes(length=4, byteorder='little',signed=True) #everywhere we use little endian
				self.data_buffer[5:9]=numbers[1].to_bytes(length=4, byteorder='little',signed=True)
				self.communicate()
				return True
			else:
				print("Please select a valid command. Enter 0 to get menu")
				return True
		else:
			print("Only integer numbers allowed")
			return True

	def check_int(self,string):
		if (len(string)>0) and (string[0] in ('-', '+')):
			return string[1:].strip().isdigit()
		return string.strip().isdigit()

		
if __name__=="__main__":
	cl=PythonClient('\0hidden_socket')
	cl.run()
			



		

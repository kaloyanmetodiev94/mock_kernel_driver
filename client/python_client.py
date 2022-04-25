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
		self.client.send(self.data_buffer)
		print(self.client.recv(self.receive_buffer_size).decode())

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
			else:
				return True
		else:
			print("Only integer numbers allowed")
			return True

	def check_int(self,string):
		if string[0] in ('-', '+'):
			return string[1:].strip().isdigit()
		return string.strip().isdigit()

		
if __name__=="__main__":
	cl=PythonClient('\0hidden_socket')
	cl.run()
			



		

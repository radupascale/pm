import serial
import serial.tools.list_ports

class Arduino():
	def __init__(self):
		self.set_device()
		self.set_serial()
		pass

	def set_device(self):
		"""_summary_

		Returns:
			_type_: _description_
		"""
		driver = "CH340"
		for p in serial.tools.list_ports.comports():
			if driver in p.description:
				self.device = p.device
				return
			
	def set_serial(self):
		"""_summary_

		Args:
			port (_type_): _description_

		Returns:
			_type_: _description_
		"""
		serialConnection = serial.Serial(port = self.device, baudrate = 9600, timeout=1)
		self.serial = serialConnection

	def send_serial_data(self, data: bytes):
		"""_summary_

		Args:
			connection (serial.Serial): _description_
			data (str): _description_

		Returns:
			_type_: _description_
		"""
		if not self.serial.is_open:
			try:
				self.serial.open()
			except serial.SerialException:
				print("Could not open serial port. Is the Arduino connected?")
				return False
		self.serial.write(data)

	def get_serial_data(self):  
		"""_summary_

		Args:
			connection (serial.Serial): _description_

		Returns:
			_type_: _description_
		"""
		if not self.serial.is_open:
			try:
				self.serial.open()
			except serial.SerialException:
				print("Could not open serial port. Is the Arduino connected?")
				return False
		data = self.serial.readline()
		return data
import psutil # Import psutil module for cpu info

class HardwareMonitor:
	def __init__(self, user_input_throttling):
		self.throttling = user_input_throttling
		self.cpu_use = 0; # default

	def get_cpu_info(self):
		self.cpu_use = psutil.cpu_percent(1) # CPU times elapsed before and after 1 sec
		return self.cpu_use

	def get_throttling(self):
		return self.throttling
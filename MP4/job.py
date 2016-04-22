class Job(object):
	def __init__(self, start_idx, length, data_vector):
    	self.id = start_idx
    	self.data_vector = data_vector
    	self.start_idx = start_idx
    	self.length = length

	def compute(self):
		for i in xrange(self.length):
			for _ in xrange(200):
				self.data_vector[i] += 1.111111


# remote
import pika
import sys
import time
import threading
import pickle
import argparse
import logging
from job import Job
from hardware_monitor import HardwareMonitor

LOCAL_IP = '172.22.146.196'
REMOTE_IP = '172.17.82.56'

QUEUE_THRESHOLD = 400
MY_TASK_QUEUE = None 
TASK_CONNECTION = None
TASK_CHANNEL = None
TASK_THREADS = []
COMPLETED_JOBS = []
THROTTLING = 1
TASK_DESTINATION = 'local_task_queue'
TASK_SOURCE = 'remote_task_queue'
STATE_DESTINATION = 'local_state_queue'
STATE_SOURCE = 'remote_state_queue'

# class myThread (threading.Thread):
# 	def __init__(self, threadID, name, counter):
# 		threading.Thread.__init__(self)
# 		self.threadID = threadID
# 		self.name = name
# 		self.counter = counter

# 	def run(self):
# 		print "Starting " + self.name
# 		receiveTask()
# 		print "Exiting " + self.name


def sendTask(task):
	connection = pika.BlockingConnection(pika.ConnectionParameters(
	        host=REMOTE_IP))
	channel = connection.channel()

	channel.queue_declare(queue=TASK_DESTINATION, durable=True)
	wow = {'Name': 'Zara', 'Age': 7, 'Class': 'First'};
	sendable = pickle.dumps(wow)

	myJob1 = Job(0, 5, [1,2,3,4,5])
	myJob2 = Job(0, 5, [1,2,3,4,6])

	message = pickle.dumps(myJob1)

	channel.basic_publish(exchange='',
	                      routing_key=TASK_DESTINATION,
	                      body=message,
	                      properties=pika.BasicProperties(
	                         delivery_mode = 2, # make message persistent
	                      ))

	message = pickle.dumps(myJob2)

	channel.basic_publish(exchange='',
	                      routing_key=TASK_DESTINATION,
	                      body=message,
	                      properties=pika.BasicProperties(
	                         delivery_mode = 2, # make message persistent
	                      ))

	print(" [TASK] Sent %r" % message)
	connection.close()

def receiveTask():
	print(LOCAL_IP)
	connection = pika.BlockingConnection(pika.ConnectionParameters(
	        host=LOCAL_IP))
	channel = connection.channel()

	TASK_CONNECTION = connection
	TASK_CHANNEL = channel

	MY_TASK_QUEUE = channel.queue_declare(queue=TASK_SOURCE, durable=True)
	print(' [TASK] Waiting for messages. To exit press CTRL+C')

	channel.basic_qos(prefetch_count=1)
	channel.basic_consume(receiveTaskCallback,
	                      queue=TASK_SOURCE)
	channel.start_consuming()


def receiveTaskCallback(ch, method, properties, body):
	task = pickle.loads(body)
	print('[TASK] Received: ', task)
	taskReceivingThread = threading.Thread(target=worker_thread, args=(task,))
	taskReceivingThread.start()
	ch.basic_ack(delivery_tag = method.delivery_tag)

#def calculateTask(task):
#	print (task.data_vector)
#	task.compute()
#	print ('[TASK] Processed: ', task.data_vector)
#	time.sleep(0.5) #throttling


def worker_thread(job):
	global THROTTLING, COMPLETED_JOBS
	logging.info("Worker thread started with ", job.data_vector)

	start_time = time.time()
	job.compute()
	COMPLETED_JOBS.extend(job)
	end_time = time.time()

	logging.info("Worker thread job finished with", job.data_vector)

	sleeping_time = (end_time - start_time) * (1 - THROTTLING)
	logging.warning("Worker thread will sleep for %f seconds..." % sleeping_time)
	time.sleep(sleeping_time)

def sendState(message):
	connection = pika.BlockingConnection(pika.ConnectionParameters(
	        host=REMOTE_IP))
	channel = connection.channel()

	channel.queue_declare(queue=STATE_DESTINATION, durable=True)


	channel.basic_publish(exchange='',
	                      routing_key=STATE_DESTINATION,
	                      body=message,
	                      properties=pika.BasicProperties(
	                         delivery_mode = 2, # make message persistent
	                      ))
	print(" [x] Sent %r" % message)
	connection.close()

def receiveState():
	connection = pika.BlockingConnection(pika.ConnectionParameters(
	        host=LOCAL_IP))
	channel = connection.channel()

	channel.queue_declare(queue=STATE_SOURCE, durable=True)
	print(' [STATE] Waiting for state. To exit press CTRL+C')

	def callback(ch, method, properties, body):
		printable = pickle.loads(body)
		print(" [STATE] Received %r" % printable)
		time.sleep(body.count(b'.'))
		print(" [STATE] Done")
		ch.basic_ack(delivery_tag = method.delivery_tag)

	channel.basic_qos(prefetch_count=1)
	channel.basic_consume(callback,
	                      queue=STATE_SOURCE)
	channel.start_consuming()


def adaptor():
	curLen = MY_TASK_QUEUE.method.message_count
	if curLen > QUEUE_THRESHOLD:
		numTransfer = curLen - QUEUE_THRESHOLD
		taskArr = []
		while numTransfer>0:
			curTask = TASK_CHANNEL.basic_get(queue='remote_state_queue')
			taskArr.append(curTask)
			numTransfer -= 1
		sendTask(taskArr)

class SystemState:
	def __init__(self, job_queue, hardware_monitor):
		self.num_job = job_queue.method.message_count
		self.throttling = hardware_monitor.get_throttling
		self.cpu_use = hardware_monitor.get_cpu_info  

def state_manager(hardware_monitor):
	# peiodic policy
	while True:
		state = SystemState(hardware_monitor)
		statestr = pickle.dumps(state)
		sendState(statestr)
		time.sleep(1)

def bootstrap():
	pass

def processing():
	pass

def aggregation():
	logging.info("Aggregation phase started")

	#TODO: get results

	fp = open('result.data', 'w+')
	print result, fp.name
	fp.close()

	logging.info("Aggregation phase ended")

def main(argv):
	#parser = argparse.ArgumentParser()
	#parser.add_argument("throttling_value")
	#args = parser.parse_args()
	hardware_monitor = HardwareMonitor(0.75)#args.throttling_value)
	global STATE_DESTINATION, STATE_SOURCE, TASK_DESTINATION, TASK_SOURCE, REMOTE_IP, LOCAL_IP
	isLocal = sys.argv[1]
	if isLocal == 'true':
		STATE_DESTINATION, STATE_SOURCE = STATE_SOURCE, STATE_DESTINATION
		TASK_DESTINATION, TASK_SOURCE = TASK_SOURCE, TASK_DESTINATION
		REMOTE_IP, LOCAL_IP = LOCAL_IP, REMOTE_IP

	taskReceivingThread = threading.Thread(target=receiveTask)
	stateReceivingThread = threading.Thread(target=receiveState)
	stateManagerThread = threading.Thread(target=state_manager, args=(hardware_monitor,))

	taskReceivingThread.daemon = True
	stateReceivingThread.daemon = True
	stateManagerThread.daemon = True
	
	taskReceivingThread.start()
	stateReceivingThread.start()
	stateManagerThread.start()

	bootstrap()
	processing()
	aggregation()

	while True:
		time.sleep(1)
	
if __name__ == "__main__":
    main(sys.argv)
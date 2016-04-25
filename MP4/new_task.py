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
from enum import Enum

LOCAL_IP = '172.22.146.196'
REMOTE_IP = '172.17.82.56'

TOTAL_ELEMENT_NUM = 1024 * 1024 * 4
TOTAL_JOB_NUM = 512
QUEUE_THRESHOLD = 112
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
RESULT_SOURCE = 'local_result_queue'
IS_LOCAL = False


def sendHelper(connection, taskArr, msgType, destination):
	channel = connection.channel()

	channel.queue_declare(queue=destination, durable=True)

	for task in taskArr:
		message = pickle.dumps(task)
		channel.basic_publish(exchange='',
		                      routing_key=destination,
		                      body=message,
		                      properties=pika.BasicProperties(
		                         delivery_mode = 2, # make message persistent
		                      ))
	if msgType!='state':
		print(" [" + msgType + "] Sent %r" % task)


def receiveHelper(connection, msgType, callback, source):
	channel = connection.channel()
	
	queue = channel.queue_declare(queue=source, durable=True)

	global TASK_CHANNEL, MY_TASK_QUEUE

	if msgType == 'task' and TASK_CHANNEL is None:
		TASK_CHANNEL = channel

	if msgType == 'task' and MY_TASK_QUEUE is None:
		MY_TASK_QUEUE = queue

	if msgType!='state':
		print(' [' + msgType + '] Waiting. To exit press CTRL+C')

	channel.basic_qos(prefetch_count=1)
	channel.basic_consume(callback,
	                      queue=source)
	channel.start_consuming()


def receiveTaskCallback(ch, method, properties, body):
	task = pickle.loads(body)
	print('[TASK] Received: ', task)
	taskReceivingThread = threading.Thread(target=worker_thread, args=(task,))
	taskReceivingThread.start()
	ch.basic_ack(delivery_tag = method.delivery_tag)


def worker_thread(job):
	global THROTTLING, COMPLETED_JOBS, IS_LOCAL, TOTAL_JOB_NUM
	logging.info("Worker thread started with ", job.id)

	start_time = time.time()
	job.compute()
	COMPLETED_JOBS.append(job)
	end_time = time.time()

	logging.info("Worker thread job finished with", job.id)

	if not(IS_LOCAL):
		sendConnection = pika.BlockingConnection(pika.ConnectionParameters(
	    	host=REMOTE_IP))
		sendHelper(sendConnection, [job], 'result', RESULT_SOURCE)
		sendConnection.close()
	if IS_LOCAL:
		TOTAL_JOB_NUM -= 1
		fp = open('result.data', 'a')
		for item in job.data_vector:
			fp.write(str(item)+' ')
		fp.write('\n')
		fp.close()

	sleeping_time = (end_time - start_time) * (1 - THROTTLING)
	logging.warning("Worker thread will sleep for %f seconds..." % sleeping_time)
	time.sleep(sleeping_time)


def receiveStateCallback(ch, method, properties, body):
	printable = pickle.loads(body)
	# print(" [STATE] Received %r" % printable)
	# time.sleep(body.count(b'.'))
	# print(" [STATE] Done")
	ch.basic_ack(delivery_tag = method.delivery_tag)


def receiveResultCallback(ch, method, properties, body):
	global TOTAL_JOB_NUM
	TOTAL_JOB_NUM -= 1
	result = pickle.loads(body)
	print('[RESULT] Received: ', result.id)
	fp = open('result.data', 'a')
	for item in result.data_vector:
		fp.write(str(item)+' ')
	fp.write('\n')
	fp.close()
	ch.basic_ack(delivery_tag = method.delivery_tag)


def adaptor():
	global MY_TASK_QUEUE, TASK_CHANNEL, TASK_SOURCE
	while MY_TASK_QUEUE is None:
		time.sleep(1)

	while True:	
		curLen = MY_TASK_QUEUE.method.message_count
		if curLen > QUEUE_THRESHOLD:
			numTransfer = curLen - QUEUE_THRESHOLD
			taskArr = []
			while numTransfer>0:
				curTask = TASK_CHANNEL.basic_get(queue=TASK_SOURCE)
				taskArr.append(curTask)
				numTransfer -= 1

			sendConnection = pika.BlockingConnection(pika.ConnectionParameters(
	        	host=REMOTE_IP))
			sendHelper(sendConnection, taskArr, 'task', TASK_DESTINATION)
			sendConnection.close()


class SystemState:
	def __init__(self, job_queue, hardware_monitor):
		self.num_job = job_queue.method.message_count
		self.throttling = hardware_monitor.get_throttling()
		self.cpu_use = hardware_monitor.get_cpu_info()


def state_manager(hardware_monitor):
	# peiodic policy
	while MY_TASK_QUEUE is None:
		time.sleep(1)

	sendConnection = pika.BlockingConnection(pika.ConnectionParameters(
        	host=REMOTE_IP))
	while True:
		state = SystemState(MY_TASK_QUEUE, hardware_monitor)
		statestr = pickle.dumps(state)
		sendHelper(sendConnection, [statestr], 'state', STATE_DESTINATION)
		time.sleep(1)
	sendConnection.close()


def bootstrap(work):
	global TOTAL_ELEMENT_NUM, TOTAL_JOB_NUM
	jobs_for_local = []
	jobs_for_remote = []

	length = TOTAL_ELEMENT_NUM/TOTAL_JOB_NUM
	for i in xrange(0, TOTAL_JOB_NUM/2):
		curr_job = Job(i, length, work[(i*length):((i+1)*length)])
		jobs_for_local.append(curr_job)
	for i in xrange(TOTAL_JOB_NUM/2, TOTAL_JOB_NUM):
		curr_job = Job(i, length, work[(i*length):((i+1)*length)])
		jobs_for_remote.append(curr_job)

	sendConnection = pika.BlockingConnection(pika.ConnectionParameters(
            host=LOCAL_IP))
	sendHelper(sendConnection, jobs_for_local, 'task', TASK_SOURCE)
	sendConnection.close()
	sendConnection = pika.BlockingConnection(pika.ConnectionParameters(
            host=REMOTE_IP))
	sendHelper(sendConnection, jobs_for_remote, 'task', TASK_DESTINATION)
	sendConnection.close()


def aggregation():
	logging.info("Aggregation phase started")

	with open('result.data', 'r') as fi:
		for line in fi:
			print (line)
	fi.close()
	logging.info("Aggregation phase ended")

def main(argv):
	parser = argparse.ArgumentParser()
	parser.add_argument('throttling_value', type=float)
	parser.add_argument('is_local', type=bool)
	args = parser.parse_args()

	hardware_monitor = HardwareMonitor(args.throttling_value)
	global STATE_DESTINATION, STATE_SOURCE, TASK_DESTINATION, TASK_SOURCE, REMOTE_IP, LOCAL_IP, IS_LOCAL, TOTAL_JOB_NUM
	IS_LOCAL = args.is_local
	if IS_LOCAL:
		STATE_DESTINATION, STATE_SOURCE = STATE_SOURCE, STATE_DESTINATION
		TASK_DESTINATION, TASK_SOURCE = TASK_SOURCE, TASK_DESTINATION
		REMOTE_IP, LOCAL_IP = LOCAL_IP, REMOTE_IP
		receiveResultConnection = pika.BlockingConnection(pika.ConnectionParameters(
        	host=LOCAL_IP))
		resultReceivingThread = threading.Thread(target=receiveHelper, args=(receiveResultConnection, 'result', receiveResultCallback, RESULT_SOURCE))
		resultReceivingThread.daemon = True
		resultReceivingThread.start()

	receiveTaskConnection = pika.BlockingConnection(pika.ConnectionParameters(
        host=LOCAL_IP))
	receiveStateConnection = pika.BlockingConnection(pika.ConnectionParameters(
        host=LOCAL_IP))

	taskReceivingThread = threading.Thread(target=receiveHelper, args=(receiveTaskConnection, 'task', receiveTaskCallback, TASK_SOURCE))
	stateReceivingThread = threading.Thread(target=receiveHelper, args=(receiveStateConnection, 'state', receiveStateCallback, STATE_SOURCE))
	stateManagerThread = threading.Thread(target=state_manager, args=(hardware_monitor,))
	adaptorThread = threading.Thread(target=adaptor)

	taskReceivingThread.daemon = True
	stateReceivingThread.daemon = True
	stateManagerThread.daemon = True
	adaptorThread.daemon = True

	taskReceivingThread.start()
	stateReceivingThread.start()
	stateManagerThread.start()
	adaptorThread.start()

	if IS_LOCAL:
		work = [1.111111] * TOTAL_ELEMENT_NUM
		bootstrap(work)
		while TOTAL_JOB_NUM>0:
			time.sleep(1)
		aggregation()
	else:
		while True:
			time.sleep(1)
	
if __name__ == "__main__":
    main(sys.argv)

# local
import pika
import sys
import time
import threading
import pickle
from job import Job

LOCAL_IP = '172.22.146.196'
REMOTE_IP = '172.22.146.245'
QUEUE_THRESHOLD = 400
MY_TASK_QUEUE = None 
TASK_CONNECTION = None
TASK_CHANNEL = None
TASK_THREADS = []
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
	message = sendable
	channel.basic_publish(exchange='',
	                      routing_key=TASK_DESTINATION,
	                      body=message,
	                      properties=pika.BasicProperties(
	                         delivery_mode = 2, # make message persistent
	                      ))
	print(" [TASK] Sent %r" % message)
	connection.close()

def receiveTask():
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
	print(task)
	taskReceivingThread = threading.Thread(target=calculateTask, args=(task,))
	ch.basic_ack(delivery_tag = method.delivery_tag)

def calculateTask(task):
	task.compute()


def sendState():
	connection = pika.BlockingConnection(pika.ConnectionParameters(
	        host=REMOTE_IP))
	channel = connection.channel()

	channel.queue_declare(queue=STATE_DESTINATION, durable=True)

	curState = _get_state_;

	channel.basic_publish(exchange='',
	                      routing_key=STATE_DESTINATION,
	                      body=curState,
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


def main(argv):
	global STATE_DESTINATION, STATE_SOURCE, TASK_DESTINATION, TASK_SOURCE, REMOTE_IP, LOCAL_IP
	isLocal = sys.argv[1]
	if isLocal == 'true':
		STATE_DESTINATION, STATE_SOURCE = STATE_SOURCE, STATE_DESTINATION
		TASK_DESTINATION, TASK_SOURCE = TASK_SOURCE, TASK_DESTINATION
		REMOTE_IP, LOCAL_IP = LOCAL_IP, REMOTE_IP

	taskReceivingThread = threading.Thread(target=receiveTask)
	stateReceivingThread = threading.Thread(target=receiveState)
	taskReceivingThread.start()
	stateReceivingThread.start()
	taskReceivingThread.join()
	stateReceivingThread.join()

	
if __name__ == "__main__":
    main(sys.argv)
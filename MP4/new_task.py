# remote
import pika
import sys
import time
import threading
import pickle
from job import Job

REMOTE_IP = '172.22.146.196'
LOCAL_IP = '172.22.146.245'
QUEUE_THRESHOLD = 400
MY_TASK_QUEUE = None 
TASK_CONNECTION = None
TASK_CHANNEL = None
TASK_THREADS = []
THROTTLING = 1

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
	        host=LOCAL_IP))
	channel = connection.channel()

	channel.queue_declare(queue='local_task_queue', durable=True)
	wow = {'Name': 'Zara', 'Age': 7, 'Class': 'First'};
	sendable = pickle.dumps(wow)
	message = ' '.join(sys.argv[1:]) or sendable
	channel.basic_publish(exchange='',
	                      routing_key='local_task_queue',
	                      body=message,
	                      properties=pika.BasicProperties(
	                         delivery_mode = 2, # make message persistent
	                      ))
	print(" [x] Sent %r" % message)
	connection.close()

def receiveTask():
	connection = pika.BlockingConnection(pika.ConnectionParameters(
	        host=REMOTE_IP))
	channel = connection.channel()

	TASK_CONNECTION = connection
	TASK_CHANNEL = channel

	MY_TASK_QUEUE = channel.queue_declare(queue='remote_task_queue', durable=True)
	print(' [*] Waiting for messages. To exit press CTRL+C')

	def callback(ch, method, properties, body):
		printable = pickle.loads(body)
		print(" [x] Received %r" % printable)
		time.sleep(body.count(b'.'))
		print(" [x] Done")
		ch.basic_ack(delivery_tag = method.delivery_tag)

	channel.basic_qos(prefetch_count=1)
	channel.basic_consume(callback,
	                      queue='remote_task_queue')
	channel.start_consuming()

def sendState():
	connection = pika.BlockingConnection(pika.ConnectionParameters(
	        host=LOCAL_IP))
	channel = connection.channel()

	channel.queue_declare(queue='local_state_queue', durable=True)

	curState = _get_state_;

	channel.basic_publish(exchange='',
	                      routing_key='local_state_queue',
	                      body=curState,
	                      properties=pika.BasicProperties(
	                         delivery_mode = 2, # make message persistent
	                      ))
	print(" [x] Sent %r" % message)
	connection.close()

def receiveTask():
	connection = pika.BlockingConnection(pika.ConnectionParameters(
	        host=REMOTE_IP))
	channel = connection.channel()

	channel.queue_declare(queue='remote_state_queue', durable=True)
	print(' [*] Waiting for messages. To exit press CTRL+C')

	channel.basic_qos(prefetch_count=1)
	channel.basic_consume(receiveTaskCallback,
	                      queue='remote_state_queue')
	channel.start_consuming()

def receiveTaskCallback(ch, method, properties, body):
	task = pickle.loads(body)
	taskReceivingThread = threading.Thread(target=calculateTask, args=(task,))
	ch.basic_ack(delivery_tag = method.delivery_tag)

def calculateTask(task):
	task.compute()

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


def main():
	taskReceivingThread = threading.Thread(target=receiveTask)
	stateReceivingThread = threading.Thread(target=receiveState)
	taskReceivingThread.start()
	stateReceivingThread.start()
	taskReceivingThread.join()
	stateReceivingThread.join()

	
if __name__ == "__main__":
    main()
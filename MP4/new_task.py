# remote
import pika
import sys
import time
import threading
import pickle

REMOTE_IP = '172.22.146.196'
LOCAL_IP = '172.22.146.245'

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


def sendTask():
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

	channel.queue_declare(queue='remote_task_queue', durable=True)
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

	def callback(ch, method, properties, body):
		printable = pickle.loads(body)
		print(" [x] Received %r" % printable)
		time.sleep(body.count(b'.'))
		print(" [x] Done")
		ch.basic_ack(delivery_tag = method.delivery_tag)

	channel.basic_qos(prefetch_count=1)
	channel.basic_consume(callback,
	                      queue='remote_state_queue')
	channel.start_consuming()

def main():
	taskReceivingThread = threading.Thread(target=receiveTask)
	stateReceivingThread = threading.Thread(target=receiveState)
	taskReceivingThread.start()
	stateReceivingThread.start()
	sendTask()
	taskReceivingThread.join()
	stateReceivingThread.join()

	
if __name__ == "__main__":
    main()
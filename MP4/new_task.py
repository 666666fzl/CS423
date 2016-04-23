# remote
import pika
import sys
import time
import threading

REMOTE_IP = '172.22.146.196'
LOCAL_IP = '172.22.146.245'

class myThread (threading.Thread):
	def __init__(self, threadID, name, counter):
		threading.Thread.__init__(self)
		self.threadID = threadID
		self.name = name
		self.counter = counter

	def run(self):
		print "Starting " + self.name
		receiveTask()
		print "Exiting " + self.name


def sendTask():
	connection = pika.BlockingConnection(pika.ConnectionParameters(
	        host=REMOTE_IP))
	channel = connection.channel()

	channel.queue_declare(queue='task_queue', durable=True)

	message = ' '.join(sys.argv[1:]) or "Hello World!"
	channel.basic_publish(exchange='',
	                      routing_key='task_queue',
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

	channel.queue_declare(queue='task_queue', durable=True)
	print(' [*] Waiting for messages. To exit press CTRL+C')

	def callback(ch, method, properties, body):
	    print(" [x] Received %r" % body)
	    time.sleep(body.count(b'.'))
	    print(" [x] Done")
	    ch.basic_ack(delivery_tag = method.delivery_tag)

	channel.basic_qos(prefetch_count=1)
	channel.basic_consume(callback,
	                      queue='task_queue')

	channel.start_consuming()


def main():
	receivingThread = myThread(1, "Thread-1", 1)
	receivingThread.start()
	sendTask()

if __name__ == "__main__":
    main()
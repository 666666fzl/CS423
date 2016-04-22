import socket               # Import socket module

def main()
	MY_IP = "172.22.146.196"
	HIS_IP = "172.22.146.245"
	TRANSFER_PORT = 5000

	serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)         # Create a socket object
	serverSocket.bind((MY_IP, TRANSFER_PORT))        # Bind to the port
	serverSocket.listen(5)                 # Now wait for client connection.

	clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)         # Create a socket object

	while True:
		ret = clientSocket.connect_ex((HIS_IP, TRANSFER_PORT))
		if ret==0:
			print "connect to "+HIS_IP
			break

	c, addr = serverSocket.accept()     # Establish connection with client.
	print 'Got connection from', addr
	c.send('Thank you for connecting')

if __name__ == "__main__":
    main()



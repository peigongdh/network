import socket

local_ip = "127.0.0.1"
localPort = 9099
bufferSize = 1024
msg = "Hello UDP Client"

bytes_to_send = str.encode(msg)
server_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
server_socket.bind((local_ip, localPort))
print("UDP server up and listening")
while (True):
    bytes_address_pair = server_socket.recvfrom(bufferSize)
    message = bytes_address_pair[0]
    address = bytes_address_pair[1]
    client_msg = "client:{}, msg: {}".format(address, message)
    print(client_msg)
    # Sending a reply to client
    # server_socket.sendto(bytes_to_send, address)

import socket
import threading
import sys
import time

def receive_messages(client_socket):
    while True:
        try:
            message = client_socket.recv(1024).decode('utf-8')
            if not message:
                print("Conexión cerrada por el servidor.")
                break
            print(f"Servidor: {message}")
        except ConnectionResetError:
            print("Conexión con el servidor perdida.")
            break

def send_messages(client_socket):
    for i in range(1,200):
        message = "grog "+sys.argv[1]+" iter "+str(i)+"\n"
        client_socket.send(message.encode('utf-8'))
        time.sleep(0.2)

def main():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect(("127.0.0.1", 8080))
    print("Conectado al servidor TCP en 127.0.0.1:8080")
    
    receive_thread = threading.Thread(target=receive_messages, args=(client_socket,))
    send_thread = threading.Thread(target=send_messages, args=(client_socket,))
    
    receive_thread.start()
    send_thread.start()
    
    receive_thread.join()
    send_thread.join()
    
    print("Cliente desconectado.")

if __name__ == "__main__":
    main()

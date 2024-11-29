import socket
import threading
import time
import re

def recvuntil(sock, delimiter):
    data = b''
    while not data.endswith(delimiter):
        chunk = sock.recv(1)
        if not chunk:  # Si el socket se cierra
            break
        data += chunk
    return data

class Examen:
    def __init__(self, nombre, preguntas=None):
        if preguntas is None:
            preguntas = []
        self.nombre = nombre
        self.preguntas = preguntas
        self.disponible = True

    def mostrar_preguntas(self):
        if not self.disponible:
            print(f"El examen {self.nombre} ya ha sido seleccionado y no está disponible.")
            return None
        self.disponible = False
        return self.preguntas

def connect_to_server(server_ip, server_port):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((server_ip, server_port))
    return client_socket

def authenticate(client_socket, matricula, password):
    credentials = f"{matricula},{password}"
    client_socket.sendall(credentials.encode())
    response = client_socket.recv(2048).decode()
    print(response)
    return "exitosa" in response

def request_exam(client_socket, exam_name):
    client_socket.sendall(exam_name.encode())  # Envía el nombre del examen
    buffer = ""
    while True:
        # Recibe datos del socket
        data = client_socket.recv(2048).decode()
        if not data:
            print("Desconexión inesperada del servidor.")
            break
        
        buffer += data.replace('\x03', '')  # Limpia caracteres no deseados
        match = re.search(r'^(.*?\?.*?)a\)(.+?)b\)(.+?)c\)(.+?)$', buffer, re.DOTALL)
        
        if match:
            pregunta = match.group(1).strip()
            opcion_a = match.group(2).strip()
            opcion_b = match.group(3).strip()
            opcion_c = match.group(4).strip()

            print(f"Pregunta: {pregunta}")
            print(f"A) {opcion_a}")
            print(f"B) {opcion_b}")
            print(f"C) {opcion_c}")

            # Limpiar buffer y preparar para la siguiente pregunta
            buffer = buffer[match.end():]

            # Solicitar respuesta al usuario
            while True:
                respuesta = input("Selecciona una opción (A/B/C): ").strip().upper()
                if respuesta in ['A', 'B', 'C']:
                    break
                print("Respuesta inválida. Por favor selecciona A, B o C.")

            # Enviar respuesta al servidor
            client_socket.sendall(respuesta.encode())
            print(f"Respuesta {respuesta} enviada.")
            
            # Opcional: Esperar confirmación del servidor
            confirmacion = client_socket.recv(2048).decode()
            print(f"Confirmación del servidor: {confirmacion}")

        elif len(buffer) > 0 and len(data) == 0:
            print("Advertencia: Datos incompletos o desconexión.")
            break


def send_answers(client_socket, respuestas):
    respuestas_str = ",".join(respuestas)  # Convertir las respuestas a una cadena separada por comas
    client_socket.sendall(respuestas_str.encode())
    print(f"Respuestas enviadas: {respuestas_str}")


def main():
    server_ip = '192.168.100.142'
    server_port = 8080
    matricula = input("Introduce tu matrícula: ")
    password = input("Introduce tu contraseña: ")
    client_socket = connect_to_server(server_ip, server_port)
    
    if authenticate(client_socket, matricula, password):
        examenes = [
            Examen("Matemáticas"),
            Examen("Español"),
            Examen("Inglés")
        ]
        
        count_exam = len(examenes)-1
        
        while True:
            print("Exámenes disponibles:")
            for i, examen in enumerate(examenes):
                if examen.disponible:
                    print(f"{i+1}. {examen.nombre}")
            seleccion = int(input("Selecciona un examen (1-3): ")) - 1
            if seleccion < len(examenes) and count_exam > 0:
                if seleccion + 1 == 1: 
                    preguntas = request_exam(client_socket, "Matemáticas")
                elif seleccion + 1 == 2:
                    print("[!] Entro")
                    preguntas = request_exam(client_socket, "Español")
                elif seleccion + 1 == 3:
                    preguntas = request_exam(client_socket, "Inglés")
                else:
                    print("[!] No hay ningún examen...")

                if preguntas:
                    take_exam(client_socket, preguntas)
                
                count_exam -= 1

            elif count_exam == 0:
                print("[!] Ya no existen examenes...")
                break
            else:
                print("Selección inválida.")
    
    client_socket.close()


if __name__ == "__main__":
    main()

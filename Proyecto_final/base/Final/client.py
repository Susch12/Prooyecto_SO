import socket

def main():
    # Configuración del cliente
    host = '127.0.0.1'  # Dirección IP del servidor
    port = 8080         # Puerto del servidor

    # Crear un socket TCP/IP
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        # Conectar al servidor
        client_socket.connect((host, port))
        print("[+] Conectado al servidor")

        # Recibir y enviar matrícula
        matricula = input("[!] Por favor, ingrese su matrícula: ")
        client_socket.sendall(matricula.encode())
        
        # Recibir y enviar contraseña
        response = client_socket.recv(1024).decode()
        print(response)
        password = input("[!] Por favor, ingrese su contraseña: ")
        client_socket.sendall(password.encode())

        # Recibir respuesta de autenticación
        response = client_socket.recv(1024).decode()
        print(response)
        if "Autenticación fallida" in response:
            client_socket.close()
            return

        # Seleccionar examen
        response = client_socket.recv(1024).decode()
        print(response)
        examen = input("[+] Seleccione el examen (1. Matemáticas, 2. Español, 3. Inglés): ")
        client_socket.sendall(examen.encode())

        # Recibir preguntas y enviar respuestas
        for _ in range(10):  # Suponiendo que hay un máximo de 10 preguntas
            response = client_socket.recv(1024).decode()
            print(response)
            if "Examen terminado" in response or "No hay preguntas disponibles" in response:
                break
            respuesta = input("Ingrese su respuesta (A, B, C): ")
            client_socket.sendall(respuesta.encode())

        # Recibir puntaje final
        response = client_socket.recv(1024).decode()
        print(response)

    except Exception as e:
        print(f"[!] Error: {e}")
    finally:
        client_socket.close()

if __name__ == "__main__":
    main()

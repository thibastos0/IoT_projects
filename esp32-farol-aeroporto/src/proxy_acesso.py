import socket
import threading

# CONFIGURAÇÕES
PORTA_DO_SIMULADOR = 8180  # <-- Mude para a porta que o VSCode abriu aí
PORTA_PARA_O_OUTRO_PC = 9000  # Porta que o outro computador vai digitar no navegador

def lidar_com_cliente(origem, destino_porta):
    # Conecta com o simulador local do Wokwi
    destino = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        destino.connect(('127.0.0.1', destino_porta))
    except Exception as e:
        print(f"Erro ao conectar ao simulador: {e}")
        origem.close()
        return

    # Função para encaminhar dados de um lado para o outro
    def encaminhar(origem_sock, destino_sock):
        try:
            while True:
                dados = origem_sock.recv(4096)
                if not dados:
                    break
                destino_sock.sendall(dados)
        except:
            pass
        finally:
            origem_sock.close()
            destino_sock.close()

    # Cria duas threads para fluxo bidirecional (ida e volta de dados)
    threading.Thread(target=encaminhar, args=(origem, destino), daemon=True).start()
    threading.Thread(target=encaminhar, args=(destino, origem), daemon=True).start()

def iniciar_proxy():
    # 0.0.0.0 permite conexões vindas de OUTROS computadores na rede
    servidor = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    servidor.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    servidor.bind(('0.0.0.0', PORTA_PARA_O_OUTRO_PC))
    servidor.listen(5)
    print(f"🚀 Proxy Ativo! No outro PC, acesse: http://192.168.0.13:{PORTA_PARA_O_OUTRO_PC}")

    try:
        while True:
            cliente_sock, endereco = servidor.accept()
            threading.Thread(target=lidar_com_cliente, args=(cliente_sock, PORTA_DO_SIMULADOR), daemon=True).start()
    except KeyboardInterrupt:
        print("\nProxy finalizado.")
    finally:
        servidor.close()

if __name__ == "__main__":
    iniciar_proxy()
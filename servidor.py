import tkinter as tk
from tkinter import filedialog, messagebox
import socket
import threading
import json
import queue
import time  # <--- Usaremos esta biblioteca
import csv
from datetime import datetime

# --- Configurações ---
UDP_IP = "0.0.0.0"
UDP_PORT = 4444
LIMITE_ALERTA = 2.5
HISTORICO_SEGUNDOS = 60 # O gráfico mostrará os últimos 60 segundos

class UDPListenerThread(threading.Thread):
    """
    Esta é a "Thread do Rádio".
    """
    def __init__(self, data_queue):
        super().__init__()
        self.data_queue = data_queue
        self.daemon = True
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((UDP_IP, UDP_PORT))
        print(f"Servidor escutando em {UDP_IP}:{UDP_PORT}...")

    def run(self):
        while True:
            try:
                data_bytes, addr = self.sock.recvfrom(1024) 
                data_str = data_bytes.decode('utf-8')
                data_json = json.loads(data_str)
                
                # Adiciona nosso próprio timestamp (a hora do PC)
                data_json['timestamp_servidor'] = time.time()
                
                self.data_queue.put(data_json)
                
            except Exception as e:
                print(f"Erro ao processar pacote UDP: {e}")

class MainApplication(tk.Tk):
    """
    Esta é a Classe da Janela Principal.
    """
    def __init__(self):
        super().__init__()
        
        self.title("Monitor de Sensor de Som")
        self.geometry("800x600")

        self.data_history = []  # Armazena tuplas (timestamp_servidor, valor)
        self.data_queue = queue.Queue()

        # --- 1. Criar os Elementos Visuais (Widgets) ---
        
        self.lbl_title = tk.Label(self, text="Valor Atual do Sensor:", font=("Arial", 16))
        self.lbl_title.pack(pady=10)

        self.lbl_valor = tk.Label(self, text="Aguardando dados...", font=("Arial", 36, "bold"))
        self.lbl_valor.pack(pady=20)

        # --- Gráfico (Matplotlib) ---
        try:
            from matplotlib.figure import Figure
            from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
            
            self.fig = Figure(figsize=(8, 4), dpi=100)
            self.ax = self.fig.add_subplot(111)
            self.ax.set_title(f"Histórico (Últimos {HISTORICO_SEGUNDOS}s)")
            self.ax.set_xlabel("Tempo (Segundos)") # Eixo X agora é tempo real
            self.ax.set_ylabel("Tensão (V)")
            self.line, = self.ax.plot([], [], 'r-')

            self.canvas = FigureCanvasTkAgg(self.fig, self)
            self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
            print("DEBUG: Canvas do Matplotlib criado com sucesso.")

        except ImportError:
            tk.Label(self, text="Instale 'matplotlib' para ver o gráfico.", fg="red").pack()
            self.canvas = None

        self.btn_save = tk.Button(self, text="Salvar Log em CSV", command=self.salvar_csv)
        self.btn_save.pack(pady=10)

        # --- 2. Iniciar os Processos ---
        self.start_listener()
        self.check_queue()

    def start_listener(self):
        self.listener_thread = UDPListenerThread(self.data_queue)
        self.listener_thread.start()

    def check_queue(self):
        """
        Função Mágica do Tkinter (Otimizada)
        """
        try:
            ultimo_dado_valido = None 
            
            while not self.data_queue.empty():
                data_json = self.data_queue.get_nowait()
                
                if "valor" in data_json and "timestamp_servidor" in data_json:
                    valor = data_json["valor"]
                    timestamp = data_json["timestamp_servidor"] 
                    self.data_history.append((timestamp, valor))
                    ultimo_dado_valido = (valor, timestamp)
            
            if ultimo_dado_valido:
                valor_atual, ts_atual = ultimo_dado_valido
                # Chama a função de desenho APENAS UMA VEZ
                self.update_display(valor_atual, ts_atual)
                
        except queue.Empty:
            pass 
        
        self.after(100, self.check_queue) # Re-agenda a verificação

    def update_display(self, valor, ultimo_timestamp):
        # --- Requisito: Valor Atual ---
        self.lbl_valor.config(text=f"{valor:.4f} V")

        # --- Requisito: Alerta Visual ---
        if valor > LIMITE_ALERTA:
            self.lbl_valor.config(bg="red", fg="white")
        else:
            # --- LINHA CORRIGIDA ---
            # Trocamos "SystemButtonFace" por "grey90", uma cor segura
            self.lbl_valor.config(bg="grey90", fg="black") 
            # --- FIM DA CORREÇÃO ---

        # --- Requisito: Histórico Gráfico ---
        if self.canvas:
            agora = ultimo_timestamp 
            limite_tempo = agora - HISTORICO_SEGUNDOS
            
            self.data_history = [ponto for ponto in self.data_history if ponto[0] >= limite_tempo]
            
            tempos = [p[0] for p in self.data_history]
            valores = [p[1] for p in self.data_history]
            
            self.line.set_data(tempos, valores)
            
            self.ax.set_xlim(limite_tempo, agora + 1) 
            
            if valores:
                min_y = min(valores)
                max_y = max(valores)
                padding = (max_y - min_y) * 0.1 
                if padding < 0.01: padding = 0.01 
                self.ax.set_ylim(min_y - padding, max_y + padding)
            else:
                self.ax.set_ylim(-1, 1)
            
            # Usando o draw_idle() que é mais 'gentil' com o Tkinter
            self.fig.canvas.draw_idle()

    def salvar_csv(self):
        try:
            filepath = filedialog.asksaveasfilename(
                defaultextension=".csv",
                filetypes=[("Arquivos CSV", "*.csv"), ("Todos os arquivos", "*.*")],
                title="Salvar Log do Sensor"
            )
            
            if not filepath: return

            with open(filepath, 'w', newline='', encoding='utf-8') as f:
                writer = csv.writer(f)
                writer.writerow(["Timestamp_Servidor", "Valor (V)", "Data/Hora Legivel"])
                
                for timestamp, valor in self.data_history:
                    data_legivel = datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([timestamp, valor, data_legivel])
            
            messagebox.showinfo("Sucesso", f"Log salvo com sucesso em:\n{filepath}")

        except Exception as e:
            messagebox.showerror("Erro ao Salvar", f"Não foi possível salvar o arquivo:\n{e}")

# --- Ponto de Entrada Principal ---
if __name__ == "__main__":
    try:
        app = MainApplication()
        app.mainloop()
    except ImportError:
        print("Erro: 'matplotlib' não está instalado.")
        print("Por favor, instale com: sudo apt install python3-matplotlib")
    except KeyboardInterrupt:
        print("\nServidor interrompido.")
    except Exception as e:
        # Se o erro "unknown color name" acontecer de novo, ele aparecerá aqui
        print(f"ERRO CRÍTICO: {e}")
        input("Pressione Enter para sair...")

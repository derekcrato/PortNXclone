import json
import os
import random
import subprocess
import sys
import tkinter as tk
from tkinter import messagebox, ttk

# Verificador automático de bibliotecas antes de carregar o resto do app
MISSING_LIBS = []

try:
    import zstandard as zstd
except ImportError:
    MISSING_LIBS.append("zstandard")

try:
    from Crypto.PublicKey import RSA
    from Crypto.Cipher import PKCS1_OAEP, AES
    from Crypto.Hash import SHA256
except ImportError:
    MISSING_LIBS.append("pycryptodome")


# Chave Pública RSA embutida diretamente no código
EMBEDDED_PUBLIC_KEY = """-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA8I7Rmtn4ccvjHgQDxalH
QfuP6y8cb8Yy4FZQdzw26FK+6XvoqW1pRj21/cXET/PcsovST1DFOORx2Ep2Ud0N
u7ssdlU+kVwi6/c9NG2r+N36zPS9tOohN18nRztCeODRpzLrS85Tt/m+vX1/YRJD
3jMTHv8xkpctKFxhVewNqE3vUf8fHwK2xjHQj80poer168FwC6TYeZaUdmrwSqs3
Z/ROdowIuPxZfOVNy+8S+SK1/H+wzrdRxpK/Hz1olXwEHhy8syt0yWy5AeIILWhd
u0AXJeX3/o7HUVfY5t2Aow9mvQWQFlGGbpYvKAifUJ6CCjMpd8Bj0IIvWUAu3Kfa
kQIDAQAB
-----END PUBLIC KEY-----"""


def check_and_install_dependencies():
    """
    Verifica se existem bibliotecas ausentes e oferece a opção 
    de baixá-las e instalá-las automaticamente usando o pip.
    """
    if MISSING_LIBS:
        root_init = tk.Tk()
        root_init.withdraw()

        libs_formatadas = ", ".join(MISSING_LIBS)
        pergunta = (
            f"O sistema detectou que as seguintes bibliotecas necessárias estão ausentes:\n\n"
            f"👉 {libs_formatadas}\n\n"
            f"Deseja que o PortShop baixe e instale-as automaticamente agora?"
        )
        
        resposta = messagebox.askyesno("Dependências Ausentes", pergunta)
        
        if resposta:
            messagebox.showinfo("Instalando", "A instalação começou. Por favor, aguarde alguns segundos...\n(Não feche o programa)")
            try:
                comando = [sys.executable, "-m", "pip", "install"] + MISSING_LIBS
                subprocess.check_call(comando, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                
                messagebox.showinfo("Sucesso", "Todas as bibliotecas foram instaladas com sucesso!\nO programa será iniciado.")
                root_init.destroy()
                return True
            except Exception as e:
                messagebox.showerror("Erro na Instalação", f"Não foi possível instalar automaticamente:\n{e}\n\nTente instalar manualmente usando:\npip install zstandard pycryptodome")
                root_init.destroy()
                return False
        else:
            messagebox.showwarning(
                "Aviso", 
                "O programa vai iniciar, mas a função de 'Gerar arquivo index.tfl' não funcionará até que você instale as dependências."
            )
            root_init.destroy()
            return False
    return True


class JSONEditorApp:

    def __init__(self, root):
        self.root = root
        self.root.title("PortShop JSON Editor & Crypt - By Xeno")
        self.root.geometry("920x680")

        # 1º: Definição dos caminhos absolutos dinâmicos
        base_dir = os.path.dirname(os.path.abspath(__file__))
        self.filename = os.path.join(base_dir, "index.json")
        self.output_tfl = os.path.join(base_dir, "index.tfl")

        # Carrega a chave pública embutida no código
        self.public_key = None
        self.load_embedded_key()

        self.data = {"files": [], "titledb": {}, "success": "Bem-vindos ao PortShop!"}
        self.recently_added_id = None

        # 2º: Carregar dados do JSON e criar interface
        self.load_or_create_json()
        self.clean_orphan_urls()  
        self.create_widgets()
        self.update_game_list()

    def load_embedded_key(self):
        """Importa a chave pública diretamente da constante no código."""
        try:
            self.public_key = RSA.import_key(EMBEDDED_PUBLIC_KEY.strip())
        except Exception as e:
            print(f"[Aviso] Erro ao carregar a chave embutida: {e}")

    def load_or_create_json(self):
        """Carrega o arquivo JSON ou cria um arquivo limpo se não existir."""
        if os.path.exists(self.filename):
            try:
                with open(self.filename, "r", encoding="utf-8") as f:
                    self.data = json.load(f)
            except Exception as e:
                messagebox.showerror(
                    "Erro", f"Não foi possível ler o arquivo JSON: {e}"
                )
        else:
            try:
                with open(self.filename, "w", encoding="utf-8") as f:
                    json.dump(self.data, f, indent=4, ensure_ascii=False)
                print(f"[Info] Arquivo '{self.filename}' não foi encontrado. Um novo arquivo limpo foi gerado.")
            except Exception as e:
                messagebox.showerror("Erro", f"Não foi possível criar o arquivo inicial JSON: {e}")

    def clean_orphan_urls(self):
        """Verifica e limpa URLs órfãs logo na inicialização."""
        if "files" not in self.data or "titledb" not in self.data:
            return

        valid_ids = self.data["titledb"].keys()
        original_count = len(self.data["files"])
        cleaned_files = []

        for file_obj in self.data["files"]:
            url = file_obj.get("url", "")
            has_valid_game = False
            for game_id in valid_ids:
                if game_id in url:
                    has_valid_game = True
                    break
            
            if has_valid_game:
                cleaned_files.append(file_obj)

        if len(cleaned_files) < original_count:
            self.data["files"] = cleaned_files
            self.save_json()

    def wrap_key(self, key, pub_key):
        """Envolve a chave AES com a chave pública RSA usando OAEP."""
        cipher = PKCS1_OAEP.new(pub_key, hashAlgo=SHA256, label=b'')
        return cipher.encrypt(key)

    def create_widgets(self):
        # Painel Esquerdo: Lista de Jogos
        left_frame = tk.Frame(self.root, width=300)
        left_frame.pack(side=tk.LEFT, fill=tk.Y, padx=10, pady=10)

        self.list_label = tk.Label(left_frame, text="Jogos Cadastrados (0):", font=("Arial", 12, "bold"))
        self.list_label.pack(anchor=tk.W)

        self.game_listbox = tk.Listbox(left_frame, width=40)
        self.game_listbox.pack(fill=tk.BOTH, expand=True, pady=5)
        self.game_listbox.bind("<<ListboxSelect>>", self.on_game_select)

        # Botões sob a lista
        btn_frame = tk.Frame(left_frame)
        btn_frame.pack(fill=tk.X, pady=5)

        tk.Button(btn_frame, text="Novo Jogo", bg="#4CAF50", fg="white", command=self.prepare_for_new_game).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=2)
        tk.Button(btn_frame, text="Remover", bg="#f44336", fg="white", command=self.delete_game).pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=2)

        # Painel Direito: Formulário de Edição
        right_frame = tk.Frame(self.root)
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=10, pady=10)

        tk.Label(right_frame, text="Dados do Jogo / Port", font=("Arial", 12, "bold")).grid(row=0, column=0, columnspan=2, sticky=tk.W, pady=5)

        # Campos do Formulário
        fields = [
            ("ID do Jogo (16 chars):", "id"),
            ("Nome do Jogo:", "name"),
            ("URL de Download (.nsz):", "url"),
            ("URL do Ícone:", "iconUrl"),
            ("Descrição:", "description"),
            ("Informações / Instruções:", "notes"),
            ("Tamanho (em MB):", "size"), 
            ("Região:", "region"),
            ("Data de Lançamento (AAAAMMDD):", "releaseDate"),
        ]

        self.entries = {}
        for i, (label_text, field_name) in enumerate(fields, start=1):
            tk.Label(right_frame, text=label_text).grid(row=i, column=0, sticky=tk.W, pady=3)
            entry = tk.Entry(right_frame, width=50)
            entry.grid(row=i, column=1, sticky=tk.EW, pady=3, padx=5)
            self.entries[field_name] = entry

        # Inicializa o formulário com o estado limpo e padrões para um novo jogo
        self.prepare_for_new_game()

        # Botão para Salvar / Atualizar Dados no JSON comum
        tk.Button(right_frame, text="Salvar / Atualizar Dados (JSON)", font=("Arial", 11, "bold"), bg="#2196F3", fg="white", command=self.save_or_update_game).grid(row=10, column=0, columnspan=2, pady=(20, 10), sticky=tk.EW)

        # Botão para criptografar diretamente para index.tfl
        tk.Button(right_frame, text="🔒 Gerar arquivo index.tfl", font=("Arial", 11, "bold"), bg="#009688", fg="white", command=self.generate_tfl_file).grid(row=11, column=0, columnspan=2, pady=5, sticky=tk.EW)

    def update_game_list(self):
        """Atualiza a lista visual de jogos do lado esquerdo em ORDEM ALFABÉTICA e atualiza o contador."""
        self.game_listbox.delete(0, tk.END)
        
        total_jogos = len(self.data.get("titledb", {}))
        self.list_label.config(text=f"Jogos Cadastrados ({total_jogos}):")
        
        for index, (game_id, game_info) in enumerate(self.data.get("titledb", {}).items()):
            display_text = f"{game_info.get('name')} [{game_id}]"
            self.game_listbox.insert(tk.END, display_text)
            
            if game_id == self.recently_added_id:
                self.game_listbox.itemconfig(index, {'bg': '#D4EDDA', 'fg': '#155724'})

    def on_game_select(self, event):
        """Preenche o formulário ao selecionar um jogo na lista."""
        selection = self.game_listbox.curselection()
        if not selection:
            return

        index = selection[0]
        item_text = self.game_listbox.get(index)
        game_id = item_text.split("[")[-1].replace("]", "").strip()

        game_info = self.data["titledb"].get(game_id, {})

        download_url = ""
        for file_obj in self.data.get("files", []):
            if game_id in file_obj.get("url", ""):
                download_url = file_obj.get("url")
                if "#%5B" in download_url:
                    download_url = download_url.split("#%5B")[0]
                break

        # Limpa COMPLETAMENTE o formulário
        self.clear_form_absolute()
        
        # Insere as novas informações diretamente do JSON
        self.entries["id"].insert(0, game_id)
        self.entries["id"].config(state="disabled") # Bloqueia ID para edição
        self.entries["name"].insert(0, game_info.get("name", ""))
        self.entries["url"].insert(0, download_url)
        self.entries["iconUrl"].insert(0, game_info.get("iconUrl", ""))
        self.entries["description"].insert(0, game_info.get("description", ""))
        self.entries["notes"].insert(0, game_info.get("notes", ""))
        
        size_in_bytes = game_info.get("size", 0)
        size_in_mb = round(size_in_bytes / 1048576, 2)
        if size_in_mb.is_integer():
            size_in_mb = int(size_in_mb)
        
        self.entries["size"].insert(0, str(size_in_mb))
        self.entries["region"].insert(0, game_info.get("region", "US"))
        self.entries["releaseDate"].insert(0, str(game_info.get("releaseDate", 20260624)))

    def clear_form_absolute(self):
        """Força a limpeza total e absoluta de todas as caixas de texto."""
        for entry in self.entries.values():
            entry.config(state="normal")
            entry.delete(0, tk.END)

    def prepare_for_new_game(self):
        """Prepara os campos com os valores padrão ideais para cadastrar um Novo Jogo."""
        self.clear_form_absolute()
        self.entries["region"].insert(0, "US")
        self.entries["releaseDate"].insert(0, "20260624")
        self.entries["size"].insert(0, "0")

    def save_or_update_game(self):
        """Adiciona ou atualiza as informações no dicionário e salva no JSON."""
        game_id = self.entries["id"].get().strip()
        name = self.entries["name"].get().strip()
        url = self.entries["url"].get().strip()
        icon_url = self.entries["iconUrl"].get().strip()
        description = self.entries["description"].get().strip()
        notes = self.entries["notes"].get().strip()
        region = self.entries["region"].get().strip() 

        if not game_id or not name or not url:
            messagebox.showwarning("Atenção", "Os campos ID, Nome e URL de Download são obrigatórios!")
            return

        try:
            size_mb = float(self.entries["size"].get().strip())
            size_bytes = int(size_mb * 1048576)
            release_date = int(self.entries["releaseDate"].get().strip()) 
        except ValueError:
            messagebox.showerror("Erro", "O Tamanho (em MB) deve ser um número e a Data de Lançamento um valor inteiro (AAAAMMDD).")
            return

        if "#%5B" in url:
            url = url.split("#%5B")[0]

        formatted_url = f"{url}#%5B{game_id}%5D.nsz"

        self.data["titledb"][game_id] = {
            "id": game_id,
            "name": name,
            "version": 0,
            "region": region,
            "releaseDate": release_date,
            "rating": 10,
            "publisher": "",
            "description": description,
            "notes": notes,
            "size": size_bytes,
            "rank": 1,
            "iconUrl": icon_url,
        }

        self.data["files"] = [f for f in self.data["files"] if game_id not in f.get("url", "")]
        self.data["files"].append({"url": formatted_url})

        self.recently_added_id = game_id

        self.save_json()
        self.update_game_list()
        self.prepare_for_new_game()

    def save_json(self):
        """Ordena as estruturas internamente por ordem alfabética e grava no arquivo JSON."""
        try:
            sorted_titledb = sorted(
                self.data.get("titledb", {}).items(),
                key=lambda item: item[1].get("name", "").lower()
            )
            self.data["titledb"] = dict(sorted_titledb)

            def get_game_name_for_url(file_obj):
                url_str = file_obj.get("url", "")
                for g_id, g_info in self.data["titledb"].items():
                    if g_id in url_str:
                        return g_info.get("name", "").lower()
                return "zzz_unknown"

            self.data["files"] = sorted(self.data.get("files", []), key=get_game_name_for_url)

            with open(self.filename, "w", encoding="utf-8") as f:
                json.dump(self.data, f, indent=4, ensure_ascii=False)
            print("Arquivo index.json atualizado e ordenado alfabeticamente no disco.")
        except Exception as e:
            messagebox.showerror("Erro", f"Não foi possível salvar o arquivo JSON: {e}")

    def generate_tfl_file(self):
        """
        Gera o arquivo .tfl usando a chave pública embutida no código.
        Aplica compressão zstandard e criptografia AES-ECB + RSA-OAEP.
        """
        try:
            import zstandard as zstd
            from Crypto.PublicKey import RSA
            from Crypto.Cipher import PKCS1_OAEP, AES
            from Crypto.Hash import SHA256
        except ImportError as e:
            messagebox.showerror("Erro de Dependência", f"Biblioteca necessária não instalada: {e}")
            return

        if not self.public_key:
            messagebox.showerror(
                "Erro de Chave",
                "A chave pública embutida é inválida ou não foi configurada corretamente!"
            )
            return

        try:
            # Salva o JSON primeiro
            self.save_json()

            # Converte o JSON para bytes
            json_str = json.dumps(self.data, indent=4, ensure_ascii=False)
            input_bytes = json_str.encode('utf-8')

            # Comprime com Zstandard
            cctx = zstd.ZstdCompressor(level=22)
            buf = cctx.compress(input_bytes)
            sz = len(buf)

            flag = 0xFD  # 0x0D | 0xF0 (Criptografado)

            # Gera chave AES aleatória (128 bits)
            aes_key = random.randint(0, 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF).to_bytes(0x10, 'big')
            
            # Envolve a chave AES com a chave pública RSA embutida
            session_key = self.wrap_key(aes_key, self.public_key)
            
            # Criptografa com AES-ECB
            cipher = AES.new(aes_key, AES.MODE_ECB)
            buf = cipher.encrypt(buf + (b'\x00' * (0x10 - (sz % 0x10))))

            # Escreve o arquivo .tfl no formato TINFOIL
            with open(self.output_tfl, 'wb') as f:
                f.write(b'TINFOIL')                            # Magic
                f.write(flag.to_bytes(1, byteorder='little'))  # Flag
                f.write(session_key)                           # Session key (256 bytes)
                f.write(sz.to_bytes(8, 'little'))              # Tamanho original
                f.write(buf)                                   # Dados comprimidos/criptografados

            msg = (
                f"Arquivo criptografado gerado com sucesso!\n\n"
                f"📁 Arquivo: '{self.output_tfl}'\n"
                f"🔒 Criptografia: AES-128-ECB + RSA-OAEP (Chave Embutida)\n"
                f"📦 Compressão: Zstandard (nível 22)\n"
                f"🏷️ Flag: 0x{flag:02X}"
            )
            
            messagebox.showinfo("Sucesso", msg)
            
        except Exception as e:
            messagebox.showerror("Erro na Geração do .tfl", f"Ocorreu um erro:\n{e}")

    def delete_game(self):
        """Remove o jogo selecionado do JSON (titledb e files)."""
        selection = self.game_listbox.curselection()
        if not selection:
            messagebox.showwarning("Atenção", "Selecione um jogo na lista para remover.")
            return

        index = selection[0]
        item_text = self.game_listbox.get(index)
        game_id = item_text.split("[")[-1].replace("]", "").strip()

        if messagebox.askyesno("Confirmar", f"Tem certeza que deseja remover o jogo de ID {game_id}?"):
            if game_id == self.recently_added_id:
                self.recently_added_id = None

            if game_id in self.data["titledb"]:
                del self.data["titledb"][game_id]

            self.data["files"] = [f for f in self.data["files"] if game_id not in f.get("url", "")]

            self.save_json()
            self.update_game_list()
            self.prepare_for_new_game()


if __name__ == "__main__":
    check_and_install_dependencies()
    root = tk.Tk()
    app = JSONEditorApp(root)
    root.mainloop()
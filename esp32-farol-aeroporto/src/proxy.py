# proxy.py — roda localmente e redireciona requisições para a API de METAR, evitando CORS e simplificando o acesso para o ESP32
from http.server import BaseHTTPRequestHandler, HTTPServer
import urllib.request

class Proxy(BaseHTTPRequestHandler):
    def do_GET(self):
        icao = self.path.strip("/") or "SBKP"
        url = f"https://aviationweather.gov/api/data/metar?ids={icao}&format=json"
        req = urllib.request.Request(url, headers={"User-Agent": "ESP32-Proxy/1.0"})
        try:
            with urllib.request.urlopen(req) as r:
                data = r.read()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(data)
        except Exception as e:
            self.send_response(502)
            self.end_headers()
            self.wfile.write(str(e).encode())
            print(f"Erro: {e}")

    def log_message(self, format, *args):
        print(f"[proxy] {args[0]} {args[1]}")

HTTPServer(("0.0.0.0", 80), Proxy).serve_forever()
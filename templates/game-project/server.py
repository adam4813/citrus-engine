"""Local development server for the WASM build.

Browsers require Cross-Origin Isolation (COOP + COEP headers) before they will
expose SharedArrayBuffer, which the emscripten pthread runtime relies on. The
stock `python -m http.server` does not send those headers, so the page either
fails to start or runs in single-threaded fallback mode.

Run from this directory:

    python server.py

then open http://localhost:8080/<your-game>.html
"""

from http.server import HTTPServer, SimpleHTTPRequestHandler


class CrossOriginIsolatedHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()


if __name__ == "__main__":
    server_address = ("", 8080)
    httpd = HTTPServer(server_address, CrossOriginIsolatedHandler)
    print("Serving on http://localhost:8080 (Cross-Origin Isolated)")
    httpd.serve_forever()

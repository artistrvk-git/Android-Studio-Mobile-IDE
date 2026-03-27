#!/usr/bin/env python3
"""
RVK Backend Server - Bundled asset copy
This file is copied to app data directory at runtime
"""
# Content same as main python server
import os, sys, json, subprocess, threading, time
from http.server import HTTPServer, BaseHTTPRequestHandler

HOST = '127.0.0.1'
PORT = 8765
BASE_DIR = os.environ.get('RVK_BASE_DIR', '/storage/emulated/0/AndroidStudioMobile')

class RVKHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/status':
            self._json({'status': 'running', 'name': 'RVK Backend', 'version': '1.0'})
        else:
            self._json({'error': 'not found'}, 404)
    
    def do_POST(self):
        length = int(self.headers.get('Content-Length', 0))
        body = json.loads(self.rfile.read(length).decode()) if length > 0 else {}
        
        if self.path == '/terminal/exec':
            cmd = body.get('command', '')
            cwd = body.get('working_dir', os.path.join(BASE_DIR, 'Projects'))
            try:
                r = subprocess.run(['/system/bin/sh', '-c', cmd], cwd=cwd,
                    capture_output=True, text=True, timeout=300)
                self._json({'stdout': r.stdout, 'stderr': r.stderr, 'exit_code': r.returncode})
            except Exception as e:
                self._json({'error': str(e)}, 500)
        else:
            self._json({'error': 'not found'}, 404)
    
    def _json(self, data, code=200):
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())
    
    def log_message(self, *args): pass

if __name__ == '__main__':
    print(f"RVK Backend starting on {HOST}:{PORT}")
    HTTPServer((HOST, PORT), RVKHandler).serve_forever()

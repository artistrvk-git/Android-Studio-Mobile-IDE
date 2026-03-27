#!/usr/bin/env python3
"""
RVK Backend Server - Python backend for Android Studio Mobile
Provides WebSocket/HTTP API for real-time terminal, build logs, and Gradle sync
Presented By RVK EDITION
"""

import os
import sys
import json
import subprocess
import threading
import time
import signal
from http.server import HTTPServer, BaseHTTPRequestHandler
import socket

# Configuration
HOST = '127.0.0.1'
PORT = 8765
BASE_DIR = os.environ.get('RVK_BASE_DIR', '/storage/emulated/0/AndroidStudioMobile')

class BuildManager:
    """Manages build processes"""
    
    def __init__(self):
        self.current_process = None
        self.build_log = []
        self.is_building = False
    
    def execute_build(self, project_path, task="assembleDebug"):
        """Execute Gradle build with proper environment"""
        self.is_building = True
        self.build_log = []
        
        gradlew = os.path.join(project_path, 'gradlew')
        
        # Set executable permission
        if os.path.exists(gradlew):
            os.chmod(gradlew, 0o755)
        
        # Find JAVA_HOME
        java_home = self._find_java_home()
        if not java_home:
            self.build_log.append("ERROR: JAVA_HOME not found")
            self.is_building = False
            return False
        
        # Build environment
        env = os.environ.copy()
        env['JAVA_HOME'] = java_home
        env['ANDROID_HOME'] = os.path.join(BASE_DIR, 'SDK')
        env['ANDROID_SDK_ROOT'] = os.path.join(BASE_DIR, 'SDK')
        env['GRADLE_USER_HOME'] = os.path.join(BASE_DIR, '.gradle')
        env['HOME'] = BASE_DIR
        env['TMPDIR'] = os.path.join(BASE_DIR, '.temp')
        env['PATH'] = ':'.join([
            os.path.join(java_home, 'bin'),
            os.path.join(BASE_DIR, 'Gradle', 'gradle-8.14.4', 'bin'),
            os.path.join(BASE_DIR, 'SDK', 'platform-tools'),
            '/system/bin', '/system/xbin'
        ])
        
        # Execute: use sh gradlew (avoids permission issues)
        cmd = ['/system/bin/sh', gradlew, task, '--stacktrace', '--no-daemon',
               f'-Dorg.gradle.java.home={java_home}']
        
        try:
            self.current_process = subprocess.Popen(
                cmd,
                cwd=project_path,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1
            )
            
            for line in iter(self.current_process.stdout.readline, ''):
                line = line.rstrip()
                self.build_log.append(line)
            
            self.current_process.wait()
            exit_code = self.current_process.returncode
            
            self.is_building = False
            
            if exit_code == 0:
                self.build_log.append("BUILD SUCCESSFUL")
                apk_path = self._find_apk(project_path, task)
                if apk_path:
                    self.build_log.append(f"APK: {apk_path}")
                return True
            else:
                self.build_log.append(f"BUILD FAILED (exit code: {exit_code})")
                return False
                
        except Exception as e:
            self.build_log.append(f"ERROR: {str(e)}")
            self.is_building = False
            return False
    
    def _find_java_home(self):
        candidates = [
            os.path.join(BASE_DIR, 'JDK', 'jdk-17'),
            os.path.join(BASE_DIR, 'JDK', 'jdk-21'),
            os.path.join(BASE_DIR, 'JDK', 'jdk17'),
            os.path.join(BASE_DIR, 'JDK', 'jdk21'),
        ]
        for path in candidates:
            java_bin = os.path.join(path, 'bin', 'java')
            if os.path.exists(java_bin):
                os.chmod(java_bin, 0o755)
                return path
        return None
    
    def _find_apk(self, project_path, task):
        build_type = 'debug' if 'Debug' in task else 'release'
        search_paths = [
            os.path.join(project_path, 'app', 'build', 'outputs', 'apk', build_type),
            os.path.join(project_path, 'app', 'build', 'outputs', 'apk'),
            os.path.join(project_path, 'build', 'outputs', 'apk', build_type),
        ]
        for sp in search_paths:
            if os.path.isdir(sp):
                for f in os.listdir(sp):
                    if f.endswith('.apk'):
                        return os.path.join(sp, f)
        return None
    
    def kill_build(self):
        if self.current_process:
            self.current_process.kill()
            self.is_building = False
            self.build_log.append("[Build killed by user]")


class TerminalManager:
    """Manages terminal sessions"""
    
    def __init__(self):
        self.sessions = {}
    
    def execute_command(self, command, working_dir=None):
        if working_dir is None:
            working_dir = os.path.join(BASE_DIR, 'Projects')
        
        env = os.environ.copy()
        java_home = self._find_java()
        if java_home:
            env['JAVA_HOME'] = java_home
            env['PATH'] = os.path.join(java_home, 'bin') + ':' + env.get('PATH', '')
        
        env['ANDROID_HOME'] = os.path.join(BASE_DIR, 'SDK')
        env['HOME'] = BASE_DIR
        
        try:
            result = subprocess.run(
                ['/system/bin/sh', '-c', command],
                cwd=working_dir,
                env=env,
                capture_output=True,
                text=True,
                timeout=300
            )
            return {
                'stdout': result.stdout,
                'stderr': result.stderr,
                'exit_code': result.returncode
            }
        except subprocess.TimeoutExpired:
            return {'stdout': '', 'stderr': 'Command timed out', 'exit_code': -1}
        except Exception as e:
            return {'stdout': '', 'stderr': str(e), 'exit_code': -1}
    
    def _find_java(self):
        for name in ['jdk-17', 'jdk-21', 'jdk17', 'jdk21']:
            path = os.path.join(BASE_DIR, 'JDK', name)
            if os.path.exists(os.path.join(path, 'bin', 'java')):
                return path
        return None


# Global instances
build_manager = BuildManager()
terminal_manager = TerminalManager()


class RVKRequestHandler(BaseHTTPRequestHandler):
    """HTTP Request Handler for RVK Backend"""
    
    def do_GET(self):
        if self.path == '/status':
            self._send_json({
                'status': 'running',
                'version': '1.0.0',
                'name': 'Android Studio Mobile Backend',
                'slogan': 'Presented By RVK EDITION',
                'building': build_manager.is_building
            })
        elif self.path == '/build/log':
            self._send_json({
                'log': build_manager.build_log,
                'building': build_manager.is_building
            })
        elif self.path == '/sdk/status':
            self._send_json(self._get_sdk_status())
        else:
            self._send_json({'error': 'Not found'}, 404)
    
    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length).decode('utf-8')
        
        try:
            data = json.loads(body) if body else {}
        except json.JSONDecodeError:
            self._send_json({'error': 'Invalid JSON'}, 400)
            return
        
        if self.path == '/build':
            project_path = data.get('project_path', '')
            task = data.get('task', 'assembleDebug')
            
            if not project_path:
                self._send_json({'error': 'project_path required'}, 400)
                return
            
            # Run build in background thread
            thread = threading.Thread(target=build_manager.execute_build, args=(project_path, task))
            thread.daemon = True
            thread.start()
            
            self._send_json({'status': 'build_started', 'task': task})
            
        elif self.path == '/build/kill':
            build_manager.kill_build()
            self._send_json({'status': 'build_killed'})
            
        elif self.path == '/terminal/exec':
            command = data.get('command', '')
            working_dir = data.get('working_dir', None)
            result = terminal_manager.execute_command(command, working_dir)
            self._send_json(result)
            
        elif self.path == '/gradle/sync':
            project_path = data.get('project_path', '')
            thread = threading.Thread(
                target=build_manager.execute_build,
                args=(project_path, 'dependencies')
            )
            thread.daemon = True
            thread.start()
            self._send_json({'status': 'sync_started'})
            
        else:
            self._send_json({'error': 'Not found'}, 404)
    
    def _send_json(self, data, status=200):
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode('utf-8'))
    
    def _get_sdk_status(self):
        components = {}
        checks = {
            'JDK 17': os.path.join(BASE_DIR, 'JDK', 'jdk-17', 'bin', 'java'),
            'JDK 21': os.path.join(BASE_DIR, 'JDK', 'jdk-21', 'bin', 'java'),
            'Android SDK': os.path.join(BASE_DIR, 'SDK'),
            'NDK': os.path.join(BASE_DIR, 'NDK'),
            'Gradle 8.14.4': os.path.join(BASE_DIR, 'Gradle', 'gradle-8.14.4', 'bin', 'gradle'),
            'Flutter': os.path.join(BASE_DIR, 'Flutter', 'bin', 'flutter'),
            'Node.js': os.path.join(BASE_DIR, 'NodeJS', 'bin', 'node'),
            'Python': os.path.join(BASE_DIR, 'Python', 'bin', 'python3'),
        }
        for name, path in checks.items():
            components[name] = os.path.exists(path)
        return components
    
    def log_message(self, format, *args):
        pass  # Suppress default logging


def start_server():
    """Start the RVK backend HTTP server"""
    print(f"RVK Backend Server starting on {HOST}:{PORT}")
    print("Presented By RVK EDITION")
    
    server = HTTPServer((HOST, PORT), RVKRequestHandler)
    server.serve_forever()


if __name__ == '__main__':
    start_server()

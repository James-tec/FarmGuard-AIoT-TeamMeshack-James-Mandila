#!/usr/bin/env python3
"""
FarmGuard-AIoT Dashboard
Smart Irrigation & Microclimate Monitoring System
"""

from flask import Flask, render_template, jsonify, request
from flask_socketio import SocketIO, emit
import json
import sqlite3
import random
from datetime import datetime, timedelta
import threading
import time

app = Flask(__name__)
app.config['SECRET_KEY'] = 'farmguard-aiot-secret-key-2026'
socketio = SocketIO(app, cors_allowed_origins="*")

DB_PATH = 'farmguard_data.db'

def init_db():
    """Initialize SQLite database with tables for sensor data and system logs."""
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()

    c.execute('''
        CREATE TABLE IF NOT EXISTS sensor_readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            temperature REAL,
            humidity REAL,
            soil_moisture REAL,
            light_intensity REAL,
            soil_ph REAL,
            rainfall_mm REAL,
            wind_speed REAL,
            device_id TEXT,
            location TEXT
        )
    ''')

    c.execute('''
        CREATE TABLE IF NOT EXISTS irrigation_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            trigger_type TEXT,
            duration_minutes INTEGER,
            soil_moisture_before REAL,
            soil_moisture_after REAL,
            status TEXT
        )
    ''')

    c.execute('''
        CREATE TABLE IF NOT EXISTS system_alerts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            alert_type TEXT,
            severity TEXT,
            message TEXT,
            acknowledged BOOLEAN DEFAULT 0
        )
    ''')

    c.execute('''
        CREATE TABLE IF NOT EXISTS device_status (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT UNIQUE,
            last_seen DATETIME,
            battery_level REAL,
            signal_strength REAL,
            status TEXT,
            firmware_version TEXT
        )
    ''')

    conn.commit()
    conn.close()
    print("[DB] Database initialized successfully")

class SensorSimulator:
    """
    Simulates sensor data for demonstration and testing.
    In production, this would interface with actual IoT hardware via MQTT/LoRaWAN.
    """

    def __init__(self):
        self.running = False
        self.thread = None
        self.base_values = {
            'temperature': 24.5,
            'humidity': 65.0,
            'soil_moisture': 45.0,
            'light_intensity': 850.0,
            'soil_ph': 6.5,
            'rainfall_mm': 0.0,
            'wind_speed': 3.2
        }

    def generate_reading(self):
        """Generate realistic sensor readings with natural variation."""
        return {
            'temperature': round(self.base_values['temperature'] + random.uniform(-2.0, 2.0), 1),
            'humidity': round(max(0, min(100, self.base_values['humidity'] + random.uniform(-5.0, 5.0))), 1),
            'soil_moisture': round(max(0, min(100, self.base_values['soil_moisture'] + random.uniform(-3.0, 3.0))), 1),
            'light_intensity': round(max(0, self.base_values['light_intensity'] + random.uniform(-100, 100)), 1),
            'soil_ph': round(max(0, min(14, self.base_values['soil_ph'] + random.uniform(-0.3, 0.3))), 2),
            'rainfall_mm': round(max(0, self.base_values['rainfall_mm'] + random.uniform(-0.5, 2.0)), 1),
            'wind_speed': round(max(0, self.base_values['wind_speed'] + random.uniform(-1.0, 1.0)), 1),
            'device_id': 'FARMGUARD-001',
            'location': 'Plot-A-North',
            'timestamp': datetime.now().isoformat()
        }

    def store_reading(self, reading):
        """Store sensor reading in database."""
        conn = sqlite3.connect(DB_PATH)
        c = conn.cursor()
        c.execute('''
            INSERT INTO sensor_readings 
            (temperature, humidity, soil_moisture, light_intensity, soil_ph, rainfall_mm, wind_speed, device_id, location)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            reading['temperature'], reading['humidity'], reading['soil_moisture'],
            reading['light_intensity'], reading['soil_ph'], reading['rainfall_mm'],
            reading['wind_speed'], reading['device_id'], reading['location']
        ))
        conn.commit()
        conn.close()

    def check_thresholds(self, reading):
        """Check sensor readings against thresholds and generate alerts."""
        alerts = []

        if reading['soil_moisture'] < 30:
            alerts.append({
                'type': 'irrigation_needed',
                'severity': 'warning',
                'message': f"Soil moisture critically low: {reading['soil_moisture']}%"
            })

        if reading['temperature'] > 35:
            alerts.append({
                'type': 'high_temperature',
                'severity': 'critical',
                'message': f"Temperature exceeds safe threshold: {reading['temperature']}C"
            })

        if reading['humidity'] > 90:
            alerts.append({
                'type': 'high_humidity',
                'severity': 'warning',
                'message': f"High humidity detected: {reading['humidity']}% - Fungal disease risk"
            })

        if reading['soil_ph'] < 5.5 or reading['soil_ph'] > 7.5:
            alerts.append({
                'type': 'ph_imbalance',
                'severity': 'warning',
                'message': f"Soil pH out of optimal range: {reading['soil_ph']}"
            })

        for alert in alerts:
            self.store_alert(alert)

        return alerts

    def store_alert(self, alert):
        """Store alert in database."""
        conn = sqlite3.connect(DB_PATH)
        c = conn.cursor()
        c.execute('''
            INSERT INTO system_alerts (alert_type, severity, message)
            VALUES (?, ?, ?)
        ''', (alert['type'], alert['severity'], alert['message']))
        conn.commit()
        conn.close()

    def run(self):
        """Main simulation loop - runs every 5 seconds."""
        while self.running:
            reading = self.generate_reading()
            self.store_reading(reading)
            alerts = self.check_thresholds(reading)

            socketio.emit('sensor_update', {
                'reading': reading,
                'alerts': alerts
            })

            time.sleep(5)

    def start(self):
        """Start the simulator in a background thread."""
        if not self.running:
            self.running = True
            self.thread = threading.Thread(target=self.run)
            self.thread.daemon = True
            self.thread.start()
            print("[SIM] Sensor simulator started")

    def stop(self):
        """Stop the simulator."""
        self.running = False
        if self.thread:
            self.thread.join(timeout=1)
        print("[SIM] Sensor simulator stopped")

simulator = SensorSimulator()

@app.route('/')
def index():
    """Main dashboard page."""
    return render_template('index.html')

@app.route('/api/current-readings')
def get_current_readings():
    """API endpoint for latest sensor readings."""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    c = conn.cursor()
    c.execute('SELECT * FROM sensor_readings ORDER BY timestamp DESC LIMIT 1')
    row = c.fetchone()
    conn.close()

    if row:
        return jsonify(dict(row))
    return jsonify({'error': 'No data available'}), 404

@app.route('/api/historical-data')
def get_historical_data():
    """API endpoint for historical sensor data."""
    hours = request.args.get('hours', 24, type=int)
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    c = conn.cursor()
    c.execute('''
        SELECT * FROM sensor_readings 
        WHERE timestamp >= datetime('now', '-{} hours')
        ORDER BY timestamp ASC
    '''.format(hours))
    rows = c.fetchall()
    conn.close()
    return jsonify([dict(row) for row in rows])

@app.route('/api/alerts')
def get_alerts():
    """API endpoint for system alerts."""
    severity = request.args.get('severity', None)
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    c = conn.cursor()

    if severity:
        c.execute('SELECT * FROM system_alerts WHERE severity = ? AND acknowledged = 0 ORDER BY timestamp DESC', (severity,))
    else:
        c.execute('SELECT * FROM system_alerts WHERE acknowledged = 0 ORDER BY timestamp DESC LIMIT 20')

    rows = c.fetchall()
    conn.close()
    return jsonify([dict(row) for row in rows])

@app.route('/api/irrigation/history')
def get_irrigation_history():
    """API endpoint for irrigation event history."""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    c = conn.cursor()
    c.execute('SELECT * FROM irrigation_events ORDER BY timestamp DESC LIMIT 50')
    rows = c.fetchall()
    conn.close()
    return jsonify([dict(row) for row in rows])

@app.route('/api/irrigation/trigger', methods=['POST'])
def trigger_irrigation():
    """API endpoint to trigger irrigation."""
    data = request.json
    trigger_type = data.get('trigger_type', 'manual')
    duration = data.get('duration_minutes', 10)

    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute('''
        INSERT INTO irrigation_events (trigger_type, duration_minutes, status)
        VALUES (?, ?, 'in_progress')
    ''', (trigger_type, duration))
    conn.commit()
    conn.close()

    socketio.emit('irrigation_event', {
        'type': trigger_type,
        'duration': duration,
        'status': 'started',
        'timestamp': datetime.now().isoformat()
    })

    return jsonify({'status': 'success', 'message': f'Irrigation triggered: {duration} minutes'})

@app.route('/api/device-status')
def get_device_status():
    """API endpoint for IoT device status."""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    c = conn.cursor()
    c.execute('SELECT * FROM device_status')
    rows = c.fetchall()
    conn.close()
    return jsonify([dict(row) for row in rows])

@app.route('/api/statistics')
def get_statistics():
    """API endpoint for aggregated statistics."""
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()

    c.execute('''
        SELECT 
            AVG(temperature) as avg_temp,
            AVG(humidity) as avg_humidity,
            AVG(soil_moisture) as avg_soil_moisture,
            MIN(soil_moisture) as min_soil_moisture,
            MAX(temperature) as max_temp,
            COUNT(*) as total_readings
        FROM sensor_readings 
        WHERE timestamp >= datetime('now', '-24 hours')
    ''')
    stats = dict(c.fetchone())

    c.execute('SELECT COUNT(*) as irrigation_count FROM irrigation_events WHERE timestamp >= datetime("now", "-24 hours")')
    stats['irrigation_count'] = c.fetchone()[0]

    c.execute('SELECT COUNT(*) as alert_count FROM system_alerts WHERE timestamp >= datetime("now", "-24 hours") AND acknowledged = 0')
    stats['alert_count'] = c.fetchone()[0]

    conn.close()
    return jsonify(stats)

@socketio.on('connect')
def handle_connect():
    """Handle client connection."""
    print(f'[WS] Client connected: {request.sid}')
    emit('connection_response', {'status': 'connected', 'message': 'FarmGuard-AIoT Dashboard Active'})

@socketio.on('disconnect')
def handle_disconnect():
    """Handle client disconnection."""
    print(f'[WS] Client disconnected: {request.sid}')

@socketio.on('request_irrigation')
def handle_irrigation_request(data):
    """Handle manual irrigation request from client."""
    duration = data.get('duration', 10)
    trigger_irrigation()
    emit('irrigation_status', {'status': 'started', 'duration': duration})

if __name__ == '__main__':
    init_db()
    simulator.start()

    try:
        print("[APP] Starting FarmGuard-AIoT Dashboard...")
        print("[APP] Access at: http://localhost:5000")
        socketio.run(app, host='0.0.0.0', port=5000, debug=False)
    except KeyboardInterrupt:
        print("\n[APP] Shutting down...")
        simulator.stop()

from flask import Flask, request, jsonify
from datetime import datetime
import json

app = Flask(__name__)

# In-memory storage for GPS data (use a database in production)
gps_data_store = []

@app.route('/')
def home():
    return jsonify({
        'message': 'ESP32 GPS Tracker API',
        'endpoints': {
            '/api/gps': 'POST - Send GPS data',
            '/api/gps/latest': 'GET - Get latest GPS data',
            '/api/gps/all': 'GET - Get all GPS data',
            '/api/gps/history': 'GET - Get GPS history with limit'
        }
    })

@app.route('/api/gps', methods=['POST'])
def receive_gps():
    """
    Receive GPS data from ESP32
    Expected JSON format:
    {
        "latitude": 12.9716,
        "longitude": 77.5946,
        "altitude": 920.5,
        "speed": 0.0,
        "satellites": 8
    }
    """
    try:
        data = request.get_json()
        
        # Validate required fields
        if not data or 'latitude' not in data or 'longitude' not in data:
            return jsonify({
                'status': 'error',
                'message': 'Missing required fields: latitude and longitude'
            }), 400
        
        # Add timestamp and store data
        gps_entry = {
            'timestamp': datetime.now().isoformat(),
            'latitude': float(data['latitude']),
            'longitude': float(data['longitude']),
            'altitude': float(data.get('altitude', 0)),
            'speed': float(data.get('speed', 0)),
            'satellites': int(data.get('satellites', 0))
        }
        
        gps_data_store.append(gps_entry)
        
        # Keep only last 1000 entries
        if len(gps_data_store) > 1000:
            gps_data_store.pop(0)
        
        return jsonify({
            'status': 'success',
            'message': 'GPS data received',
            'data': gps_entry
        }), 201
        
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

@app.route('/api/gps/latest', methods=['GET'])
def get_latest_gps():
    """Get the most recent GPS data"""
    if not gps_data_store:
        return jsonify({
            'status': 'error',
            'message': 'No GPS data available'
        }), 404
    
    return jsonify({
        'status': 'success',
        'data': gps_data_store[-1]
    })

@app.route('/api/gps/all', methods=['GET'])
def get_all_gps():
    """Get all GPS data"""
    return jsonify({
        'status': 'success',
        'count': len(gps_data_store),
        'data': gps_data_store
    })

@app.route('/api/gps/history', methods=['GET'])
def get_gps_history():
    """Get GPS history with optional limit"""
    limit = request.args.get('limit', default=10, type=int)
    
    if limit <= 0:
        return jsonify({
            'status': 'error',
            'message': 'Limit must be positive'
        }), 400
    
    history = gps_data_store[-limit:] if len(gps_data_store) > limit else gps_data_store
    
    return jsonify({
        'status': 'success',
        'count': len(history),
        'data': history
    })

@app.route('/api/gps/clear', methods=['DELETE'])
def clear_gps_data():
    """Clear all GPS data"""
    global gps_data_store
    gps_data_store = []
    
    return jsonify({
        'status': 'success',
        'message': 'All GPS data cleared'
    })

if __name__ == '__main__':
    # Run on all interfaces so ESP32 can connect
    # Use host='0.0.0.0' to allow external connections
    app.run(host='0.0.0.0', port=5000, debug=True)

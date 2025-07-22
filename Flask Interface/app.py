from flask import Flask, request, jsonify
import requests
import sqlite3
import socket
from createDatabase import create_all_databases  

app = Flask(__name__)

def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # Non invia un pacchetto, ma solo per ottenere l'IP locale in uso
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = "127.0.0.1"  # Fallback a localhost
    finally:
        s.close()
    return ip

# DEVI PRIMA RICHIEDERE LA KEY ALL'API #
API_KEY = "e8dc7c9b81544d368939f199aa48f59c" 
BASE_URL = "https://api.openweathermap.org/data/2.5/forecast"

@app.route("/weathermap", methods=["GET"])
def weathermap():
    latitude = request.args.get("lat")
    longitude = request.args.get("lon")
    
    print(latitude)
    print(longitude)
    
    if not latitude or not longitude:
        return jsonify({"error": "Missing 'lat' or 'lon' parameter"}), 400
    
    try:
        params = {
            "lat": latitude,
            "lon": longitude,
            "appid": API_KEY,
            "units": "metric"
        }
        response = requests.get(BASE_URL, params=params)
        data = response.json()
        
        if response.status_code != 200:
            return jsonify({"error": data.get("message", "An error occurred")}), response.status_code
        
        city_name = data["city"]["name"]
        forecast = data["list"][0]
        weather_data = {
            "city": city_name,
            "pop": forecast.get("pop", 0),
            "prediction": forecast["weather"][0]["main"],
            "description": forecast["weather"][0]["description"],
            "temperature": forecast["main"]["temp"],
            "humidity": forecast["main"]["humidity"],
            "clouds": forecast["clouds"]["all"],
            "mm": forecast.get("rain", {}).get("3h", 0)
        }
        
        print(weather_data)
        return jsonify(weather_data)
    
    except requests.RequestException as e:
        return jsonify({"error": str(e)}), 500

@app.route("/liveserver", methods=["POST", "GET"])
def liveserver():
    
    if request.method == "POST":
        data = request.json
        print(data)
        city = data.get("city")
        umbrella_id = int(data.get("id"))
        latitude = data.get("latitude")
        longitude = data.get("longitude")
        status = data.get("status")
        last_update = data.get("last_update")

        conn = sqlite3.connect('live_data.db')
        c = conn.cursor()
        # Controlla se l'ID è 0 o meno
        response = ""
        if umbrella_id == 0:
            # Insert a new tuple into database
            c.execute("INSERT INTO live_data (city, latitude, longitude, status, last_update) VALUES (?, ?, ?, ?, ?)",
                      (city, latitude, longitude, status, last_update))
            conn.commit()
            new_umbrella_id = c.lastrowid
            response = {"status": "success", "message": "Live data stored", "umbrella_id": new_umbrella_id}
        else:
            # Update the existent tuple with the new state
            c.execute("UPDATE live_data SET city=?, latitude=?, longitude=?, status = ?, last_update = ? WHERE id = ?",
                      (city, latitude, longitude, status, last_update, umbrella_id))
            conn.commit()
            if c.rowcount == 0:
                # If the ID doesn't exists, error
                response = {"status": "error", "message": "ID not found"}
                return jsonify(response), 404
            response = {"status": "success", "message": "Live data updated", "umbrella_id": umbrella_id}
        
        return jsonify(response)

    elif request.method == "GET":
        city = request.args.get("city")
        
        if not city:
            return jsonify({"error": "City parameter is required"}), 400

        conn = sqlite3.connect('live_data.db')
        c = conn.cursor()
        
        c.execute("SELECT COUNT(*) FROM live_data WHERE city = ?", (city,))
        total_count = c.fetchone()[0]

        c.execute("SELECT COUNT(*) FROM live_data WHERE city = ? AND status = 'open'", (city,))
        open_count = c.fetchone()[0]
        
        result = "true"
        if total_count > 0:
            ratio = open_count / total_count
            if ratio > 0.6 : result = "true"
            else:
                result = "false" 
        
        print(result)
        conn.close()
        return jsonify({"city": city, "status_open_ratio_above_0.2": result})

@app.route("/persistentserver", methods=["POST", "GET"])
def persistentserver():
    if request.method == "POST":
        data = request.json
        city = data.get("city")
        timestamp = data.get("timestamp")
        action = data.get("action")

        conn = sqlite3.connect('persistent_data.db')
        c = conn.cursor()
        c.execute("INSERT INTO persistent_data (city, timestamp, action) VALUES (?, ?, ?)",
                  (city, timestamp, action))
        conn.commit()
        conn.close()

        return jsonify({"status": "success", "message": "Persistent data stored"}), 201

if __name__ == "__main__":
    local_ip = get_local_ip()
    create_all_databases()
    app.run(host=local_ip, port=5000, debug=True)
    
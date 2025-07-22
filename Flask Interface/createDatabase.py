import sqlite3

def create_live_db():
    conn = sqlite3.connect('live_data.db')
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS live_data (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    city TEXT,
                    latitude REAL,
                    longitude REAL,
                    status TEXT,
                    last_update TEXT
                 )''')
    conn.commit()
    conn.close()

def create_persistent_db():
    conn = sqlite3.connect('persistent_data.db')
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS persistent_data (
                    city TEXT,
                    timestamp TEXT,
                    action TEXT
                 )''')
    conn.commit()
    conn.close()

def create_all_databases():
    create_live_db()
    create_persistent_db()
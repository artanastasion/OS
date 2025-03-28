from flask import Flask, render_template
import sqlite3

app = Flask(__name__)

def get_db_data(query, params=()):
    conn = sqlite3.connect('temps.db')
    c = conn.cursor()
    c.execute(query, params)
    data = c.fetchall()
    conn.close()
    return data

@app.route('/')
def dashboard():
    current_temp = get_db_data("SELECT temp FROM raw_temps ORDER BY timestamp DESC LIMIT 1")[0][0]

    hourly_data = get_db_data("""
        SELECT strftime('%H:%M', timestamp), temp 
        FROM raw_temps 
        WHERE timestamp >= datetime('now', '-1 day')
        ORDER BY timestamp
    """)

    monthly_avg = get_db_data("""
        SELECT strftime('%Y-%m-%d', hour_start), avg_temp 
        FROM hourly_avg 
        WHERE hour_start >= datetime('now', '-1 month')
        ORDER BY hour_start
    """)

    yearly_avg = get_db_data("""
        SELECT strftime('%Y-%m', day), avg_temp 
        FROM daily_avg 
        WHERE day >= date('now', '-1 year')
        ORDER BY day
    """)

    return render_template('dashboard.html',
                           current_temp=current_temp,
                           hourly_data=hourly_data,
                           monthly_avg=monthly_avg,
                           yearly_avg=yearly_avg)

if __name__ == '__main__':
    app.run(port=5000, debug=True)
#!/usr/bin/env python3

from flask import Flask, render_template, send_file
import matplotlib.pyplot as plt
import io
import requests
import time

PROBEL8="-------"

app = Flask(__name__)

host = 'http://gitlab.ivl.ua:9000'

sql_query = """SELECT DHT_Temperature, DHT_Humidity,
                      BME_Temperature, BME_Humidity, BME_Pressure,
                      DS_Temperature1, DS_Temperature2, timestamp
               FROM sensors
               ORDER BY timestamp DESC
               LIMIT 60;
            """
def get_data():
    response = requests.get(
        host + '/exec',
        params={'query': sql_query}
    )
    response.raise_for_status()
    response_json = response.json()
    return response_json['dataset']

def create_figure(data):
    timestamps = [row[7] for row in data]
    dht_temp = [row[0] for row in data]
    dht_hum = [row[1] for row in data]
    bme_temp = [row[2] for row in data]
    bme_hum = [row[3] for row in data]
    bme_press = [row[4] for row in data]
    ds_temp1 = [row[5] for row in data]
    ds_temp2 = [row[6] for row in data]

    # Увеличиваем высоту каждого графика на 20%
    fig, ax = plt.subplots(3, 1, figsize=(10, 8), gridspec_kw={'height_ratios': [1.2, 1.2, 1.2]})

    ax[0].plot(timestamps, dht_temp, label='DHT Temperature')
    ax[0].plot(timestamps, bme_temp, label='BME Temperature')
    ax[0].plot(timestamps, ds_temp1, label='DS Temperature 1')
    ax[0].plot(timestamps, ds_temp2, label='DS Temperature 2')
    ax[0].set_ylabel('Temperature (°C)')
    ax[0].legend()

    ax[1].plot(timestamps, dht_hum, label='DHT Humidity')
    ax[1].plot(timestamps, bme_hum, label='BME Humidity')
    ax[1].set_ylabel('Humidity (%)')
    ax[1].legend()

    ax[2].plot(timestamps, bme_press, label='BME Pressure')
    ax[2].set_ylabel('Pressure (hPa)')
    ax[2].legend()

    for a in ax:
        a.set_xlabel('Timestamp')
        a.legend()
        a.grid(True)

        # Проредить метки на оси X
        xticks = a.get_xticks()
        a.set_xticks(xticks[::4])
        a.set_xticklabels([timestamps[int(i)] for i in xticks[::4]])

    fig.autofmt_xdate()
    return fig


def get_terminal_output():
    data = get_data()
    
    output = []
    output.append("-------\t-------\t-------\t-------\t-------\t-------\t-------\t-------\t-------")
    output.append(f"total_rows {len(data)}")
    output.append("-------\t-------\t-------\t-------\t-------\t-------\t-------\t-------\t-------")
    for i, row in enumerate(data):
        output.append(f"{i}\t{row[0]}\t{row[1]}\t{row[2]}\t{row[3]}\t{row[4]}\t{row[5]}\t{row[6]}\t{row[7]}")
    output.append("-------\t-------\t-------\t-------\t-------\t-------\t-------\t-------\t-------")
    output.append("ROW_num\tDHT_T\tDHT_H\tBME_T\tBME_H\tBME_P\tDS_T1\tDS_T2\tTimestamp")
    output.append("-------\t-------\t-------\t-------\t-------\t-------\t-------\t-------\t-------")

    return "\n".join(output)


@app.route('/')
def index():
    return render_template('index.html', terminal_output=get_terminal_output())

@app.route('/plot.png')
def plot_png():
    data = get_data()
    fig = create_figure(data)
    output = io.BytesIO()
    fig.savefig(output, format='png')
    output.seek(0)
    return send_file(output, mimetype='image/png')

@app.route('/terminal_output')
def terminal_output():
    return get_terminal_output()


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=9000)


#!/usr/bin/env python3

import sys
import requests
import time
import json  # Добавим для форматированного вывода JSON

PROBEL8="-------"

def Polosa():
    print(f"{PROBEL8}\t{PROBEL8}\t{PROBEL8}\t{PROBEL8}\t{PROBEL8}\t{PROBEL8}\t{PROBEL8}\t{PROBEL8}\t{PROBEL8}")

host = 'http://gitlab.ivl.ua:9000'

sql_query = """SELECT DHT_Temperature, DHT_Humidity,
                      BME_Temperature, BME_Humidity, BME_Pressure,
                      DS_Temperature1, DS_Temperature2, timestamp
               FROM sensors
               ORDER BY timestamp DESC
               LIMIT 32; 
            """

while True:

    starttime = int(time.time() * 1000)

    counter = 0

    try:
        headers = {
            'Cache-Control': 'no-cache',
            'Pragma': 'no-cache'
        }

        response = requests.get(
            host + '/exec',
            params={'query': sql_query}
        )
        response.raise_for_status()  # Проверка на успешный статус ответа
        response_json = response.json()

        responsetime = int(time.time() * 1000) - starttime

        # Вывод структуры JSON-ответа для отладки
        #print(json.dumps(response_json, indent=2))

        # Убедимся, что ответ содержит данные
        if 'dataset' in response_json:

            total_rows = len(response_json['dataset'])

            Polosa()
            print(f"total_rows {total_rows}")
            Polosa()

            pos = 0
            for row in response_json['dataset']:
                #row_number = total_rows - pos
                row_number = pos
                pos += 1
                print(f"{row_number}\t{row[0]}\t{row[1]}\t{row[2]}\t{row[3]}\t{row[4]}\t{row[5]}\t{row[6]}\t{row[7]}")
                counter += 1

            Polosa()
            print(f"ROW_num\tDHT_T\tDHT_H\tBME_T\tBME_H\tBME_P\tDS_T1\tDS_T2\tTimestamp")
            Polosa()

        else:
            print("No data found in the response.", file=sys.stderr)

    except requests.exceptions.RequestException as e:
        print(f'Error: {e}', file=sys.stderr)
    except ValueError as e:
        print(f'Error decoding JSON: {e}', file=sys.stderr)

    print("responsetime (ms): ", responsetime)

    fulltime = int(time.time() * 1000) - starttime
    print("fulltime (ms): ", fulltime)
    print("counter: ", counter)

    # Ждем 30 секунд перед следующим запросом
    time.sleep(30)


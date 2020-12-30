import eventlet
import json
from flask import Flask, render_template
from flask_mqtt import Mqtt
from flask_socketio import SocketIO
from flask_bootstrap import Bootstrap
from threading import Thread

eventlet.monkey_patch()

app = Flask(__name__)

app.config['SECRET'] = 'my secret key'
app.config['TEMPLATES_AUTO_RELOAD'] = True
app.config['MQTT_BROKER_URL'] = '127.0.0.1'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_REFRESH_TIME'] = 1.0
app.config['MQTT_USERNAME'] = ''
app.config['MQTT_PASSWORD'] = ''
app.config['MQTT_KEEPALIVE'] = 5
app.config['MQTT_TLS_ENABLED'] = False
thread = None

mqtt = Mqtt(app)
socketio = SocketIO(app)
bootstrap = Bootstrap(app)

def background_stuff(args):
    pass

@app.route('/')
def index():
    global thread
    if thread is None:
        thread = Thread(target=background_stuff)
        thread.start()
    return render_template('testhaha14.html')

@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
    mqtt.subscribe('MPU9250_data')

@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
        payload = message.payload.decode()
        str_payload = str(message.payload.decode())
        p = json.loads(payload)

        str_deviceID = str(p['DeviceID'])

        if str_deviceID == "A":
            socketio.emit('mqtt_message_A', data=str_payload)
        elif str_deviceID == "B":
            socketio.emit('mqtt_message_B', data=str_payload)
        elif str_deviceID == "C":
            socketio.emit('mqtt_message_C', data=str_payload)
        elif str_deviceID == "D":
            socketio.emit('mqtt_message_D', data=str_payload)

if __name__ == '__main__':
    socketio.run(app, debug=True)



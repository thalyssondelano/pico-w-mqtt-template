import json
import os
import paho.mqtt.client as mqtt

BROKER_HOST = os.getenv("MQTT_BROKER_HOST", "localhost")
BROKER_PORT = int(os.getenv("MQTT_BROKER_PORT", 1883))

DEVICE_ID = "pico_w_01"
TOPIC_TELEMETRY = f"devices/{DEVICE_ID}/telemetry"
TOPIC_COMMAND = f"devices/{DEVICE_ID}/command"

def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print(f"[OK] Connected to MQTT Broker ({BROKER_HOST}:{BROKER_PORT})")
        client.subscribe(TOPIC_TELEMETRY)
        
        command_payload = json.dumps({"action": "set_led", "state": "ON"})
        client.publish(TOPIC_COMMAND, command_payload)
        print(f"[CMD] Sent to {TOPIC_COMMAND} -> {command_payload}")
    else:
        print(f"[ERROR] Connection failed with code: {reason_code}")

def on_message(client, userdata, msg):
    raw_payload = msg.payload.decode("utf-8", errors="ignore")
    try:
        data = json.loads(raw_payload)
        print(f"[{msg.topic}] Telemetry Received -> Device: {data.get('device_id')} | Uptime: {data.get('uptime')}s")
    except json.JSONDecodeError:
        print(f"[{msg.topic}] Raw Payload -> {raw_payload}")

if __name__ == "__main__":
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="python_backend_consumer")
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"Listening for device '{DEVICE_ID}'... (Press Ctrl+C to exit)")
    client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
    client.loop_forever()
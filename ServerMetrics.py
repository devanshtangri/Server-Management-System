import paho.mqtt.client as mqtt
import psutil
import time

mqtt_broker = "0.0.0.0"  # Server IP
mqtt_port = 11111  # MQTT broker port
mqtt_topic = "Metrics"

def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    client.subscribe(mqtt_topic)

def on_disconnect(client, userdata, rc):
   client.reconnect()
   time.sleep(2)

client = mqtt.Client()
client.on_connect = on_connect
client.on_disconnect = on_disconnect
# Connect to the MQTT broker
client.connect(mqtt_broker, mqtt_port, 60)
client.loop_start()

def publish_metrics():
    # CPU Usage and RAM Usage
    cpu_usage = psutil.cpu_percent()
    ram = psutil.virtual_memory()
    ram_used = ram.used / (1024 ** 3)
    ram_total = ram.total / (1024 ** 3)

    # Disk Usage
    disk = psutil.disk_usage('/mnt/Repository')
    disk_used = disk.used / (1024 ** 3)
    disk_total = disk.total / (1024 ** 3)

    # Network speed
    net_io_prev = psutil.net_io_counters()
    time.sleep(2)
    net_io_curr = psutil.net_io_counters()
    up_speed = ((net_io_curr.bytes_sent - net_io_prev.bytes_sent) / (1024 * 1024))
    down_speed = ((net_io_curr.bytes_recv - net_io_prev.bytes_recv) / (1024 * 1024))

    # CPU temperature
    cpu_temp = psutil.sensors_temperatures()["coretemp"][0].current

    # Uptime
    uptime = str(time.strftime("%H:%M", time.gmtime(time.time() - psutil.boot_time())))

    # Top CPU process
    top_cpu_process = max(psutil.process_iter(['cpu_percent', 'name']), key=lambda p: p.info['cpu_percent'])
    top_cpu_name = top_cpu_process.info['name']
    top_cpu_usage = top_cpu_process.info['cpu_percent']

    # Top RAM process
    top_ram_process = max(psutil.process_iter(['memory_info', 'name']), key=lambda p: p.info['memory_info'].rss)
    top_ram_name = top_ram_process.info['name']
    top_ram_usage = top_ram_process.info['memory_info'].rss / (1024 ** 2)

    json_payload = (
        f'{{"cpu_usage": {round(cpu_usage, 1)}, '
        f'"cpu_temp": {round(cpu_temp)}, '
        f'"ram_used": {round(ram_used, 1)}, '
        f'"ram_total": {round(ram_total, 1)}, '
        f'"disk_used": {round(disk_used)}, '
        f'"disk_total": {round(disk_total)}, '
        f'"uptime": "{uptime}", '
        f'"up_speed": {round(up_speed * 4, 1)}, '
        f'"down_speed": {round(down_speed * 4, 1)}, '
        f'"top_cpu_process": {{"name": "{top_cpu_process.info["name"]}", "usage": {round(top_cpu_usage, 1)}}}, '
        f'"top_ram_process": {{"name": "{top_ram_process.info["name"]}", "usage": {round(top_ram_usage)}}}'
    )

    client.publish(mqtt_topic, json_payload)
#Main loop
while True:
    publish_metrics()

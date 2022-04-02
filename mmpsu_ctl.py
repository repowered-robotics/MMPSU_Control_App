from flask import Flask, Response, url_for, render_template, request, send_file, send_from_directory
from markupsafe import escape
import mmpsu
import json
import urllib
import serial
import queue
import threading
import time

class Debugger(threading.Thread):
    def __init__(self):
        super().__init__()
        self.uart = serial.Serial("/dev/ttyAMA0")
        self.uart.baudrate = 115200
        self.msgs = queue.Queue(maxsize=16)
        self.listeners = []
        self.stop_event = threading.Event()
        self.daemon = True

    def listen(self):
        return self.msgs
    
    def stop(self):
        self.stop_event.set()

    def run(self):
        while not self.stop_event.is_set():
            line = self.uart.readline().strip()
            try:
                self.msgs.put_nowait(line)
            except queue.Full:
                pass # print("debug msg queue full")


app = Flask(__name__)

supply = mmpsu.MMPSU()

# debugger = Debugger()
# debugger.start()


@app.route("/")
def index(name="Index"):
    return render_template('index.html', name=name)


@app.route("/imgs/<filename>")
def serve_file(filename):
    return send_from_directory("imgs", filename)
    

@app.route("/mmpsu/poll", methods=['GET'])
def mmpsu_get_data():
    # print("Got poll")
    if 'type' in request.args.keys():
        poll_type = request.args.get('type', '')
        if poll_type == 'all':
            retval_dict = {}
            retval_dict['connected'] = supply.connected
            retval_dict['output_enabled'] = supply.enabled
            retval_dict['voltage'] = supply.voltage
            retval_dict['present'] = supply.get_phases_present()
            retval_dict['enabled'] = supply.get_phases_enabled()
            retval_dict['current'] = supply.get_phase_currents()
            retval_dict['overtemp'] = supply.get_phases_in_overtemp()

            i_total = 0.0
            for i in retval_dict['current']:
                i_total += i
            retval_dict['iout'] = i_total
            retval_dict['duty_cycle'] = supply.get_phase_duty_cycles()
            json_str = json.dumps(retval_dict)
            print(json_str)
            return json_str
        elif poll_type == 'voltage':
            return "{:.6f}".format(supply.voltage)
        elif poll_type == 'connected':
            return str(supply.connected)
        elif poll_type == 'enabled':
            return str(supply.enabled)
    else:
        return "Done"


@app.route("/mmpsu/config", methods=['GET'])
def mmpsu_configure():
    if 'voltage' in request.args.keys():
        try:
            vset = float(request.args.get('voltage', ''))
        except ValueError:
            vset = 0.0
        print("Setting voltage to {:.3f}V".format(vset))
        supply.set_voltage(vset)
    if 'enable' in request.args.keys():
        try:
            enab = request.args.get('enable', '') == 'True'
        except ValueError:
            enab = False
        print("Setting enable to {}".format(enab))
        supply.enabled = enab
    if 'phase_current_limits' in request.args.keys():
        json_str = urllib.parse.unquote(request.args.get('phase_current_limits', ''))
        # print(json_str)
        limits = json.loads(json_str)

        for phase in limits.keys():
            print("Setting current limit on phase {} to {} A".format(phase, limits[phase]))
            supply.set_phase_current_limit(phase, float(limits[phase]))

    return "Done"


# @app.route('/debug', methods=['GET'])
# def listen():
#     return None

#     def stream():
#         msgs = debugger.listen()
#         while True:
#             msg = msgs.get()
#             sse_msg = "data: {}\n\n".format(msg)
#             yield sse_msg
    
#     return Response(stream(), mimetype='text/event-stream')
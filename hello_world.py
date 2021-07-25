from flask import Flask, url_for, render_template, request
from markupsafe import escape
import mmpsu


app = Flask(__name__)

supply = mmpsu.MMPSU()


@app.route("/")
def index(name="Index"):
    return render_template('index.html', name=name)

@app.route("/mmpsu/", methods=['POST', 'GET'])
def mmpsu_control():
    retval = ""
    if request.method == 'GET':
        if 'enable' in request.args.keys():
            print(request.args.get('enable', ''))
            retval = "Done"
    
    return retval

@app.route("/mmpsu/poll", methods=['GET'])
def mmpsu_get_data():
    if 'type' in request.args.keys():
        poll_type = request.args.get('type', '')
        if poll_type == 'all':
            connected_str = str(supply.connected).lower()
            enabled_str = str(supply.enabled).lower()
            vout = supply.voltage
            return '{{"connected":{}, "enabled":{}, "voltage":{:.6f} }}'.format(connected_str, enabled_str, vout)
        elif poll_type == 'voltage':
            return "12.3141V"
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

    return "Done"
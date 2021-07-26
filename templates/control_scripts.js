const phase_names = ["a", "b", "c", "d", "e", "f"];

function get_mmpsu_voltage(){
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "mmpsu/poll?type=voltage", false);
    xhr.send( null );
    document.getElementById("voltage").innerHTML = xhr.responseText;
}

function getConnected(){
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "mmpsu/poll?type=connected", false);
    xhr.send( null );
    let resp = xhr.responseText;

    if(resp === "True"){
        document.getElementById("connection-status").innerHTML = "Connected";
    }else{
        document.getElementById("connection-status").innerHTML = "Not Connected";
    }
    
}

function getAllData(){
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "mmpsu/poll?type=all", false);
    xhr.send( null );
    var all_data = JSON.parse(xhr.responseText);

    if(all_data.connected){
        var conn_status = document.getElementById("connection-status");
        conn_status.setAttribute("style", "color: green;");
        conn_status.innerHTML = "CONNECTED";
        
        if(all_data.output_enabled){
            document.getElementById("enabled-status").innerHTML = "ENABLED";
        }else{
            document.getElementById("enabled-status").innerHTML = "DISABLED";
        }

        document.getElementById("voltage").innerHTML = all_data.voltage.toFixed(3) + " V";
        document.getElementById("current").innerHTML = all_data.current.toFixed(3) + " A";
        
        var present = all_data.present;
        var enabled = all_data.enabled;
        var current = all_data.current;
        var duty_cycle = all_data.duty_cycle;
        var overtemps = all_data.overtemp;

        var present_statuses = [false, false, false, false, false, false];
        var enabled_statuses = [false, false, false, false, false, false];
        var overtemp_statuses = [false, false, false, false, false, false];

        /* set corresponding statuses member to true if phase is present */
        for(var i in present){
            present_statuses[present[i]] = true;
        }
        /* set corresponding statuses member to true if phase is enabled */
        for(var i in enabled){
            enabled_statuses[enabled[i]] = true;
        }
        /* set corresponding statuses member to true if phase is enabled */
        for(var i in overtemps){
            overtemp_statuses[overtemps[i]] = true;
        }

        /* set elements for each statuses member */
        for(var i in present_statuses){
            var phase_name = phase_names[i];
            var present_id = "phase-" + phase_name + "-present";
            var enabled_id = "phase-" + phase_name + "-enabled";
            var current_id = "phase-" + phase_name + "-current";
            var duty_cycle_id = "phase-" + phase_name + "-duty-cycle";
            var overtemp_id = "phase-" + phase_name + "-overtemp";
            if(present_statuses[i]){
                document.getElementById(present_id).innerHTML = "YES";
                if(enabled_statuses[i]){
                    document.getElementById(enabled_id).innerHTML = "YES";
                }else{
                    document.getElementById(enabled_id).innerHTML = "NO";
                }

                if(overtemp_statuses[i]){
                    document.getElementById(overtemp_id).setAttribute("style", "background-color: red;");
                }else{
                    document.getElementById(overtemp_id).setAttribute("style", "background-color: gray;");
                }
                document.getElementById(current_id).innerHTML = current[i].toFixed(3) + " A";
                document.getElementById(duty_cycle_id).innerHTML = (duty_cycle[i]*100.0).toFixed(2) + " &percnt;";
            }else{
                document.getElementById(present_id).innerHTML = "NO";
                document.getElementById(enabled_id).innerHTML = "NO";
                document.getElementById(current_id).innerHTML = "--- A";
                document.getElementById(duty_cycle_id).innerHTML = "--- &percnt;";
                document.getElementById(overtemp_id).setAttribute("style", "background-color: gray;");
            }
        }

    }else{
        var conn_status = document.getElementById("connection-status");
        conn_status.setAttribute("style", "color: red;");
        conn_status.innerHTML = "NOT CONNECTED";
        
        document.getElementById("enabled-status").innerHTML = "---";
        document.getElementById("voltage").innerHTML = "--- V"
        document.getElementById("current").innerHTML = "--- A";
    }

}

function appendDebugMessage(msg){
    var window = document.getElementById("debugger-window");
    var existingText = window.innerHTML;
    window.innerHTML = existingText + "<p>" + msg + "</p>";
    var lastLine = window.lastElementChild;
    var scroll = window.scrollTop;
    scroll += lastLine.offsetHeight + 0.5;
    window.scrollTop = scroll;
}

function pollForData(interval){
    let eventSource = new EventSource("http://adams-pi3.local:5000/debug");

    eventSource.onmessage = function(event){
        appendDebugMessage(event.data);
    };

    setInterval(function(){
        getAllData();
    }, interval);
}

function toggle_enable_mmspu(){
    var btn = document.getElementById("toggle-enable-btn");
    
    if(btn.innerHTML === "ENABLE"){
        enable_mmspu();
        btn.innerHTML = "DISABLE";
    }else{
        disable_mmspu();
        btn.innerHTML = "ENABLE";
    }

}

function setCurrentLimit(phase_number){
    var name = phase_names[phase_number];
    limit_id = "phase-" + name + "-limit-entry";
    limit = document.getElementById(limit_id).value
    var obj = new Object();
    obj[name] = limit;
    var url_str = "mmpsu/config?phase_current_limits=" + JSON.stringify(obj);
    var xhr = new XMLHttpRequest();
    xhr.open("GET", url_str, false);
    xhr.send( null );
}

function setTotalCurrentLimit(){
    var limit = parseFloat(document.getElementById("total-ilimit-entry").value);
    var phase_count = 0;
    var phase_names_present = [];
    for(var i in phase_names){
        var present_id = "phase-" + phase_names[i] + "-present";
        var present_str = document.getElementById(present_id).innerHTML;
        if(present_str.toUpperCase() === "YES"){
            phase_count++;
            phase_names_present.push(phase_names[i]);
        }
    }
    if(phase_count > 0){
        var per_phase_limit = limit/phase_count;
        for(var i in phase_names_present){
            var entry_id = "phase-" + phase_names_present[i] + "-limit-entry";
            var btn_id = "phase-" + phase_names_present[i] + "-set-limit-btn";
            document.getElementById(entry_id).value = per_phase_limit.toFixed(3);
            document.getElementById(btn_id).click();
        }
    }
    
    
}

function enable_mmspu() {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "mmpsu/?enable=True", false);
    xhr.send( null );
}

function disable_mmspu(){
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "mmpsu/?enable=False", false);
    xhr.send( null );
}


function setVoltage(){
    var entry = document.getElementById("vset-entry")
    var url_str = "mmpsu/config?voltage=" + entry.value;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", url_str, false);
    xhr.send( null );
}


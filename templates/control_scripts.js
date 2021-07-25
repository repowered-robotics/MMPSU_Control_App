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

    if(all_data.enabled){
        document.getElementById("enabled-status").innerHTML = "ENABLED";
    }else{
        document.getElementById("enabled-status").innerHTML = "DISABLED";
    }

    if(all_data.connected){
        document.getElementById("connection-status").innerHTML = "CONNECTED";
    }else{
        document.getElementById("connection-status").innerHTML = "NOT CONNECTED";
    }

    document.getElementById("voltage").innerHTML = all_data.voltage.toFixed(3) + " V"
}

function pollForData(interval){
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
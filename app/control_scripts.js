const phase_names = ["a", "b", "c", "d", "e", "f"];


var app = angular.module('mmpsuApp', []);

app.controller('mmpsuAppCtl', function($scope) {
    $scope.socket = io();

    $scope.mmpsu = {
        enabled: false,
        connected: false,
        vout_setpt: 12.0,
        iout_limit: 150.0,
        vout_str: "0.0 V",
        iout_str: "0.0 A",
        phases: {},
        enabled_str: "---",
        enabled_style: {color:'white'},
        connected_str: "NOT CONNECTED",
        connected_style: {color: 'red'}
    };

    $scope.socket.on('update', function(data){
        $scope.apply(function(){
            if(data.connected){
                // 
                $scope.mmpsu.connected = true;
                $scope.mmpsu.connected_str = "CONNECTED";
                $scope.mmpsu.connected_style = {color:'green'};
                $scope.mmpsu.enabled = data.enabled;
                if($scope.mmpsu.enabled){
                    $scope.mmpsu.enabled_str = "ENABLED";
                    $scope.mmpsu.enabled_style = {color:'green'};
                }else{
                    $scope.mmpsu.enabled_str = "DISABLED";
                    $scope.mmpsu.enabled_style = {color:'red'};
                }
                $scope.mmpsu.vout = data.vout;
                $scope.mmpsu.vout_str = $scope.mmpsu.vout.toFixed(3) + " V";
                for(var ch in data.phases){
                    $scope.mmpsu.phases[ch].present = data.phases[ch].present;
                    if($scope.mmpsu.phases[ch].present){
                        // things to do when present
                        $scope.mmpsu.phases[ch].present_str = "YES";
                        $scope.mmpsu.phases[ch].enabled_str = (data.phases[ch].enabled ? "YES": "NO");
                        $scope.mmpsu.phases[ch].duty_cycle_str = data.phases[ch].duty_cycle.toFixed(2);
                        $scope.mmpsu.phases[ch].current_str = data.phases[ch].current.toFixed(2) + " A";
                        $scope.mmpsu.phases[ch].overtemp = data.phases[ch].overtemp;
                        $scope.mmpsu.phases[ch].overtemp_style = (data.phases[ch].overtemp ? {'background-color':"red"} : {'background-color':"gray"});
                    }else{
                        // things to do when not present
                        $scope.mmpsu.phases[ch].present_str = "NO";
                        $scope.mmpsu.phases[ch].enabled_str = "---";
                        $scope.mmpsu.phases[ch].duty_cycle_str = "---";
                        $scope.mmpsu.phases[ch].current_str = "---";
                        $scope.mmpsu.phases[ch].overtemp_style = {'background-color':"gray"};
                    }
                }
            }else{
                $scope.mmpsu.connected = false;
                $scope.mmpsu.connected_str = "NOT CONNECTED";
                $scope.mmpsu.connected_style = {color:'red'};
                $scope.mmpsu.enabled_str = "---";
                $scope.mmpsu.enabled_style = {color:'white'};
            }
        });
    });

    $scope.enableMmpsu = function() {
        $scope.socket.emit('configure', {enabled: true});
    };

    $scope.disableMmpsu = function() {
        $scope.socket.emit('configure', {enabled: false});
    };

    $scope.toggleEnabled = function() {
        if($scope.mmpsu.enabled){
            $scope.disableMmpsu();
        }else{
            $scope.enableMmpsu();
        }
    };

    $scope.setVoltage = function() {
        config = {vout_setpt: $scope.mmpsu.vout_setpt};
        $scope.socket.emit('configure', config);
    }
});

function appendDebugMessage(msg){
    var window = document.getElementById("debugger-window");
    var existingText = window.innerHTML;
    window.innerHTML = existingText + "<p>" + msg + "</p>";
    var lastLine = window.lastElementChild;
    var scroll = window.scrollTop;
    scroll += lastLine.offsetHeight + 0.5;
    window.scrollTop = scroll;
}

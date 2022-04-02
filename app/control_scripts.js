var app = angular.module('mmpsuApp', []);

app.controller('mmpsuAppCtl', function($scope) {
    $scope.socket = io();

    $scope.mmpsu = {
        enabled: false,
        connected: false,
        vout_setpt: 12.0,
        iout_limit: 150.0,
        vout_str: "0.0 V",
        iout: 0.0,
        iout_str: "0.0 A",
        phases: {},
        enabled_str: "---",
        enabled_style: {color:'white'},
        connected_str: "NOT CONNECTED",
        connected_style: {color: 'red'},
        devel_mode: false,
        state: 0,
        state_str: "OUTPUT_OFF",
        voltage_kp: 1,
        voltage_ki: 32,
        current_kp: 8,
        current_ki: 13,
        i2c_error_count: 0,
        manual_mode: false,
        phase_count: 0,
        balance_phase_current: false
    };

    $scope.socket.on('update', function(data){
        $scope.$apply(function(){
            $scope.mmpsu.connected = data.connected;
            if($scope.mmpsu.connected){
                // 
                $scope.mmpsu.connected_str = "CONNECTED";
                $scope.mmpsu.connected_style = {color:'green'};
                $scope.mmpsu.enabled = data.enabled;
                $scope.mmpsu.state = data.state;
                $scope.mmpsu.state_str = data.state_str;
                $scope.mmpsu.i2c_error_count = data.i2c_error_count;
                if($scope.mmpsu.enabled){
                    $scope.mmpsu.enabled_str = "ENABLED";
                    $scope.mmpsu.enabled_style = {color:'green'};
                }else{
                    $scope.mmpsu.enabled_str = "DISABLED";
                    $scope.mmpsu.enabled_style = {color:'red'};
                }
                $scope.mmpsu.vout = data.vout;
                $scope.mmpsu.vout_str = $scope.mmpsu.vout.toFixed(3) + " V";
                var raw_iout = 0.0;
                for(const ch in data.phases){
                    $scope.mmpsu.phases[ch] = {};
                    $scope.mmpsu.phases[ch].present = data.phases[ch].present;
                    if($scope.mmpsu.phases[ch].present){
                        // things to do when present
                        $scope.mmpsu.phases[ch].present_str = "YES";
                        if(data.phases[ch].enabled){
                            $scope.mmpsu.phases[ch].enabled_str = "YES";
                            $scope.mmpsu.phases[ch].enabled_style = {'background-color':"green"};
                            $scope.mmpsu.phases[ch].phase_style = {color: "white"};
                            $scope.mmpsu.phases[ch].duty_cycle_str = data.phases[ch].duty_cycle.toFixed(2);
                            raw_iout += data.phases[ch].current;
                            $scope.mmpsu.phases[ch].current_str = data.phases[ch].current.toFixed(2) + " A";
                        }else{
                            $scope.mmpsu.phases[ch].enabled_str = "NO";
                            $scope.mmpsu.phases[ch].enabled_style = {'background-color':"gray"};
                            $scope.mmpsu.phases[ch].phase_style = {"color":"LightGray"};
                            $scope.mmpsu.phases[ch].duty_cycle_str = "---";
                            $scope.mmpsu.phases[ch].current_str = "---";
                        }
                        $scope.mmpsu.iout = ($scope.mmpsu.iout + raw_iout)/2.0;
                        $scope.mmpsu.iout_str = $scope.mmpsu.iout.toFixed(2) + " A";
                        $scope.mmpsu.phases[ch].overtemp = data.phases[ch].overtemp;
                        $scope.mmpsu.phases[ch].overtemp_style = (data.phases[ch].overtemp ? {'background-color':"red"} : {'background-color':"gray"});
                    }else{
                        // things to do when not present
                        $scope.mmpsu.phases[ch].phase_style = {"color":"Gray"};
                        $scope.mmpsu.phases[ch].enabled_style = {'background-color':"gray"};
                        $scope.mmpsu.phases[ch].present_str = "NO";
                        $scope.mmpsu.phases[ch].enabled_str = "---";
                        $scope.mmpsu.phases[ch].duty_cycle_str = "---";
                        $scope.mmpsu.phases[ch].current_str = "---";
                        $scope.mmpsu.phases[ch].overtemp_style = {'background-color':"gray"};
                    }
                }
            }else{
                $scope.mmpsu.connected_str = "NOT CONNECTED";
                $scope.mmpsu.connected_style = {color:'red'};
                $scope.mmpsu.enabled_str = "---";
                $scope.mmpsu.enabled_style = {color:'white'};
            }
        });
    });

    $scope.enableMmpsu = function() {
        var config =  {enabled: true};
        $scope.socket.emit('configure', config);
    };

    $scope.disableMmpsu = function() {
        var config =  {enabled: false};
        $scope.socket.emit('configure', config);
    };

    $scope.toggleEnabled = function() {
        if($scope.mmpsu.enabled){
            $scope.disableMmpsu();
        }else{
            $scope.enableMmpsu();
        }
    };

    $scope.setVoltage = function() {
        config = {vout_setpt: parseFloat($scope.mmpsu.vout_setpt)};
        $scope.socket.emit('configure', config);
    };

    $scope.setDeveloperMode = function() {
        config = {devel_mode: $scope.mmpsu.devel_mode};
        $scope.socket.emit('configure', config);
    };

    $scope.setVoltageKp = function() {
        config = {voltage_kp: parseFloat($scope.mmpsu.voltage_kp)};
        $scope.socket.emit('configure', config);
    };

    $scope.setVoltageKi = function() {
        config = {voltage_ki: parseFloat($scope.mmpsu.voltage_ki)};
        $scope.socket.emit('configure', config);
    };

    $scope.setCurrentKp = function() {
        config = {current_kp: parseFloat($scope.mmpsu.current_kp)};
        $scope.socket.emit('configure', config);
    };

    $scope.setCurrentKi = function() {
        config = {current_ki: parseFloat($scope.mmpsu.current_ki)};
        $scope.socket.emit('configure', config);
    };

    $scope.setManualMode = function() {
        config = {manual_mode: $scope.mmpsu.manual_mode};
        $scope.socket.emit('configure', config);
    };

    $scope.setPhaseCount = function() {
        config = {phase_count: $scope.mmpsu.phase_count};
        $scope.socket.emit('configure', config);
    };

    $scope.setEnablePhaseCurrentBalance = function() {
        config = {balance_phase_current: $scope.mmpsu.balance_phase_current};
        $scope.socket.emit('configure', config);
    };

});

// function appendDebugMessage(msg){
//     var window = document.getElementById("debugger-window");
//     var existingText = window.innerHTML;
//     window.innerHTML = existingText + "<p>" + msg + "</p>";
//     var lastLine = window.lastElementChild;
//     var scroll = window.scrollTop;
//     scroll += lastLine.offsetHeight + 0.5;
//     window.scrollTop = scroll;
// }

from typing import Any
import smbus
import struct
import queue
import subprocess

# BEGIN CONSTANTS
OUTPUT_ENABLED = 0
VOUT_MEASURED = 1
VOUT_SETPOINT = 2
PHASES_PRESENT = 3
PHASES_ENABLED = 4
PHASE_A_DUTY_CYCLE = 5
PHASE_A_CURRENT = 6
PHASE_A_CURRENT_LIMIT = 7
PHASE_B_DUTY_CYCLE = 8
PHASE_B_CURRENT = 9
PHASE_B_CURRENT_LIMIT = 10
PHASE_C_DUTY_CYCLE = 11
PHASE_C_CURRENT = 12
PHASE_C_CURRENT_LIMIT = 13
PHASE_D_DUTY_CYCLE = 14
PHASE_D_CURRENT = 15
PHASE_D_CURRENT_LIMIT = 16
PHASE_E_DUTY_CYCLE = 17
PHASE_E_CURRENT = 18
PHASE_E_CURRENT_LIMIT = 19
PHASE_F_DUTY_CYCLE = 20
PHASE_F_CURRENT = 21
PHASE_F_CURRENT_LIMIT = 22
PHASES_IN_OVERTEMP = 23
DEBUG_MODE = 24
NUM_FIELDS = 25

MMPSU_ADDR = 0x5A

DEBUG_MODE_OFF = 0
DEBUG_MODE_RAW = 1
DEBUG_MODE_PRETTY = 2

MMPSU_CMD_NONE = 0x00
MMPSU_CMD_READ = 0x01
MMPSU_CMD_WRITE = 0x02
MMPSU_CMD_READ_ALL = 0x03
MMPSU_FIELD_SIZE = 4

MAX_PHASES = 6

class PhaseData:
    def __init__(self):
        self.current = 0.0
        self.current_limit = 0.0
        self.duty_cycle = 0.0
    


class MMPSU:
    """A class for controlling the Repowered Robotics Modular Multipase Power Supply Unit
    """

    def __init__(self):
        self._bus = smbus.SMBus(1)
        self._connected = False
        self._addr = MMPSU_ADDR
        self.errors = queue.Queue(maxsize=128)
        self._phases = []
        for i in range(0, MAX_PHASES):
            data = PhaseData()
            data.current_limit = 30.0
            self._phases.append(data)
    def _put_error(self, msg):
        try:
            self.errors.put(msg)
        except queue.Full:
            pass

    # def _write_field(self, field: int, value: list) -> None:
    #     try:
    #         cmd = (field << 2) | MMPSU_CMD_WRITE
    #         # vals = []
    #         # for i in range(0,4):
    #         #     vals[i] = (value >> (i*8)) & 0xFF
    #         print(value)
    #         self._bus.write_byte(self._addr, cmd)
    #         self._bus.write_i2c_block_data(self._addr, 0, value)
    #         self._connected = True
    #     except IOError:
    #         self._put_error("I2C Write Error")
    #         self._connected = False
    
    # def _read_field(self, field: int) -> bytearray:
    #     cmd = (field << 2) | MMPSU_CMD_READ
    #     retval = bytearray()
    #     try:
    #         self._bus.write_byte(self._addr, cmd)
    #         data = self._bus.read_i2c_block_data(self._addr, 0, 4)
    #         print(data)
    #         for val in data:
    #             retval.append(val)
    #         self._connected = True
    #     except IOError:
    #         self._put_error("I2C Read Error")
    #         self._connected = False

    #     return retval
    
    def _write_int(self, field: int, data: int) -> None:
        parts = []
        parts.append("./bin/mmpsu_comm")
        parts.append("/dev/i2c-1")
        parts.append("{:d}".format(MMPSU_CMD_WRITE))
        parts.append("{:d}".format(field))
        parts.append("{:d}".format(data))
        subprocess.run(parts)
        self._connected = True
    
    def _read_int(self, field: int) -> int:
        parts = []
        parts.append("./bin/mmpsu_comm")
        parts.append("/dev/i2c-1")
        parts.append("{:d}".format(MMPSU_CMD_READ))
        parts.append("{:d}".format(field))
        proc = subprocess.run(parts, capture_output=True)
        data = proc.stdout
        # print(data)
        if len(data) < 1:
            self._connected = False
            return 0
        else:
            self._connected = True
            try:
                return int(data)
            except ValueError:
                self._connected = False
                return 0

    def update_all(self) -> None:
        cmd = MMPSU_CMD_READ_ALL
        length = MMPSU_FIELD_SIZE * NUM_FIELDS
        data = self._bus.read_i2c_block_data(self._addr, cmd, length)
        # TODO unpack all the data
        # OUTPUT_ENABLED
        offset = MMPSU_FIELD_SIZE*OUTPUT_ENABLED
        self.is_enabled = struct.unpack_from("<i", data, offset)
        return None

    def enable_output(self) -> None:
        self.is_enabled = True
        self._write_int(OUTPUT_ENABLED, 1)
        
    def disable_output(self) -> None:
        self.is_enabled = False
        self._write_int(OUTPUT_ENABLED, 0)
    
    def set_output_enabled(self, enabled: bool) -> None:
        if enabled:
            self.enable_output()
        else:
            self.disable_output()

    def get_output_enabled(self) -> bool:
        enabled = self._read_int(OUTPUT_ENABLED)
        print(enabled)
        if enabled <= 0:
            return False
        else:
            return True

    def set_voltage(self, voltage: float) -> None:
        vsend = int(1000.0*voltage)
        self._write_int(VOUT_SETPOINT, vsend)
    
    def get_voltage(self) -> float:
        vout_raw = self._read_int(VOUT_MEASURED)
        return float(vout_raw)/1000.0
    
    def get_phases_present(self) -> list:
        retval = []
        phases_present = self._read_int(PHASES_PRESENT)
        for i in range(0, MAX_PHASES):
            if (phases_present >> i) & 1 == 1:
                retval.append(i)
        
        return retval
        
    def get_phases_enabled(self) -> list:
        retval = []
        phases_enabled = self._read_int(PHASES_ENABLED)
        for i in range(0, MAX_PHASES):
            if (phases_enabled >> i) & 1 == 1:
                retval.append(i)
        
        return retval

    def get_phase_currents(self) -> list:
        retval = []
        value = float(self._read_int(PHASE_A_CURRENT))/1000.0
        retval.append(value)
        value = float(self._read_int(PHASE_B_CURRENT))/1000.0
        retval.append(value)
        value = float(self._read_int(PHASE_C_CURRENT))/1000.0
        retval.append(value)
        value = float(self._read_int(PHASE_D_CURRENT))/1000.0
        retval.append(value)
        value = float(self._read_int(PHASE_E_CURRENT))/1000.0
        retval.append(value)
        value = float(self._read_int(PHASE_F_CURRENT))/1000.0
        retval.append(value)
        return retval
    
    def get_phase_duty_cycles(self) -> list:
        retval = []
        value = float(self._read_int(PHASE_A_DUTY_CYCLE))/5440.0
        retval.append(value)
        value = float(self._read_int(PHASE_B_DUTY_CYCLE))/5440.0
        retval.append(value)
        value = float(self._read_int(PHASE_C_DUTY_CYCLE))/5440.0
        retval.append(value)
        value = float(self._read_int(PHASE_D_DUTY_CYCLE))/5440.0
        retval.append(value)
        value = float(self._read_int(PHASE_E_DUTY_CYCLE))/5440.0
        retval.append(value)
        value = float(self._read_int(PHASE_F_DUTY_CYCLE))/5440.0
        retval.append(value)
        return retval
    
    def get_phases_in_overtemp(self):
        retval = []
        phases_in_overtemp = self._read_int(PHASES_IN_OVERTEMP)
        for i in range(0, MAX_PHASES):
            if (phases_in_overtemp >> i) & 1 == 1:
                retval.append(i)
        return retval

    def set_phase_current_limit(self, phase, limit) -> None:
        phase_num = 0
        if type(phase) is str:
            if phase.upper() == 'A' or phase == '0':
                phase_num = PHASE_A_CURRENT_LIMIT
            elif phase.upper() == 'B' or phase == '1':
                phase_num = PHASE_B_CURRENT_LIMIT
            elif phase.upper() == 'C' or phase == '2':
                phase_num = PHASE_C_CURRENT_LIMIT
            elif phase.upper() == 'D' or phase == '3':
                phase_num = PHASE_D_CURRENT_LIMIT
            elif phase.upper() == 'E' or phase == '4':
                phase_num = PHASE_E_CURRENT_LIMIT
            elif phase.upper() == 'F' or phase == '5':
                phase_num = PHASE_F_CURRENT_LIMIT
            else:
                return None
        elif type(phase) is int:
            if phase == 0:
                phase_num = PHASE_A_CURRENT_LIMIT
            elif phase == 1:
                phase_num = PHASE_B_CURRENT_LIMIT
            elif phase == 2:
                phase_num = PHASE_C_CURRENT_LIMIT
            elif phase == 3:
                phase_num = PHASE_D_CURRENT_LIMIT
            elif phase == 4:
                phase_num = PHASE_E_CURRENT_LIMIT
            elif phase == 5:
                phase_num = PHASE_F_CURRENT_LIMIT
            else:
                return None
        else:
            return None
        try:
            limit_int = int(float(limit)*1000.0 + 0.5)
        except ValueError:
            return None
            
        self._write_int(phase_num, limit_int)

    def get_current_out(self) -> float:
        currents = self.get_phase_currents()
        retval = 0.0
        for i in currents:
            retval += i
        return retval
    
    def enable_pretty_debug(self):
        self._write_int(DEBUG_MODE, DEBUG_MODE_PRETTY)
    
    def enable_raw_debug(self):
        self._write_int(DEBUG_MODE, DEBUG_MODE_RAW)

    # ==== BEGIN PROPERTY GUBBINS ====
    connected = property(lambda self: self._connected, None, None, "Get the status of whether or not MMPSU is found on I2C bus")
    enabled = property(get_output_enabled, set_output_enabled, None, "Get or Set the output enabled status")
    voltage = property(get_voltage, set_voltage, None, "Get or Set the output voltage")
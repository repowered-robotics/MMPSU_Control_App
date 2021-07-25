import smbus
import struct
import queue

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
NUM_FIELDS = 23

MMPSU_ADDR = 0x5A

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
        self.errors = queue.Queue()
        self._phases = []
        for i in range(0, MAX_PHASES):
            data = PhaseData()
            data.current_limit = 30.0
            self._phases.append(data)
    
    def _write_field(self, field: int, value: bytearray) -> None:
        try:
            cmd = (field << 2) | MMPSU_CMD_WRITE
            self._bus.write_i2c_block_data(self._addr, cmd, value)
            self._connected = True
        except IOError:
            self.errors.put("Write Error")
            self._connected = False
    
    def _read_field(self, field: int) -> bytearray:
        cmd = (field << 2) | MMPSU_CMD_READ
        retval = bytearray()
        try:
            data = self._bus.read_i2c_block_data(self._addr, cmd, 4)
            for val in data:
                retval.append(val)
            self._connected = True
        except IOError:
            self._connected = False

        return retval
    
    def _write_int(self, field: int, data: int) -> None:
        arr = bytearray(4)
        struct.pack_into('<i', data, 0, 1)
        self._write_field(field, arr)
    
    def _read_int(self, field: int) -> int:
        data = self._read_field(field)
        if len(data) < 4:
            return 0
        else:
            return struct.unpack('<i', data)[0]

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

    # ==== BEGIN PROPERTY GUBBINS ====
    connected = property(lambda self: self._connected, None, None, "Get the status of whether or not MMPSU is found on I2C bus")
    enabled = property(get_output_enabled, set_output_enabled, None, "Get or Set the output enabled status")
    voltage = property(get_voltage, set_voltage, None, "Get or Set the output voltage")
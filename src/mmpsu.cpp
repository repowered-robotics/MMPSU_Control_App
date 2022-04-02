#include "mmpsu.h"
#include <cstring>
#include <cstdlib>
#include <string.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <i2c/smbus.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <chrono>
#include <thread>


// PhaseData::PhaseData(){
//     present = false;
//     enabled = false;
//     duty_cycle = 0.0;
//     current = 0.0;
// }

// PhaseData::PhaseData(const PhaseData& ph){
//     present = ph.present;
//     enabled = ph.enabled;
//     duty_cycle = ph.duty_cycle;
//     current = ph.current;
//     overtemp = ph.overtemp;
// }

// MMPSUState::MMPSUState(){

// }


// void MMPSUState::update_phase_data(int ind, PhaseData phase){
//     m_phases[ind] = phase;
// }

// PhaseData MMPSUState::get_phase_data(int ind){
//     try{
//         return m_phases.at(ind);
//     }catch(const std::out_of_range& e){
//         return PhaseData();
//     }
// }

/**
 * 
 */
void start_i2c_connection(int fd, uint8_t addr, MMPSUError& error){
    if(ioctl(fd, I2C_SLAVE, addr) < 0){
        error = MMPSUError::I2C_SLAVE_CONNECT_ERROR;
    }else{
        error = MMPSUError::NONE;
    }
}

void close_i2c_connection(int fd){
    close(fd);
}


bool mmpsu_test_connection(int fd, MMPSUError& error){
    int test = mmpsu_read_field(fd, PHASES_PRESENT, 50000, error);
    return (test >= 64) || error != MMPSUError::NONE;
}

/**
 * @brief read a field from mmpsu over I2C
 * @param fd i2c sysfs file descriptor
 * @param field field number to read
 * @param timeout read and write timeout in microseconds
 * @param error reference to an instance of MMPSUError
 * @return the value of the field read
 */
int mmpsu_read_field(int fd, int field, int timeout, MMPSUError& error){
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout;
    fd_set write_fds;
    fd_set read_fds;
    int retval = 0;

    uint8_t cmd_byte = (field << 2) | MMPSU_CMD_READ;

    FD_ZERO(&write_fds);
    FD_SET(fd, &write_fds);

    // check that we can write the cmd byte
    int sel = select(fd + 1, NULL, &write_fds, NULL, &tv);
    if(sel > 0){
        if(write(fd, &cmd_byte, 1) != 1){
            error = MMPSUError::I2C_WRITE_ERROR;
            return 0;
        }
    }else if(sel == 0){
        // timeout occurred
        error = MMPSUError::I2C_WRITE_TIMEOUT;
    }else{
        // other error occurred
        error = MMPSUError::I2C_ERROR;
    }
    
    std::this_thread::sleep_for(std::chrono::microseconds(500)); // brief delay for slave-side processing

    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    tv.tv_sec = 0;
    tv.tv_usec = timeout;

    // check that we can read the response bytes
    sel = select(fd + 1, &read_fds, NULL, NULL, &tv);
    if(sel > 0){
        uint8_t buf[4];
        memset(buf, 0, 4);
        int read_count = read(fd, buf, 4);
        if(read_count < 4){
            error = MMPSUError::I2C_READ_ERROR;
            return 0;
        }
        for(int i = 0; i < 4; i++){
            retval |= buf[i] << (i*8);
        }
        
    }else if(sel == 0){
        // timeout occurred
        error = MMPSUError::I2C_READ_TIMEOUT;
    }else{
        // other error occurred
        error = MMPSUError::I2C_ERROR;
    }
    
    return retval;
}

/**
 * @brief write a value to a specified field over I2C
 * 
 * @param fd i2c sysfs file descriptor
 * @param field field number to write
 * @param value integer value to write to field
 * @param timeout read and write timeout in microseconds
 * @param error reference to an instance of MMPSUError
 */
void mmpsu_write_field(int fd, int field, int value, int timeout, MMPSUError& error){
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout;
    fd_set write_fds;
    fd_set read_fds;
    int retval = 0;

    uint8_t cmd_byte = (field << 2) | MMPSU_CMD_WRITE;

    FD_ZERO(&write_fds);
    FD_SET(fd, &write_fds);

    // check that we can write the cmd byte
    int sel = select(fd + 1, NULL, &write_fds, NULL, &tv);
    if(sel > 0){
        if(write(fd, &cmd_byte, 1) != 1){
            error = MMPSUError::I2C_WRITE_ERROR;
            return;
        }
    }else if(sel == 0){
        // timeout occurred
        error = MMPSUError::I2C_WRITE_TIMEOUT;
    }else{
        // other error occurred
        error = MMPSUError::I2C_ERROR;
    }
    
    std::this_thread::sleep_for(std::chrono::microseconds(500)); // brief delay for slave-side processing
    
    FD_ZERO(&write_fds);
    FD_SET(fd, &write_fds);
    tv.tv_sec = 0;
    tv.tv_usec = timeout;

    // check that we can now write the data bytes
    sel = select(fd + 1, NULL, &write_fds, NULL, &tv);
    if(sel > 0){
        uint8_t buf[4];
        // copy bytes in little endian format
        for(int i = 0; i < 4; i++){
            buf[i] = (value >> (i*8)) & 0xFF;
        }
        if(write(fd, buf, 4) != 4){
            error = MMPSUError::I2C_WRITE_ERROR;
            return;
        }
    }else if(sel == 0){
        // timeout occurred
        error = MMPSUError::I2C_WRITE_TIMEOUT;
    }else{
        // other error occurred
        error = MMPSUError::I2C_ERROR;
    }
    
}

void mmpsu_set_enabled(int fd, bool enabled, MMPSUError& error){
    int value = (enabled ? 1 : 0);
    mmpsu_write_field(fd, OUTPUT_ENABLED, value, 50000, error);
}

void mmpsu_set_vout(int fd, float vout, MMPSUError& error){
    int value =(int)(vout * 1000.0);
    mmpsu_write_field(fd, VOUT_SETPOINT, value, 50000, error);
}

float mmpsu_get_vout(int fd, MMPSUError& error){
    int vout_mv = mmpsu_read_field(fd, VOUT_MEASURED, 50000, error);
    return (vout_mv / 1000.0);
}

int mmpsu_get_phases_present(int fd, MMPSUError& error){
    return mmpsu_read_field(fd, PHASES_PRESENT, 50000, error);
}


int mmpsu_get_phases_enabled(int fd, MMPSUError& error){
    return mmpsu_read_field(fd, PHASES_ENABLED, 50000, error);
}


int mmpsu_get_phases_in_overtemp(int fd, MMPSUError& error){
    return mmpsu_read_field(fd, PHASES_IN_OVERTEMP, 50000, error);
}


float mmpsu_get_phase_current(int fd, int channel, MMPSUError& error){
    int i_mv = 0;
    switch (channel) {
        case 0:{
            i_mv = mmpsu_read_field(fd, PHASE_A_CURRENT, 50000, error);
            break;
        }case 1:{
            i_mv = mmpsu_read_field(fd, PHASE_B_CURRENT, 50000, error);
            break;
        }case 2:{
            i_mv = mmpsu_read_field(fd, PHASE_C_CURRENT, 50000, error);
            break;
        }case 3:{
            i_mv = mmpsu_read_field(fd, PHASE_D_CURRENT, 50000, error);
            break;
        }case 4:{
            i_mv = mmpsu_read_field(fd, PHASE_E_CURRENT, 50000, error);
            break;
        }case 5:{
            i_mv = mmpsu_read_field(fd, PHASE_F_CURRENT, 50000, error);
            break;
        }default:{
            i_mv = 0;
            break;
        }
    }
    return i_mv/1000.0;
}

float mmpsu_get_phase_duty_cycle(int fd, int channel, MMPSUError& error){
    int duty = 0;
    switch (channel) {
        case 0:{
            duty = mmpsu_read_field(fd, PHASE_A_DUTY_CYCLE, 50000, error);
            break;
        }case 1:{
            duty = mmpsu_read_field(fd, PHASE_B_DUTY_CYCLE, 50000, error);
            break;
        }case 2:{
            duty = mmpsu_read_field(fd, PHASE_C_DUTY_CYCLE, 50000, error);
            break;
        }case 3:{
            duty = mmpsu_read_field(fd, PHASE_D_DUTY_CYCLE, 50000, error);
            break;
        }case 4:{
            duty = mmpsu_read_field(fd, PHASE_E_DUTY_CYCLE, 50000, error);
            break;
        }case 5:{
            duty = mmpsu_read_field(fd, PHASE_F_DUTY_CYCLE, 50000, error);
            break;
        }default:{
            duty = 0;
            break;
        }
    }
    return 100.0*duty/5440.0;
}


void mmpsu_set_developer_mode(int fd, bool on, MMPSUError& error){
    int value =(int)on;
    mmpsu_write_field(fd, VOUT_SETPOINT, value, 50000, error);
}


void mmpsu_set_voltage_kp(int fd, int k, MMPSUError& error){
    mmpsu_write_field(fd, VOLTAGE_KP, k, 50000, error);
}

void mmpsu_set_voltage_ki(int fd, int k, MMPSUError& error){
    mmpsu_write_field(fd, VOLTAGE_KI, k, 50000, error);
}

void mmpsu_set_current_kp(int fd, int k, MMPSUError& error){
    mmpsu_write_field(fd, CURRENT_KP, k, 50000, error);
}

void mmpsu_set_current_ki(int fd, int k, MMPSUError& error){
    mmpsu_write_field(fd, CURRENT_KI, k, 50000, error);
}

int mmpsu_get_state(int fd, MMPSUError& error){
    return mmpsu_read_field(fd, SYSTEM_STATE, 50000, error);
}

std::string mmpsu_get_state_str(int fd, MMPSUError& error){
    int state = mmpsu_read_field(fd, SYSTEM_STATE, 50000, error);
    return decode_state(state);
}


std::string decode_state(int state){
    switch(state){
        case ONE_PHASE_INIT:
            return "ONE_PHASE_INIT";
        case TWO_PHASE_INIT:
            return "TWO_PHASE_INIT";
        case THREE_PHASE_INIT:
            return "THREE_PHASE_INIT";
        case FOUR_PHASE_INIT:
            return "FOUR_PHASE_INIT";
        case FIVE_PHASE_INIT:
            return "FIVE_PHASE_INIT";
        case SIX_PHASE_INIT:
            return "SIX_PHASE_INIT";
        case ONE_PHASE:
            return "ONE_PHASE";
        case TWO_PHASE:
            return "TWO_PHASE";
        case THREE_PHASE:
            return "THREE_PHASE";
        case FOUR_PHASE:
            return "FOUR_PHASE";
        case FIVE_PHASE:
            return "FIVE_PHASE";
        case SIX_PHASE:
            return "SIX_PHASE";
        case OUTPUT_OFF:
            return "OUTPUT_OFF";
        case FAULT:
            return "FAULT";
        default:
            return "UNKNOWN";
    }
}


int mmpsu_get_i2c_error_count(int fd, MMPSUError& error){
    return mmpsu_read_field(fd, I2C_ERROR_COUNT, 50000, error);
}


void mmpsu_set_manual_mode(int fd, bool manual_mode, MMPSUError& error){
    int value = (manual_mode ? 1 : 0);
    mmpsu_write_field(fd, MANUAL_MODE, value, 50000, error);
}

void mmpsu_set_phase_count(int fd, int phase_count, MMPSUError& error){
    mmpsu_write_field(fd, PHASE_COUNT_REQUESTED, phase_count, 50000, error);
}

void mmpsu_set_balance_phase_current(int fd, bool balance, MMPSUError& error){
    int value = (balance ? 1 : 0);
    mmpsu_write_field(fd, BALANCE_PHASE_CURRENT, value, 50000, error);
}
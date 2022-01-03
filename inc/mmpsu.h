#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <cinttypes>
#include <vector>
#include <map>

#define MMPSU_CMD_NONE      0x00
#define MMPSU_CMD_READ      0x01
#define MMPSU_CMD_WRITE     0x02
#define MMPSU_CMD_READ_ALL  0x03

#define MMPSU_FIELD_SIZE    4

enum class MMPSUError {
  NONE,
  I2C_SLAVE_CONNECT_ERROR,
  I2C_WRITE_TIMEOUT,
  I2C_WRITE_ERROR,
  I2C_READ_TIMEOUT,
  I2C_READ_ERROR,
  I2C_ERROR
};

typedef enum {
  ONE_PHASE_INIT,
  TWO_PHASE_INIT,
  THREE_PHASE_INIT,
  FOUR_PHASE_INIT,
  FIVE_PHASE_INIT,
  SIX_PHASE_INIT,
  ONE_PHASE,
  TWO_PHASE,
  THREE_PHASE,
  FOUR_PHASE,
  FIVE_PHASE,
  SIX_PHASE,
  OUTPUT_OFF,
  FAULT
} Mode_TypeDef;


enum MMPSUMemMap {
  OUTPUT_ENABLED,
  VOUT_MEASURED,
  VOUT_SETPOINT,
  PHASES_PRESENT,
  PHASES_ENABLED,
  PHASE_A_DUTY_CYCLE,
  PHASE_A_CURRENT,
  PHASE_A_CURRENT_LIMIT,
  PHASE_B_DUTY_CYCLE,
  PHASE_B_CURRENT,
  PHASE_B_CURRENT_LIMIT,
  PHASE_C_DUTY_CYCLE,
  PHASE_C_CURRENT,
  PHASE_C_CURRENT_LIMIT,
  PHASE_D_DUTY_CYCLE,
  PHASE_D_CURRENT,
  PHASE_D_CURRENT_LIMIT,
  PHASE_E_DUTY_CYCLE,
  PHASE_E_CURRENT,
  PHASE_E_CURRENT_LIMIT,
  PHASE_F_DUTY_CYCLE,
  PHASE_F_CURRENT,
  PHASE_F_CURRENT_LIMIT,
  PHASES_IN_OVERTEMP,
  DEBUG_MODE,
  DEVELOPER_MODE,
  SYSTEM_STATE,
  VOLTAGE_KP,
  VOLTAGE_KI,
  CURRENT_KP,
  CURRENT_KI,
  I2C_ERROR_COUNT,
  MANUAL_MODE,
  PHASE_COUNT_REQUESTED,
  NUM_FIELDS
};


typedef void (*mmpsu_set_field_bool)(int fd, bool value, MMPSUError& err);
typedef void (*mmpsu_set_field_int)(int fd, int value, MMPSUError& err);
typedef void (*mmpsu_set_field_float)(int fd, float value, MMPSUError& err);

void start_i2c_connection(int fd, uint8_t addr, MMPSUError& error);

void close_i2c_connection(int fd);

bool mmpsu_test_connection(int fd, MMPSUError& error);

int mmpsu_read_field(int fd, int field, int timeout, MMPSUError& error);

void mmpsu_write_field(int fd, int field, int value, int timeout, MMPSUError& error);

void mmpsu_set_enabled(int fd, bool enabled, MMPSUError& error);

void mmpsu_set_vout(int fd, float vout, MMPSUError& error);

float mmpsu_get_vout(int fd, MMPSUError& error);

int mmpsu_get_phases_present(int fd, MMPSUError& error);

int mmpsu_get_phases_enabled(int fd, MMPSUError& error);

int mmpsu_get_phases_in_overtemp(int fd, MMPSUError& error);

float mmpsu_get_phase_current(int fd, int channel, MMPSUError& error);

float mmpsu_get_phase_duty_cycle(int fd, int channel, MMPSUError& error);

void mmpsu_set_developer_mode(int fd, bool on, MMPSUError& error);

void mmpsu_set_voltage_kp(int fd, int k, MMPSUError& error);

void mmpsu_set_voltage_ki(int fd, int k, MMPSUError& error);

void mmpsu_set_current_kp(int fd, int k, MMPSUError& error);

void mmpsu_set_current_ki(int fd, int k, MMPSUError& error);

int mmpsu_get_state(int fd, MMPSUError& error);

std::string mmpsu_get_state_str(int fd, MMPSUError& error);

std::string decode_state(int state);

int mmpsu_get_i2c_error_count(int fd, MMPSUError& error);

void mmpsu_set_manual_mode(int fd, bool manual_mode, MMPSUError& error);

void mmpsu_set_phase_count(int fd, int phase_count, MMPSUError& error);
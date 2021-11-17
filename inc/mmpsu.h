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
  I2C_READ_TIMEOUT,
  I2C_ERROR
};

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
  NUM_FIELDS
};

// class PhaseData {
// public:
//   PhaseData();
//   PhaseData(const PhaseData& ph);
//   bool present;
//   bool enabled;
//   bool overtemp;
//   float duty_cycle;
//   float current;
// };

// class MMPSUState {
// private:
//   std::map<int, PhaseData> m_phases;
// public:
//   bool connected;
//   bool enabled;
//   int phases_present;
//   int phases_enabled;
//   int phases_overtemp;
//   float vout;
//   float vout_setpt;
//   void update_phase_data(int ind, PhaseData phase);
//   PhaseData get_phase_data(int ind);
// };

void start_i2c_connection(int fd, uint8_t addr, MMPSUError& error);

void close_i2c_connection(int fd);

int mmpsu_read_field(int fd, int field, int timeout, MMPSUError& error);

void mmpsu_write_field(int fd, int field, int value, int timeout, MMPSUError& error);

void mmpsu_set_enabled(int fd, bool enabled, MMPSUError& error);

void mmpsu_set_vout(int fd, float vout, MMPSUError& error);

float mmpsu_get_vout(int fd, MMPSUError& error);

int mmpsu_get_phases_present(int fd, MMPSUError& error);

int mmpsu_get_phases_enabled(int fd, MMPSUError& error);

float mmpsu_get_phase_current(int fd, int channel, MMPSUError& error);

float mmpsu_get_phase_duty_cycle(int fd, int channel, MMPSUError& error);
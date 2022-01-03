#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <csignal>
#include <typeinfo>
#include <any>
#include "json.hpp"
#include "mmpsu.h"

using json = nlohmann::json;

char * data_out_path = "/tmp/mmpsu_data_out";
char * data_in_path = "/tmp/mmpsu_data_in";
char * debug_out_path = "/tmp/mmpsu_debug_out";
char * i2c_path = NULL;
char * debug_path = NULL;
int uart_fd = -1;

std::mutex done_mtx;
bool done =  false;

std::mutex mmpsu_state_mtx;
json mmpsu_state = {
    {"connected", false},
    {"update_time", 0.0},
    {"enabled", false},
    {"vout_setpt", 0.0},
    {"vout", 0.0},
    {"devel_mode", false},
    {"state", 0},
    {"state_str", "UNKNOWN"},
    {"voltage_kp", 1},
    {"voltage_ki", 1},
    {"current_kp", 1},
    {"current_ki", 1},
    {"i2c_error_count", 0},
    {"manual_mode", false},
    {"phase_count", 0}
};

/* map of fields to setter functions */
std::map<std::string, std::any> mmpsu_setters {
    {"enabled", std::any_cast<mmpsu_set_field_bool>(mmpsu_set_enabled)},
    {"vout_setpt", std::any_cast<mmpsu_set_field_float>(mmpsu_set_vout)},
    {"devel_mode", std::any_cast<mmpsu_set_field_bool>(mmpsu_set_developer_mode)},
    {"voltage_kp", std::any_cast<mmpsu_set_field_int>(mmpsu_set_voltage_kp)},
    {"voltage_ki", std::any_cast<mmpsu_set_field_int>(mmpsu_set_voltage_ki)},
    {"current_kp", std::any_cast<mmpsu_set_field_int>(mmpsu_set_current_kp)},
    {"current_ki", std::any_cast<mmpsu_set_field_int>(mmpsu_set_current_ki)},
    {"manual_mode", std::any_cast<mmpsu_set_field_bool>(mmpsu_set_manual_mode)},
    {"phase_count", std::any_cast<mmpsu_set_field_int>(mmpsu_set_phase_count)}
};

/* map of fields to setter function types */
std::map<std::string, std::size_t> mmpsu_setter_types {
    {"enabled", typeid(bool).hash_code()},
    {"vout_setpt", typeid(float).hash_code()},
    {"devel_mode", typeid(bool).hash_code()},
    {"voltage_kp", typeid(int).hash_code()},
    {"voltage_ki", typeid(int).hash_code()},
    {"current_kp", typeid(int).hash_code()},
    {"current_ki", typeid(int).hash_code()},
    {"manual_mode", typeid(bool).hash_code()},
    {"phase_count", typeid(int).hash_code()}
};

std::map<int, std::string> phase_names { {0,"a"}, {1,"b"}, {2,"c"}, {3,"d"}, {4,"e"}, {5,"f"}, };

std::mutex i2c_mutex;
int i2c_fd = -1;
uint8_t mmpsu_addr = 0x5A;
MMPSUError comm_err;


/**
 * @brief Attempt to close and recconect to the I2C character device
 * @param fname path to the I2C character device
 * @param fd reference to the file descriptor to close and reassign the new file descriptor to
 * @param addr slave address to connect to
 * @return true if reconnect was successful, false if it failed
 */
bool reconnect_i2c(char * fname, int& fd, uint8_t addr, MMPSUError& error){
    printf("Reconnecting I2C...\n");
    close_i2c_connection(fd);
    std::this_thread::sleep_for(std::chrono::milliseconds(25));  // brief delay
    if((fd = open(fname, O_RDWR)) < 0){
        return false;
    }
    start_i2c_connection(fd, addr, error);
    return (error == MMPSUError::NONE);
}

/**
 * @brief thread for reading debug info from MMPSU over UART
 */
void debug(){
    if(debug_path != NULL){
        std::fstream uart_stream(debug_path, std::ios::out | std::ios::in);
        umask(0);
        mknod(debug_out_path, S_IFIFO|0666, 0);
        std::ofstream debug_stream(debug_out_path, std::ios::out);

        std::string line_in;
        while(!done){
            std::getline(uart_stream, line_in);

            debug_stream << line_in << std::endl;
            debug_stream.flush();
        }
    }
}

/**
 * @brief Thread for listening to a named pipe and updating mmpsu accoringly
 */
void listener(){
    // mmpsu_setters["enabled"] = std::any_cast<mmpsu_set_field_bool>(mmpsu_set_enabled);

    umask(0);
    mknod(data_in_path, S_IFIFO|0666, 0);
    std::ifstream data_in_stream(data_in_path, std::ios::in);
    // data_in_stream.open(data_in_path, std::ios::in);
    std::string input;

    while(!done){
        if(!data_in_stream.good()){
            printf("Reopening input stream\n");
            data_in_stream.close();
            std::this_thread::sleep_for(std::chrono::seconds(1)); 
            umask(0);
            mknod(data_in_path, S_IFIFO|0666, 0);
            data_in_stream.open(data_in_path, std::ios::in);
            continue;
        }

        // check if characters are available in the stream
        std::getline(data_in_stream, input);
        try{
            auto obj = json::parse(input.c_str());

            // for each item in the incoming object
            for(auto& item : obj.items()){
                std::string key = item.key();

                if(mmpsu_state.contains(key)){
                
                    if(mmpsu_setters.find(key) != mmpsu_setters.end()){
                        // we have a setter for it
                        /* call the associated function */
                        try{
                            /* check each possible type, cast as necessary */
                            if(mmpsu_setter_types[key] == typeid(bool).hash_code()){
                                bool value = obj[key].get<bool>();
                                std::cout << "Setting " << key << " to " << value << std::endl;
                                i2c_mutex.lock();
                                std::any_cast<mmpsu_set_field_bool>(mmpsu_setters[key])(i2c_fd, value, comm_err);
                                i2c_mutex.unlock();
                                mmpsu_state_mtx.lock();
                                mmpsu_state[key] = value; // stash the value in the state object
                                mmpsu_state_mtx.unlock();
                            }else if(mmpsu_setter_types[key] == typeid(int).hash_code()){
                                int value = obj[key].get<int>();
                                std::cout << "Setting " << key << " to " << value << std::endl;
                                i2c_mutex.lock();
                                std::any_cast<mmpsu_set_field_int>(mmpsu_setters[key])(i2c_fd, value, comm_err);
                                i2c_mutex.unlock();
                                mmpsu_state_mtx.lock();
                                mmpsu_state[key] = value; // stash the value in the state object
                                mmpsu_state_mtx.unlock();
                            }else if(mmpsu_setter_types[key] == typeid(float).hash_code()){
                                float value = obj[key].get<float>();
                                std::cout << "Setting " << key << " to " << value << std::endl;
                                i2c_mutex.lock();
                                std::any_cast<mmpsu_set_field_float>(mmpsu_setters[key])(i2c_fd, value, comm_err);
                                i2c_mutex.unlock();
                                mmpsu_state_mtx.lock();
                                mmpsu_state[key] = value; // stash the value in the state object
                                mmpsu_state_mtx.unlock();
                            }else{
                                std::cout << "Type is not int, bool, or float ..." << std::endl;
                            }
                        }catch(const std::bad_any_cast& err) {
                            std::cout << "Bad cast occurred: " << err.what() << std::endl;
                        }
                    }else{
                        std::cout << "We don't have a setter function for " << key << std::endl;
                    }
                    
                    
                }else{
                    std::cout << "mmpsu_state doesn't contain " << key << std::endl;
                }

            } 
            /* end foreach key in obj */
        }catch(json::exception& err){
            printf("Error parsing from this string: %s\n", input.c_str());
        }

    }
    /* end while not done */
    printf("Listener thread is exiting...\n");
    data_in_stream.close();
}


int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Not enough arguments.\n");
        return -1;
    }
    
    i2c_path = argv[1]; // stash i2c device path
 
    bool mmpsu_connected = false;
    
    double start_time = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count()/1.0e9;

    std::thread listener_thread(listener);

    if(argc > 2){
        debug_path = argv[2];
    }
    std::thread debug_thread(debug);

    umask(0);
    mknod(data_out_path, S_IFIFO|0666, 0);
    std::ofstream data_out_stream(data_out_path, std::ios::out);

    while(!done){
        /* ==== BEGIN I2C & MMPSU THINGS ==== */
        i2c_mutex.lock();
        mmpsu_state_mtx.lock();

        /* if I2C is not started/connected, attempt to do so */
        if(i2c_fd < 0 || comm_err != MMPSUError::NONE){
            // MMPSU is not connected or some error occurred
            bool success = reconnect_i2c(i2c_path, i2c_fd, mmpsu_addr, comm_err);
            if(success){
                mmpsu_state["connected"] = mmpsu_test_connection(i2c_fd, comm_err);
            }else{
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                mmpsu_state["connected"] = false;
            }
        }else{
            /* MMPSU is connected
            ==== update the MMPSU state ==== */
            mmpsu_state["vout"] = mmpsu_get_vout(i2c_fd, comm_err); // read vout measured
            int phases_present = mmpsu_get_phases_present(i2c_fd, comm_err);
            int phases_enabled = mmpsu_get_phases_enabled(i2c_fd, comm_err);
            int phases_overtemp = mmpsu_get_phases_in_overtemp(i2c_fd, comm_err);
            mmpsu_state["state"] = mmpsu_get_state(i2c_fd, comm_err);
            mmpsu_state["state_str"] = decode_state(mmpsu_state["state"]);
            mmpsu_state["i2c_error_count"] = mmpsu_get_i2c_error_count(i2c_fd, comm_err);

            mmpsu_state["connected"] = comm_err == MMPSUError::NONE;

            if(!mmpsu_state.contains("phases")){
                mmpsu_state["phases"] = json::object();
            }
            for(int i = 0; i < 6; i++){
                std::string ch = phase_names.at(i);//std::to_string(i);
                if(!mmpsu_state["phases"].contains(ch)){
                    mmpsu_state["phases"][ch] = json::object();
                }
                
                if(mmpsu_state["phases"][ch]["present"] = (bool)((phases_present >> i) & 1)){
                    mmpsu_state["phases"][ch]["overtemp"] = (bool)((phases_overtemp >> i) & 1);
                    if(mmpsu_state["phases"][ch]["enabled"] = (bool)((phases_enabled >> i) & 1)){
                        // i2c_mutex is already locked
                        mmpsu_state["phases"][ch]["current"] = mmpsu_get_phase_current(i2c_fd, i, comm_err);
                        mmpsu_state["phases"][ch]["duty_cycle"] = mmpsu_get_phase_duty_cycle(i2c_fd, i, comm_err);
                    }
                }
            }
        }
        mmpsu_state_mtx.unlock();
        i2c_mutex.unlock();
        /* ==== END I2C & MMPSU THINGS ==== */
        
        /* ==== Check on the data out stream ==== */
        if(!data_out_stream.good()){
            printf("Reopening output stream...\n");
            data_out_stream.close();
            std::this_thread::sleep_for(std::chrono::seconds(100));
            umask(0);
            mknod(data_out_path, S_IFIFO|0666, 0);
            data_out_stream.open(data_out_path, std::ios::out);
        }else{
            /* === PRINT JSON TO STREAM === */
            mmpsu_state_mtx.lock();
            data_out_stream << mmpsu_state.dump() << std::endl;
            data_out_stream.flush();
            mmpsu_state_mtx.unlock();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    data_out_stream.close();
    close_i2c_connection(i2c_fd);

    listener_thread.join();

    printf("Main thread is exiting...\n");

    return 0;

}
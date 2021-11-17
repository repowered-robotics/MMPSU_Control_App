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
#include "json.hpp"
#include "mmpsu.h"

using json = nlohmann::json;

char * data_out_path = "/tmp/mmpsu_data_out";
char * data_in_path = "/tmp/mmpsu_data_in";
char * i2c_path;

std::mutex done_mtx;
bool done =  false;

std::mutex mmpsu_state_mtx;
json mmpsu_state = {
    {"connected", false},
    {"update_time", 0.0},
    {"enabled", false},
    {"vout_setpt", 0.0},
    {"vout", 0.0},
    {"phases", {}},
    {"errors", {}}
};

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
 * @brief Thread for listening to a named pipe and updating mmpsu accoringly
 */
void listener(){
    umask(0);
    mknod(data_in_path, S_IFIFO|0666, 0);
    std::ifstream data_in_stream;
    data_in_stream.open(data_in_path, std::ios::in);
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

        std::getline(data_in_stream, input);
        auto obj = json::parse(input.c_str());

        if(obj.contains("enabled")){
            if(obj["enabled"] != mmpsu_state["enabled"]){
                i2c_mutex.lock();
                // Do the I2C transaction to set enabled
                mmpsu_set_enabled(i2c_fd, obj["enabled"], comm_err);
                i2c_mutex.unlock();

                mmpsu_state_mtx.lock();
                mmpsu_state["enabled"] = obj["enabled"];
                mmpsu_state_mtx.unlock();
            }
        }
        if(obj.contains("vout_setpt")){
            if(obj["vout_setpt"] != mmpsu_state["vout_setpt"]){
                i2c_mutex.lock();
                // Do the I2C transaction to set vout
                mmpsu_set_vout(i2c_fd, obj["vout_setpt"], comm_err);
                i2c_mutex.unlock();

                mmpsu_state_mtx.lock();
                mmpsu_state["vout_setpt"] = obj["vout_setpt"];
                mmpsu_state_mtx.unlock();
            }
        }
    }
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

    umask(0);
    mknod(data_out_path, S_IFIFO|0666, 0);
    std::ofstream data_out_stream(data_out_path, std::ios::out);

    while(!done){
        /* ==== BEGIN I2C & MMPSU THINGS ==== */
        i2c_mutex.lock();
        mmpsu_state_mtx.lock();

        mmpsu_state["connected"] = comm_err == MMPSUError::NONE; // if any MMPSU error, connected -> false

        /* if I2C is not started/connected, attempt to do so */
        if(i2c_fd < 0 || !mmpsu_state["connected"] || comm_err != MMPSUError::NONE){
            // MMPSU is not connected or some error occurred
            bool success = reconnect_i2c(i2c_path, i2c_fd, mmpsu_addr, comm_err);
            /* delay conditionally (1 sec if we have an error) */
            std::this_thread::sleep_for(std::chrono::milliseconds( !success ? 1000 : 0) );
        }else{
            /* MMPSU is connected
            ==== update the MMPSU state and print to stream ==== */
            mmpsu_state_mtx.lock();
            mmpsu_state["vout"] = mmpsu_get_vout(i2c_fd, comm_err); // read vout measured
            int phases_present = mmpsu_get_phases_present(i2c_fd, comm_err);
            int phases_enabled = mmpsu_get_phases_enabled(i2c_fd, comm_err);
            for(int i = 0; i < 6; i++){
                std::string ch = std::to_string(i);
                if((phases_present >> i) & 1){
                    if(!mmpsu_state["phases"].contains(ch)){
                        mmpsu_state["phases"][ch] = R"({})"_json;
                    }
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
            std::this_thread::sleep_for(std::chrono::seconds(1));
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
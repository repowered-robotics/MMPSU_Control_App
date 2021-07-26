#! /bin/bash

# activate virtual environment
. venv/bin/activate

export FLASK_APP=mmpsu_ctl

flask run --host=0.0.0.0 
# FLASK_PID=$(echo $!)
# STDOUT_FILE_PATH=$(readlink -f /proc/$FLASK_PID/fd/1)

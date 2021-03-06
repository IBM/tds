#!/usr/bin/env python
# 
# Copyright 2018 IBM
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

import subprocess
import time
import os.path
import sys
from datetime import datetime
from sys import stdout, stderr


TIMEOUT = 120  # seconds


def log_msg(msg):
    timestamp = datetime.now()
    return '[' + str(timestamp) + ']\t' + msg + '\n'


if (len(sys.argv) != 2):
    print('Usage: ' + sys.argv[0] + ' <JSON config file>')
    sys.exit(-1)

launcher_output = open('launcher.out', 'a')

tds_id = 0

while (True):
        
    # Create subdirectory to keep all log and output files associated with this run
    sim_dir = time.strftime('run_y%Ym%md%d_h%Hm%Ms%S')
    if os.path.exists(sim_dir):
        shutil.rmtree(sim_dir)
    os.makedirs(sim_dir)

    tds_output = open(sim_dir + '/tds.out', 'wb')
    cmd = ['./tds', '-c', sys.argv[1], '-d', sim_dir, '-l', 'predictions.out', '-i', str(tds_id)]
    launcher_output.write(log_msg('Launching command ' + ' '.join(cmd)))
    launcher_output.flush()
    
    proc = subprocess.Popen(cmd, stdout=tds_output, stderr=tds_output)
    alive = True
    
    while (alive):

        open(sim_dir + '/alive','w')
        time.sleep(TIMEOUT)
        if (os.path.isfile(sim_dir + '/alive')):
            # The TDS program didn't remove the alive file.
            # This means that the TDS program hung or isn't
            # operating properly. We need to restart it.
            proc.terminate()
            alive = False
            tds_output.close()
            launcher_output.write(log_msg('TDS application did not respond for ' + str(TIMEOUT) + ' sec (restarting it)'))
            launcher_output.flush()
            tds_id += 1
        else:
            launcher_output.write(log_msg('TDS application is alive'))
            launcher_output.flush()

launcher_output.close()

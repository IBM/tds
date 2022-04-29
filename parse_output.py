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
from pprint import pprint


stats = {
    'runs':       0,
    'start':      '',
    'end':        '',
    'malformed':  0
}
preds = {
    'total': 0
}
first_one = True

if (len(sys.argv) != 2):
    print('Usage: ' + sys.argv[0] + ' <tds_output_dir>')
    sys.exit(-1)

consolidated_output = open('consolidated_output.csv', 'w')
consolidated_output.write('time,epoch_time,object_name,object_id,prob,cam_id,read_time_sec,conv_time_sec,pred_time_sec,bbox_time_sec\n')

path = sys.argv[1]
print('Parsing output in ' + path + '\n')

for directory in sorted(os.listdir(path)):
    
    # Process run_yXXXXmXXdXX_hYYmYYsYY directories one by one
    if directory.startswith('run_'):

        stats['runs'] += 1
            
        if (first_one):
            stats['start'] = directory[4:]
            first_one = False            
        stats['end'] = directory[4:]
        
        predictions_file = path + '/' + directory + '/' + 'predictions.log'
        if (os.path.isfile(predictions_file)):
            predictions = open(predictions_file, 'r')
            next(predictions)
            for prediction in predictions:
                # Format: cam_id,time,object_id,object_name,prob,read_time_sec,conv_time_sec,pred_time_sec,bbox_time_sec
                fields = prediction.strip().split(',')
                if (len(fields) != 9):
                    # WARNING: Malformed line!
                    stats['malformed'] += 1
                    continue
                consolidated_output.write(time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(int(fields[1]))) + ',' +
                                          fields[1] + ',' +
                                          fields[3] + ',' +
                                          fields[2] + ',' +
                                          fields[4] + ',' +
                                          fields[0] + ',' +
                                          fields[5] + ',' +
                                          fields[6] + ',' +
                                          fields[7] + ',' +
                                          fields[8] + '\n'
                                         )
                preds['total'] += 1
                if (not fields[3] in preds):
                    preds[fields[3]] = 1
                else:
                    preds[fields[3]] += 1
            predictions.close()
        
        #print(directory)
        
    #for file in files:
    #    print(file)

print('Number of runs:   ' + str(stats['runs']))
print('Start date/time:  ' + str(stats['start']))
print('End date/time:    ' + str(stats['end']))
print('Malformed lines:  ' + str(stats['malformed']))
print()

sort_preds = sorted(preds.items(), key=lambda x: x[1], reverse=True)
pprint(sort_preds)

consolidated_output.close()

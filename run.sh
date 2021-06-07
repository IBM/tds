#!/bin/bash
make clean
make 
./tds -c conf.json -l log.txt

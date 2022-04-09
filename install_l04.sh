#!/bin/bash

# Instructions from: http://www.let.rug.nl/kleiweg/L04/download.html

# Download and unpack
cd scripts
wget http://www.let.rug.nl/kleiweg/L04/L04-src.tar.gz
tar -xf L04-src.tar.gz
rm L04-src.tar.gz

# Install
cd RuG-L04/src
make

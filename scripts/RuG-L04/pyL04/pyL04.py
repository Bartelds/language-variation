#!/usr/bin/env python

print 'Starting pyL04...\n'

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(sys.argv[0])), 'lib'))

import app

app.run()

print '\n\npyL04 finished.\n'

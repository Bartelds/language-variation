#!/usr/bin/env python

from msg import _
import os
import sys

if sys.platform[:3] == 'win':
    target = 'leven.exe'
else:
    target = 'leven'

bindir = ''

p = os.path.realpath(sys.argv[0])
while not bindir:
    p1, p2 = os.path.split(p)
    if p == p1:
        break
    p = p1
    pb = os.path.join(p, 'bin')
    if sys.platform[:3] == 'win':
        pw = os.path.join(p, 'windows')
        pbw = os.path.join(pb, 'windows')
        pwb = os.path.join(pw, 'bin')
        pp = [p, pb, pw, pbw, pwb]
    else:
        pp = [p, pb]
    for d in pp:
        if os.path.isfile(os.path.join(d, target)):
            bindir = d
            break

if bindir:
    sys.stdout.write(_('Attaching directory "%s" at front of PATH\n\n') % bindir)
    os.environ['PATH'] = bindir + os.path.pathsep + os.environ['PATH']

if __name__ == '__main__':
    for i in os.environ['PATH'].split(os.path.pathsep):
        print i

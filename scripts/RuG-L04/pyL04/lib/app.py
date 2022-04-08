#!/usr/bin/env python

# first thing first: adjusting PATH
import bindir

import help
from menu import Z
from msg import _
import task

# in verband met bug:
import Tkinter
Tkinter.wantobjects = 0

from Tkinter import *
import tkFileDialog
import tkMessageBox
import tempfile
import os
import sys
import time
import re

# Threads cause 'gv' to hang with Python 2.3.4
# Python 2.4.1 seems to work fine
if sys.version_info[:3] >= (2, 4, 1):
    import thread
    draden = True
else:
    draden = False

draadteller = 0

if sys.platform[:3] == 'win':
    fixed = 'courier 10'
    make = 'nmake'
    def psview(target):
        os.system('start ' + target)

elif sys.platform[:6] == 'darwin':
    # UNTESTED
    fixed = 'courier 10'
    make = 'make'
    def psview(target):
        os.system('/Applications/Preview.app/Contents/MacOS/Preview ' + target)
        
else:
    fixed = 'courier 10'
    make = 'make'
    def psview(target):
        os.system('gv %s &' % target)

CANCEL = '_CANCEL_.L04'

pMI = '4m'  # inner margin
pMU = '2m'  # outer margin
pM2 = '1m'  # half of outer margin
pBW = '1m'  # border width

def shortPath(filename):
    here = os.getcwd()
    if sys.platform[:3] == 'win':
        filename = filename.replace('/', '\\')
    head, tail = os.path.split(filename)
    if head == here:
        return tail
    pre = os.path.commonprefix([here, filename])
    if pre == here:
        filename = filename.replace(pre, '', 1)
        if filename.startswith(os.path.sep):
            filename = filename.replace(os.path.sep, '', 1)
    return filename

def dialPosition(w):
    try:
        r = re.search(r'\+([0-9]+)\+([0-9]+)', root.winfo_geometry())
        x = int(r.group(1)) + 10
        y = int(r.group(2)) + 10
        return "+%i+%i" % (x, y)
    except:
        return '+300+200'

def stateAll(state1, state2):
    mFile.entryconfigure(3, state=state1)
    mFile.entryconfigure(4, state=state1)
    mFile.entryconfigure(5, state=state1)
    bProject.configure(state=state1)
    bAdd.configure(state=state1)

    if not draadteller:
        mTasks.entryconfigure(1, state=state2)
        mTasks.entryconfigure(2, state=state2)
        bView.configure(state=state2)

    bUp.configure(state=state2)
    bDown.configure(state=state2)
    bDel.configure(state=state2)
    bInfo.configure(state=state2)

def draadGestart():
    global draadteller
    draadteller = 1
    mFile.entryconfigure(1, state='disabled')
    mFile.entryconfigure(2, state='disabled')
    mFile.entryconfigure(6, state='disabled')
    mTasks.entryconfigure(1, state='disabled')
    mTasks.entryconfigure(2, state='disabled')
    mTasks.entryconfigure(3, state='normal')
    bView.configure(state='disabled')

def draadGestopt():
    global draadteller
    draadteller = 0
    mFile.entryconfigure(1, state='normal')
    mFile.entryconfigure(2, state='normal')
    mFile.entryconfigure(6, state='normal')
    mTasks.entryconfigure(1, state='normal')
    mTasks.entryconfigure(2, state='normal')
    mTasks.entryconfigure(3, state='disabled')
    bView.configure(state='normal')
    try:
        os.remove(CANCEL)
    except:
        pass

def noTaskSelected():
    if lst.curselection():
        return False
    tkMessageBox.showerror(_('Task action error'), _('No task selected'))
    return True

def comProject(event=None):
    print
    print _('Project file:'), projectfile
    print _('Map config file:'), configfile
    print _('Label file:'), labelfile
    print _('Working directory:'), os.getcwd()
    print

def comUp(event=None):
    global lst, tasks, changes
    if noTaskSelected():
        return
    idx = int(lst.curselection()[0])
    if idx > 0:
        tasks[idx - 1], tasks[idx] = tasks[idx], tasks[idx - 1]
        lst.delete(idx)
        idx -= 1
        lst.insert(idx, str(tasks[idx]))
        lst.select_set(idx)
        lst.activate(idx)
        lst.see(idx)
        changes = True

def comDown(event=None):
    global lst, tasks, changes
    if noTaskSelected():
        return
    idx = int(lst.curselection()[0])
    if idx < len(tasks) - 1:
        tasks[idx + 1], tasks[idx] = tasks[idx], tasks[idx + 1]
        lst.delete(idx)
        idx += 1
        lst.insert(idx, str(tasks[idx]))
        lst.select_set(idx)
        lst.activate(idx)
        lst.see(idx)
        changes = True

def addTaskFinal(p):
        global tasks, lst, changes
        askRemove(tasks[-1].makeTarget())
        if p >= 0 and p < len(tasks) - 2:
            lst.insert(p + 1, str(tasks[-1]))
            tasks = tasks[:p+1] + tasks[-1:] + tasks[p+1:-1]
        else:
            lst.insert(END, str(tasks[-1]))
        if task.Task.needRedraw:
            lst.delete(0, END)
            for a in tasks:
                lst.insert(END, str(a))
            task.Task.needRedraw = False
        if p >= 0:
            p = p + 1
        else:
            p = len(tasks) - 1
        lst.select_set(p)
        lst.activate(p)
        lst.see(p)

        if tasks:
            stateAll('normal', 'normal')
        changes = True

def addTaskData(p, tskname):
    global addTaskDataMessage

    T = _('Add input task error')

    f = tkFileDialog.askopenfilename(title=_('Select ONE data file for input'), filetypes=((_('all'), '*'),))
    if not f:
        return

    def fr1info():
        help.addTaskDataFilepattern1()

    def fr2info():
        help.addTaskDataFilepattern2()

    def infoCRAL():
        help.addTaskDataCRAL()

    def infof():
        help.addTaskDatainfof()

    def infoF():
        help.addTaskDatainfoF()

    def infoexpert():
        help.addTaskDataExp()

    def check():

        args = {}

        meth = method.get()
        if not meth:
            tkMessageBox.showerror(T, _('No method selected'))
            return
        args['type'] = meth

        pattern = pat.get()
        if not pattern:
            tkMessageBox.showerror(T, _('No pattern selected'))
            return
        args['data'] = pattern

        if cral.get():
            args['cral'] = '1'
        else:
            args['cral'] = '0'

        try:
            f = int(optf.get())
        except:
            f = 0
        if f < 1:
            tkMessageBox.showerror(T, _('Invalid value for minimum frequecy'))
            return
        args['minfreq'] = optf.get()

        if optF.get():
            args['skipnovars'] = '1'
        else:
            args['skipnovars'] = '0'

        args['expert'] = optexp.get()

        inp.grab_release()
        inp.destroy()

        if meth == 'leven':
            tasks.append(task.Leven(tskname, args))
        elif meth == 'giw':
            tasks.append(task.Giw(tskname, args))
        elif meth == 'bin':
            tasks.append(task.Binary(tskname, args))
        addTaskFinal(p)

    def cancel():
        inp.grab_release()
        inp.destroy()

    f = shortPath(f)
    d, f = os.path.split(f)
    r, e = os.path.splitext(f)

    inp = Toplevel()
    inp.title(_('Add dialect data: ') + tskname)

    Label(inp, text=_('Adding input task\nDialect data: ') + tskname, relief='groove', borderwidth=pBW, pady=pMI).pack(
        fill=X, padx=pMU, pady=pMU)

    frm = Frame(inp)
    frm.pack(side=TOP, fill=X, padx=pMU, pady='0m')

    frml = Frame(frm)
    frml.pack(side=LEFT, fill=BOTH, expand=True)

    fr = Frame(frml, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)
    Label(frr, text=_('Method:'), anchor=W).pack(side=TOP, fill=X)
    fr1 = Frame(frr)
    fr1.pack(side=TOP,fill=X)
    Button(fr1, text='?', command=fr1info).pack(side=LEFT,fill=Y)
    fr12 = Frame(fr1)
    fr12.pack(side=TOP,fill=X)
    method = StringVar()
    method.set('leven')
    Radiobutton(fr12, text=_('Levenshtein'),variable=method,value='leven').pack(anchor=NW)
    Radiobutton(fr12, text=_('Gewichteter Identit\xe4tswert'),variable=method,value='giw').pack(anchor=NW)
    Radiobutton(fr12, text=_('Binary differences'),variable=method,value='bin').pack(anchor=NW)

    Frame(frml).pack(anchor=N, pady=pM2, fill=X)

    fr = Frame(frml, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)
 
    frr = Frame(fr)
    frr.pack(fill=X, padx=pMI, pady=pMI)
    Label(frr, text=_('File pattern:'), anchor=W).pack(side=TOP, fill=X)
    fr2 = Frame(frr)
    fr2.pack(side=TOP,fill=X)
    qu2 = Button(fr2, text='?', command=fr2info)
    qu2.pack(side=LEFT,fill=Y)
    fr22 = Frame(fr2)
    fr22.pack(side=TOP,fill=X)
    fr221 = Frame(fr22)
    fr221.pack(side=TOP,fill=X)
    i = os.path.join(d, '*' + e)
    pat = StringVar()
    pat.set(i)
    Radiobutton(fr221, text=i,variable=pat,value=i).pack(anchor=NW)
    i = os.path.join(d, r + '.*')
    Radiobutton(fr221, text=i,variable=pat,value=i).pack(anchor=NW)
    i = os.path.join(d, f)
    Radiobutton(fr221, text=i,variable=pat,value=i).pack(anchor=NW)
    i = os.path.join(d, '*.*')
    Radiobutton(fr221, text=i,variable=pat,value=i).pack(anchor=NW)
    i = os.path.join(d, '*')
    Radiobutton(fr221, text=i,variable=pat,value=i).pack(anchor=NW)

    Frame(frm).pack(side=LEFT, padx=pM2)

    frmr = Frame(frm)
    frmr.pack(side=LEFT, fill=BOTH, expand=True)

    fr = Frame(frmr, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(fill=X, padx=pMI, pady=pMI)


    Label(frr, text=_('Options:'), anchor=W).pack(side=TOP,fill=X)

    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infof).pack(side=LEFT)
    Label(f, text=_('Minimum frequency:')).pack(side=LEFT, fill=X)
    optf = Entry(f, width=4)
    optf.insert(0, '1')
    optf.pack(side=LEFT, expand=YES, fill=X)

    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infoF).pack(side=LEFT)
    optF = IntVar()
    optF.set(0)
    Checkbutton(f, text=_('Skip single variant files'), variable=optF).pack(side=LEFT, fill=X)

    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infoCRAL).pack(side=LEFT)
    cral = IntVar()
    cral.set(0)
    Checkbutton(f, text=_('Cronbach Alpha'), variable=cral).pack(side=LEFT, fill=X)
    Frame(frmr).pack(pady=pM2)

    fr = Frame(frmr, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Expert options:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infoexpert).pack(side=LEFT)
    optexp = Entry(f, width=12)
    optexp.pack(side=LEFT, expand=YES, fill=X)

    f = Frame(inp)
    f.pack(padx=pMU, pady=pMU, fill=X)

    fr3 = Frame(f)
    fr3.pack(side=TOP,fill=X)
    ok = Button(fr3, text=_('OK'),command=check)
    ok.pack(side=LEFT)
    cancel = Button(fr3, text=_('Cancel'), command=cancel)
    cancel.pack(side=LEFT)

    inp.transient(root)
    inp.geometry(dialPosition(inp))
    inp.grab_set()
    inp.focus()

def addTaskDif(p, tskname):

    f = tkFileDialog.askopenfilename(title=_('Select existing differences'),
                                     filetypes=((_('difference files'), '*.dif'), (_('difference files'), '*.txt'), (_('all'), '*'),))
    if not f:
        return

    tasks.append(task.Dif(tskname, {
        'type'  : 'dif',
        'file'  : shortPath(f)}))
    addTaskFinal(p)

def addTaskDiffix(p, tskname):

    def check():

        i = lst.curselection()
        if not i:
            tkMessageBox.showerror(_('Add input task error'), _('No input task selected'))
            return
        uses = lst.get(i[0])

        pro.grab_release()
        pro.destroy()
        tasks.append(task.Diffix(tskname, {
            'type'  : 'diffix',
            'uses'  : uses,
            'coo'   : coofile,
            'aspect': aspect,
            'expert': ''}))
        addTaskFinal(p)

    def cancel():
        pro.grab_release()
        pro.destroy()

    pro = Toplevel()
    pro.title(_('Add incomplete differences: ') + tskname)

    Label(pro, text=_('Adding input task\nIncomplete differences: ') + tskname, relief='groove', borderwidth=pBW, pady=pMI).pack(
        fill=X,padx=pMU, pady=pMU)

    frm = Frame(pro)
    frm.pack(side=TOP, fill=X, padx=pMU, pady='0m')

    fr = Frame(frm, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Input:'), anchor=NW).pack(side=TOP,fill=X)
    bar = Scrollbar(frr)
    lst = Listbox(frr, width=30, height=4)
    bar.config(command=lst.yview)
    lst.config(yscrollcommand=bar.set)
    for i in tasks:
        if i.givesDiff():
            lst.insert(END, i.name)
    lst.select_set(0)
    bar.pack(side=RIGHT, fill=Y)
    lst.pack(side=LEFT, expand=YES, fill=X)

    f = Frame(pro)
    f.pack(padx=pMU, pady=pMU, fill=X)

    fr = Frame(f)
    fr.pack(side=TOP,fill=X)
    Button(fr, text=_('OK'),command=check).pack(side=LEFT)
    Button(fr, text=_('Cancel'), command=cancel).pack(side=LEFT)

    pro.transient(root)
    pro.geometry(dialPosition(pro))
    pro.grab_set()
    pro.focus()


def addTaskDen(p, tskname):
    T = _('Add output task error')

    def infomethod():
        help.addTaskDenMethod()

    def infonog():
        help.addTaskDenNog()

    def infolabels():
        help.addTaskDenLabels()

    def infocolor():
        help.addTaskDenColors()

    def infoexpert():
        help.addTaskDenExpert()

    def check():

        args = {}

        args['type'] = 'den'

        i = lst.curselection()
        if not i:
            tkMessageBox.showerror(T, _('No input task selected'))
            return
        args['uses'] = lst.get(i[0])

        args['method'] = mthd.get()

        n = 0
        try:
            n = int(nog.get())
        except:
            n = 0
        if n < 1:
            tkMessageBox.showerror(T, _('Invalid number of groups'))
            return
        args['groups'] = nog.get()

        args['labels'] = lbls.get()

        args['colors'] = clrs.get()

        args['expert'] = optexp.get()

        pro.grab_release()
        pro.destroy()

        tasks.append(task.Den(tskname, args))
        addTaskFinal(p)

    def cancel():
        pro.grab_release()
        pro.destroy()

    pro = Toplevel()
    pro.title(_('Add dendrogram: ') + tskname)

    Label(pro, text=_('Adding output task\nDendrogram: ') + tskname, relief='groove', borderwidth=pBW, pady=pMI).pack(
        fill=X, padx=pMU, pady=pMU)

    frm = Frame(pro)
    frm.pack(side=TOP, fill=X, padx=pMU, pady='0m')

    frml = Frame(frm)
    frml.pack(side=LEFT, fill=BOTH, expand=True)

    fr = Frame(frml, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)
    Label(frr, text=_('Input:'), anchor=NW).pack(side=TOP,fill=X)
    bar = Scrollbar(frr)
    lst = Listbox(frr, width=30, height=4)
    bar.config(command=lst.yview)
    lst.config(yscrollcommand=bar.set)
    for i in tasks:
        if i.givesDiff():
            lst.insert(END, i.name)
    lst.select_set(0)
    bar.pack(side=RIGHT, fill=Y)
    lst.pack(side=LEFT, expand=YES, fill=X)

    Frame(frml).pack(anchor=N, pady=pM2, fill=X)

    fr = Frame(frml, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)
 
    frr = Frame(fr)
    frr.pack(fill=X, padx=pMI, pady=pMI)
    Label(frr, text=_('Method:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infomethod).pack(side=LEFT,fill=Y)
    f2 = Frame(f)
    f2.pack(side=TOP,fill=X)
    mthd = StringVar()
    mthd.set('wm')
    Radiobutton(f2, text=_('Single Link (Nearest Neighbor)'),variable=mthd,value='sl').pack(anchor=NW)
    Radiobutton(f2, text=_('Complete Link'),variable=mthd, value='cl').pack(anchor=NW)
    Radiobutton(f2, text=_('Group Average'),variable=mthd, value='ga').pack(anchor=NW)
    Radiobutton(f2, text=_('Weighted Average'),variable=mthd, value='wa').pack(anchor=NW)
    Radiobutton(f2, text=_('Unweighted Centroid (Centroid)'),variable=mthd, value='uc').pack(anchor=NW)
    Radiobutton(f2, text=_('Weighted Centroid (Median)'),variable=mthd, value='wc').pack(anchor=NW)
    Radiobutton(f2, text=_("Ward's Method (Minimum Variance)"),variable=mthd, value='wm').pack(anchor=NW)

    Frame(frm).pack(side=LEFT, padx=pM2)

    frmr = Frame(frm)
    frmr.pack(side=LEFT, fill=BOTH, expand=True)

    fr = Frame(frmr, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Number of groups:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infonog).pack(side=LEFT)
    nog = Entry(f, width=4)
    nog.insert(0, '4')
    nog.pack(side=LEFT, expand=YES, fill=X)

    Frame(frmr).pack(side=TOP, pady=pM2)

    fr = Frame(frmr, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Labels:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infolabels).pack(side=LEFT,fill=Y)
    f2 = Frame(f)
    f2.pack(side=TOP,fill=X)
    lbls = StringVar()
    lbls.set('2')
    Radiobutton(f2, text=_('One column'),variable=lbls,value='1').pack(anchor=NW)
    Radiobutton(f2, text=_('Two columns'),variable=lbls,value='2').pack(anchor=NW)
    Radiobutton(f2, text=_('No labels'),variable=lbls,value='0').pack(anchor=NW)

    Frame(frmr).pack(side=TOP, pady=pM2)

    fr = Frame(frmr, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Colours:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infocolor).pack(side=LEFT,fill=Y)
    f2 = Frame(f)
    f2.pack(side=TOP,fill=X)
    clrs = StringVar()
    clrs.set('links')
    Radiobutton(f2, text=_('None'),variable=clrs,value='none').pack(anchor=NW)
    Radiobutton(f2, text=_('Links'),variable=clrs,value='links').pack(anchor=NW)
    Radiobutton(f2, text=_('Labels'),variable=clrs,value='labels').pack(anchor=NW)
    Radiobutton(f2, text=_('Links and labels'),variable=clrs,value='both').pack(anchor=NW)

    Frame(frmr).pack(side=TOP, pady=pM2)

    fr = Frame(frmr, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Expert options:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infoexpert).pack(side=LEFT)
    optexp = Entry(f, width=20)
    optexp.pack(side=LEFT, expand=YES, fill=X)

    f = Frame(pro)
    f.pack(padx=pMU, pady=pMU, fill=X)

    fr3 = Frame(f)
    fr3.pack(side=TOP,fill=X)
    ok = Button(fr3, text=_('OK'),command=check)
    ok.pack(side=LEFT)
    cancel = Button(fr3, text=_('Cancel'), command=cancel)
    cancel.pack(side=LEFT)

    pro.transient(root)
    pro.geometry(dialPosition(pro))
    pro.grab_set()
    pro.focus()


def addTaskCCC(p, tskname):

    def check():

        i = lst.curselection()
        if not i:
            tkMessageBox.showerror(_('Add output task error'), ('No input task selected'))
            return
        uses = lst.get(i[0])

        pro.grab_release()
        pro.destroy()
        tasks.append(task.MapCcc(tskname, {
            'type': 'mapccc',
            'uses': uses,
            'expert': ''}))
        addTaskFinal(p)

    def cancel():
        pro.grab_release()
        pro.destroy()

    pro = Toplevel()
    pro.title(_('Add CCC map: ') + tskname)

    Label(pro, text=_('Adding output task\nCCC map: ') + tskname, relief='groove', borderwidth=pBW, pady=pMI).pack(
        fill=X,padx=pMU, pady=pMU)

    frm = Frame(pro)
    frm.pack(side=TOP, fill=X, padx=pMU, pady='0m')

    fr = Frame(frm, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Input:'), anchor=NW).pack(side=TOP,fill=X)
    bar = Scrollbar(frr)
    lst = Listbox(frr, width=30, height=4)
    bar.config(command=lst.yview)
    lst.config(yscrollcommand=bar.set)
    for i in tasks:
        if i.givesDiff():
            lst.insert(END, i.name)
    lst.select_set(0)
    bar.pack(side=RIGHT, fill=Y)
    lst.pack(side=LEFT, expand=YES, fill=X)

    f = Frame(pro)
    f.pack(padx=pMU, pady=pMU, fill=X)

    fr = Frame(f)
    fr.pack(side=TOP,fill=X)
    Button(fr, text=_('OK'),command=check).pack(side=LEFT)
    Button(fr, text=_('Cancel'), command=cancel).pack(side=LEFT)

    pro.transient(root)
    pro.geometry(dialPosition(pro))
    pro.grab_set()
    pro.focus()

def addTaskMDS(p, tskname):
    T = _('Add output task error')

    def infomethod():
        help.addTaskMDSMethod()

    def infoexpertmds():
        help.addTaskMDSExpertMds()

    def infoexpertmap():
        help.addTaskMDSExpertMap()

    def check():

        args = {}

        args['type'] = 'mapmds'

        i = lst.curselection()
        if not i:
            tkMessageBox.showerror(T, _('No input task selected'))
            return
        args['uses'] = lst.get(i[0])

        args['method'] = mthd.get()

        args['expertmds'] = optexpmds.get()

        args['expertmap'] = optexpmap.get()

        pro.grab_release()
        pro.destroy()

        tasks.append(task.MapMds(tskname, args))
        addTaskFinal(p)

    def cancel():
        pro.grab_release()
        pro.destroy()

    pro = Toplevel()
    pro.title(_('Add MDS map: ') + tskname)

    Label(pro, text=_('Adding output task\nMDS map: ') + tskname, relief='groove', borderwidth=pBW, pady=pMI).pack(
        fill=X,padx=pMU, pady=pMU)

    frm = Frame(pro)
    frm.pack(side=TOP, fill=X, padx=pMU, pady='0m')

    frml = Frame(frm)
    frml.pack(side=LEFT, fill=BOTH, expand=True)

    fr = Frame(frml, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)
    Label(frr, text=_('Input:'), anchor=NW).pack(side=TOP,fill=X)
    bar = Scrollbar(frr)
    lst = Listbox(frr, width=30, height=4)
    bar.config(command=lst.yview)
    lst.config(yscrollcommand=bar.set)
    for i in tasks:
        if i.givesDiff():
            lst.insert(END, i.name)
    lst.select_set(0)
    bar.pack(side=RIGHT, fill=Y)
    lst.pack(side=LEFT, expand=YES, fill=X)

    Frame(frml).pack(anchor=N, pady=pM2, fill=X)

    fr = Frame(frml, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Method:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infomethod).pack(side=LEFT,fill=Y)
    f2 = Frame(f)
    f2.pack(side=TOP,fill=X)
    mthd = StringVar()
    mthd.set('standard')
    Radiobutton(f2, text=_('Classical'),variable=mthd,value='standard').pack(anchor=NW)
    Radiobutton(f2, text=_("Kruskal's method"),variable=mthd, value='kruskal').pack(anchor=NW)
    Radiobutton(f2, text=_('Sammon mapping'),variable=mthd, value='sammon').pack(anchor=NW)

    Frame(frm).pack(side=LEFT, padx=pM2)

    frmr = Frame(frm)
    frmr.pack(side=LEFT, fill=BOTH, expand=True)

    fr = Frame(frmr, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Expert options:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infoexpertmds).pack(side=LEFT)
    Label(f, text=_('MDS:')).pack(side=LEFT)
    optexpmds = Entry(f, width=12)
    optexpmds.pack(side=LEFT, expand=YES, fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infoexpertmap).pack(side=LEFT)
    Label(f, text=_('Map:')).pack(side=LEFT)
    optexpmap = Entry(f, width=12)
    optexpmap.pack(side=LEFT, expand=YES, fill=X)

    f = Frame(pro)
    f.pack(padx=pMU, pady=pMU, fill=X)

    fr = Frame(f)
    fr.pack(side=TOP,fill=X)
    ok = Button(fr, text=_('OK'),command=check)
    ok.pack(side=LEFT)
    cancel = Button(fr, text=_('Cancel'), command=cancel)
    cancel.pack(side=LEFT)

    pro.transient(root)
    pro.geometry(dialPosition(pro))
    pro.grab_set()
    pro.focus()

def addTaskCLU(p, tskname):
    T = _('Add output task error')

    def infomethod():
        help.addTaskCluMethod()

    def infonog():
        help.addTaskCluNog()

    def infoexpert():
        help.addTaskCluExpert()

    def check():

        args = {}

        args['type'] = 'mapclust'

        i = lst.curselection()
        if not i:
            tkMessageBox.showerror(T, _('No input task selected'))
            return
        args['uses'] = lst.get(i[0])

        args['method'] = mthd.get()

        n = 0
        try:
            n = int(nog.get())
        except:
            n = 0
        if n < 2:
            tkMessageBox.showerror(T, _('Invalid number of groups'))
            return
        args['groups'] = nog.get()

        args['expert'] = optexp.get()

        pro.grab_release()
        pro.destroy()

        tasks.append(task.MapClust(tskname, args))
        addTaskFinal(p)

    def cancel():
        pro.grab_release()
        pro.destroy()

    pro = Toplevel()
    pro.title(_('Add cluster map: ') + tskname)

    Label(pro, text=_('Adding output task\nCluster map: ') + tskname, relief='groove', borderwidth=pBW, pady=pMI).pack(
        fill=X,padx=pMU, pady=pMU)

    frm = Frame(pro)
    frm.pack(side=TOP, fill=X, padx=pMU, pady='0m')

    frml = Frame(frm)
    frml.pack(side=LEFT, fill=BOTH, expand=True)

    fr = Frame(frml, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)
    Label(frr, text=_('Input:'), anchor=NW).pack(side=TOP,fill=X)
    bar = Scrollbar(frr)
    lst = Listbox(frr, width=30, height=4)
    bar.config(command=lst.yview)
    lst.config(yscrollcommand=bar.set)
    for i in tasks:
        if i.givesDiff():
            lst.insert(END, i.name)
    lst.select_set(0)
    bar.pack(side=RIGHT, fill=Y)
    lst.pack(side=LEFT, expand=YES, fill=X)

    Frame(frml).pack(anchor=N, pady=pM2, fill=X)

    fr = Frame(frml, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Method:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infomethod).pack(side=LEFT,fill=Y)
    f2 = Frame(f)
    f2.pack(side=TOP,fill=X)
    mthd = StringVar()
    mthd.set('wm')
    Radiobutton(f2, text=_('Single Link (Nearest Neighbor)'),variable=mthd,value='sl').pack(anchor=NW)
    Radiobutton(f2, text=_('Complete Link'),variable=mthd, value='cl').pack(anchor=NW)
    Radiobutton(f2, text=_('Group Average'),variable=mthd, value='ga').pack(anchor=NW)
    Radiobutton(f2, text=_('Weighted Average'),variable=mthd, value='wa').pack(anchor=NW)
    Radiobutton(f2, text=_('Unweighted Centroid (Centroid)'),variable=mthd, value='uc').pack(anchor=NW)
    Radiobutton(f2, text=_('Weighted Centroid (Median)'),variable=mthd, value='wc').pack(anchor=NW)
    Radiobutton(f2, text=_("Ward's Method (Minimum Variance)"),variable=mthd, value='wm').pack(anchor=NW)


    Frame(frm).pack(side=LEFT, padx=pM2)

    frmr = Frame(frm)
    frmr.pack(side=LEFT, fill=BOTH, expand=True)

    fr = Frame(frmr, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Number of groups:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infonog).pack(side=LEFT)
    nog = Entry(f, width=4)
    nog.insert(0, '4')
    nog.pack(side=LEFT, expand=YES, fill=X)

    Frame(frmr).pack(anchor=N, pady=pM2, fill=X)

    fr = Frame(frmr, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)
    Label(frr, text=_('Expert options:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infoexpert).pack(side=LEFT)
    optexp = Entry(f, width=12)
    optexp.pack(side=LEFT, expand=YES, fill=X)


    f = Frame(pro)
    f.pack(padx=pMU, pady=pMU, fill=X)

    fr = Frame(f)
    fr.pack(side=TOP,fill=X)
    ok = Button(fr, text=_('OK'),command=check)
    ok.pack(side=LEFT)
    cancel = Button(fr, text=_('Cancel'), command=cancel)
    cancel.pack(side=LEFT)

    pro.transient(root)
    pro.geometry(dialPosition(pro))
    pro.grab_set()
    pro.focus()

def addTaskVEC(p, tskname):
    T = _('Add output task error')

    def infonsize():
        help.addTaskVecNsize()

    def infoexpert():
        help.addTaskVecExpert()

    def check():

        args = {}

        args['type'] = 'mapvec'

        i = lst.curselection()
        if not i:
            tkMessageBox.showerror(T, _('No input task selected'))
            return
        args['uses'] = lst.get(i[0])

        try:
            n = float(nsize.get())
        except:
            tkMessageBox.showerror(T, _('Missing value for neighbourhood size'))
            return

        if n <= 0.0 or n > 1.0:
            tkMessageBox.showerror(T, _('Invalid value for neighbourhood size'))
            return

        args['nsize'] = n

        args['expert'] = optexp.get()

        pro.grab_release()
        pro.destroy()

        tasks.append(task.MapVec(tskname, args))
        addTaskFinal(p)

    def cancel():
        pro.grab_release()
        pro.destroy()

    pro = Toplevel()
    pro.title(_('Add vector map: ') + tskname)

    Label(pro, text=_('Adding output task\nVector map: ') + tskname, relief='groove', borderwidth=pBW, pady=pMI).pack(
        fill=X,padx=pMU, pady=pMU)

    frm = Frame(pro)
    frm.pack(side=TOP, fill=X, padx=pMU, pady='0m')

    fr = Frame(frm, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)
    Label(frr, text=_('Input:'), anchor=NW).pack(side=TOP,fill=X)
    bar = Scrollbar(frr)
    lst = Listbox(frr, width=30, height=4)
    bar.config(command=lst.yview)
    lst.config(yscrollcommand=bar.set)
    for i in tasks:
        if i.givesDiff():
            lst.insert(END, i.name)
    lst.select_set(0)
    bar.pack(side=RIGHT, fill=Y)
    lst.pack(side=LEFT, expand=YES, fill=X)

    Frame(frm).pack(anchor=N, pady=pM2, fill=X)

    fr = Frame(frm, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)

    Label(frr, text=_('Neighbourhood size:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infonsize).pack(side=LEFT)
    nsize = Entry(f, width=4)
    nsize.insert(0, '0.125')
    nsize.pack(side=LEFT, expand=YES, fill=X)

    Frame(frm).pack(anchor=N, pady=pM2, fill=X)

    fr = Frame(frm, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)
    Label(frr, text=_('Expert options:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(frr)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infoexpert).pack(side=LEFT)
    optexp = Entry(f, width=12)
    optexp.pack(side=LEFT, expand=YES, fill=X)


    f = Frame(pro)
    f.pack(padx=pMU, pady=pMU, fill=X)

    fr = Frame(f)
    fr.pack(side=TOP,fill=X)
    ok = Button(fr, text=_('OK'),command=check)
    ok.pack(side=LEFT)
    cancel = Button(fr, text=_('Cancel'), command=cancel)
    cancel.pack(side=LEFT)

    pro.transient(root)
    pro.geometry(dialPosition(pro))
    pro.grab_set()
    pro.focus()

def comAdd(event=None):
    T = _('Add task error')
    if not projectfile:
        tkMessageBox.showerror(T, _("No project in use. Choose 'New' or 'Open' from the File menu first."))
        return

    pp = lst.curselection()
    if pp:
        p = int(lst.curselection()[0])
    else:
        p = -1

    def fr1info():
        help.addTask1()

    def fr2info():
        help.addTask2()

    def fr3info():
        help.addTask3()

    def check():
        tskname = ent.get().strip()
        tsktype = key.get()
        if not tskname:
            tkMessageBox.showerror(T, _('No task name given'))
            return
        if tskname == 'L04':
            tkMessageBox.showerror(T, _("Task name 'L04' reserved for internal use"))
            return
        if not re.search(r'^[a-zA-Z0-9_][a-zA-Z0-9_-]*$', tskname):
            tkMessageBox.showerror(T, _('Invalid characters in task name'))
            return
        for i in tasks:
            if i.name == tskname:
                tkMessageBox.showerror(T, _('Task %s already in use') % tskname)
                return
        if not tsktype:
            tkMessageBox.showerror(T, _('No task type selected'))
            return
        if tsktype != 'data' and tsktype != 'dif':
            found = False
            for i in tasks:
                if i.givesDiff():
                    found = True
                    break
            if not found:
                tkMessageBox.showerror(T, _('Select an input task first'))
                return

        tsk.grab_release()
        tsk.destroy()

        if tsktype == 'data':
            addTaskData(p, tskname)
        elif tsktype == 'dif':
            addTaskDif(p, tskname)
        elif tsktype == 'diffix':
            addTaskDiffix(p, tskname)
        elif tsktype == 'den':
            addTaskDen(p, tskname)
        elif tsktype == 'mapccc':
            addTaskCCC(p, tskname)
        elif tsktype == 'mapmds':
            addTaskMDS(p, tskname)
        elif tsktype == 'mapclust':
            addTaskCLU(p, tskname)
        elif tsktype == 'mapvec':
            addTaskVEC(p, tskname)

    def cancel():
        tsk.grab_release()
        tsk.destroy()

    tsk = Toplevel()
    tsk.title(_('Add task'))

    Label(tsk, text=_('Add task'), relief='groove', borderwidth=pBW, pady=pMI).pack(
        fill=X,padx=pMU, pady=pMU)
    
    frm = Frame(tsk)
    frm.pack(side=TOP, fill=X, padx=pMU, pady='0m')

    fr = Frame(frm, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)

    frr = Frame(fr)
    frr.pack(side=TOP, fill=X, padx=pMI, pady=pMI)

    lab1 = Label(frr, text=_('Task name:'), anchor=W)
    lab1.pack(side=TOP, fill=X)
    fr1 = Frame(frr)
    fr1.pack(side=TOP,fill=X)
    qu1 = Button(fr1, text='?', command=fr1info)
    qu1.pack(side=LEFT)
    ent = Entry(fr1)
    ent.pack(side=LEFT, expand=YES, fill=X)

    Frame(frm).pack(anchor=N, pady=pM2)

    fr = Frame(frm, relief='groove', borderwidth=pBW)
    fr.pack(side=TOP, fill=X)
 
    frr = Frame(fr)
    frr.pack(fill=X, padx=pMI, pady=pMI)

    lab2 = Label(frr, text=_('Task type:'), anchor=W)
    lab2.pack(side=TOP,fill=X)

    if tasks:
        state = 'normal'
    else:
        state = 'disabled'

    key = StringVar()

    fr2 = Frame(frr)
    fr2.pack(side=TOP,fill=X)
    qu2 = Button(fr2, text='?', command=fr2info)
    qu2.pack(side=LEFT,fill=Y)
    fr22 = Frame(fr2)
    fr22.pack(side=TOP,fill=X)
    fr221 = Frame(fr22)
    fr221.pack(side=TOP,fill=X)
    Radiobutton(fr221, text=_("Input: dialect data"), variable=key, value='data').pack(anchor=NW)
    Radiobutton(fr221, text=_("Input: existing differences"), variable=key, value='dif').pack(anchor=NW)
    Radiobutton(fr221, text=_("Input: incomplete differences"), variable=key, value='diffix', state=state).pack(anchor=NW)

    fr3 = Frame(frr)
    fr3.pack(side=TOP,fill=X)
    qu3 = Button(fr3, text='?', command=fr3info)
    qu3.pack(side=LEFT,fill=Y)
    fr32 = Frame(fr3)
    fr32.pack(side=TOP,fill=X)
    fr321 = Frame(fr32)
    fr321.pack(side=TOP,fill=X)
    Radiobutton(fr321, text=_("Output: dendrogram"), variable=key, value='den', state=state).pack(anchor=NW)
    Radiobutton(fr321, text=_("Output: CCC map"), variable=key, value='mapccc', state=state).pack(anchor=NW)
    Radiobutton(fr321, text=_("Output: MDS map"), variable=key, value='mapmds', state=state).pack(anchor=NW)
    Radiobutton(fr321, text=_("Output: cluster map"), variable=key, value='mapclust', state=state).pack(anchor=NW)
    Radiobutton(fr321, text=_("Output: vector map"), variable=key, value='mapvec', state=state).pack(anchor=NW)

    f = Frame(tsk)
    f.pack(padx=pMU, pady=pMU, fill=X)

    fr3 = Frame(f)
    fr3.pack(side=TOP,fill=X)
    ok = Button(fr3, text=_('OK'),command=check)
    ok.pack(side=LEFT)
    cancel = Button(fr3, text=_('Cancel'), command=cancel)
    cancel.pack(side=LEFT)

    tsk.transient(root)
    tsk.geometry(dialPosition(tsk))
    tsk.grab_set()
    tsk.focus()

def comDel(event=None):
    T = _('Remove task error')
    global lst, tasks, changes
    if noTaskSelected():
        return
    idx = int(lst.curselection()[0])
    for t in tasks:
        if t.dct.has_key('uses') and t.dct['uses'] == tasks[idx].name:
            tkMessageBox.showerror(T, _('This task is required for task "%s"') % t.name)
            return
    if not tkMessageBox.askyesno(_('Remove task'), _('Remove task "%s"?') % tasks[idx].name):
        return
    if tasks[idx].makeTarget() or tasks[idx].makeIntermedTargets():
        if tkMessageBox.askyesno(_('Remove task'), _('Remove files created by this task?')):
            targets = tasks[idx].makeIntermedTargets()
            targets.append(tasks[idx].makeTarget())
            removeFiles(targets)
    tasks[idx].cleanup()
    tasks.pop(idx)
    lst.delete(idx)
    if not tasks:
        stateAll('normal', 'disabled')
    changes = True

def doViewInput(target, notarget):
    draadGestart()
    if target:
        runMake(target)
    else:
        target = notarget
        print ''
    try:
        f = open(target, 'r')
        for i in f:
            if i[0] == '#':
                print i.rstrip()
            else:
                break
        f.close()
    except:
        pass
    if os.access(target, os.F_OK):
        print _('\nLocal incoherence of'), target, ':'
        os.system('linc -a %s %s %s' % (aspect, target, coofile))
    draadGestopt()
    if draden:
        thread.exit_thread()

def doViewOutput(target):
    draadGestart()
    runMake(target)
    draadGestopt()
    if os.access(target, os.F_OK):
        psview(target)
    if draden:
        thread.exit_thread()

def comView(event=None):
    if noTaskSelected():
        return
    if draadteller:
        tkMessageBox.showerror(_('Run make'), _('Make is still running'))
        return
    idx = int(lst.curselection()[0])
    s = tasks[idx].makeTarget()
    if tasks[idx].givesDiff():
        s2 = tasks[idx].makeNoTarget()
        if draden:
            thread.start_new(doViewInput, (s, s2))
        else:
            doViewInput(s, s2)
    else:
        if draden:
            thread.start_new(doViewOutput, (s,))
        else:
            doViewOutput(s)

def comInfo(event=None):
    if noTaskSelected():
        return
    idx = int(lst.curselection()[0])
    print tasks[idx].info()

def clearTasks():
    global tasks, configfile, coofile, aspect, labelfile, numlocations, mapdeps, projectfile, changes
    tasks = []
    configfile = ''
    mapdeps = ''
    coofile = ''
    aspect = '1'
    labelfile = ''
    numlocations = 0
    projectfile = ''
    changes = False
    task.Task.resetGlobals()

def loadProject(filepath):
    global tasks, configfile, coofile, aspect, labelfile, numlocations, mapdeps, projectfile, lst
    lst.delete(0, END)
    clearTasks()

    path, filename = os.path.split(filepath)
    if path:
        os.chdir(path)  # works on Windows? need to change drive if necessary?

    tasks, configfile, labelfile, coofile, aspect, numlocations, mapdeps = task.readProject(filename)
    for a in tasks:
        lst.insert(END, str(a))
    task.Task.needRedraw = False
    projectfile = filename
    root.title('pyL04 - ' + projectfile)
    if tasks:
        stateAll('normal', 'normal')
    else:
        stateAll('normal', 'disabled')
    print
    print _('Existing project loaded in directory:'), os.getcwd()
    print _('Project file:'), projectfile
    print _('Map config file:'), configfile
    print _('Label file:'), labelfile
    print

def newProject():
    T = _('New project error')
    if changes:
        if not tkMessageBox.askyesno(_('New project'), _('Current project is not saved. Ignore changes and create new project?')):
            return

    def fr1info():
        help.newProject1()
    def fr2info():
        help.newProject2()
    def infolabel():
        help.newProject3()
    def browse():
        f = tkFileDialog.askopenfilename(title=_('Select map configuration file'),
                                         filetypes=((_("map config files"),"*.cfg"), (_("all"),"*")))
        if f:
            ent2.delete(0,END)
            ent2.insert(0,f)
    def browse2():
        f = tkFileDialog.askopenfilename(title=_('Select label file'),
                                         filetypes=((_("label files"),"*.lbl"), (_("all"),"*")))
        if f:
            ent3.delete(0,END)
            ent3.insert(0,f)
    def check():
        global configfile, coofile, aspect, labelfile, numlocations, projectfile, changes
        proname = ent1.get().strip()
        if not proname:
            tkMessageBox.showerror(T, _('No project name given'))
            return
        cfg = ent2.get().strip()
        if not cfg:
            tkMessageBox.showerror(T, _('No map configuration file given'))
            return
        dirname, filename = os.path.split(cfg)
        r, e = os.path.splitext(proname)
        if not e:
            proname += '.ini'
        profile = os.path.join(dirname, proname)
        if os.path.exists(profile):
            if not tkMessageBox.askyesno(_('New project'), _('Project file "%s" exist. Overwrite?') % profile):
                return
        try:
            f = open(cfg, 'r')
        except IOError, info:
            tkMessageBox.showerror(T, _('Opening file "%s": %s') % (cfg, info))
            return
        lbl = ent3.get()
        coo = ''
        asp = ''
        for line in f:
            w = line.split()
            if w and w[0] == 'labels:' and not lbl:
                lbl = w[1]
            elif w and w[0] == 'labels' and w[1] == ':' and not lbl:
                lbl = w[2]
            elif w and w[0] == 'coordinates:':
                coo = w[1]
            elif w and w[0] == 'coordinates' and w[1] == ':':
                coo = w[2]
            elif w and w[0] == 'aspect:':
                asp = w[1]
            elif w and w[0] == 'aspect' and w[1] == ':':
                asp = w[2]
        f.close()
        if not lbl:
            tkMessageBox.showerror(T, _('No label file defined in file "%s"') % cfg)
            return
        if not coo:
            tkMessageBox.showerror(T, _('No coordinate file defined in file "%s"') % cfg)
            return
        if not asp:
            asp = '1'
        numloc = 0
        lblfile = os.path.join(dirname, lbl)
        try:
            f = open(lblfile, 'r')
        except IOError, info:
            tkMessageBox.showerror(T, _('Opening file "%s" from "%s": %s') % (lblfile, cfg, info))
            return
        for line in f:
            w = line.split()
            try:
                n = int(w[0])
            except IndexError:
                pass
            except ValueError:
                pass
            if n > numloc:
                numloc = n
        f.close()
        if numloc < 1:
            tkMessageBox.showerror(T, _('Invalid label file "%s"') % lblfile)
            return

        lst.delete(0, END)
        clearTasks()
        if dirname:
            os.chdir(dirname)
        configfile = filename
        coofile = coo
        aspect = asp
        labelfile = shortPath(lbl)
        numlocations = numloc
        projectfile = proname
        changes = True
        root.title('pyL04 - ' + projectfile)
        pro.grab_release()
        pro.destroy()
        stateAll('normal', 'disabled')
        print
        print _('New project started in directory:'), os.getcwd()
        print _('Project file:'), projectfile
        print _('Map config file:'), configfile
        print _('Label file:'), labelfile
        print

    def cancel():
        pro.grab_release()
        pro.destroy()

    pro = Toplevel()
    pro.title(_('New project'))

    Label(pro, text=_('New project'), relief='groove', borderwidth=pBW, pady=pMI).pack(
        fill=X, padx=pMU, pady=pMU)

    f = Frame(pro, relief='groove', borderwidth=pBW)
    f.pack(side=TOP, fill=X, padx=pMU, pady='0m')

    ff = Frame(f)
    ff.pack(side=TOP, fill=X, padx=pMI, pady=pMI)

    lab1 = Label(ff, text=_('Project name:'), anchor=W)
    lab1.pack(side=TOP, fill=X)
    fr1 = Frame(ff)
    fr1.pack(side=TOP,fill=X)
    qu1 = Button(fr1, text='?', command=fr1info)
    qu1.pack(side=LEFT)
    ent1 = Entry(fr1)
    ent1.insert(0, 'Project1.ini')
    ent1.pack(side=LEFT, expand=YES, fill=X)

    f = Frame(pro, relief='groove', borderwidth=pBW)
    f.pack(side=TOP, fill=X, padx=pMU, pady=pMU)

    ff = Frame(f)
    ff.pack(side=TOP, fill=X, padx=pMI, pady=pMI)

    lab2 = Label(ff, text=_('Map configuration file:'), anchor=W)
    lab2.pack(side=TOP,fill=X)
    fr2 = Frame(ff)
    fr2.pack(side=TOP,fill=X)
    qu2 = Button(fr2, text='?', command=fr2info)
    qu2.pack(side=LEFT)
    ent2 = Entry(fr2,width=60)
    ent2.pack(side=LEFT, expand=YES, fill=X)
    select = Button(fr2, text=_('Browse'), command=browse)
    select.pack(side=LEFT)

    f = Frame(pro, relief='groove', borderwidth=pBW)
    f.pack(side=TOP, fill=X, padx=pMU, pady='0m')

    ff = Frame(f)
    ff.pack(side=TOP, fill=X, padx=pMI, pady=pMI)

    Label(ff, text=_('OPTIONAL, label file:'), anchor=W).pack(side=TOP,fill=X)
    f = Frame(ff)
    f.pack(side=TOP,fill=X)
    Button(f, text='?', command=infolabel).pack(side=LEFT)
    ent3 = Entry(f,width=60)
    ent3.pack(side=LEFT, expand=YES, fill=X)
    select = Button(f, text=_('Browse'), command=browse2)
    select.pack(side=LEFT)

    f = Frame(pro)
    f.pack(padx=pMU, pady=pMU, fill=X)
    ok = Button(f, text=_('OK'),command=check)
    ok.pack(side=LEFT)
    cancel = Button(f, text=_('Cancel'), command=cancel)
    cancel.pack(side=LEFT)

    pro.transient(root)
    pro.geometry(dialPosition(pro))
    pro.grab_set()
    pro.focus()

def openProject():
    if changes:
        if not tkMessageBox.askyesno(_('Open project'), _('Current project is not saved. Ignore changes and load new project?')):
            return
    filepath = tkFileDialog.askopenfilename(title=_('Open project'),
                                            filetypes=((_("project files"),"*.ini"), (_("all"),"*")))
    if filepath:
        loadProject(filepath)

def saveProject():
    global changes
    if not changes:
        tkMessageBox.showinfo(_('Save project'), _('No changes, not saving'))
        return
    task.writeProject(projectfile, tasks, configfile, labelfile)
    changes = False

def saveAsProject():
    T = _('Save as... error')
    if not tasks:
        tkMessageBox.showerror(T, _('No tasks defined'))
        return

    def check():
        global projectfile, changes
        proname = ent.get().strip()
        if not proname:
            tkMessageBox.showerror(T, _('No project name given'))
            return
        if proname.find(' ') >= 0:
            tkMessageBox.showerror(T, _('No spaces allowed in project name'))
            return
        d, f = os.path.split(proname)
        if d:
            tkMessageBox.showerror(T, _('No path allowed in new project name'))
            return
        r, e = os.path.splitext(proname)
        if not e:
            proname += '.ini'

        if proname != projectfile:
            if os.path.exists(proname):
                if not tkMessageBox.askyesno(T, _('Project file "%s" exist. Overwrite?') % proname):
                    return
            projectfile = proname
            changes = True
            root.title('pyL04 - ' + projectfile)

        pro.grab_release()
        pro.destroy()

        if changes:
            saveProject()

    def cancel():
        pro.grab_release()
        pro.destroy()

    pro = Toplevel()
    pro.title(_('Save as'))

    Label(pro, text=_('Save as...'), relief='groove', borderwidth=pBW, pady=pMI).pack(
        fill=X, padx=pMU, pady=pMU)

    f = Frame(pro, relief='groove', borderwidth=pBW)
    f.pack(side=TOP, fill=X, padx=pMU, pady='0m')

    ff = Frame(f)
    ff.pack(side=TOP, fill=X, padx=pMI, pady=pMI)

    lab = Label(ff, text=_('New project name:'), anchor=W)
    lab.pack(side=TOP, fill=X)
    ent = Entry(ff)
    ent.insert(0, projectfile)
    ent.pack(side=TOP, expand=YES, fill=X)

    fr = Frame(pro)
    fr.pack(padx=pMU, pady=pMU, fill=X)
    ok = Button(fr, text=_('OK'),command=check)
    ok.pack(side=LEFT)
    cancel = Button(fr, text=_('Cancel'), command=cancel)
    cancel.pack(side=LEFT)

    pro.transient(root)
    pro.geometry(dialPosition(pro))
    pro.grab_set()
    pro.focus()

def saveMakefile():
    if not tasks:
        tkMessageBox.showerror(_('Save Makefile error'), _('No tasks defined'))
        return
    filename = tkFileDialog.asksaveasfilename(title=_('Write Makefile'),
                                              initialfile = 'Makefile',
                                              filetypes=((_("Makefiles"),"Makefile*"), (_("Makefiles"),"makefile*"), (_("all"),"*")))
    if filename:
        print _("Saving makefile as"), filename
        task.writeMakefile(filename, tasks, configfile, labelfile, numlocations, mapdeps)

def runMake(target):
    global tasks, configfile, labelfile, numlocations
    if not tasks:
        tkMessageBox.showerror(_('Run make error'), _('No tasks defined'))
        return
    f, name = tempfile.mkstemp()
    os.close(f)
    task.writeMakefile(name, tasks, configfile, labelfile, numlocations, mapdeps)
    c = '%s -f %s %s' % (make, name, target)
    print _('\nRunning:'), c
    os.system(c)
    print _('\nMake done\n')
    os.remove(name)

def runMakeAll():
    draadGestart()
    runMake('all')
    draadGestopt()
    if draden:
        thread.exit_thread()

def makeAll():
    if draadteller:
        tkMessageBox.showerror(_('Run make error'), _('Make is still running'))
        return
    if draden:
        thread.start_new(runMakeAll, ())
    else:
        runMakeAll()

def makeClean():
    if not tasks:
        tkMessageBox.showerror(_('Make clean error'), _('No tasks defined'))
        return
    if draadteller:
        tkMessageBox.showerror(_('Make clean error'), _('Make is still running'))
        return
    if tkMessageBox.askyesno(_('Make clean'), _('Remove all files created by Make?')):
        files = []
        for t in tasks:
            if t.makeTarget():
                files.append(t.makeTarget())
            for f in t.makeIntermedTargets():
                files.append(f)
        removeFiles(files)

def makeCancel():
    f = open(CANCEL, 'w')
    f.close()

def quit():
    if draadteller:
        if tkMessageBox.askyesno(_('Quit'), _('Make is still running. Abort make?')):
            makeCancel()
            while draadteller:
                time.sleep(1)
        else:
            return
        
    if not changes:
        root.quit()
    else:
        if tkMessageBox.askyesno(_('Quit'), _('Project is not saved. Ignore changes and quit?')):
            root.quit()

def askRemove(f):
     if os.path.exists(f):
         if tkMessageBox.askyesno(_('Remove file'), _('Remove old target file %s?') % f):
            removeFiles([f])

def removeFiles(files):
    total = 0
    for f in files:
        try:
            os.remove(f)
        except OSError:
            pass
        else:
            print _('Removed'), f
            total += 1
    if total == 1:
        print _('\n1 file removed\n')
    else:
        print _('\n%i files removed\n') % total

def about():
    help.about()

def run():
    global root, lst, mFile, mTasks, bProject, bUp, bDown, bAdd, bDel, bView, bInfo

    root = Tk()
    root.title('pyL04')

    top = Menu(root)
    root.config(menu=top)
    mFile = Menu(top)
    mFile.add_command(label=_('New'),              command=newProject,    underline=Z['New'])
    mFile.add_command(label=_('Open...'),          command=openProject,   underline=Z['Open...'])
    mFile.add_command(label=_('Save'),             command=saveProject,   underline=Z['Save'], state='disabled')
    mFile.add_command(label=_('Save as...'),       command=saveAsProject, underline=Z['Save as...'], state='disabled')
    mFile.add_command(label=_('Save makefile...'), command=saveMakefile,  underline=Z['Save makefile'], state='disabled')
    mFile.add_command(label=_('Quit'),             command=quit,          underline=Z['Quit'])
    top.add_cascade(label=_('File'), menu=mFile, underline=Z['File'])
    mTasks = Menu(top)
    mTasks.add_command(label=_('Make all'),   command=makeAll,    underline=Z['Make all'],   state='disabled')
    mTasks.add_command(label=_('Make clean'), command=makeClean,  underline=Z['Make clean'], state='disabled')
    mTasks.add_command(label=_('Abort Make'), command=makeCancel, underline=Z['Abort Make'], state='disabled')
    top.add_cascade(label=_('Tasks'), menu=mTasks, underline=Z['Tasks'])
    mHelp = Menu(top)
    mHelp.add_command(label=_('About...'), command=about, underline=Z['About...'])
    top.add_cascade(label=_('Help'), menu=mHelp, underline=Z['Help'])

    buttons = Frame(root)
    buttons.pack(side=RIGHT, fill=Y)
    bProject = Button(buttons, state='disabled', text=_('Project'), command=comProject)
    bUp      = Button(buttons, state='disabled', text=_('Up'),      command=comUp)
    bDown    = Button(buttons, state='disabled', text=_('Down'),    command=comDown)
    bAdd     = Button(buttons, state='disabled', text=_('Add'),     command=comAdd)
    bDel     = Button(buttons, state='disabled', text=_('Remove'),  command=comDel)
    bView    = Button(buttons, state='disabled', text=_('View'),    command=comView)
    bInfo    = Button(buttons, state='disabled', text=_('Info'),    command=comInfo)
    bProject.pack (side=TOP, fill=X)
    bUp.pack      (side=TOP, fill=X)
    bDown.pack    (side=TOP, fill=X)
    bAdd.pack     (side=TOP, fill=X)
    bDel.pack     (side=TOP, fill=X)
    bView.pack    (side=TOP, fill=X)
    bInfo.pack    (side=TOP, fill=X)
    bProject.bind ('<Return>', comProject)
    bUp.bind      ('<Return>', comUp)
    bDown.bind    ('<Return>', comDown)
    bAdd.bind     ('<Return>', comAdd)
    bDel.bind     ('<Return>', comDel)
    bView.bind    ('<Return>', comView)
    bInfo.bind    ('<Return>', comInfo)

    frm = Frame(root)
    frm.pack(expand=YES, fill=BOTH)
    xbar = Scrollbar(frm, orient="horizontal")
    ybar = Scrollbar(frm)
    lst = Listbox(frm, font=fixed, width=80, height=24)
    xbar.config(command=lst.xview)
    ybar.config(command=lst.yview)
    lst.config(yscrollcommand=ybar.set, xscrollcommand=xbar.set)
    xbar.pack(side=BOTTOM, fill=X)
    ybar.pack(side=RIGHT, fill=Y)
    lst.pack(side=LEFT, expand=YES, fill=BOTH)
    lst.bind('<Double-1>', comView)
    lst.bind('<Return>', comView)

    clearTasks()
    if len(sys.argv) > 1:
        loadProject(sys.argv[1])

    root.protocol("WM_DELETE_WINDOW", quit)
    root.mainloop()

if __name__ == '__main__':
    run()

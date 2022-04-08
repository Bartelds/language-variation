#!/usr/bin/env python

from msg import _
import os, sys, re, ConfigParser

if sys.version_info[:2] < (2, 4):
    from future import sorted

class Task:

    _clusmeth = {
        'sl': 'Single Link,',
        'cl': 'Complete Link',
        'ga': 'Group Average',
        'wa': 'Weighted Average',
        'uc': 'Unweighted Centroid',
        'wc': 'Weighted Centroid',
        'wm': "Ward's Method"}

    _mdsmeths = {
        'standard': 'Standard Method',
        'kruskal': "Kruskal's Method",
        'sammon': 'Sammon Mapping'}

    needRedraw = False

    _maxlabel = 0
    _maxuses = 0
    _givesDiff = {}

    def resetGlobals():
        needRedraw = False
        Task._maxlabel = 0
        Task._maxuses = 0
        Task._givesDiff = {}
    resetGlobals = staticmethod(resetGlobals)

    def _depends(self):
        return Task._givesDiff[self.dct['uses']]

    def __init__(self, name, p):
        self.name = name
        self.dct = {}
        if type(p) == type({}):
            for i in p:
                self.dct[i] = p[i]
        else:
            for i in p.options(name):
                self.dct[i] = p.get(name, i)
        if len(name) > Task._maxlabel:
            Task._maxlabel = len(name)
            Task.needRedraw = True
        if self.dct.has_key('uses') and len(self.dct['uses']) > Task._maxuses:
            Task._maxuses = len(self.dct['uses'])
            Task.needRedraw = True

    _info_r = re.compile(r'([\r\n])[ \t]+')

    def _info(self, s):
        return '\n' + Task._info_r.sub(r'\1', s.strip()) + '\n'

    def givesDiff(self):
        if Task._givesDiff.has_key(self.name):
            return Task._givesDiff[self.name]
        else:
            return ''

    def makeTarget(self):
        return ''

    def makeNoTarget(self):
        return ''

    def makeRule(self):
        return ''

    def makeIntermedTargets(self):
        return []

    def makeIntermedRules(self):
        return []

    def info(self):
        return ''

    def cleanup(self):
        if Task._givesDiff.has_key(self.name):
            Task._givesDiff.pop(self.name)

class Leven(Task):

    def __init__(self, name, p):
        Task.__init__(self, name, p)
        Task._givesDiff[name] = '%s.txt' % name

    def __str__(self):
        return '     %s  <%s>  Levenshtein' % (self.name.ljust(Task._maxlabel), self.dct['data'])

    def makeTarget(self):
        return '%s.txt' % self.name

    def makeRule(self):
        opts = ''
        if int(self.dct['cral']):
            opts += ' -c'
        if int(self.dct['minfreq']) > 1:
            opts += ' -f ' + self.dct['minfreq']
        if int(self.dct['skipnovars']):
            opts += ' -F'
        return '%s.txt: $(LABELFILE) %s\n\tleven -n $(LOCATIONS) -l $(LABELFILE) -L%s %s  -o %s.txt %s\n\n' % (
                self.name, self.dct['data'], opts, self.dct['expert'], self.name, self.dct['data'])

    def info(self):
        s = _('''
        Input task: %(name)s
        Type of task: Dialect data
        Input file pattern: %(pat)s
        Output file: %(name)s.txt
        Method: Levenshtein (string edit distance)
        ''') % {
            'name'  : self.name,
            'pat'   : self.dct['data']}
        if int(self.dct['cral']):
            s += _('With Cronbach Alpha\n')
        if int(self.dct['minfreq']) > 1:
            s += _('Minimum frequency: %s\n') % self.dct['minfreq']
        if int(self.dct['skipnovars']):
            s += _('Skip files with less than two variants\n')
        if self.dct['expert']:
            s += _('Expert options: %s\n') % self.dct['expert']
        return self._info(s)

class Giw(Task):

    def __init__(self, name, p):
        Task.__init__(self, name, p)
        Task._givesDiff[name] = '%s.txt' % name

    def __str__(self):
        return '     %s  <%s>  G.I.W.' % (self.name.ljust(Task._maxlabel), self.dct['data'])

    def makeTarget(self):
        return '%s.txt' % self.name

    def makeRule(self):
        opts = ''
        if int(self.dct['cral']):
            opts += ' -c'
        if int(self.dct['minfreq']) > 1:
            opts += ' -f ' + self.dct['minfreq']
        if int(self.dct['skipnovars']):
            opts += ' -F'
        return '%s.txt: $(LABELFILE) %s\n\tgiw -n $(LOCATIONS) -l $(LABELFILE) -L%s %s -o %s.txt %s\n\n' % (
                self.name, self.dct['data'], opts, self.dct['expert'], self.name, self.dct['data'])

    def info(self):
        s = _('''
        Input task: %(name)s
        Type of task: Dialect data
        Input file pattern: %(pat)s
        Output file: %(name)s.txt
        Method: G.I.W. (Gewichteter Identit\xe4tswert)
        ''') % {
            'name'  : self.name,
            'pat'   : self.dct['data']}
        if int(self.dct['cral']):
            s += _('With Cronbach Alpha\n')
        if int(self.dct['minfreq']) > 1:
            s += _('Minimum frequency: %s\n') % self.dct['minfreq']
        if int(self.dct['skipnovars']):
            s += _('Skip files with less than two variants\n')
        if self.dct['expert']:
            s += _('Expert options: %s\n') % self.dct['expert']
        return self._info(s)


class Binary(Task):

    def __init__(self, name, p):
        Task.__init__(self, name, p)
        Task._givesDiff[name] = '%s.txt' % name

    def __str__(self):
        return _('     %s  <%s>  Binary') % (self.name.ljust(Task._maxlabel), self.dct['data'])

    def makeTarget(self):
        return '%s.txt' % self.name

    def makeRule(self):
        opts = ''
        if int(self.dct['cral']):
            opts += ' -c'
        if int(self.dct['minfreq']) > 1:
            opts += ' -f ' + self.dct['minfreq']
        if int(self.dct['skipnovars']):
            opts += ' -F'
        return '%s.txt: $(LABELFILE) %s\n\tleven -B -n $(LOCATIONS) -l $(LABELFILE) -L%s %s -o %s.txt %s\n\n' % (
                self.name, self.dct['data'], opts, self.dct['expert'], self.name, self.dct['data'])

    def info(self):
        s = _('''
        Input task: %(name)s
        Type of task: Dialect data
        Input file pattern: %(pat)s
        Output file: %(name)s.txt
        Method: Binary differences
        ''') % {
            'name'  : self.name,
            'pat'   : self.dct['data']}
        if int(self.dct['cral']):
            s += _('With Cronbach Alpha\n')
        if int(self.dct['minfreq']) > 1:
            s += _('Minimum frequency: %s\n') % self.dct['minfreq']
        if int(self.dct['skipnovars']):
            s += _('Skip files with less than two variants\n')
        if self.dct['expert']:
            s += _('Expert options: %s\n') % self.dct['expert']
        return self._info(s)

class Dif(Task):

    def __init__(self, name, p):
        Task.__init__(self, name, p)
        Task._givesDiff[name] = self.dct['file']

    def __str__(self):
        return _('     %s  <%s>  Existing differences') % (self.name.ljust(Task._maxlabel), self.dct['file'])

    def makeNoTarget(self):
        return self.dct['file']

    def info(self):
        s = _('''
        Input task: %(name)s
        Type of task: Existing differences
        File name: %(file)s
        ''') % {
            'name'  : self.name,
            'file'  : self.dct['file']}
        return self._info(s)

class Diffix(Task):

    def __init__(self, name, p):
        Task.__init__(self, name, p)
        Task._givesDiff[name] = '%s.txt' % name

    def __str__(self):
        return _('     %s  [ %s ]  repair incomplete dialect differences') % (
            self.name.ljust(Task._maxlabel), self.dct['uses'].center(Task._maxuses))

    def makeTarget(self):
        return '%s.txt' % self.name

    def makeRule(self):
        return '%s.txt: %s\n\tdiffix -a %s -o %s.txt %s %s\n\n' % (
                self.name, self._depends(), self.dct['aspect'], self.name, self._depends(), self.dct['coo'])

    def info(self):
        s = _('''
        Input task: %(name)s
        Type of task: Repair incomplete dialect differences
        Uses input task: %(uses)s
        Output file: %(name)s.txt
        ''') % {
            'name'  : self.name,
            'uses'  : self.dct['uses']}
        return self._info(s)

class Den(Task):

    def __str__(self):
        s = ''
        n = int(self.dct['groups'])
        if n > 1:
            s += _(', %i groups') % n
        return _('EPS  %s  [ %s ]  dendrogram, %s%s') % (
            self.name.ljust(Task._maxlabel), self.dct['uses'].center(Task._maxuses), Task._clusmeth[self.dct['method']], s)

    def makeTarget(self):
        return '%s.eps' % self.name

    def makeRule(self):
        imed = '%s__clu%s__.txt' % (self.dct['uses'], self.dct['method'])
        opts = ''
        if self.dct['method'] == 'wm':
            opts += ' -e .3333'
        if int(self.dct['groups']) > 1:
            opts += ' -n ' + self.dct['groups']
            if int(self.dct['groups']) > 19:
                opts += ' -h'
            if self.dct['colors'] == 'both' or self.dct['colors'] == 'links':
                opts += ' -c'
            if self.dct['labels'] != '0' and (self.dct['colors'] == 'both' or self.dct['colors'] == 'labels'):
                opts += ' -C'
        if self.dct['labels'] == '0':
            opts += ' -L'
        elif self.dct['labels'] == '2':
            opts += ' -p'
        return '%s.eps: %s\n\tden%s %s -o %s.eps %s\n\n' % (self.name, imed, opts, self.dct['expert'], self.name, imed)

    def makeIntermedTargets(self):
        return ['%s__clu%s__.txt' % (self.dct['uses'], self.dct['method'])]

    def makeIntermedRules(self):
        method = self.dct['method']
        imed = '%s__clu%s__.txt' % (self.dct['uses'], self.dct['method'])
        dep = self._depends()
        return ['%s: %s\n\tcluster -%s -o %s %s\n\n' % (imed, dep, method, imed, dep)]

    def info(self):
        s = _('''
        Output task: %(name)s
        Type of task: Dendrogram
        Uses input task: %(uses)s
        Output file: %(name)s.eps
        Method: %(method)s
        ''') % {
            'name'  : self.name,
            'uses'  : self.dct['uses'],
            'method': Task._clusmeth[self.dct['method']]}
        lab = int(self.dct['labels'])
        if lab == 0:
            s += _('No labels\n')
        if lab == 2:
            s += _('Labels in two columns\n')
        n = int(self.dct['groups'])
        if n > 1:
            s += _('Groups: %i\n') % n
            if lab > 0 and (self.dct['colors'] == 'labels' or self.dct['colors'] == 'both'):
                s += _('Coloured labels\n')
            if self.dct['colors'] == 'links' or self.dct['colors'] == 'both':
                s += _('Coloured links\n')
        if self.dct['expert']:
            s += _('Expert options: %s\n') % self.dct['expert']
        return self._info(s)

class MapCcc(Task):

    def __str__(self):
        return _('EPS  %s  [ %s ]  CCC map') % (self.name.ljust(Task._maxlabel), self.dct['uses'].center(Task._maxuses))

    def makeTarget(self):
        return '%s.eps' % self.name

    def makeRule(self):
        filename = '%s.eps' % self.name
        dep = self._depends()
        t = '%s: %s $(CONFIGFILE) $(MAPDEPS)\n' % (filename, dep)
        t += '\tcluster -wa -c -N .5 -r 50 -o __tmp1__.txt %s\n' % dep
        t += '\tcluster -ga -c -N .5 -r 50 -o __tmp2__.txt %s\n' % dep
        t += '\tdifsum -a -o __tmp3__.txt __tmp1__.txt __tmp2__.txt\n'
        t += '\tmds -o __tmp1__.txt 3 __tmp3__.txt\n'
        t += '\tmaprgb -e -o %s $(CONFIGFILE) __tmp1__.txt\n\n' % filename
        return t

    def makeIntermedTargets(self):
        return ['__tmp1__.txt', '__tmp2__.txt', '__tmp3__.txt']

    def info(self):
        s = _('''
        Output task: %(name)s
        Type of task: CCC Map
        Uses input task: %(uses)s
        Output file: %(name)s.eps
        ''') % {
            'name'  : self.name,
            'uses'  : self.dct['uses']}
        if self.dct['expert']:
            s += _('Expert options: %s\n') % self.dct['expert']
        return self._info(s)

class MapMds(Task):

    def __init__(self, name, p):
        Task.__init__(self, name, p)
        self.opt = e = ''
        m = self.dct['method']
        if m == 'kruskal':
            e = 'K'
            self.opt = ' -K'
        elif m == 'sammon':
            e = 'S'
            self.opt = ' -S'
        self.imed = '%s__mds%s3%s__.txt' % (
            self.dct['uses'],
            e,
            self.dct['expertmds'].replace(' ', '_'))

    def __str__(self):
        return _('EPS  %s  [ %s ]  MDS map, %s') % (
            self.name.ljust(Task._maxlabel), self.dct['uses'].center(Task._maxuses), Task._mdsmeths[self.dct['method']])

    def makeTarget(self):
        return '%s.eps' % self.name

    def makeRule(self):
        return '%s.eps: %s $(CONFIGFILE) $(MAPDEPS)\n\tmaprgb %s -o %s.eps $(CONFIGFILE) %s\n\n' % (
            self.name, self.imed, self.dct['expertmap'], self.name, self.imed)

    def makeIntermedTargets(self):
        return [self.imed]

    def makeIntermedRules(self):
        dep = self._depends()
        return ['%s: %s\n\tmds%s %s -o %s 3 %s\n\n' % (self.imed, dep, self.opt, self.dct['expertmds'], self.imed, dep)]

    def info(self):
        s = _('''
        Output task: %(name)s
        Type of task: MDS Map
        Uses input task: %(uses)s
        Output file: %(name)s.eps
        Method: %(method)s
        ''') % {
            'name'  : self.name,
            'uses'  : self.dct['uses'],
            'method': Task._mdsmeths[self.dct['method']]}
        if self.dct['expertmds']:
            s += _('Expert MDS options: %s\n') % self.dct['expertmds']
        if self.dct['expertmap']:
            s += _('Expert map options: %s\n') % self.dct['expertmap']
        return self._info(s)

class MapClust(Task):

    def __init__(self, name, p):
        Task.__init__(self, name, p)
        self.method = self.dct['method']
        self.imed = '%s__clu%s__.txt' % (self.dct['uses'], self.method)

    def __str__(self):
        return _('EPS  %s  [ %s ]  cluster map, %s, %i groups') % (
            self.name.ljust(Task._maxlabel), self.dct['uses'].center(Task._maxuses),
            Task._clusmeth[self.dct['method']], int(self.dct['groups']))

    def makeTarget(self):
        return '%s.eps' % self.name

    def makeRule(self):
        opts = ''
        if int(self.dct['groups']) > 19:
            opts += ' -r'
        return '%s.eps: %s $(CONFIGFILE) $(MAPDEPS)\n\tmapclust%s %s -o %s.eps $(CONFIGFILE) %s %i\n\n' % (
            self.name, self.imed, opts, self.dct['expert'], self.name, self.imed, int(self.dct['groups']))

    def makeIntermedTargets(self):
        return [self.imed]

    def makeIntermedRules(self):
        dep = self._depends()
        return ['%s: %s\n\tcluster -%s -o %s %s\n\n' % (self.imed, dep, self.method, self.imed, dep)]

    def info(self):
        s = _('''
        Output task: %(name)s
        Type of task: Cluster Map
        Uses input task: %(uses)s
        Output file: %(name)s.eps
        Method: %(method)s
        groups: %(groups)s
        ''') % {
            'name'  : self.name,
            'uses'  : self.dct['uses'],
            'method': Task._clusmeth[self.dct['method']],
            'groups': self.dct['groups']}
        if self.dct['expert']:
            s += _('Expert options: %s\n') % self.dct['expert']
        return self._info(s)

class MapVec(Task):

    def __str__(self):
        return _('EPS  %s  [ %s ]  vector map, neighbourhood size %g') % (
            self.name.ljust(Task._maxlabel),
            self.dct['uses'].center(Task._maxuses),
            float(self.dct['nsize']))

    def makeTarget(self):
        return '%s.eps' % self.name

    def makeRule(self):
        dep = self._depends()
        return '%s.eps: %s $(CONFIGFILE) $(MAPDEPS)\n\tmapvec -n %s %s -o %s.eps $(CONFIGFILE) %s\n\n' % (
            self.name, dep, self.dct['nsize'], self.dct['expert'], self.name, dep)

    def info(self):
        s = _('''
        Output task: %(name)s
        Type of task: Vector Map
        Uses input task: %(uses)s
        Output file: %(name)s.eps
        Neighbourhood size: %(nsize)s
        ''') % {
            'name'  : self.name,
            'uses'  : self.dct['uses'],
            'nsize' : self.dct['nsize']}
        if self.dct['expert']:
            s += _('Expert options: %s\n') % self.dct['expert']
        return self._info(s)

def readProject(filename):

    p = ConfigParser.RawConfigParser()
    p.read(filename)

    configfile = p.get('L04', 'config')

    try:
        labelfile = p.get('L04', 'labels')
    except:
        labelfile = ''

    depsd = {'transform'    : '',
             'labels'       : '',
             'coordinates'  : '',
             'clipping'     : '',
             'map'          : '',
             'othermarkers' : '',
             }

    aspect = ''
    f = open(configfile, 'r')
    for line in f:
        w = line.split(':', 1)
        if len(w) < 2:
            continue
        key = w[0].strip()
        value = w[1].strip()
        depsd[key] = value
        
        if key == 'labels' and not labelfile:
            labelfile = value
        elif key == 'coordinates':
            coofile = value
        elif key == 'aspect':
            aspect = value
    f.close()
    if not aspect:
        aspect = '1'

    deps = ' '.join([depsd['transform'],
                     depsd['labels'],
                     depsd['coordinates'],
                     depsd['clipping'],
                     depsd['map'],
                     depsd['othermarkers']])

    numlocations = 0
    f = open(labelfile, 'r')
    for line in f:
        w = line.split()
        try:
            n = int(w[0])
        except IndexError:
            pass
        except ValueError:
            pass
        if n > numlocations:
            numlocations = n
    f.close()

    tasks = []
    Task._maxlabel = Task._maxuses = 0
    r = re.compile('^\s*\[\s*(.*?)\s*\]\s*$')
    f = open(filename, 'r')
    for line in f:
        sre = r.match(line)
        if sre:
            name = sre.group(1)
            if name == 'L04':
                continue
            t = p.get(name, 'type')
            if t == 'leven':
                tasks.append(Leven(name, p))
            elif t == 'giw':
                tasks.append(Giw(name, p))
            elif t == 'binary':
                tasks.append(Binary(name, p))
            elif t == 'dif':
                tasks.append(Dif(name, p))
            elif t == 'diffix':
                tasks.append(Diffix(name, p))
            elif t == 'den':
                tasks.append(Den(name, p))
            elif t == 'mapccc':
                tasks.append(MapCcc(name, p))
            elif t == 'mapmds':
                tasks.append(MapMds(name, p))
            elif t == 'mapclust':
                tasks.append(MapClust(name, p))
            elif t == 'mapvec':
                tasks.append(MapVec(name, p))
            else:
                assert 0, 'Invalid type %s' % t
    f.close()

    return tasks, configfile, labelfile, coofile, aspect, numlocations, deps

def writeProject(filename, tasks, configfile, labelfile):
    f = open(filename, 'w')
    f.write('[L04]\nconfig: %s\nlabels: %s\n' % (configfile, labelfile))
    for task in tasks:
        f.write('\n[%s]\n' % task.name)
        for key in sorted(task.dct.keys()):
            f.write('%s: %s\n' % (key, task.dct[key]))
    f.close()

def writeMakefile(filename, tasks, configfile, labelfile, numlocations, deps):

    f = open(filename, 'w')
    f.write('.SUFFIXES:\n.DELETE_ON_ERROR:\n\n')
    f.write('MAPDEPS = %s\n' % deps)
    f.write('CONFIGFILE = %s\n' % configfile)
    f.write('LABELFILE = %s\n' % labelfile)
    f.write('LOCATIONS = %i\n' % numlocations)
    f.write('\nRM = rm -f\n\n')


    f.write('TARGETS =')
    for a in tasks:
        if a.makeTarget():
            f.write(' \\\n\t%s' % a.makeTarget())

    used = {}
    f.write('\n\nINTERMED =')
    for a in tasks:
        for t in a.makeIntermedTargets():
            if not used.has_key(t):
                used[t] = True
                f.write(' \\\n\t%s' % t)


    f.write('\n\nall: $(TARGETS)\n\n.PHONY: clean\nclean:\n\t$(RM) $(TARGETS) $(INTERMED)\n\n')

    for a in tasks:
        f.write(a.makeRule())

    used = {}
    for a in tasks:
        for t in a.makeIntermedRules():
            if not used.has_key(t):
                used[t] = True
                f.write(t)

    f.close()

#| if main
if __name__ == '__main__':

    import os

    infile = '../demo/Project.ini'
    cpfile = 'Project.temp'
    outfile = 'Makefile.temp'
    if len(sys.argv) > 1:
        infile = sys.argv[1]

    here = os.getcwd()
    directory, filename = os.path.split(infile)
    if directory:
        os.chdir(directory)

    tasks, configfile, labelfile, coofile, aspect, numlocations, deps = readProject(filename)

    os.chdir(here)

    for a in tasks:
        print a

    writeProject(cpfile, tasks, configfile, labelfile)

    writeMakefile(outfile, tasks, configfile, labelfile, numlocations, deps)

    print 'Project read from', infile
    print 'Project copied to', cpfile
    print 'Makefile written to', outfile

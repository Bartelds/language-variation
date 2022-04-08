# -*- coding: iso-8859-1 -*-

from helper import *
import version

about = Help('''
    pyL04 - Version %s
    
    pyL04 is a partial interface to RuG/L04
    software for dialectometrics and cartography

    (c) Peter Kleiweg 2005, 2008

    more info at: http://www.let.rug.nl/~kleiweg/L04/
''' % version.version)

newProject1 = Help('''
New project: Project name

Give the name of a new project. This must be a valid file name.

If you give a name without a file extension, the extension .ini will be
added.
''')

newProject2 = Help('''
New project: Map configuration file

Give the name of an existing map configuration file. For details, see
the manual pages: Cartography -> Configuration

The directory of this file will be the working directory of the project.
''')

newProject3 = Help('''
New project: Label file

This is optional. If you don't give the name of a label file, the one
defined in the map configuration file will be used.

You specify an alternate label file if you want to use only part of the
dialect data. Just make a copy of the default label file, and leave out
the locations you don't want to use. Then, make sure you renumber the
labels from 1 upwards.

For details, see the manual pages: General -> Formats -> Label file
''')

addTask1 = Help('''
Add task: Task name

Give a short and descriptive name of a new task.

A valid name consists of:
- lower case letters from a to z
- upper case letters from A to Z
- numbers from 0 to 9
- underscores: _
- minuses: -

The name may not begin with a minus
''')

addTask2 = Help('''
Add task: Task type

Select what type of task you want to add.

An input task is a task that calculates the dialect differences between
the set of locations.

Input: dialect data - based on a set of dialect files, calculate the
    dialect differences
Input: existing differences - use differences that have been calculated
    by other means
Input: incomplete differences - guess the dialect differences that are
    missing from a calculation
''')

addTask3 = Help('''
Add task: Task type

Select what type of task you want to add.

An output task visualises the dialect differences that are produces by
an input task.

Output: dendrogram - display a graph of an hierarchical clustering 
Output: CCC map - display a map based on stochastic clustering followed
    by multidimensional scaling
Output: MDS map - display a map based on multidimensional scaling
Output: cluster map - display a map based on clustering
Output: vector map - display a map with vectors
''')

addTaskDataFilepattern1 = Help('''
Add input task dialect data: Measurement method

The Levenshtein method (string edit distance) calculates the difference
between two strings based on the dissimilarity of those strings. You can
use this method for phonetic as well as lexical measurements.
For details, see the manual pages: Dialectometrics -> leven

G.I.W., or Gewichteter Identitätswert, determines the difference
between two strings based on two things:
- strings are identical, or they are not
- if strings are identical, what is the frequency of this variant in the
  data?
This method is not suitable for phonetic measurements.
For details, see the manual pages: Dialectometrics -> giw

Binary differences simply looks whether two strings are identical or
not. This method is not suitable for phonetic measurements.
''')

addTaskDataFilepattern2 = Help('''
Add input task dialect data: Input file pattern

Instead of listing all input files, you have to choose a pattern that
matches all the input files, and none other.
''')

addTaskDataCRAL = Help('''
Add input task dialect data: Cronbach Alpha

Cronbach Alpha is a statistics that tells you something about the
reliability of the measurement. Calculating the Cronbach Alpha will
increase the computation time.
''')

addTaskDatainfof = Help('''
Add input task dialect data: Minimum frequency

If you set a value higher than 1, all variants that occur less than that
many times in an input file are removed. This is a crude method to
remove some of the noise from the data. It may improve the result of
lexical measurements, i.e. data that has many identical items. Don't use
this for phonetic measurements, as it will remove meaningful differences.
''')

addTaskDatainfoF = Help('''
Add input task dialect data: Skip single variant files

If you select this option, all input files that have only one variant
will be skipped.

This option is effected by the selection of a minimum frequency. First,
the infrequent variants are removed from the input file, then, if only a
single variant is left, the entire file is skipped.
''')

addTaskDataExp = Help('''
Add input task dialect data: Expert options

For additional command line options, see manual pages:
- Levenshtein method: Dialectometrics -> leven
- Gewichteter Identitätswert: Dialectometrics -> giw
- Binary differences: Dialectometrics -> leven
''')

addTaskDenMethod = Help('''
Add output task dendrogram: Method

For details about clustering algorithms, see:
    Anil K. Jain and Richard C. Dubes.
    "Algorithms for Clustering Data."
    Prentice Hall, Englewood Cliffs, NJ, 1988.

Both Centroid methods give graphs with parts that 'fold back' on
themselves. This is not a bug. It is a consequence of the clustering
method.

Ward's Method has the effect that cluster differences increase
eightfold, if the area size doubles in diameter. This effect is
compensated for in the diagram by adjusting the x-scale. You can undo
this compensation by adding this with the Expert options at the bottom:
  -e 1
''')

addTaskDenNog = Help('''
Add output task dendrogram: Number of groups

If you give a number larger than 1, the graph is split in this many
sub-clusters.
''')

addTaskDenLabels = Help('''
Add output task dendrogram: Labels

If the data set is too large, you can decrease the size of the
dendrogram by setting the labels in two columns.

You can decrease the size of the graph further by omitting labels
entirely.
''')

addTaskDenColors = Help('''
Add output task dendrogram: Colours

Should sub-clusters be coloured? Each cluster has the same colour as the
cluster on the corresponding cluster map, provided the same clustering
method and the same number of groups are used.

This option has no effect if you set number of groups to 1.
''')

addTaskDenExpert = Help('''
Add output task dendrogram: Expert options

For additional command line options, see manual pages:
Hierarchical clustering -> den
''')

addTaskMDSMethod = Help('''
Add output task MDS map: Method

Choose an MDS method.

Classical: Classical Multidimensional Scaling
Kruskal's method: Kruskal's Non-metric Multidimensional Scaling
Sammon mapping: Sammon's Non-Linear Mapping

The last two methods both use Classical Multidimensional Scaling as an
initialisation step.
''')

addTaskMDSExpertMds = Help('''
Add output task MDS map: Expert MDS options

For additional command line options, see manual pages:
Multidimensional scaling -> mds
''')

addTaskMDSExpertMap = Help('''
Add output task MDS map: Expert map options

For additional command line options, see manual pages:
Cartography -> maprgb
''')

addTaskCluMethod = Help('''
Add output task cluster map: Method

Choose a clustering method.

Suitable for a cluster map are:
- Ward's Method
- Complete link

You may also want to try:
- Group Average
- Weighted Average
''')

addTaskCluNog = Help('''
Add output task cluster map: Number of groups

You need to set the number of groups at least at 2.
''')

addTaskCluExpert = Help('''
Add output task cluster map: Expert options

For additional command line options, see manual pages:
Cartography -> mapclust

For instance, if you want maps suitable for black and white printing,
add: -B
''')

addTaskVecNsize = Help('''
Add output task vector map: Neighbourhood size

The size of the neighbourhood as a fraction of the longest geographic
distance. This must be a value between 0 (exclusive) and 1 (inclusive).
''')

addTaskVecExpert = Help('''
Add output task vector map: Expert options

For additional command line options, see manual pages:
Cartography -> mapvec
''')


# -*- coding: iso-8859-1 -*-

from helper import *
import version

about = Help('''
    pyL04 - versie %s
    
    pyL04 is een gedeeltelijke interface voor RuG/L04
    programmatuur voor dialectometrie en cartografie

    (c) Peter Kleiweg 2005, 2008

    meer info op: http://www.let.rug.nl/~kleiweg/L04/
''' % version.version)

newProject1 = Help('''
Nieuw project: Naam voor project

Geef de naam voor een nieuw project. Dit moet een geldige bestandsnaam
zijn.

Als je een naam zonder extensie geeft wordt de extensie .ini toegevoegd.
''')

newProject2 = Help('''
Nieuw project: Bestand met kaartconfiguratie

Geef de naam van een bestaand bestand met een kaartconfiguratie. Voor
details, zie de handleiding: Cartography -> Configuration

De directory van dit bestand zal gebruikt worden als actuele directory
voor het project.
''')

newProject3 = Help('''
Nieuw project: Labelbestand

Dit is optioneel. Als je geen naam van een labelbestand geeft dan wordt
gebruik gemaakt van het labelbestand dat is gedefinieerd in de
kaartconfiguratie.

Je gebruikt een apart labelbestand als je van de dialectgegevens alleen
de gegevens uit een beperkt gebied wilt gebruiken. Maak daarvoor een
kopie van het gewone labelbestand, en verwijder daaruit alle lokaties
die je niet wilt gebruiken. Vergeet niet de overgebleven lokaties
opnieuw te nummeren, vanaf 1.

Voor details, zie de handleiding: General -> Formats -> Label file
''')

addTask1 = Help('''
Voeg taak toe: Taaknaam

Geef een korte en betekenisvolle naam voor een nieuwe taak.

Een geldige naam bevat:
- kleine letters van a tot z
- hoofdletters van A tot Z
- cijfers van 0 tot 9
- onderstrepen: _
- mintekens: -

De naam mag niet beginnen met een minteken.
''')

addTask2 = Help('''
Voeg taak toe: Soort taak

Kies wat voor soort taak je wilt toevoegen.

Een invoertaak is een taak die de afstanden tussen een set lokaties
berekent.

Invoer: dialectgegevens - bereken de dialectafstanden, gebaseerd op een
    set dialectbestanden
Invoer: bestaande verschillen - gebruik afstanden die op een andere manier
    zijn verkregen
Invoer: incomplete verschillen - vul door benadering de afstanden aan,
    die ontbreken in een berekening van dialectafstanden
''')

addTask3 = Help('''
Voeg taak toe: Soort taak

Kies wat voor soort taak je wilt toevoegen.

Een uitvoertaak visualiseert de dialectafstanden die door een invoertaak
zijn berekend.

Uitvoer: dendrogram - toon een grafiek van een stapsgewijze clustering
Uitvoer: CCC-kaart - toon een kaart gebaseerd op clustering met ruis
    gevolgd door multi-dimensionale schaling (MDS)
Uitvoer: MDS-kaart - toon een kaart gebaseerd op multi-dimensionale
    schaling
Uitvoer: clusterkaart - toon een kaart gebaseerd op clustering
Uitvoer: vectorkaart - toon een kaart met vectoren
''')

addTaskDataFilepattern1 = Help('''
Toevoegen van invoertaak dialectgegevens: Methode

De Levenshteinmethode (string edit distance) berekent de afstand tussen
twee reeksen aan de hand van de mate van verschil tussen de twee
reeksen. Je kunt deze methode gebruiken voor zowel fonetische metingen
als lexicale metingen.
Voor details, zie de handleiding: Dialectometrics -> leven

G.I.W., of Gewichteter Identitätswert, bepaalt de afstand tussen twee
reeksen aan de hand van twee dingen:
- de reeksen zijn identiek of niet
- als de reeksen identiek zijn, hoe vaak komt deze variant dan voor in
  de data?
Deze methode is niet geschikt voor fonetische metingen.
Voor details, zie de handleiding: Dialectometrics -> giw

Binairy differences: hier telt alleen of twee reeksen gelijk zijn of
niet. Deze methode is niet geschikt voor fonetische metingen.
''')

addTaskDataFilepattern2 = Help('''
Toevoegen van invoertaak dialectgegevens: Bestandspatroon

In plaats van alle invoerbestanden te selecteren moet je een patroon
selecteren dat op alle invoerbestanden past, en op geen enkel ander
bestand.
''')

addTaskDataCRAL = Help('''
Toevoegen van invoertaak dialectgegevens: Cronbach's alfa

Cronbach's alfa is een statistische maat die je iets zegt over de
betrouwbaarheid van de meting. Het bepalen van Cronbach's alfa kost
extra rekentijd.
''')

addTaskDatainfof = Help('''
Toevoegen van invoertaak dialectgegevens: Minimumfrequentie

Als je hier een getal groter dan 1 invult worden alle varianten die
minder vaak dan de gekozen waarde in een invoerbestand voorkomen
verwijderd. Dit is een ruwe methode om ruis uit de data te verwijderen.
Het kan het resultaat van een lexicale meting verbeteren, dat wil
zeggen, een meting aan data die veel identieke vormen bevat. Gebruik dit
niet voor een fonetische meting, want dan verlies je veel aan
betekenisvol onderscheid.
''')

addTaskDatainfoF = Help('''
Toevoegen van invoertaak dialectgegevens: Negeer bestanden met maar één
variant

Als je deze optie selecteert worden alle invoerbestanden die maar één
variant bevatten overgeslagen.

Deze optie wordt beïnvloed door de selectie van een minimumfrequentie.
Eerst worden zeldzame varianten uit een bestand verwijderd, en als er in
het bestand dan nog maar één variant over is wordt heel het bestand
overgeslagen.
''')

addTaskDataExp = Help('''
Toevoegen van invoertaak dialectgegevens: Expertopties

Voor extra opties voor de commandoregel, zie de handleiding:
- Levenshtein method: Dialectometrics -> leven
- Gewichteter Identitätswert: Dialectometrics -> giw
- Binary differences: Dialectometrics -> leven
''')

addTaskDenMethod = Help('''
Toevoegen van uitvoertaak dendrogram: Methode

Voor details over clusteralgoritmes, zie:
    Anil K. Jain en Richard C. Dubes.
    "Algorithms for Clustering Data."
    Prentice Hall, Englewood Cliffs, NJ, 1988.

Beide centroidmethodes resulteren in diagrammen die over zichzelf
'terugvouwen'. Dit is geen bug, maar een consequentie van de
clustermethode.

Ward's Method heeft als effect dat clusterafstanden ongeveer
verachtvoudigen als het gebied verdubbelt in doorsnee. Voor dit effect
wordt gecompenseerd door de x-as aan te passen. Je kunt deze aanpassing
ongedaan maken door onderaan bij 'Expert options' in te vullen:
  -e 1
''')

addTaskDenNog = Help('''
Toevoegen van uitvoertaak dendrogram: Aantal groepen

Als je een aantal groter dan 1 invult wordt de diagram gesplitst in
zoveel subclusters.
''')

addTaskDenLabels = Help('''
Toevoegen van uitvoertaak dendrogram: Labels

Als je een grote dataset hebt kun je het dendrogram verkleinen door de
labels in twee kolommen te zetten.

Om het dendrogram nog kleiner te maken kun je de labels helemaal
weglaten.
''')

addTaskDenColors = Help('''
Toevoegen van uitvoertaak dendrogram: Kleur

Wil je kleur gebruiken? Elk cluster heeft dezelfde kleur als de
corresponderende clusterkaart, op voorwaarde dat je hetzelfde aantal
groepen en dezelfde clustermethode gebruikt.

Deze optie heeft geen effect als je het aantal groepen op 1 set.
''')

addTaskDenExpert = Help('''
Toevoegen van uitvoertaak dendrogram: Expertopties

Voor extra opties voor de commandoregel, zie de handleiding:
Hierarchical clustering -> den
''')

addTaskMDSMethod = Help('''
Toevoegen van uitvoer taak MDS-kaart: Methode

Kies een methode voor multi-dimensionaal schalen.

Classical: Classical Multidimensional Scaling
Kruskal's method: Kruskal's Non-metric Multidimensional Scaling
Sammon mapping: Sammon's Non-Linear Mapping

De laatste twee methodes gebruiken allebei de eerste methode om de
berekeningen te initialiseren.
''')

addTaskMDSExpertMds = Help('''
Toevoegen van uitvoer taak MDS-kaart: Expertopties MDS

Voor extra opties voor de commandoregel, zie de handleiding:
Multidimensional scaling -> mds
''')

addTaskMDSExpertMap = Help('''
Toevoegen van uitvoer taak MDS-kaart: Expertopties Kaart

Voor extra opties voor de commandoregel, zie de handleiding:
Cartography -> maprgb
''')

addTaskCluMethod = Help('''
Toevoegen van uitvoertaak clusterkaart: Methode

Kies een clustermethode.

Voor een clusterkaart zijn deze methodes geschikt:
- Ward's Method
- Complete link

Je zou ook een van deze kunnen proberen:
- Group Average
- Weighted Average
''')

addTaskCluNog = Help('''
Toevoegen van uitvoertaak clusterkaart: Aantal groepen

Je moet hier minstens een waarde van 2 kiezen.
''')

addTaskCluExpert = Help('''
Toevoegen van uitvoertaak clusterkaart: Expertopties

Voor extra opties voor de commandoregel, zie de handleiding:
Cartography -> mapclust

Bijvoorbeeld, als je kaarten wilt die je in zwart-wit kunt afdrukken,
gebruik: -B
''')

addTaskVecNsize = Help('''
Toevoegen van uitvoertaak vectorkaart: Grootte van de omgeving

De grootte van de omgeving als een fractie van de langste geografische
afstand. Dit moet een waarde zijn tussen 0 (exclusief) en 1 (inclusief).
''')

addTaskVecExpert = Help('''
Toevoegen van uitvoertaak vectorkaart: Expertopties

Voor extra opties voor de commandoregel, zie de handleiding:
Cartography -> mapvec
''')


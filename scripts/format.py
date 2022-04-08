#!/usr/bin/env python
import os
import pandas as pd
from glob import glob

labels = ["Aalten Gl", "Abbekerk NH", "Angerlo Gl", "Arum Fr", "Austerlitz Ut", "Bellingwolde Gn", "Benschop Ut", "Bleskensgraaf ZH", "Boekel NB", "Breda NB", "Brunssum Lb", "Delden Ov", "Deventer Ov", "Diemen NH", "Dwingelo Dr", "Echt Lb", "Edam NH", "Eeksterveen Dr", "Eelde Dr", "Eibergen Gl", "Eijsden Lb", "Eindhoven NB", "Elburg Gl", "Finsterwolde Gn", "Formerum Fr - Formearum Fr", "Grijpskerk Gn", "Groenekan Ut", "Grouw Fr - Grou Fr", "Haamstede Ze", "Heinkenszand Ze", "Hoenderlo Gl", "Hunsel Lb", "IJmuiden NH", "Ingen Gl", "Joure Fr - De Jouwer Fr", "Jutfaas Ut", "Kampen Ov", "Kerkrade Lb", "Koekange Dr", "Koevorden Dr", "Koewacht Ze", "Laren NH", "Leermens Gn", "Leeuwarden Fr - Ljouwert Fr", "Loenen Ut", "Maastricht Lb", "Makkum Fr", "Marken NH", "Marum Gn", "Meije ZH", "Meppel Dr", "Middelburg Ze", "Monnickendam NH", "Nijeholtpade Fr", "Nijverdal Ov", "Nunspeet Gl", "Oldenzaal Ov", "Ootmarsum Ov", "Ouddorp ZH", "Oudewater Ut", "Piershil ZH", "Posterholt Lb", "Putten Gl", "Raalte Ov", "Reusel NB", "Reuver Lb", "Rhenen Ut", "Rijkevoort NB", "Rilland Ze", "Roderwolde Dr", "Roodeschool Gn", "Roswinkel Dr", "Rozendaal Gl", "Ruinen Dr", "Scheemda Gn", "Scherpenzeel Fr", "Scheveningen ZH", "Schiermonnikoog Fr - Skiermantseach Fr", "Sevenum Lb", "Sliedrecht ZH", "Slochteren Gn", "Smilde Dr", "Spakenburg Ut", "Tegelen Lb", "Tiel Gl", "Tilburg NB", "Urk Fl", "Utrecht Ut", "Vaals Lb", "Vaassen Gl", "Veenendaal Ut", "Volendam NH", "Vriezenveen Ov", "Wagenborgen Gn", "Wateringen ZH", "Westerbork Dr", "Westkapelle Ze", "Wijhe Ov", "Willemstad NB", "Windesheim Ov", "Wolvega Fr", "Zandvoort NH", "Zeeland NB", "Zuidzande Ze", "Zutphen Gl", "Zwartsluis Ov"]
models = ['wav2vec2-large-960h', 'wav2vec2-large-nl-ft-cgn', 'wav2vec2-large-xlsr-53-ft-cgn']

fs = sorted(glob('../output-seg/*/*/*.csv'))
fs = [f for f in fs if f.split('/')[-3] in models]

for f in fs:
    outdir = '/'.join(f.replace('../output-seg/', '../output-seg-106/').split('/')[:-1])
    os.makedirs(outdir, exist_ok=True)
    
    dat = pd.read_csv(f, index_col=0)
    dat = dat.drop(columns=["Brielle ZH", "Stavoren Fr - Starum Fr"])
    dat = dat.drop(["Brielle ZH", "Stavoren Fr - Starum Fr"])
    dat.to_csv(outdir + "/w2v2_distances.csv")

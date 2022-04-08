#!/usr/bin/env python
import os
import dtw
import numpy as np
import pandas as pd
from glob import glob
from collections import defaultdict


def read_data(model, rename_dict):
    """Reads data and brings feats in correct format"""
    formatted_feats = defaultdict(list)

    print("{}".format("### Loading features ###"))    
    files = glob('../feats-seg/' + model + '/*/*.npy')

    print("{}".format("### Formatting features ###"))
    for feat in sorted(files):
        formatted_feats[rename_dict[feat.split('/')[-2].split('_')[0].upper()]].append((feat.split('/')[-2].split('_')[1], np.load(feat)))   

    return formatted_feats


def compute_distances(feats, model, distance_matrix):
    """ Computes the normalised DTW distance between every combination of villages on the basis of shared words """
    words_used = defaultdict(list)
    
    for target_village, target_words in sorted(feats.items()):
        for reference_village, reference_words in sorted(feats.items()):
            distances = []
            for target_word in target_words:
                for reference_word in reference_words:
                    if target_word[0] == reference_word[0]:
                        dobj = dtw.dtw(target_word[1], reference_word[1])
                        distances.append(dobj.normalizedDistance)
                        words_used[(target_village, reference_village)].append(target_word[0])

            if len(distances) == 0:
                distance_matrix[reference_village][target_village] = 'NA'
            else:
                distance_matrix[reference_village][target_village] = round(sum(distances)/len(distances), 4)

    if not os.path.exists("../output-seg/" + model + "/"):
        os.makedirs("../output-seg/" + model + "/")

    distance_matrix.to_csv("../output-seg/" + model + "/w2v2_distances.csv", index=True)


def main():
    mdls = ['wav2vec2-large-960h', 'wav2vec2-large-nl-ft-cgn', 'wav2vec2-large-xlsr-53-ft-cgn']

    for mdl in mdls:
        for l in range(1, 25):
            model = mdl + '/layer-' + str(l)
            rename_dict = {'AALTEN':'Aalten Gl', 'ABBEKERK':'Abbekerk NH', 'ALMKERK':'Almkerk NB', 'AMERSFOORT':'Amersfoort Ut', 'ANGERLO':'Angerlo Gl', 'ARUM' : 'Arum Fr', 'APELDOORN':'Apeldoorn Gl', 'AUSTERLITZ':'Austerlitz Ut', 'AXEL':'Axel Ze', 'BELLINGWOLDE':'Bellingwolde Gn', 'BENSCHOP':'Benschop Ut', 'BLESKENSGRAAF':'Bleskensgraaf ZH', 'BOEKEL':'Boekel NB', 'BREDA':'Breda NB', 'DEN-BRIEL':'Brielle ZH', 'BRUNSSUM':'Brunssum Lb', 'DELDEN':'Delden Ov', 'DEVENTER':'Deventer Ov', 'DIEMEN':'Diemen NH', 'DWINGELO':'Dwingelo Dr', 'ECHT':'Echt Lb', 'EDAM':'Edam NH', 'EEKSTERVEEN':'Eeksterveen Dr', 'EELDE':'Eelde Dr', 'EIBERGEN':'Eibergen Gl', 'EISDEN':'Eijsden Lb', 'EINDHOVEN':'Eindhoven NB', 'ELBURG':'Elburg Gl', 'FINSTERWOLDE':'Finsterwolde Gn', 'FORMEARUM':'Formerum Fr - Formearum Fr', 'GELDROP':'Geldrop NB', 'GOES':'Goes Ze', 'GOUDA':'Gouda ZH', 'GRIJPSKERK':'Grijpskerk Gn', 'GROENEKAN':'Groenekan Ut', 'GROUW': 'Grouw Fr - Grou Fr', 'HAAMSTEDE':'Haamstede Ze', 'HEEREWAARDEN':'Heerewaarden Gl', 'HEINKENSZAND':'Heinkenszand Ze', 'HETEREN':'Heteren Gl', 'HOENDERLO':'Hoenderlo Gl', 'HUNSEL':'Hunsel Lb', 'IJMUIDEN':'IJmuiden NH', 'INGEN':'Ingen Gl', 'JOURE': 'Joure Fr - De Jouwer Fr', 'JUTFAAS':'Jutfaas Ut', 'KAMPEN':'Kampen Ov', 'KERKRADE':'Kerkrade Lb', 'KOEKANGE':'Koekange Dr', 'KOEVORDEN':'Koevorden Dr', 'KOEWACHT':'Koewacht Ze', 'LAREN':'Laren NH', 'LEERMENS':'Leermens Gn', 'LEEUWARDEN': 'Leeuwarden Fr - Ljouwert Fr', 'LOENEN-UTRECHT':'Loenen Ut', 'MAASTRICHT':'Maastricht Lb', 'MAKKUM': 'Makkum Fr', 'MARKEN':'Marken NH', 'MARUM':'Marum Gn', 'MEIJE':'Meije ZH', 'MEPPEL':'Meppel Dr', 'MIDDELBURG':'Middelburg Ze', 'MONNICKENDAM':'Monnickendam NH', 'NIJEHOLTPADE':'Nijeholtpade Fr', 'NIJVERDAL':'Nijverdal Ov', 'NUNSPEET':'Nunspeet Gl', 'OLDENZAAL':'Oldenzaal Ov', 'OOSTERLAND':'Oosterland Ze', 'OOTMARSUM':'Ootmarsum Ov', 'OSPEL':'Ospel Lb', 'OUDDORP':'Ouddorp ZH', 'OUDEWATER':'Oudewater Ut', 'PIERSHIL':'Piershil ZH', 'POSTERHOLT':'Posterholt Lb', 'PUTTEN':'Putten Gl', 'PUTTERSHOEK':'Puttershoek ZH', 'RAALTE':'Raalte Ov', 'REUSEL':'Reusel NB', 'REUVER':'Reuver Lb', 'RHENEN':'Rhenen Ut', 'RIJKEVOORT':'Rijkevoort NB', 'RILLAND':'Rilland Ze', 'RODERWOLDE':'Roderwolde Dr', 'ROERMOND':'Roermond Lb', 'ROODESCHOOL':'Roodeschool Gn', 'ROSWINKEL':'Roswinkel Dr', 'ROZENDAAL':'Rozendaal Gl', 'RUINEN':'Ruinen Dr', 'SCHEEMDA':'Scheemda Gn', 'SCHERPENZEEL':'Scherpenzeel Fr', 'SCHEVENINGEN':'Scheveningen ZH', 'SCHIERMONNIKOOG': 'Schiermonnikoog Fr - Skiermantseach Fr', 'SEVENUM':'Sevenum Lb', 'SLIEDRECHT':'Sliedrecht ZH', 'SLOCHTEREN':'Slochteren Gn', 'SMILDE':'Smilde Dr', 'SPAKENBURG':'Spakenburg Ut', 'STAVOREN': 'Stavoren Fr - Starum Fr', 'TEGELEN':'Tegelen Lb', 'TIEL':'Tiel Gl', 'TILBURG':'Tilburg NB', 'URK':'Urk Fl', 'UTRECHT':'Utrecht Ut', 'VAALS':'Vaals Lb', 'VAASSEN':'Vaassen Gl', 'VEENENDAAL':'Veenendaal Ut', 'VENLO':'Venlo Lb', 'VOLENDAM':'Volendam NH', 'FRIEZENVEEN':'Vriezenveen Ov', 'WAGENBORGEN':'Wagenborgen Gn', 'WATERINGEN':'Wateringen ZH', 'WESTERBORK':'Westerbork Dr', 'WESTKAPELLE':'Westkapelle Ze', 'WIJHE':'Wijhe Ov', 'WILLEMSTAD':'Willemstad NB', 'WINDESHEIM':'Windesheim Ov', 'WISSENKERKE':'Wissekerke Ze', 'WOLVEGA':'Wolvega Fr', 'ZANDVOORT':'Zandvoort NH', 'ZEELAND':'Zeeland NB', 'ZOETERMEER':'Zoetermeer ZH', 'ZUIDZANDE':'Zuidzande Ze', 'ZUTPHEN':'Zutphen Gl', 'ZWARTSLUIS':'Zwartsluis Ov'}
            names = ['Aalten Gl', 'Abbekerk NH', 'Angerlo Gl', 'Arum Fr', 'Austerlitz Ut', 'Bellingwolde Gn', 'Benschop Ut', 'Bleskensgraaf ZH', 'Boekel NB', 'Breda NB', 'Brielle ZH', 'Brunssum Lb', 'Delden Ov', 'Deventer Ov', 'Diemen NH', 'Dwingelo Dr', 'Echt Lb', 'Edam NH', 'Eeksterveen Dr', 'Eelde Dr', 'Eibergen Gl', 'Eijsden Lb', 'Eindhoven NB', 'Elburg Gl', 'Finsterwolde Gn', 'Formerum Fr - Formearum Fr', 'Grijpskerk Gn', 'Groenekan Ut', 'Grouw Fr - Grou Fr', 'Haamstede Ze', 'Heinkenszand Ze', 'Hoenderlo Gl', 'Hunsel Lb', 'IJmuiden NH', 'Ingen Gl', 'Joure Fr - De Jouwer Fr', 'Jutfaas Ut', 'Kampen Ov', 'Kerkrade Lb', 'Koekange Dr', 'Koevorden Dr', 'Koewacht Ze', 'Laren NH', 'Leermens Gn', 'Leeuwarden Fr - Ljouwert Fr', 'Loenen Ut', 'Maastricht Lb', 'Makkum Fr', 'Marken NH', 'Marum Gn', 'Meije ZH', 'Meppel Dr', 'Middelburg Ze', 'Monnickendam NH', 'Nijeholtpade Fr', 'Nijverdal Ov', 'Nunspeet Gl', 'Oldenzaal Ov', 'Ootmarsum Ov', 'Ouddorp ZH', 'Oudewater Ut', 'Piershil ZH', 'Posterholt Lb', 'Putten Gl', 'Raalte Ov', 'Reusel NB', 'Reuver Lb', 'Rhenen Ut', 'Rijkevoort NB', 'Rilland Ze', 'Roderwolde Dr', 'Roodeschool Gn', 'Roswinkel Dr', 'Rozendaal Gl', 'Ruinen Dr', 'Scheemda Gn', 'Scherpenzeel Fr', 'Scheveningen ZH', 'Schiermonnikoog Fr - Skiermantseach Fr', 'Sevenum Lb', 'Sliedrecht ZH', 'Slochteren Gn', 'Smilde Dr', 'Spakenburg Ut', 'Stavoren Fr - Starum Fr', 'Tegelen Lb', 'Tiel Gl', 'Tilburg NB', 'Urk Fl', 'Utrecht Ut', 'Vaals Lb', 'Vaassen Gl', 'Veenendaal Ut', 'Volendam NH', 'Vriezenveen Ov', 'Wagenborgen Gn', 'Wateringen ZH', 'Westerbork Dr', 'Westkapelle Ze', 'Wijhe Ov', 'Willemstad NB', 'Windesheim Ov', 'Wolvega Fr', 'Zandvoort NH', 'Zeeland NB', 'Zuidzande Ze', 'Zutphen Gl', 'Zwartsluis Ov']
            distance_matrix = pd.DataFrame(index = names, columns = names)

            feats = read_data(model, rename_dict)
            compute_distances(feats, model, distance_matrix)

            print("Done! Computed distances for", " ".join(model.split('/')))

if __name__ == '__main__':
    main()

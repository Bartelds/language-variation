#!/usr/bin/env python
import os
import pandas as pd
from glob import glob
from argparse import ArgumentParser
from scipy.stats import pearsonr
from natsort import natsort_keygen
from cdistance import clusterify, cdistance


def get_coords():
    """Get coordinates to compare clusterings in space """
    dat = pd.read_csv('../data/coordinates.csv', header = None, names = ['locality', 'longitude', 'latitude'])
    dat = dat.sort_values(by=['locality'])

    lats = dat['latitude'].to_list()
    lons = dat['longitude'].to_list()

    coords = []
    for lat, lon in zip (lats, lons):
        coords.append((lat, lon))

    return coords


def coph_r(labels, models, methods, save_coph):
    """Computes the cophenetic correlation coefficient for the neural models"""
    best_layers = []
    n_models = []
    n_layers = []
    clustering = []
    rs = []
    for method in methods:
        print(f"Processing {method} clustering...")
        # loading data
        nn_cophs = sorted(glob('../data-l04-106/*/*/*.tab'))
        nn_cophs = [dist for dist in nn_cophs if dist.split('.')[-4] == method]
        nn_dists = sorted(glob('../output-seg-106/*/*/*.csv'))
        nn_dists = [dist for dist in nn_dists if dist.split('/')[-3] in models]
        ld_cophs = sorted(glob('../data-l04-106/transcription-distances/ld/*.tab'))
        ld_dists = '../data/transcription-distances/GTRP_subset10_distances_LD_106.csv'

        # compute cophenetic correlation coefficient for neural models
        for a in nn_dists:
            for b in nn_cophs:
                if (a.split('/')[-3] == b.split('/')[-3]) and (a.split('/')[-2] == b.split('/')[-2]) and (b.split('.')[-4] == method):
                    a_df = pd.read_csv(a, index_col=0)
                    b_df = pd.read_csv(b, sep = '\t', index_col=0, names=labels)

                    a_np = a_df.to_numpy()
                    a_np = [j for i in a_np for j in i]
                    b_np = b_df.to_numpy()
                    b_np = [j for i in b_np for j in i]
                    r, p = pearsonr(a_np, b_np)

                    n_models.append(a.split('/')[-3])
                    n_layers.append(a.split('/')[-2])
                    rs.append(r)
                    clustering.append(method)

        # compute cophenetic correlation coefficient for ld
        for c in ld_cophs:
            if c.split('.')[-4] == method:
                d_df = pd.read_csv(ld_dists, index_col=0)
                c_df = pd.read_csv(c, sep = '\t', index_col=0, names=labels)

                d_np = d_df.to_numpy()
                d_np = [j for i in d_np for j in i]
                c_np = c_df.to_numpy()
                c_np = [j for i in c_np for j in i]
                r, p = pearsonr(d_np, c_np)

                n_models.append("LD")
                n_layers.append("LD")
                rs.append(r)
                clustering.append(method)

    coph_r = pd.DataFrame(
        {'model': n_models,
        'layer': n_layers,
        'clustering': clustering,
        'r': rs
        })
    
    # compute max value per model and write to file
    outdir = '../output/coph_r/'
    os.makedirs(outdir, exist_ok=True)

    h = coph_r.groupby(['model', 'layer'])['r'].transform(max) == coph_r['r']
    
    # write best performing layers to file
    best_layers.append(coph_r[h])
    if save_coph: coph_r[h].to_csv(outdir + "coph_highest.csv", index = False)

    # write all layers to file
    if save_coph:
        h_s = ['' if i == False else 'X' for i in h.to_list()]
        coph_r['max_r'] = h_s
        coph_r = coph_r.sort_values(by=['model', 'layer'], key=natsort_keygen())
        coph_r.to_csv(outdir + "coph.csv", index = False)
        print(coph_r)
    
    return coph_r[h]


def add_label(split_list):
    """Remaps label to numeric label"""
    n = 0
    villages = []
    clusters = []
    for list in split_list:
        for item in list:
            villages.append(item)
            clusters.append(n)
        n += 1
    
    clusters = pd.DataFrame(
            {'locality': villages,
            'label': clusters
            })
    
    return clusters


def eval_clust(data, best_layers, save):
    """Computes CDistance score between gold standard and the computed clusters"""
    # load data
    gold = pd.read_csv('../data/gold_numeric_106.csv')
    nn_cs = sorted(glob('../data-l04-106/*/*/*.n4'))

    # format best layers
    models = best_layers['model'].to_list()
    layers = best_layers['layer'].to_list()
    clustering = best_layers['clustering'].to_list()

    # init
    n_models = []
    n_layers = []
    n_clusterings = []
    cid = []

    keep_fs = []
    keep_ld = []
    for m, l, c in zip(models, layers, clustering):
        if m == 'LD':
            keep_ld.append(m + '.' + c + '.initclust.n4')
        else:
            keep_fs.append(m + '-' + l + '.' + c + '.initclust.n4')
    
    f_cs = [cs for cs in nn_cs if cs.split('/')[-1] in keep_fs]
    f_cs = f_cs + ['../data-l04-106/transcription-distances/ld/' + keep_ld[0].lower()]

    for file in f_cs:
        with open(file, 'r') as f:
            lst = f.readlines()

            # split list in list of lists where each sublist is a sep cluster
            idx_list = [idx + 1 for idx, val in enumerate(lst) if val == '\n']
            split_lst = [lst[i: j] for i, j in zip([0] + idx_list, idx_list +  ([len(lst)] if idx_list[-1] != len(lst) else []))]
            split_lst = [[item.replace('\n', '') for item in lst] for lst in split_lst]
            split_lst = [[item for item in lst if item != ''] for lst in split_lst]
            
            # label clusters and compare with gold
            c_labels = add_label(split_lst)
            c_labels = c_labels.sort_values(by = ["locality"])
            c_labels = c_labels.reset_index(drop=True)
            gold = gold.sort_values(by = ["locality"])

            # process results
            c_gold = clusterify(data, gold['label'].to_list())
            c_eval = clusterify(data, c_labels['label'].to_list())
            n_models.append('ld') if file.split('/')[-3] == 'data-l04-106' else n_models.append(file.split('/')[-3]) 
            n_layers.append(file.split('/')[-2])
            n_clusterings.append(file.split('.')[-3])
            cid.append(cdistance(c_eval, c_gold))
    
    res = pd.DataFrame(
            {'model': n_models,
            'layer': n_layers,
            'clustering': n_clusterings,
            'CDistance': cid
            })
    
    h = res.groupby(['model'])['CDistance'].transform(min) == res['CDistance']
    res = res.round({'CDistance': 2})

    # results
    res['model'] = res['model'].replace({'transcription-distances': 'ld'})
    res['layer'] = res['layer'].replace({'ld': '-'})
    print(res[h].sort_values(by = ['model', 'layer', 'clustering']).reset_index(drop=True))

    # save CDistance score per layer
    if save:
        print(f"Saving cid scores...")
        outdir = '../output/cid/'
        os.makedirs(outdir, exist_ok=True)
        res.to_csv(outdir + "cid.csv", index = False)


def main():
    parser = ArgumentParser()
    parser.add_argument("-sc", "--save_coph", default=False)
    parser.add_argument("-s", "--save", default=False)
    args = parser.parse_args()

    labels = ["Aalten Gl", "Abbekerk NH", "Angerlo Gl", "Arum Fr", "Austerlitz Ut", "Bellingwolde Gn", "Benschop Ut", "Bleskensgraaf ZH", "Boekel NB", "Breda NB", "Brunssum Lb", "Delden Ov", "Deventer Ov", "Diemen NH", "Dwingelo Dr", "Echt Lb", "Edam NH", "Eeksterveen Dr", "Eelde Dr", "Eibergen Gl", "Eijsden Lb", "Eindhoven NB", "Elburg Gl", "Finsterwolde Gn", "Formerum Fr - Formearum Fr", "Grijpskerk Gn", "Groenekan Ut", "Grouw Fr - Grou Fr", "Haamstede Ze", "Heinkenszand Ze", "Hoenderlo Gl", "Hunsel Lb", "IJmuiden NH", "Ingen Gl", "Joure Fr - De Jouwer Fr", "Jutfaas Ut", "Kampen Ov", "Kerkrade Lb", "Koekange Dr", "Koevorden Dr", "Koewacht Ze", "Laren NH", "Leermens Gn", "Leeuwarden Fr - Ljouwert Fr", "Loenen Ut", "Maastricht Lb", "Makkum Fr", "Marken NH", "Marum Gn", "Meije ZH", "Meppel Dr", "Middelburg Ze", "Monnickendam NH", "Nijeholtpade Fr", "Nijverdal Ov", "Nunspeet Gl", "Oldenzaal Ov", "Ootmarsum Ov", "Ouddorp ZH", "Oudewater Ut", "Piershil ZH", "Posterholt Lb", "Putten Gl", "Raalte Ov", "Reusel NB", "Reuver Lb", "Rhenen Ut", "Rijkevoort NB", "Rilland Ze", "Roderwolde Dr", "Roodeschool Gn", "Roswinkel Dr", "Rozendaal Gl", "Ruinen Dr", "Scheemda Gn", "Scherpenzeel Fr", "Scheveningen ZH", "Schiermonnikoog Fr - Skiermantseach Fr", "Sevenum Lb", "Sliedrecht ZH", "Slochteren Gn", "Smilde Dr", "Spakenburg Ut", "Tegelen Lb", "Tiel Gl", "Tilburg NB", "Urk Fl", "Utrecht Ut", "Vaals Lb", "Vaassen Gl", "Veenendaal Ut", "Volendam NH", "Vriezenveen Ov", "Wagenborgen Gn", "Wateringen ZH", "Westerbork Dr", "Westkapelle Ze", "Wijhe Ov", "Willemstad NB", "Windesheim Ov", "Wolvega Fr", "Zandvoort NH", "Zeeland NB", "Zuidzande Ze", "Zutphen Gl", "Zwartsluis Ov"]
    models = ['wav2vec2-large-960h', 'wav2vec2-large-nl-ft-cgn', 'wav2vec2-large-xlsr-53-ft-cgn']
    methods = ["sl", "cl", "ga", "wa", "uc", "wc", "wm"]

    data = get_coords()
    best_layers = coph_r(labels, models, methods, args.save_coph)
    eval_clust(data, best_layers, args.save)

if __name__ == '__main__':
    main()

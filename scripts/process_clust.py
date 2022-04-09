#!/usr/bin/env python
import os
import pandas as pd
from glob import glob
from argparse import ArgumentParser
from scipy.stats import pearsonr
from natsort import natsort_keygen
from cdistance import clusterify, cdistance
import collections
from sklearn import metrics


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
        ld_cophs = sorted(glob('../data-l04-106/*/*.tab'))
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

                n_models.append(ld_dists.split('/')[-4].lower())
                n_layers.append(ld_dists.split('/')[-4].lower())
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


def format_gabmap(lines):
    """ Formats the files and returns the cluster labels"""

    clusters_d = {}
    for line in lines:
        line = line.strip()
        locality = line.split('(')[1].split(')')[0]
        cluster = line.split()[-2]
        clusters_d[locality] = cluster

    clusters_d = collections.OrderedDict(sorted(clusters_d.items()))
    labels = [label.strip().lower() for _, label in clusters_d.items()]

    return labels


def check_sim_gabmap(c_labels, gabmap):
    """Computes similarity with labels produced in gabmap"""

    for distance_f in gabmap:
        if (distance_f.split('/')[-2] == c_labels[0].split('/')[-3]) and ('-'.join(distance_f.split('-')[-2:]).split('.')[0] == c_labels[0].split('/')[-2]) and (distance_f.split('.')[-2] == c_labels[0].split('.')[-3]):
            print(f"Processing {distance_f.split('/')[-1][:-4]}")
            with open(distance_f, 'r') as f:
                    lines = f.readlines()
                    labels = format_gabmap(lines)
                
                    print(f"# Similarity with gabmap: {metrics.rand_score(labels, c_labels[1])}")


def eval_clust(data, best_layers, methods, leven, save, gabmap):
    """Computes CDistance score between gold standard and the computed clusters"""

    # load data
    gold = pd.read_csv('../data/gold_numeric_106.csv')
    ld_cs = sorted(glob('../data-l04-106/ld/*.n4'))
    nn_cs = sorted(glob('../data-l04-106/*/*/*.n4'))
    gabmap_cs = sorted(glob('../data/*/*.txt'))

    # format best layers
    ## use layers that have highest correlation with pmi ld-based distances
    if leven:
        models = ['wav2vec2-large-960h'] * len(methods) + ['wav2vec2-large-nl-ft-cgn'] * len(methods) + ['wav2vec2-large-xlsr-53-dutch'] * len(methods)
        layers = ['layer-10'] * len(methods) + ['layer-11'] * len(methods) + ['layer-12'] * len(methods)
        c_methods = methods * 3

        best_layers = pd.DataFrame(
            {'model': models,
            'layer': layers,
            'clustering': c_methods,
            })
    
    models = best_layers['model'].to_list()
    layers = best_layers['layer'].to_list()
    clustering = best_layers['clustering'].to_list()

    # init
    n_models = []
    n_layers = []
    n_clusterings = []
    cid = []

    if not gabmap:
        keep_fs = []
        for m, l, c in zip(models, layers, clustering):
            if m == 'ld':
                keep_fs.append(m + '.' + c + '.initclust.n4')
            else:
                keep_fs.append(m + '-' + l + '.' + c + '.initclust.n4')
        
        f_cs = [cs for cs in (nn_cs + ld_cs) if cs.split('/')[-1] in keep_fs]

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

            # compute gabmap similarity
            if gabmap: check_sim_gabmap((file, c_labels['label'].to_list()), gabmap_cs)

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
            'CD': cid
            })
    
    res.round({'CD': 2})
    h = res.groupby(['model'])['CD'].transform(min) == res['CD']

    print(res[h].sort_values(by = ['model', 'layer', 'clustering']).reset_index(drop=True))

    if save:
        print(f"Saving cid scores...")
        outdir = '../output/cid/'
        os.makedirs(outdir, exist_ok=True)
        if leven:
            outdir = '../output/cid/ld/'
            os.makedirs(outdir, exist_ok=True)
        res.to_csv(outdir + "cid.csv", index = False)


def main():
    parser = ArgumentParser()
    parser.add_argument("-sc", "--save_coph", default=False)
    parser.add_argument("-s", "--save", default=False)
    parser.add_argument("-g", "--gabmap", default=False)
    parser.add_argument("-ld", "--leven", default=False)
    args = parser.parse_args()

    labels = ["Aalten Gl", "Abbekerk NH", "Angerlo Gl", "Arum Fr", "Austerlitz Ut", "Bellingwolde Gn", "Benschop Ut", "Bleskensgraaf ZH", "Boekel NB", "Breda NB", "Brunssum Lb", "Delden Ov", "Deventer Ov", "Diemen NH", "Dwingelo Dr", "Echt Lb", "Edam NH", "Eeksterveen Dr", "Eelde Dr", "Eibergen Gl", "Eijsden Lb", "Eindhoven NB", "Elburg Gl", "Finsterwolde Gn", "Formerum Fr - Formearum Fr", "Grijpskerk Gn", "Groenekan Ut", "Grouw Fr - Grou Fr", "Haamstede Ze", "Heinkenszand Ze", "Hoenderlo Gl", "Hunsel Lb", "IJmuiden NH", "Ingen Gl", "Joure Fr - De Jouwer Fr", "Jutfaas Ut", "Kampen Ov", "Kerkrade Lb", "Koekange Dr", "Koevorden Dr", "Koewacht Ze", "Laren NH", "Leermens Gn", "Leeuwarden Fr - Ljouwert Fr", "Loenen Ut", "Maastricht Lb", "Makkum Fr", "Marken NH", "Marum Gn", "Meije ZH", "Meppel Dr", "Middelburg Ze", "Monnickendam NH", "Nijeholtpade Fr", "Nijverdal Ov", "Nunspeet Gl", "Oldenzaal Ov", "Ootmarsum Ov", "Ouddorp ZH", "Oudewater Ut", "Piershil ZH", "Posterholt Lb", "Putten Gl", "Raalte Ov", "Reusel NB", "Reuver Lb", "Rhenen Ut", "Rijkevoort NB", "Rilland Ze", "Roderwolde Dr", "Roodeschool Gn", "Roswinkel Dr", "Rozendaal Gl", "Ruinen Dr", "Scheemda Gn", "Scherpenzeel Fr", "Scheveningen ZH", "Schiermonnikoog Fr - Skiermantseach Fr", "Sevenum Lb", "Sliedrecht ZH", "Slochteren Gn", "Smilde Dr", "Spakenburg Ut", "Tegelen Lb", "Tiel Gl", "Tilburg NB", "Urk Fl", "Utrecht Ut", "Vaals Lb", "Vaassen Gl", "Veenendaal Ut", "Volendam NH", "Vriezenveen Ov", "Wagenborgen Gn", "Wateringen ZH", "Westerbork Dr", "Westkapelle Ze", "Wijhe Ov", "Willemstad NB", "Windesheim Ov", "Wolvega Fr", "Zandvoort NH", "Zeeland NB", "Zuidzande Ze", "Zutphen Gl", "Zwartsluis Ov"]
    models = ['wav2vec2-large-960h', 'wav2vec2-large-nl-ft-cgn', 'wav2vec2-large-xlsr-53-ft-cgn']
    methods = ["sl", "cl", "ga", "wa", "uc", "wc", "wm"]

    data = get_coords()
    best_layers = coph_r(labels, models, methods, args.save_coph)

    eval_clust(data, best_layers, methods, args.leven, args.verbose, args.save, args.gabmap)
        

if __name__ == '__main__':
    main()

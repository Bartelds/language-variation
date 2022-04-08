#!/bin/bash

MODEL_NAMES="wav2vec2-large-960h wav2vec2-large-nl-ft-cgn wav2vec2-large-xlsr-53-ft-cgn"

##############################################################################
# Compute features
##############################################################################

echo "Computing features..."
for model in $MODEL_NAMES; do
    python3 extract_features_layers.py -m $model
done

##############################################################################
# Compute distances
##############################################################################

echo "Computing distances..."
python3 acoustic_distance.py

echo "Formatting distances..."
python3 format.py
python3 l04.py

##############################################################################
# Cluster distances
##############################################################################

echo "Clustering distances..."
bash cluster.sh

##############################################################################
# Evaluation
##############################################################################

echo "Computing scores..."
python3 process_clust.py

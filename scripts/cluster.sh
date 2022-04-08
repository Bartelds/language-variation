#!/bin/bash

METHOD_NAMES="sl cl ga wa uc wc wm"

echo "Clustering..."
for entry in ../data-l04-106/*/*/*.dif; do
    fname="`echo $entry | sed 's/.dif//'`"
    echo "$fname"
    for method in $METHOD_NAMES; do
        # normal clustering
        ./RuG-L04/bin/cluster -$method $entry > "${fname}.${method}.initclust"
        # cophenetic distances
        ./RuG-L04/bin/cluster -$method -c -m 4 $entry > "${fname}.${method}.coph.clust"
    done
done

echo "Creating cophenetic distance matrices..."
for entry in ../data-l04-106/*/*/*.coph.clust; do
    echo "$entry"
    # creating cophenetic distance matrix
    ./RuG-L04/bin/dif2tab $entry > "${entry}.tab"
    # remove tmp
    rm $entry
done

echo "Creating partitioning of the data"
for entry in ../data-l04-106/*/*/*.initclust; do
    echo "$entry"
    # creating cophenetic distance matrix
    ./RuG-L04/bin/clgroup -n 4 $entry > "${entry}.n4"
    # remove tmp
    rm $entry
done
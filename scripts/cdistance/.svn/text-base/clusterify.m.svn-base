% Transform data+indices into cells of points
%
% Usage: clusters = clusterify(Data, IDX)

function Clusters = clusterify(Data, IDX)

indices = unique(IDX);

Clusters  = cell(length(indices), 1);

for i=1:length(indices)
    Clusters{i} = Data(IDX == indices(i), :);
end

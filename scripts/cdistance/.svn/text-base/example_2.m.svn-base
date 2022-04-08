% Load the data for this example
% The data files contains the unclustered data ('Data'), two index
% assignments corresponding to three clusterings ('IDX1', 'IDX2')
% and three clusterings (i.e. cell arrays of data points 'S1', 'S2')
load half-heart


% Compute CDistance
fprintf('CDistance = %1.3f\n', cdistance(S1,S2));

% Plot the two clusterings

figure; clf;
colors = lines(length(S1)+length(S2)+2);
colors = colors(randperm(length(colors)),:);

for p=1:2
    subplot(1,2,p)
    if p==1
        title('Clustering 1', 'FontSize', 15); clusters = S1;
    elseif p==2
        title('Clustering 2', 'FontSize', 15); clusters = S2;
    end
        
    hold on
    for i=1:length(clusters),
        cluster = clusters{i};
        plot(cluster(:,1),cluster(:,2),'.', 'Color',colors(i,:),'MarkerSize',6);
        cluster_num_text = sprintf('%d', i);
        text(mean(cluster(:,1)), mean(cluster(:,2)), cluster_num_text, 'HorizontalAlignment', 'right', 'FontSize', 8)
    end
    axis equal;
    axis off;
    set(gca, 'Color', 'none')
    set(gcf, 'Color', 'w')
    hold off;
    drawnow;
end
% Load the data for this example
% The data files contains the unclustered data ('Data'), three index
% assignments corresponding to three clusterings ('IDX1', 'IDX2', 'IDX3')
% and three clusterings (i.e. cell arrays of data points 'S1', 'S2', and 'S3')
load three_bars_data

% Compute CDistance
fprintf('Reference vs. Change 1: CDistance = %1.3f\n', cdistance(S1,S2));
fprintf('Reference vs. Change 2: CDistance = %1.3f\n', cdistance(S1,S3));
fprintf('Change 1 vs. Change 2: CDistance = %1.3f\n', cdistance(S2,S3));

% Plot the three clusterings
shapes = ['o','>'];

figure; clf;
colors = [0,0.5,0;1,0,0];
facecolors(1,:) = colors(1,:);
facecolors(2,:) = colors(2,:);

for p=1:3
    subplot(1,3,p)
    if p==1
        title('Reference clustering', 'FontSize', 15); clusters = S1;
    elseif p==2
        title('Change 1', 'FontSize', 15); clusters = S2;
    elseif p==3
        title('Change 2', 'FontSize', 15); clusters = S3;
    end
        
    hold on
    for i=1:length(clusters),
        cluster = clusters{i};
        plot(cluster(:,1),cluster(:,2),shapes(i),'Color',colors(i,:),'MarkerSize',6, 'MarkerFaceColor', facecolors(i,:));
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
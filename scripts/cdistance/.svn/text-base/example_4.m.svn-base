% Create the data for this example
% There are three sets of data points ('Data1', 'Data2', 'Data3'), one index
% assignment corresponding to three clusterings ('labels')
% and three clusterings (i.e. cell arrays of data points 'S1', 'S2', and 'S3')

Data1 = [2 10;
         3 9;
         4 10;
         5 3;
         6 2;
         7 3;
         6 1;
         ];
     
labels = [1 1 1 2 2 2 2];
     
Data2 = [2 10;
         3 9;
         4 10;
         5 3.8;
         6 2.8;
         7 3.8;
         6 1;
         ];

Data3 = [2 10;
         3 9;
         4 10;
         5 3.85;
         6 2.85;
         7 3.85;
         6 1;
         ];

S1 = clusterify(Data1, labels);
S2 = clusterify(Data2, labels);
S3 = clusterify(Data3, labels);

fprintf('CDistance2(S1,S2) = %1.4f\n', cdistance(S1,S2))
fprintf('CDistance2(S2,S3) = %1.4f\n', cdistance(S2,S3))
fprintf('CDistance2(S3,S1) = %1.4f\n', cdistance(S3,S1))


%% Plotting code
    h = figure; clf;
    subplot(1,3,1);
    hold on;
    clusters = S1;
    cluster = clusters{1};
    plot(cluster(:,1),cluster(:,2),'.r','MarkerSize',40,'LineWidth',3);
    cluster = clusters{2};
    plot(cluster(:,1),cluster(:,2),'^b','MarkerSize',14,'LineWidth',3, 'MarkerFaceColor', 'b');
    %set(gca, 'Color', 'none')
    %set(gcf, 'Color', 'w')
    axis on
    ylim([0 12]);
    xlim([1 8]);
    xl = xlim;
    yl = ylim; 
    title('Clustering 1');
    %annotation(h,'line',[0.373 0.373], [0.966 0.0286]);
    
    subplot(1,3,2);
    hold on;
    clusters = S2;
    cluster = clusters{1};
    plot(cluster(:,1),cluster(:,2),'.r','MarkerSize',40,'LineWidth',3);
    cluster = clusters{2};
    plot(cluster(:,1),cluster(:,2),'^b','MarkerSize',14,'LineWidth',3, 'MarkerFaceColor', 'b');
    xlim(xl);
    ylim(yl)
    %set(gca, 'Color', 'none')
    %set(gcf, 'Color', 'w')
%    axis off
    title('Clustering 2');
    %annotation(h,'line',[0.654 0.654], [0.966 0.0286]);


    
    subplot(1,3,3);
    hold on;
    clusters = S3;
    cluster = clusters{1};
    plot(cluster(:,1),cluster(:,2),'.r','MarkerSize',40,'LineWidth',3);
    cluster = clusters{2};
    plot(cluster(:,1),cluster(:,2),'^b','MarkerSize',14,'LineWidth',3, 'MarkerFaceColor', 'b');
    xlim(xl);
    ylim(yl);
%    set(gca, 'Color', 'none')
%    set(gcf, 'Color', 'w')
%    axis off
    title('Clustering 3');

% Create the data for this example
% There are three sets of data points ('Data1', 'Data2', 'Data3'), one index
% assignment corresponding to three clusterings ('labels')
% and three clusterings (i.e. cell arrays of data points 'S1', 'S2', and 'S3')

Data1 = [51.9333 6.5833;
        52.7333 5.0167;
        52.0 6.1333;
        53.1333 5.4833;
        52.0833 5.3167;
        53.1167 7.1667;
        52.0 4.9833;
        51.8667 4.7833;
        51.6 5.6833;
        51.5667 4.8;
        50.95 5.9667;
        52.2667 6.7167;
        52.25 6.2;
        52.3333 4.9667;
        52.8333 6.3667;
        51.1 5.8833;
        52.5167 5.05;
        53.05 6.8;
        53.1333 6.5667;
        52.1 6.65;
        50.7833 5.7;
        51.45 5.4667;
        52.45 5.8333;
        53.2 7.0833;
        53.3833 5.3167;
        53.2667 6.3;
        52.1167 5.15;
        53.1 5.8333;
        51.7 3.75;
        51.4833 3.8167;
        52.1167 5.8833;
        51.1833 5.8167;
        52.4667 4.6;
        51.9667 5.4833;
        52.9667 5.7833;
        52.0333 5.0833;
        52.55 5.9167;
        50.8667 6.0667;
        52.7 6.3167;
        52.6667 6.75;
        51.2333 3.9667;
        52.25 5.2333;
        53.35 6.8;
        53.2 5.7833;
        52.2167 5.0167;
        50.85 5.6833;
        53.1 5.6833;
        52.5333 4.7833;
        53.15 6.2667;
        52.1167 4.7667;
        52.7 6.2;
        51.5 3.6167;
        52.4667 5.0333;
        52.9167 6.0833;
        52.3667 6.4667;
        52.3833 5.8;
        52.3167 6.9167;
        52.4167 6.9;
        51.8167 3.9333;
        52.0333 4.8833;
        51.8 4.3167;
        51.1333 6.0333;
        52.2667 5.6167;
        52.3833 6.2833;
        51.3667 5.1667;
        51.2833 6.0833;
        51.9667 5.5667;
        51.65 5.8833;
        51.4167 4.1833;
        53.1667 6.4667;
        53.4167 6.7667;
        52.8333 7.05;
        52.0167 5.9667;
        52.7667 6.35;
        53.1667 6.9667;
        52.8333 5.8833;
        52.1 4.2833;
        53.4833 6.1667;
        51.4167 6.0333;
        51.8167 4.7667;
        53.2167 6.8;
        52.95 6.45;
        52.25 5.3667;
        51.3333 6.1333;
        51.8833 5.4333;
        51.55 5.1167;
        52.6667 5.6;
        52.0833 5.1333;
        50.7667 6.0167;
        52.2833 5.9667;
        52.0333 5.55;
        52.5 5.0667;
        52.4167 6.6167;
        53.25 6.9333;
        52.0333 4.2833;
        52.85 6.6;
        51.5333 3.45;
        52.3833 6.1333;
        51.7 4.4333;
        52.45 6.1333;
        52.8833 6.0;
        52.3667 4.5333;
        51.7 5.6833;
        51.35 3.45;
        52.1333 6.2;
        52.6333 6.0833;
        ];

labels_gold = [0 1 1 2 1 0 1 1 1 1 3 0 0 1 0 3 1 0 0 0 3 1 0 0 2 0 1 2 1 1 0 3 1 1 2 1 0 1 0 0 1 1 0 1 1 3 2 1 0 1 0 1 1 0 0 0 0 0 1 1 1 3 0 0 1 3 1 1 1 0 0 0 0 0 0 0 1 2 3 1 0 0 0 3 1 1 0 1 1 0 1 1 0 0 1 0 1 0 1 0 0 1 1 1 0 0];
labels_en = [0 1 0 2 0 3 1 1 1 1 1 3 0 0 3 1 0 3 3 0 1 1 0 3 2 3 1 2 0 0 0 1 1 1 2 1 3 1 3 3 3 1 3 0 3 1 2 1 3 0 3 0 0 3 3 3 3 3 0 0 0 1 0 3 0 1 1 1 0 3 3 3 1 3 3 3 1 2 1 1 3 3 0 1 1 1 0 0 1 0 0 1 3 3 1 3 0 3 1 3 3 1 1 3 0 3];
labels_nl = [0 0 0 1 0 2 0 0 3 0 3 2 0 0 2 3 0 2 2 0 3 0 0 2 1 2 0 1 0 0 0 3 0 0 1 0 2 3 2 2 2 0 2 0 0 3 1 0 2 0 2 0 0 2 2 2 2 2 0 0 0 3 0 2 3 3 0 3 0 2 2 2 3 2 2 2 0 1 3 0 2 2 0 3 0 0 0 0 3 0 0 0 2 2 0 2 0 2 0 2 2 0 3 2 0 2];
labels_xlsr = [0 1 1 2 1 0 1 1 1 1 1 0 0 1 0 1 1 0 0 0 1 1 0 0 2 0 1 2 1 0 1 1 1 1 2 1 0 1 0 0 1 1 0 1 1 1 2 1 0 1 0 1 1 0 0 0 0 0 0 1 1 1 1 0 1 1 1 3 3 0 0 0 3 0 0 0 1 2 3 1 0 0 1 1 1 1 3 1 3 0 1 1 0 0 1 0 3 0 1 0 0 1 1 3 1 0];
labels_ld = [0 1 0 2 0 3 1 0 1 1 1 3 0 0 3 1 1 3 3 3 1 1 0 3 2 3 1 2 0 0 0 1 1 1 2 1 3 1 3 3 0 1 3 0 1 1 2 1 3 0 3 0 1 3 3 3 3 3 0 0 0 1 0 3 1 1 1 1 0 3 3 3 1 3 3 3 1 2 1 1 3 3 0 1 1 1 0 0 1 0 0 1 3 3 1 3 0 3 1 3 3 1 1 3 0 3];

gold = clusterify(Data1, labels_gold);
en = clusterify(Data1, labels_en);
nl = clusterify(Data1, labels_nl);
xlsr = clusterify(Data1, labels_xlsr);
ld = clusterify(Data1, labels_ld);

fprintf('CDistance2(en,gold) = %1.4f\n', cdistance(en,gold))
fprintf('CDistance2(nl,gold) = %1.4f\n', cdistance(nl,gold))
fprintf('CDistance2(xlsr,gold) = %1.4f\n', cdistance(xlsr,gold))
fprintf('CDistance2(ld,gold) = %1.4f\n', cdistance(ld,gold))

// %% Plotting code
//     h = figure; clf;
//     subplot(1,3,1);
//     hold on;
//     clusters = S1;
//     cluster = clusters{1};
//     plot(cluster(:,1),cluster(:,2),'.r','MarkerSize',40,'LineWidth',3);
//     cluster = clusters{2};
//     plot(cluster(:,1),cluster(:,2),'^b','MarkerSize',14,'LineWidth',3, 'MarkerFaceColor', 'b');
//     %set(gca, 'Color', 'none')
//     %set(gcf, 'Color', 'w')
//     axis on
//     ylim([0 12]);
//     xlim([1 8]);
//     xl = xlim;
//     yl = ylim; 
//     title('Clustering 1');
//     %annotation(h,'line',[0.373 0.373], [0.966 0.0286]);
    
//     subplot(1,3,2);
//     hold on;
//     clusters = S2;
//     cluster = clusters{1};
//     plot(cluster(:,1),cluster(:,2),'.r','MarkerSize',40,'LineWidth',3);
//     cluster = clusters{2};
//     plot(cluster(:,1),cluster(:,2),'^b','MarkerSize',14,'LineWidth',3, 'MarkerFaceColor', 'b');
//     xlim(xl);
//     ylim(yl)
//     %set(gca, 'Color', 'none')
//     %set(gcf, 'Color', 'w')
// %    axis off
//     title('Clustering 2');
//     %annotation(h,'line',[0.654 0.654], [0.966 0.0286]);


    
//     subplot(1,3,3);
//     hold on;
//     clusters = S3;
//     cluster = clusters{1};
//     plot(cluster(:,1),cluster(:,2),'.r','MarkerSize',40,'LineWidth',3);
//     cluster = clusters{2};
//     plot(cluster(:,1),cluster(:,2),'^b','MarkerSize',14,'LineWidth',3, 'MarkerFaceColor', 'b');
//     xlim(xl);
//     ylim(yl);
// %    set(gca, 'Color', 'none')
// %    set(gcf, 'Color', 'w')
// %    axis off
//     title('Clustering 3');

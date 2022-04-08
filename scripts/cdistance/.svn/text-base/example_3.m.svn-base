% This example computes the circle example in the paper.
clear;

% Make a bunch of evenly spaced points on a circle
% n = 200;
n = 28;
s = linspace(0,1,n+1);
s = s(1:(end-1)); % because 0 is identified with 1
s = s';
assert(size(s,1)==n);
  

% In the loop below,
% AA is a fixed clustering of the points into two clusters, the upper and lower hemispheres.
% BB is a rotated version; each iteration the degree of rotation increases
dss=zeros((n/2)+1,1);
for k=1:((n/2)+1)

  % make AA and BB
  AA={s(1:(n/2),:), s(((n/2)+1):n,:)};
  BB={s(k:((n/2)+k-1),:), s([((n/2)+k):n 1:(k-1)],:)};

  % display them
  BB{1};
  BB{2};
  %fprintf('----\n');
  
  % plot BB
  subplot(4,4,k);
  A=BB{1}; plot(cos(2*pi*A),sin(2*pi*A),'b^'); hold on;
  A=BB{2}; plot(cos(2*pi*A),sin(2*pi*A),'r.'); hold off;
  axis equal

  % plot AA
  subplot(4,4,15);
  A=AA{1}; plot(cos(2*pi*A),sin(2*pi*A),'b^'); hold on;
  A=AA{2}; plot(cos(2*pi*A),sin(2*pi*A),'r.'); hold off;
  axis equal

  % geodesic interpoint distance function
  %domega_fxn = @(X,Y) mod(reshape(reshape(repmat(X', size(Y,1), 1), size(X,1)*size(Y,1), 1) - reshape(repmat(Y, 1, size(X,1)), size(X,1)*size(Y,1),1), size(X,1), size(Y,1)), 1);
  domega_fxn = @(x,y) mod(x-y,1);
  % % euclidean interpoint distance function
  % domega_fxn = @(x,y) norm(exp(2*pi*i*x)-exp(2*pi*i*y));

  % compute clustering distance
  dss(k) = cdistance(AA,BB,domega_fxn);
end

% plot the clustering distances for all the clusterings above
subplot(4,4,16);
plot(s(1:((n/2)+1)),dss,'o-'); hold on;

function d=cdistance(S1,S2,varargin)
% CDISTANCE   Computes the clustering distance between clusterings S1 and S2.
%
% Input: 
% - S1     = cell array of clusters {S1_1,...,S1_M}
%            where S1_i = matrix of points in cluster i,
%            in which each row is a point.
%            This represents a clustering 
%            (\mathcal A in the paper).
% - S2     = similar, for the other clustering
%            (\mathcal B in the paper).
% - (optional) domega = (ground distance) function that returns distance
%                       between any two points x and y. Default is Euclidean
%            (d_\Omega in the paper)
%
% Output:
% - d      = the clustering distance between S1 and S2.


if nargin > 2
    domega_fxn = varargin{1};
end

%% Step 1 - build pairwise optimal transport distance matrix between
% clusters
M=length(S1);
N=length(S2);
dprime=zeros(M,N);
for i=1:M
  for j=1:N
    A=S1{i}; B=S2{j};
    m=size(A,1); n=size(B,1);

    % Compute (ground) distance matrix between points of each cluster
	 if nargin > 2
	 	domega = zeros(m,n);
	 	for i2=1:m
	 		for j2=1:n
				domega(i2,j2) = domega_fxn(A(i,:), B(j,:));
			end
	 	end
	 else
    	domega = fast_euclidean(A,B);
	 end
    % Assign equal weight to each point
    p = ones(1,m)/m;
    q = ones(1,n)/n;

    % Compute dprime entry
    dprime(i,j)=ot_mex(domega,p,q,Inf); % p and q
  end
end


%% Step 2
% Assign weights to each point in new space, proportionate to number of
% points in corresponding cluster
p=zeros(1,M); for i=1:M, p(i)=size(S1{i},1); end; p=p/sum(p);
q=zeros(1,N); for j=1:N, q(j)=size(S2{j},1); end; q=q/sum(q);

% Compute Optimal Transportation Distance
d_optimal=ot_mex(dprime,p,q,Inf);

% Compute Naive Transportation Distance
d_naive=0;
for i=1:M
  for j=1:N
    d_naive=d_naive+p(i)*q(j)*dprime(i,j);
  end
end

% Compute CDistance
d=d_optimal/d_naive;

return
end

function dists = fast_euclidean(C1,C2)
    m = size(C1,1); n = size(C2,1);
    dists = zeros(m,n);
    for i=1:m
        point = C1(i,:);
        repped_point = point(ones(1,n),:); % equiv. to repmat(C1(i,:),n,1);
        dists(i,:) = sqrt(sum((C2-repped_point).^2,2)); 
        % Take the difference of coordinates, square the 
        % differences, add them per row, take the square root
        % and return the resulting row.
    end
return
end


function r = runpv(alg,varargin)
newargin = varargin;
for(i=1:nargin-1)
    if isa(varargin{i},'MantidWorkspace')
        newargin{i} = name(varargin{i});
    end
end
r = MantidMatlabAPI('Algorithm', 'RunPV_varargin', alg.ptr, newargin);

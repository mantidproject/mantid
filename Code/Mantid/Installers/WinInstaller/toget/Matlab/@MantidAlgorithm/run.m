function r = run(alg,properties,varargin)
argstr = properties;
%sep=';';
%for(i=1:nargin-2)
%    if isa(varargin{i},'MantidWorkspace')
%		bit = strcat(sep,name(varargin{i}));
%	elseif ischar(varargin{i})
%        bit = strcat(sep, varargin{i});
%	else
%        bit = strcat(sep, num2str(varargin{i}));
%	end
%	argstr=strcat(argstr,bit);
%end
r = MantidMatlabAPI('Algorithm', 'Run_varargin', alg.ptr, argstr);

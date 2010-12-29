function res = MantidFrameworkManager( varargin )
res.ptr = int64(0);
res = class(res,'MantidFrameworkManager');
res.ptr = MantidMatlabAPI('FrameworkManager', 'Create', res, varargin);

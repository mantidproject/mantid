function res = MantidFrameworkManager( varargin )
res.ptr = int64(0);
res = class(res,'MantidFrameworkManager');
res.ptr = mantidexec('FrameworkManager', 'Create', res, varargin);

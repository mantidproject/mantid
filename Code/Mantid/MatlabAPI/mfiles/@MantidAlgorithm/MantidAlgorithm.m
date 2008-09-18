function res = MantidAlgorithm( name )
res.name = name;
res.ptr = int64(0);
res = class(res, 'MantidAlgorithm');
res.ptr = mantidexec('Algorithm', 'Create', name);

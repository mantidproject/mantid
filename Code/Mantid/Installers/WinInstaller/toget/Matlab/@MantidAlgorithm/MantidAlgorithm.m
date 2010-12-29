function res = MantidAlgorithm(name)
res.name = name;
res.ptr = int64(0);
res = class(res, 'MantidAlgorithm');
res.ptr = MantidMatlabAPI('Algorithm', 'Create', name);

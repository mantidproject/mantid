%
% Third_Party directory needs to be set in DOS PATH variable - see run_matlab.dat
%
mantidroot='../../';
addpath(strcat(mantidroot,'MatlabAPI/mfiles'),strcat(mantidroot,'debug'),strcat(mantidroot,'matlabAPI/test'));
MantidMatlabAPI('SimpleAPI','Create');
addpath(strcat(mantidroot,'MatlabAPI/mfiles/MantidSimpleAPI'));
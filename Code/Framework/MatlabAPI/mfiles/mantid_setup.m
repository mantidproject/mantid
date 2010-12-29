%
% Third_Party directory needs to be set in DOS PATH variable - see run_matlab.bat
%
mantidroot='../../';
addpath(strcat(mantidroot,'MatlabAPI/mfiles'),strcat(mantidroot,'release'),strcat(mantidroot,'matlabAPI/test'));
addpath(strcat(mantidroot,'MatlabAPI/mfiles/MantidGlobal'))
MantidMatlabAPI('SimpleAPI','Create');
addpath(strcat(mantidroot,'MatlabAPI/mfiles/MantidSimpleAPI'));
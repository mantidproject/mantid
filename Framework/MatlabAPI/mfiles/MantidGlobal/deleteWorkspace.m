function deleteWorkspace( name )
% deleteWorkspace(name) - Delete the the named workspace from the framework
% Removes the specified workspace from the service

if ( nargin == 1 )
    MantidMatlabAPI('FrameworkManager','DeleteWorkspace',name)
else
    fprintf('Error: deleteWorkpace requires only 1 argument\n')
end
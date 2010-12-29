%    val = get(var, 'field')  % returns field
%    val = get(var)           % returns structure of object contents 
%
% This is a generic method for Mantid classes ($Revision: 254 $)
% Only edit the master version in "mfiles/generic"
%
function r = get(self,field)
if (nargin == 1)
    r = MantidMatlabAPI('Workspace','GetAllFields',self.name);
    return
end
if (~ischar(field))
    error('GET: field name must be a character string')
end
r = MantidMatlabAPI('Workspace','GetField',self.name,field);

%    var = set(var, 'field1', val1, 'field2, val2, ... )
%    var = set(var, struct_val )
%
% sets the required fields and then checks the object for consistency
%
% This is a generic method for Mantid ($Revision: 245 $)
% Only edit the master version in "mfiles/generic"
%
function r = set(self,varargin)
narg = length(varargin);
if nargin == 2
    if (isstruct(varargin{1}))
        svar = varargin{1};
        names = fieldnames(svar);
        for i = 1:length(names)
            field = names{i};
            self.(field) = svar.(field);
        end
        res = libisisexc(class(self), 'check', self);
        if (res == 0)
            r = self;
        else
            error('SET: invalid structure argument given')
        end    
    else
        error('SET: second arg must be a structure if only two arguments are given')
    end
    return
end
if (rem(narg,2) ~= 0)
    error('SET: incomplete set of (field,value) pairs given')
end
i = 1;
while i < narg
    name = varargin{i};
    if (~ischar(name));
        error('SET: field name must be a character string')
    end
    new_value = varargin{i+1};
    old_value = self.(name);
    if ( strcmp(class(new_value), 'double') & strcmp(class(old_value), 'int32') )
        new_value = int32(new_value);
    end
    if ( strcmp(class(new_value), class(old_value)) == 0 )
        error('SET: cannot alter the type of a field')
    end
    self.(name) = new_value;
    i = i + 2;
end
res = libisisexc(class(self), 'check', self);
if (res == 0)
    r = self;
else
    error('SET: invalid arguments given')
end

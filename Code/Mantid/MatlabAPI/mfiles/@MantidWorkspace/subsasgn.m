%
% Implements
%                  var.field = val
% by calling
%                  var = set(var, field, val)
%
% This is a generic method for Mantid ($Revision: 606 $)
% Only edit the master version in "mfiles/generic_methods"
%
function r = subsasgn(r, s, b)
switch s(1).type
  case '.'  % self.field syntax
    if (length(s) > 1)
        r.(s(1).subs) = subsasgn(r.(s(1).subs), s(2:end), b);
    else
        r = set(r, s(1).subs, b);
%        r.(s(1).subs) = b;
%        res = libisisexc(class(r), 'check', r); % now done in set
    end
  case '()'  % self() syntax
    if (length(s) > 1)
        r(s(1).subs{:}) = subsasgn(r(s(1).subs{:}), s(2:end), b);
    else
        r(s(1).subs{:}) = b;
%        res = libisisexc(class(r), 'check', r);
    end
  case '{}'  % self{} syntax - sould i really be using {} here rather than ()?
    if (length(s) > 1)
        r(s(1).subs{:}) = subsasgn(r(s(1).subs{:}), s(2:end), b);
    else
        r(s(1).subs{:}) = b;
%        res = libisisexc(class(r), 'check', r);
    end
  otherwise
    error('invalid subsasgn syntax')       
end

% Perform some algorithms
LoadRaw('../../../../Test/Data/csp78173.raw','test');
ConvertUnits('test','converted','dSpacing');
Rebin('converted','rebinned','0.1,0.001,5');

% clear up intermediate workspaces
%mtd.deleteWorkspace('test')
%mtd.deleteWorkspace('converted')

% extract the one we want
w = MantidWorkspace('test');

plot(w.x(1:end-1),w.y)
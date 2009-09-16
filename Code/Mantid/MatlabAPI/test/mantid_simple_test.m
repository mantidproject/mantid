% Perform some algorithms
LoadRaw('../../../../Test/Data/HET15869.raw','raw');
ConvertUnits('raw','converted','dSpacing');
Rebin('converted','rebinned','0.1,0.001,5');

% clear up intermediate workspaces
%mtd.deleteWorkspace('test')
%mtd.deleteWorkspace('converted')

% extract the one we want
w = MantidWorkspace('raw');

plot(w.x(1:end-1,4:9),w.y(:,4:9))
figure(2)
surf( w.x(1:50, 1:50), w.x(1:50, 1:50)', w.y(1:50, 1:50) )
function res = LoadRaw( self, file, name )
MantidMatlabAPI('LoadRaw', 'Run', file, name);
res = MantidWorkspace(name);

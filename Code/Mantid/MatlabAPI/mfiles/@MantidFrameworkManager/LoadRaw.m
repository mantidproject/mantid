function res = LoadRaw( self, file, name )
mantidexec('LoadRaw', 'Run', file, name);
res = MantidWorkspace(name);

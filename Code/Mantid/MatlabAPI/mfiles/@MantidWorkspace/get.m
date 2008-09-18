function res = get(self, field)
res = mantidexec('Workspace','GetField', self.name, field);

# Muon context - contains all of the values from the GUI

# constant variable names
CurrentFile = "InputWorkspace"
LoadedFiles = "LoadedFiles"
Groups = "groups"
Pairs = "pairs"

class MuonContext(object):
    def __init__(self):
        self.common_context = {}
        self.common_context[CurrentFile] = " "
        self.common_context[LoadedFiles] = []
        self.common_context[Groups] = None
        self.common_context[pairs] = None


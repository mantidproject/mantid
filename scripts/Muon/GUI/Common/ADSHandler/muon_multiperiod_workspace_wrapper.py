class MuonMultiPeriodWorkspaceWrapper():
    def __init__(self):
        self.muon_workspace_list = []

    @property
    def is_hidden(self):
        hidden = True
        for muon_workspace in self.muon_workspace_list:
            hidden = hidden & muon_workspace.is_hidden
        return hidden
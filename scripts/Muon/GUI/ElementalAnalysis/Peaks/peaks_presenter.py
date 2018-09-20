class PeaksPresenter(object):
    def __init__(self, view):
        self.view = view
        self.major = self.view.major
        self.minor = self.view.minor
        self.gamma = self.view.gamma
        self.electron = self.view.electron
        self.peak_checkboxes = self.view.peak_checkboxes

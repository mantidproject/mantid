class DetectorsPresenter(object):

    def __init__(self, view):
        self.view = view
        self.detectors = []
        for name in self.view.widgets.keys():
            self.detectors.append(self.view.widgets[name])

    def setStateQuietly(self, name, state):
        self.view.setStateQuietly(name, state)

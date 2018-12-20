class DetectorsPresenter(object):
    def __init__(self, view):
        self.view = view
        self.detectors = [
            self.view.GE1,
            self.view.GE2,
            self.view.GE3,
            self.view.GE4]

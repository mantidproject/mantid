class PeriodicTablePresenter(object):
    def __init__(self, view):
        self.view = view

    @property
    def widget(self):
        return self.view

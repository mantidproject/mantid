class PeriodicTablePresenter(object):
    def __init__(self, view, model):
        self.view = view
        self.model = model

    @property
    def widget(self):
        return self.view

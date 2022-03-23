
class CutViewerPresenter:
    def __init__(self, view):
        """
        :param painter: An object responsible for drawing the representation of the cut
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        self.view = view
        # self.model = CutViewerModel(presenter)

    def on_dimension_changed(self):
        self.view.reset_table_data()

    def on_slicepoint_changed(self):
        self.view.update_slicepoint()

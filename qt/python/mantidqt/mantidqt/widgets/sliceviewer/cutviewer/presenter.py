from mantidqt.widgets.sliceviewer.cutviewer.view import CutViewerView
# from mantidqt.widgets.sliceviewer.cutviewer.model import CutViewerModel


class CutViewerPresenter:
    def __init__(self, sliceinfo_provider, canvas):
        """
        :param painter: An object responsible for drawing the representation of the cut
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        self.view = CutViewerView(canvas,  sliceinfo_provider)
        # self.model = CutViewerModel(self)

    def on_dimension_changed(self):
        self.view.reset_table_data()

    def on_slicepoint_changed(self):
        self.view.update_slicepoint()

    def on_cut_done(self, wsname):
        self.view.plot_cut_ws(wsname)

    def show_view(self):
        self.view.show()

    def hide_view(self):
        self.view.hide()

    def get_view(self):
        return self.view

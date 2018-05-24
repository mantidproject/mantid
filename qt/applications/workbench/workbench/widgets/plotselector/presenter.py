from __future__ import absolute_import, print_function

from .model import PlotSelectorModel


# class IPlotSelectorView(object):
#     """
#     The interface to the actual plot selector view.
#     Presenter interacts with the view through this interface only.
#     """
#     def __init__(self):
#         # Actual view creates its ui elements
#         self.init_ui()
#         # The view will have a reference to the Presenter
#         self.presenter = PlotSelectorPresenter(self)
#
#     def init_ui(self):
#         raise NotImplementedError('Method has to be implemented in a subclass')
#
#     def populate_ui(self, data):
#         raise NotImplementedError('Method has to be implemented in a subclass')


class PlotSelectorPresenter(object):
    """
    Presents (controls) a plot selector view. This UI element allows the user
    to select and make active a plot.
    """
    def __init__(self, view):
        self.view = view
        self.model = PlotSelectorModel(self)
        self.view.init_ui()
        plot_list = self.model.get_plot_list()
        self.view.set_plot_list(plot_list)

    def update_plot_list(self):
        plot_list = self.model.get_plot_list()
        self.view.set_plot_list(plot_list)

    def make_plot_active(self, plot_name):
        self.model.make_plot_active(plot_name)

from __future__ import absolute_import, print_function

from workbench.plotting.currentfigure import CurrentFigure

class PlotSelectorModel(object):
    """
    This is a model for the plot selector widget.
    """

    def __init__(self, presenter):
        """
        Initialise a new instance of PlotSelectorModel
        :param presenter: A presenter controlling this model.
        """
        self.presenter = presenter
        self.plot_list = []

    def get_plot_list(self):
        plot_names = []
        figures = CurrentFigure.get_all_fig_managers()
        for figure in figures:
            plot_names.append(figure.get_window_title())
        return plot_names

    def make_plot_active(self, plot_name):
        CurrentFigure.bring_to_front_by_name(plot_name)

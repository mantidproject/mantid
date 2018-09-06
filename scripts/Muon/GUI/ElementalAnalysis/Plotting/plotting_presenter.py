class PlotPresenter(object):
    def __init__(self, view):
        self.view = view

    def update_canvas(self):
        """ Redraws the canvas. """
        self.view.canvas.draw()

    def get_subplot(self, name):
        """
        Returns the subplot with the given name.

        :param name: the name of the subplot
        :returns: a matplotlib subplot object
        :raises KeyError: if the subplot name doesn't exist
        """
        return self.view.get_subplot(name)

    def get_subplots(self):
        """ Returns a dictionary of subplots {name: subplot} """
        return self.view.get_subplots()

    def add_subplot(self, name):
        """
        Adds a subplot to the plotting window.

        :param name: the name to assign to the new add_subplot
        :returns: a matplotlib subplot object
        """
        return self.view.add_subplot(name)

    def plot(self, name, workspace):
        """
        Plots a workspace to a subplot.

        :param name: the name of the subplot to plot on
        :param workspace: the workspace to plot
        :raises KeyError: if the subplot name doesn't exist
        """
        self.view.plot(name, workspace)

    def remove_subplot(self, name):
        """ Removes the subplot corresponding to 'name' from the plotting window """
        self.view.remove_subplot(name)

    def add_vline(self, plot_name, x_value, y_min, y_max, **kwargs):
        """
        Adds a vertical line to a plot.

        :param plot_name: the plot on which to add the line
        :param x_value: the x value for the axvline
        :param y_min: 0 <= y_min <= 1. The minimum y-value of the line (multiple of the y-axis)
        :param y_min: 0 <= y_max <= 1. The maximum y-value of the line (multiple of the y-axis)
        :param **kwargs: any keyword arguments for the matplotlib line object
        :returns: a matplotlib line object
        :raise KeyError: if the subplot plot_name does not exist
        """
        return self.view.add_vline(plot_name, x_value, y_min, y_max, **kwargs)

    def add_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        """
        Adds a horizontal line to a plot.

        :param plot_name: the plot on which to add the line
        :param y_value: the y value for the axvline
        :param x_min: 0 <= x_min <= 1. The minimum x-value of the line (multiple of the x-axis)
        :param x_min: 0 <= x_max <= 1. The maximum x-value of the line (multiple of the x-axis)
        :param **kwargs: any keyword arguments for the matplotlib line object
        :returns: a matplotlib line object
        :raise KeyError: if the subplot plot_name does not exist
        """
        return self.view.add_hline(plot_name, y_value, x_min, x_max, **kwargs)

    def add_moveable_vline(self, plot_name, x_value, y_minx, y_max, **kwargs):
        pass

    def add_moveable_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        pass

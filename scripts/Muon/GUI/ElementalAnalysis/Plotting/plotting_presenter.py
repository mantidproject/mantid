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

    def add_moveable_vline(self, plot_name, x_value, y_minx, y_max, **kwargs):
        pass

    def add_moveable_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        pass

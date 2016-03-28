import mplgraphicsview

import numpy as np
from matplotlib import pyplot as plt


class Detector2DView(mplgraphicsview.MplGraphicsView):
    """
    Customized 2D detector view
    """
    def __init__(self, parent):
        """

        :param parent:
        :return:
        """
        mplgraphicsview.MplGraphicsView.__init__(self, parent)

        # connect the mouse motion to interact with the canvas
        self._myCanvas.mpl_connect('button_press_event', self.on_mouse_press_event)
        self._myCanvas.mpl_connect('button_release_event', self.on_mouse_release_event)
        self._myCanvas.mpl_connect('motion_notify_event', self.on_mouse_motion)

        # class variables
        self._myPolygon = None

        return

    def add_roi(self):
        """ Add region of interest
        :return:
        """
        vertex_array = np.ndarray(shape=(4, 2))
        vertex_array[0][0] = 10.
        vertex_array[0][1] = 10.
        vertex_array[1][0] = 10.
        vertex_array[1][1] = 20.
        vertex_array[2][0] = 20.
        vertex_array[2][1] = 20.
        vertex_array[3][0] = 20.
        vertex_array[3][1] = 10.

        # TODO - Refactor to Mpl2DGraphicsview as draw_polygon()
        # TODO - create an Art_ID system in Mpl2dGraphicsView to manage artists.
        p = plt.Polygon(vertex_array, fill=False, color='w')
        self._myCanvas.axes.add_artist(p)

        # register
        self._myPolygon = p

        # Flush...
        self._myCanvas._flush()

        return

    def remove_roi(self):
        """
        Remove the rectangular for region of interest
        :return:
        """
        self._myPolygon.remove()

        self._myCanvas._flush()

        return

    def get_roi(self):
        """
        :return: A list for polygon0
        """

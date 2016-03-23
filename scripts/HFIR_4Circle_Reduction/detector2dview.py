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

        p = plt.Polygon(vertex_array, fill=False, color='w')
        self._myCanvas.axes.add_artist(p)

        # Flush...
        self._myCanvas._flush()

        return

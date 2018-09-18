from __future__ import (absolute_import, division, print_function)

from matplotlib.backends.backend_qt4agg import NavigationToolbar2QT as NavigationToolbar


class myToolbar(NavigationToolbar):
    # only display the buttons we need
    toolitems = [t for t in NavigationToolbar.toolitems if
                 t[0] in ("Home","Save","Pan","Zoom" )]

    def __init__(self, *args, **kwargs):
        super(myToolbar, self).__init__(*args, **kwargs)
        self.layout().takeAt(5)  #or more than 1 if you have more buttons

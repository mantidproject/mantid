import NTableWidget as tableBase


class UBMatrixTable(tableBase.NTableWidget):
    """
    Extended table for UB matrix
    """
    def __init__(self, parent):
        """

        :param parent:
        :return:
        """
        tableBase.NTableWidget.__init__(self, parent)

        return

    def setup(self):
        """
        Init setup
        :return:
        """
        # self.init_size(3, 3)

        for i in xrange(3):
            for j in xrange(3):
                self.set_value_cell(i, j)

        return

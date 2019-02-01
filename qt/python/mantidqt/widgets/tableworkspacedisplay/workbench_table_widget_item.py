from qtpy.QtCore import Qt
from qtpy.QtWidgets import QTableWidgetItem

from mantid.kernel import V3D


class WorkbenchTableWidgetItem(QTableWidgetItem):
    def __init__(self, data, editable=False):
        # if not editable just initialise the ItemWidget as string
        if isinstance(data, V3D):
            self.is_v3d = True
        else:
            self.is_v3d = False

        if not editable:
            QTableWidgetItem.__init__(self, str(data))
            self.setFlags(self.flags() & ~Qt.ItemIsEditable)
            return

        QTableWidgetItem.__init__(self)

        if isinstance(data, V3D):
            data = str(data)

        self.display_data = data
        # this will correctly turn all number cells into number types
        self.reset()

    def _get_v3d_from_str(self, string):
        if '[' in string and ']' in string:
            string = string[1:-1]
        if ',' in string:
            return V3D(*[float(x) for x in string.split(',')])
        else:
            raise RuntimeError("'{}' is not a valid V3D string.".format(string))

    def reset(self):
        self.setData(Qt.DisplayRole, self.display_data)

    def update(self):
        self.display_data = self.data(Qt.DisplayRole)

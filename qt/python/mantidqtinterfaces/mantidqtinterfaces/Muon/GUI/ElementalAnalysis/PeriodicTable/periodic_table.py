# This is a modified version of silx.gui.widgets.PeriodicTable
# It allows the creation of a Periodic Table Qt widget with colouring and buttons for each element
# Functionality added:
# - Enabling/Disabling element buttons
# - Left/Right click independent events

from collections import OrderedDict
import logging
from qtpy import QtGui, QtWidgets, QtCore
import json
import copy

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel

# coding: utf-8
# /*##########################################################################
#
# Copyright (c) 2004-2018 European Synchrotron Radiation Facility
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# ###########################################################################*/
__authors__ = ["E. Papillon", "V.A. Sole", "P. Knobel"]
__license__ = "MIT"
__date__ = "26/01/2017"

_logger = logging.getLogger(__name__)

#             Symbol  Atomic Number   col row  name  mass subcategory
_elements = [
    ("H", 1, 1, 1, "hydrogen", 1.00800, "diatomic nonmetal"),
    ("He", 2, 18, 1, "helium", 4.0030, "noble gas"),
    ("Li", 3, 1, 2, "lithium", 6.94000, "alkali metal"),
    ("Be", 4, 2, 2, "beryllium", 9.01200, "alkaline earth metal"),
    ("B", 5, 13, 2, "boron", 10.8110, "metalloid"),
    ("C", 6, 14, 2, "carbon", 12.0100, "polyatomic nonmetal"),
    ("N", 7, 15, 2, "nitrogen", 14.0080, "diatomic nonmetal"),
    ("O", 8, 16, 2, "oxygen", 16.0000, "diatomic nonmetal"),
    ("F", 9, 17, 2, "fluorine", 19.0000, "diatomic nonmetal"),
    ("Ne", 10, 18, 2, "neon", 20.1830, "noble gas"),
    ("Na", 11, 1, 3, "sodium", 22.9970, "alkali metal"),
    ("Mg", 12, 2, 3, "magnesium", 24.3200, "alkaline earth metal"),
    ("Al", 13, 13, 3, "aluminium", 26.9700, "post transition metal"),
    ("Si", 14, 14, 3, "silicon", 28.0860, "metalloid"),
    ("P", 15, 15, 3, "phosphorus", 30.9750, "polyatomic nonmetal"),
    ("S", 16, 16, 3, "sulphur", 32.0660, "polyatomic nonmetal"),
    ("Cl", 17, 17, 3, "chlorine", 35.4570, "diatomic nonmetal"),
    ("Ar", 18, 18, 3, "argon", 39.9440, "noble gas"),
    ("K", 19, 1, 4, "potassium", 39.1020, "alkali metal"),
    ("Ca", 20, 2, 4, "calcium", 40.0800, "alkaline earth metal"),
    ("Sc", 21, 3, 4, "scandium", 44.9600, "transition metal"),
    ("Ti", 22, 4, 4, "titanium", 47.9000, "transition metal"),
    ("V", 23, 5, 4, "vanadium", 50.9420, "transition metal"),
    ("Cr", 24, 6, 4, "chromium", 51.9960, "transition metal"),
    ("Mn", 25, 7, 4, "manganese", 54.9400, "transition metal"),
    ("Fe", 26, 8, 4, "iron", 55.8500, "transition metal"),
    ("Co", 27, 9, 4, "cobalt", 58.9330, "transition metal"),
    ("Ni", 28, 10, 4, "nickel", 58.6900, "transition metal"),
    ("Cu", 29, 11, 4, "copper", 63.5400, "transition metal"),
    ("Zn", 30, 12, 4, "zinc", 65.3800, "transition metal"),
    ("Ga", 31, 13, 4, "gallium", 69.7200, "post transition metal"),
    ("Ge", 32, 14, 4, "germanium", 72.5900, "metalloid"),
    ("As", 33, 15, 4, "arsenic", 74.9200, "metalloid"),
    ("Se", 34, 16, 4, "selenium", 78.9600, "polyatomic nonmetal"),
    ("Br", 35, 17, 4, "bromine", 79.9200, "diatomic nonmetal"),
    ("Kr", 36, 18, 4, "krypton", 83.8000, "noble gas"),
    ("Rb", 37, 1, 5, "rubidium", 85.4800, "alkali metal"),
    ("Sr", 38, 2, 5, "strontium", 87.6200, "alkaline earth metal"),
    ("Y", 39, 3, 5, "yttrium", 88.9050, "transition metal"),
    ("Zr", 40, 4, 5, "zirconium", 91.2200, "transition metal"),
    ("Nb", 41, 5, 5, "niobium", 92.9060, "transition metal"),
    ("Mo", 42, 6, 5, "molybdenum", 95.9500, "transition metal"),
    ("Tc", 43, 7, 5, "technetium", 99.0000, "transition metal"),
    ("Ru", 44, 8, 5, "ruthenium", 101.0700, "transition metal"),
    ("Rh", 45, 9, 5, "rhodium", 102.9100, "transition metal"),
    ("Pd", 46, 10, 5, "palladium", 106.400, "transition metal"),
    ("Ag", 47, 11, 5, "silver", 107.880, "transition metal"),
    ("Cd", 48, 12, 5, "cadmium", 112.410, "transition metal"),
    ("In", 49, 13, 5, "indium", 114.820, "post transition metal"),
    ("Sn", 50, 14, 5, "tin", 118.690, "post transition metal"),
    ("Sb", 51, 15, 5, "antimony", 121.760, "metalloid"),
    ("Te", 52, 16, 5, "tellurium", 127.600, "metalloid"),
    ("I", 53, 17, 5, "iodine", 126.910, "diatomic nonmetal"),
    ("Xe", 54, 18, 5, "xenon", 131.300, "noble gas"),
    ("Cs", 55, 1, 6, "caesium", 132.910, "alkali metal"),
    ("Ba", 56, 2, 6, "barium", 137.360, "alkaline earth metal"),
    ("La", 57, 3, 6, "lanthanum", 138.920, "lanthanide"),
    ("Ce", 58, 4, 9, "cerium", 140.130, "lanthanide"),
    ("Pr", 59, 5, 9, "praseodymium", 140.920, "lanthanide"),
    ("Nd", 60, 6, 9, "neodymium", 144.270, "lanthanide"),
    ("Pm", 61, 7, 9, "promethium", 147.000, "lanthanide"),
    ("Sm", 62, 8, 9, "samarium", 150.350, "lanthanide"),
    ("Eu", 63, 9, 9, "europium", 152.000, "lanthanide"),
    ("Gd", 64, 10, 9, "gadolinium", 157.260, "lanthanide"),
    ("Tb", 65, 11, 9, "terbium", 158.930, "lanthanide"),
    ("Dy", 66, 12, 9, "dysprosium", 162.510, "lanthanide"),
    ("Ho", 67, 13, 9, "holmium", 164.940, "lanthanide"),
    ("Er", 68, 14, 9, "erbium", 167.270, "lanthanide"),
    ("Tm", 69, 15, 9, "thulium", 168.940, "lanthanide"),
    ("Yb", 70, 16, 9, "ytterbium", 173.040, "lanthanide"),
    ("Lu", 71, 17, 9, "lutetium", 174.990, "lanthanide"),
    ("Hf", 72, 4, 6, "hafnium", 178.500, "transition metal"),
    ("Ta", 73, 5, 6, "tantalum", 180.950, "transition metal"),
    ("W", 74, 6, 6, "tungsten", 183.920, "transition metal"),
    ("Re", 75, 7, 6, "rhenium", 186.200, "transition metal"),
    ("Os", 76, 8, 6, "osmium", 190.200, "transition metal"),
    ("Ir", 77, 9, 6, "iridium", 192.200, "transition metal"),
    ("Pt", 78, 10, 6, "platinum", 195.090, "transition metal"),
    ("Au", 79, 11, 6, "gold", 197.200, "transition metal"),
    ("Hg", 80, 12, 6, "mercury", 200.610, "transition metal"),
    ("Tl", 81, 13, 6, "thallium", 204.390, "post transition metal"),
    ("Pb", 82, 14, 6, "lead", 207.210, "post transition metal"),
    ("Bi", 83, 15, 6, "bismuth", 209.000, "post transition metal"),
    ("Po", 84, 16, 6, "polonium", 209.000, "post transition metal"),
    ("At", 85, 17, 6, "astatine", 210.000, "metalloid"),
    ("Rn", 86, 18, 6, "radon", 222.000, "noble gas"),
    ("Fr", 87, 1, 7, "francium", 223.000, "alkali metal"),
    ("Ra", 88, 2, 7, "radium", 226.000, "alkaline earth metal"),
    ("Ac", 89, 3, 7, "actinium", 227.000, "actinide"),
    ("Th", 90, 4, 10, "thorium", 232.000, "actinide"),
    ("Pa", 91, 5, 10, "proactinium", 231.03588, "actinide"),
    ("U", 92, 6, 10, "uranium", 238.070, "actinide"),
    ("Np", 93, 7, 10, "neptunium", 237.000, "actinide"),
    ("Pu", 94, 8, 10, "plutonium", 239.100, "actinide"),
    ("Am", 95, 9, 10, "americium", 243, "actinide"),
    ("Cm", 96, 10, 10, "curium", 247, "actinide"),
    ("Bk", 97, 11, 10, "berkelium", 247, "actinide"),
    ("Cf", 98, 12, 10, "californium", 251, "actinide"),
    ("Es", 99, 13, 10, "einsteinium", 252, "actinide"),
    ("Fm", 100, 14, 10, "fermium", 257, "actinide"),
    ("Md", 101, 15, 10, "mendelevium", 258, "actinide"),
    ("No", 102, 16, 10, "nobelium", 259, "actinide"),
    ("Lr", 103, 17, 10, "lawrencium", 262, "actinide"),
    ("Rf", 104, 4, 7, "rutherfordium", 261, "transition metal"),
    ("Db", 105, 5, 7, "dubnium", 262, "transition metal"),
    ("Sg", 106, 6, 7, "seaborgium", 266, "transition metal"),
    ("Bh", 107, 7, 7, "bohrium", 264, "transition metal"),
    ("Hs", 108, 8, 7, "hassium", 269, "transition metal"),
    ("Mt", 109, 9, 7, "meitnerium", 268),
]


class PeriodicTableItem(object):
    """Periodic table item, used as generic item in :class:`PeriodicTable`,
    :class:`PeriodicCombo` and  :class:`PeriodicList`.

    This implementation stores the minimal amount of information needed by the
    widgets:

        - atomic symbol
        - atomic number
        - element name
        - atomic mass
        - column of element in periodic table
        - row of element in periodic table

    You can subclass this class to add additional information.

    :param str symbol: Atomic symbol (e.g. H, He, Li...)
    :param int Z: Proton number
    :param int col: 1-based column index of element in periodic table
    :param int row: 1-based row index of element in periodic table
    :param str name: PeriodicTableItem name ("hydrogen", ...)
    :param float mass: Atomic mass (gram per mol)
    :param str subcategory: Subcategory, based on physical properties
        (e.g. "alkali metal", "noble gas"...)
    """

    def __init__(self, symbol, Z, col, row, name, mass, subcategory=""):
        self.symbol = symbol
        """Atomic symbol (e.g. H, He, Li...)"""
        self.Z = Z
        """Atomic number (Proton number)"""
        self.col = col
        """1-based column index of element in periodic table"""
        self.row = row
        """1-based row index of element in periodic table"""
        self.name = name
        """PeriodicTableItem name ("hydrogen", ...)"""
        self.mass = mass
        """Atomic mass (gram per mol)"""
        self.subcategory = subcategory
        """Subcategory, based on physical properties
        (e.g. "alkali metal", "noble gas"...)"""

    # pymca compatibility (elements used to be stored as a list of lists)
    def __getitem__(self, idx):
        if idx == 6:
            _logger.warning("density not implemented in silx, returning 0.")

        ret = [self.symbol, self.Z, self.col, self.row, self.name, self.mass, 0.0]
        return ret[idx]

    def __len__(self):
        return 6


class ColoredPeriodicTableItem(PeriodicTableItem):
    """:class:`PeriodicTableItem` with an added :attr:`bgcolor`.
    The background color can be passed as a parameter to the constructor.
    If it is not specified, it will be defined based on
    :attr:`subcategory`.

    :param str bgcolor: Custom background color for element in
        periodic table, as a RGB string *#RRGGBB*"""

    COLORS = {
        "diatomic nonmetal": "#7FFF00",  # chartreuse
        "noble gas": "#00FFFF",  # cyan
        "alkali metal": "#FFE4B5",  # Moccasin
        "alkaline earth metal": "#FFA500",  # orange
        "polyatomic nonmetal": "#7FFFD4",  # aquamarine
        "transition metal": "#FFA07A",  # light salmon
        "metalloid": "#8FBC8F",  # Dark Sea Green
        "post transition metal": "#D3D3D3",  # light gray
        "lanthanide": "#FFB6C1",  # light pink
        "actinide": "#F08080",  # Light Coral
        "": "#FFFFFF",  # white
    }
    """Dictionary defining RGB colors for each subcategory."""

    def __init__(self, symbol, Z, col, row, name, mass, subcategory="", bgcolor=None):
        PeriodicTableItem.__init__(self, symbol, Z, col, row, name, mass, subcategory)

        self.bgcolor = self.COLORS.get(subcategory, "#FFFFFF")
        """Background color of element in the periodic table,
        based on its subcategory. This should be a string of a hexadecimal
        RGB code, with the format *#RRGGBB*.
        If the subcategory is unknown, use white (*#FFFFFF*)
        """

        # possible custom color
        if bgcolor is not None:
            self.bgcolor = bgcolor


# Sometimes the mass of an element can be slightly different to the one in the peak data file, if so use that one
def _correct_with_peak_data_file():
    with open(PeriodicTableModel().get_default_peak_data_file(), "r") as data_file:
        data = json.load(data_file)

    for i, element in enumerate(copy.deepcopy(_elements)):
        data_element = data.get(element[0], None)
        if data_element is not None and data_element["A"] is not None:
            if abs(data_element["A"] - element[5]) > 1e-6:
                if len(element) > 6:
                    _elements[i] = (element[0], data_element["Z"], element[2], element[3], element[4], data_element["A"], element[6])
                else:
                    _elements[i] = (element[0], data_element["Z"], element[2], element[3], element[4], data_element["A"])


_correct_with_peak_data_file()
_default_table_items = [ColoredPeriodicTableItem(*info) for info in _elements]


class _ElementButton(QtWidgets.QPushButton):
    """Atomic element button, used as a cell in the periodic table"""

    sigElementEnter = QtCore.Signal(object)
    """Signal emitted as the cursor enters the widget"""
    sigElementLeave = QtCore.Signal(object)
    """Signal emitted as the cursor leaves the widget"""
    sigElementLeftClicked = QtCore.Signal(object)
    """Signal emitted when the widget is left clicked"""
    sigElementRightClicked = QtCore.Signal(object)
    """Signal emitted when the widget is right clicked"""

    def __init__(self, item, parent=None):
        """

        :param parent: Parent widget
        :param PeriodicTableItem item: :class:`PeriodicTableItem` object
        """
        QtWidgets.QPushButton.__init__(self, parent)

        self.item = item
        """:class:`PeriodicTableItem` object represented by this button"""

        self.setText(item.symbol)
        self.setFlat(True)
        self.setCheckable(False)

        self.setSizePolicy(QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding))

        self.selected = False
        self.current = False

        # selection colors
        self.selected_color = QtGui.QColor(QtCore.Qt.yellow)
        self.current_color = QtGui.QColor(QtCore.Qt.gray)
        self.selected_current_color = QtGui.QColor(QtCore.Qt.darkYellow)

        # element colors

        if hasattr(item, "bgcolor"):
            self.bgcolor = QtGui.QColor(item.bgcolor)
        else:
            self.bgcolor = QtGui.QColor("#FFFFFF")

        self.brush = QtGui.QBrush()
        self._setBrush()

        self.clicked.connect(self.leftClickedSlot)
        self.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.rightClickedSlot)

    def sizeHint(self):
        return QtCore.QSize(30, 30)

    def setCurrent(self, b):
        """Set this element button as current.
        Multiple buttons can be selected.

        :param b: boolean
        """
        self.current = b
        self._setBrush()

    def isCurrent(self):
        """
        :return: True if element button is current
        """
        return self.current

    def isSelected(self):
        """
        :return: True if element button is selected
        """
        return self.selected

    def setSelected(self, b):
        """Set this element button as selected.
        Only a single button can be selected.

        :param b: boolean
        """
        self.selected = b
        self._setBrush()

    def _setBrush(self):
        """Selected cells are yellow when not current.
        The current cell is dark yellow when selected or grey when not
        selected.
        Other cells have no bg color by default, unless specified at
        instantiation (:attr:`bgcolor`)"""
        palette = self.palette()

        if self.selected:
            self.brush = QtGui.QBrush(self.selected_color)
        elif self.bgcolor is not None:
            self.brush = QtGui.QBrush(self.bgcolor)
        else:
            self.brush = QtGui.QBrush()
        palette.setBrush(self.backgroundRole(), self.brush)
        self.setPalette(palette)
        self.update()

    def paintEvent(self, pEvent):
        # get button geometry
        widgGeom = self.rect()
        paintGeom = QtCore.QRect(widgGeom.left() + 1, widgGeom.top() + 1, widgGeom.width() - 2, widgGeom.height() - 2)

        # paint background color
        painter = QtGui.QPainter(self)
        if self.brush is not None:
            painter.fillRect(paintGeom, self.brush)
        # paint frame
        pen = QtGui.QPen(QtCore.Qt.black)
        pen.setWidth(1 if not self.isCurrent() else 5)
        painter.setPen(pen)
        painter.drawRect(paintGeom)
        painter.end()
        QtWidgets.QPushButton.paintEvent(self, pEvent)

    def enterEvent(self, e):
        """Emit a :attr:`sigElementEnter` signal and send a
        :class:`PeriodicTableItem` object"""
        self.sigElementEnter.emit(self.item)

    def leaveEvent(self, e):
        """Emit a :attr:`sigElementLeave` signal and send a
        :class:`PeriodicTableItem` object"""
        self.sigElementLeave.emit(self.item)

    def leftClickedSlot(self):
        """Emit a :attr:`sigElementClicked` signal and send a
        :class:`PeriodicTableItem` object"""
        self.sigElementLeftClicked.emit(self.item)

    def rightClickedSlot(self):
        self.sigElementRightClicked.emit(self.item)


class PeriodicTable(QtWidgets.QWidget):
    """Periodic Table widget

    .. image:: img/PeriodicTable.png

    The following example shows how to connect clicking to selection::

         from silx.gui import qt
         from silx.gui.widgets.PeriodicTable import PeriodicTable
         app = qt.QApplication([])
         pt = PeriodicTable()
         pt.sigElementClicked.connect(pt.elementToggle)
         pt.show()
         app.exec_()

    To print all selected elements each time a new element is selected::

        def my_slot(item):
            pt.elementToggle(item)
            selected_elements = pt.getSelection()
            for e in selected_elements:
                print(e.symbol)

        pt.sigElementClicked.connect(my_slot)

    """

    sigElementLeftClicked = QtCore.Signal(object)
    """When any element is clicked in the table, the widget emits
    this signal and sends a :class:`PeriodicTableItem` object.
    """
    sigElementRightClicked = QtCore.Signal(object)
    """When any element is clicked in the table, the widget emits
    this signal and sends a :class:`PeriodicTableItem` object.
    """

    sigSelectionChanged = QtCore.Signal(object)
    """When any element is selected/unselected in the table, the widget emits
    this signal and sends a list of :class:`PeriodicTableItem` objects.

    .. note::

        To enable selection of elements, you must set *selectable=True*
        when you instantiate the widget. Alternatively, you can also connect
        :attr:`sigElementClicked` to :meth:`elementToggle` manually::

            pt = PeriodicTable()
            pt.sigElementClicked.connect(pt.elementToggle)


    :param parent: parent QWidget
    :param str name: Widget window title
    :param elements: List of items (:class:`PeriodicTableItem` objects) to
        be represented in the table. By default, take elements from
        a predefined list with minimal information (symbol, atomic number,
        name, mass).
    :param bool selectable: If *True*, multiple elements can be
        selected by clicking with the mouse. If *False* (default),
        selection is only possible with method :meth:`setSelection`.
    """

    def __init__(self, parent=None, name="PeriodicTable", elements=None, selectable=False):
        self.selectable = selectable
        QtWidgets.QWidget.__init__(self, parent)
        self.setWindowTitle(name)
        self.gridLayout = QtWidgets.QGridLayout(self)
        self.gridLayout.setContentsMargins(0, 0, 0, 0)
        self.gridLayout.addItem(QtWidgets.QSpacerItem(0, 5), 7, 0)

        for idx in range(10):
            self.gridLayout.setRowStretch(idx, 3)
        # row 8 (above lanthanoids  is empty)
        self.gridLayout.setRowStretch(7, 2)

        # Element information displayed when cursor enters a cell
        self.eltLabel = QtWidgets.QLabel(self)
        f = self.eltLabel.font()
        f.setBold(1)
        self.eltLabel.setFont(f)
        self.eltLabel.setAlignment(QtCore.Qt.AlignHCenter)
        self.gridLayout.addWidget(self.eltLabel, 1, 1, 3, 10)

        self._eltCurrent = None
        """Current :class:`_ElementButton` (last clicked)"""

        self._eltButtons = OrderedDict()
        """Dictionary of all :class:`_ElementButton`. Keys are the symbols
        ("H", "He", "Li"...)"""

        if elements is None:
            elements = _default_table_items
        # fill cells with elements
        for elmt in elements:
            self._addElement(elmt)
        self.elements = elements

    def silentSetElementSelected(self, symbol, state):
        """
        Identical to setElementSelected, but doesn't emit sigSelectionChanged
        """
        self._eltButtons[symbol].setSelected(state)

    def enableElementButton(self, element):
        try:
            self._eltButtons[element].setEnabled(True)
        except KeyError:
            return

    def disableElementButton(self, element):
        try:
            self._eltButtons[element].setEnabled(False)
        except KeyError:
            return

    def isElementButtonEnabled(self, element):
        try:
            return self._eltButtons[element].isEnabled()
        except KeyError:
            return None

    def _addElement(self, elmt):
        """Add one :class:`_ElementButton` widget into the grid,
        connect its signals to interact with the cursor"""
        b = _ElementButton(elmt, self)
        b.setAutoDefault(False)

        self._eltButtons[elmt.symbol] = b
        self.gridLayout.addWidget(b, elmt.row, elmt.col)

        b.sigElementEnter.connect(self.elementEnter)
        b.sigElementLeave.connect(self._elementLeave)
        b.sigElementLeftClicked.connect(self._elementLeftClicked)
        b.sigElementRightClicked.connect(self._elementRightClicked)

    def elementEnter(self, item):
        """Update label with element info (e.g. "Nb(41) - niobium")
        when mouse cursor hovers an element.

        :param PeriodicTableItem item: Element entered by cursor
        """
        self.eltLabel.setText("%s(%d) - %s" % (item.symbol, item.Z, item.name))

    def _elementLeave(self, item):
        """Clear label when the cursor leaves the cell

        :param PeriodicTableItem item: Element left
        """
        self.eltLabel.setText("")

    def _elementLeftClicked(self, item):
        """Emit :attr:`sigElementClicked`,
        toggle selected state of element

        :param PeriodicTableItem item: Element clicked
        """
        if self._eltCurrent is not None:
            self._eltCurrent.setCurrent(False)
        self._eltButtons[item.symbol].setCurrent(True)
        self._eltCurrent = self._eltButtons[item.symbol]
        if self.selectable:
            self.elementToggle(item)
        self.sigElementLeftClicked.emit(item)

    def _elementRightClicked(self, item):
        """Emit :attr:`sigElementClicked`,
        toggle selected state of element

        :param PeriodicTableItem item: Element clicked
        """
        self.sigElementRightClicked.emit(item)

    def getSelection(self):
        """Return a list of selected elements, as a list of :class:`PeriodicTableItem`
        objects.

        :return: Selected items
        :rtype: List[PeriodicTableItem]
        """
        return [b.item for b in self._eltButtons.values() if b.isSelected()]

    def setSelection(self, symbols):
        """Set selected elements.

        This causes the sigSelectionChanged signal
        to be emitted, even if the selection didn't actually change.

        :param List[str] symbols: List of symbols of elements to be selected
            (e.g. *["Fe", "Hg", "Li"]*)
        """
        # accept list of PeriodicTableItems as input, because getSelection
        # returns these objects and it makes sense to have getter and setter
        # use same type of data
        if isinstance(symbols[0], PeriodicTableItem):
            symbols = [elmt.symbol for elmt in symbols]

        for e, b in self._eltButtons.items():
            b.setSelected(e in symbols)
        self.sigSelectionChanged.emit(self.getSelection())

    def setElementSelected(self, symbol, state):
        """Modify *selected* status of a single element (select or unselect)

        :param str symbol: PeriodicTableItem symbol to be selected
        :param bool state: *True* to select, *False* to unselect
        """
        self._eltButtons[symbol].setSelected(state)
        self.sigSelectionChanged.emit(self.getSelection())

    def isElementSelected(self, symbol):
        """Return *True* if element is selected, else *False*

        :param str symbol: PeriodicTableItem symbol
        :return: *True* if element is selected, else *False*
        """
        return self._eltButtons[symbol].isSelected()

    def elementToggle(self, item):
        """Toggle selected/unselected state for element

        :param item: PeriodicTableItem object
        """
        b = self._eltButtons[item.symbol]
        b.setSelected(not b.isSelected())
        self.sigSelectionChanged.emit(self.getSelection())

    def elements_list(self):
        return self._eltButtons.keys()


class PeriodicCombo(QtWidgets.QComboBox):
    """
    Combo list with all atomic elements of the periodic table

    .. image:: img/PeriodicCombo.png

    :param bool detailed: True (default) display element symbol, Z and name.
        False display only element symbol and Z.
    :param elements: List of items (:class:`PeriodicTableItem` objects) to
        be represented in the table. By default, take elements from
        a predefined list with minimal information (symbol, atomic number,
        name, mass).
    """

    sigSelectionChanged = QtCore.Signal(object)
    """Signal emitted when the selection changes. Send
    :class:`PeriodicTableItem` object representing selected
    element
    """

    def __init__(self, parent=None, detailed=True, elements=None):
        QtWidgets.QComboBox.__init__(self, parent)

        # add all elements from global list
        if elements is None:
            elements = _default_table_items
        for i, elmt in enumerate(elements):
            if detailed:
                txt = "%2s (%d) - %s" % (elmt.symbol, elmt.Z, elmt.name)
            else:
                txt = "%2s (%d)" % (elmt.symbol, elmt.Z)
            self.insertItem(i, txt)

        self.currentIndexChanged[int].connect(self._selectionChanged)

    def _selectionChanged(self, idx):
        """Emit :attr:`sigSelectionChanged`"""
        self.sigSelectionChanged.emit(_default_table_items[idx])

    def getSelection(self):
        """Get selected element

        :return: Selected element
        :rtype: PeriodicTableItem
        """
        return _default_table_items[self.currentIndex()]

    def setSelection(self, symbol):
        """Set selected item in combobox by giving the atomic symbol

        :param symbol: Symbol of element to be selected
        """
        # accept PeriodicTableItem for getter/setter consistency
        if isinstance(symbol, PeriodicTableItem):
            symbol = symbol.symbol
        symblist = [elmt.symbol for elmt in _default_table_items]
        self.setCurrentIndex(symblist.index(symbol))


class PeriodicList(QtWidgets.QTreeWidget):
    """List of atomic elements in a :class:`QTreeView`

    .. image:: img/PeriodicList.png

    :param QWidget parent: Parent widget
    :param bool detailed: True (default) display element symbol, Z and name.
        False display only element symbol and Z.
    :param single: *True* for single element selection with mouse click,
        *False* for multiple element selection mode.
    """

    sigSelectionChanged = QtCore.Signal(object)
    """When any element is selected/unselected in the widget, it emits
    this signal and sends a list of currently selected
    :class:`PeriodicTableItem` objects.
    """

    def __init__(self, parent=None, detailed=True, single=False, elements=None):
        QtWidgets.QTreeWidget.__init__(self, parent)

        self.detailed = detailed

        headers = ["Z", "Symbol"]
        if detailed:
            headers.append("Name")
            self.setColumnCount(3)
        else:
            self.setColumnCount(2)
        self.setHeaderLabels(headers)
        self.header().setStretchLastSection(False)

        self.setRootIsDecorated(0)
        self.itemClicked.connect(self._selectionChanged)
        self.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection if single else QtWidgets.QAbstractItemView.ExtendedSelection)
        self._fill_widget(elements)
        self.resizeColumnToContents(0)
        self.resizeColumnToContents(1)
        if detailed:
            self.resizeColumnToContents(2)

    def _fill_widget(self, elements):
        """Fill tree widget with elements"""
        if elements is None:
            elements = _default_table_items

        self.tree_items = []

        previous_item = None
        for elmt in elements:
            if previous_item is None:
                item = QtWidgets.QTreeWidgetItem(self)
            else:
                item = QtWidgets.QTreeWidgetItem(self, previous_item)
            item.setText(0, str(elmt.Z))
            item.setText(1, elmt.symbol)
            if self.detailed:
                item.setText(2, elmt.name)
            self.tree_items.append(item)
            previous_item = item

    def _selectionChanged(self, treeItem, column):
        """Emit a :attr:`sigSelectionChanged` and send a list of
        :class:`PeriodicTableItem` objects."""
        self.sigSelectionChanged.emit(self.getSelection())

    def getSelection(self):
        """Get a list of selected elements, as a list of :class:`PeriodicTableItem`
        objects.

        :return: Selected elements
        :rtype: List[PeriodicTableItem]"""
        return [_default_table_items[idx] for idx in range(len(self.tree_items)) if self.tree_items[idx].isSelected()]

    # setSelection is a bad name (name of a QTreeWidget method)
    def setSelectedElements(self, symbolList):
        """

        :param symbolList: List of atomic symbols ["H", "He", "Li"...]
            to be selected in the widget
        """
        # accept PeriodicTableItem for getter/setter consistency
        if isinstance(symbolList[0], PeriodicTableItem):
            symbolList = [elmt.symbol for elmt in symbolList]
        for idx in range(len(self.tree_items)):
            self.tree_items[idx].setSelected(_default_table_items[idx].symbol in symbolList)

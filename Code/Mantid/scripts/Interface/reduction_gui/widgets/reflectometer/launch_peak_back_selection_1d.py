# used to parse files more easily
from __future__ import with_statement

# numpy module
import numpy as np

# for command-line arguments
import sys

# Qt4 bindings for core Qt functionalities (non-GUI)
from PyQt4 import QtCore
# python Qt4 bindings for GUI objects
from PyQt4 import QtGui

from mantid.simpleapi import *

#blue
CSS_PEAK = """QLineEdit {
                   background-color: #54f3f5;
                }"""

#red
CSS_BACK = """QLineEdit{
                   background-color: #f53535;
                }"""

#green
CSS_LOWRES = """QLineEdit{
                   background-color: #a7f6a1;
                }"""

class DesignerMainWindow(QtGui.QMainWindow):
    """ Customization for Qt Designer created window """

    peak_back_xlim = [100,160]
    low_res_xlim = [100,200]
    bClick = False
    _peakFromValue = 4.
    _peakToValue = 6.
    _backFromValue = 2.
    _backToValue = 8.
    _lowresFromValue = 4.
    _lowresToValue = 6.
    drs = []
    parent = None
    dataType = 'data'  #'data' or 'norm'
    x1=None
    x2=None
    y1=None
    y2=None

    def __init__(self, wk1=None, wk2=None, parent=None, type='data'):

        mt1 = mtd[wk1]
        self.x1 = mt1.readX(0)[:]
        self.y1 = mt1.readY(0)[:]

        mt2 = mtd[wk2]
        self.x2 = mt2.readX(0)[:]
        self.y2 = mt2.readY(0)[:]

        self.dataType = type
        if (type == 'data'):
            self._peakFromValue=float(parent._summary.data_peak_from_pixel.text())
            self._peakToValue=float(parent._summary.data_peak_to_pixel.text())
            self._backFromValue=float(parent._summary.data_background_from_pixel1.text())
            self._backToValue=float(parent._summary.data_background_to_pixel1.text())
            self._lowresFromValue=float(parent._summary.x_min_edit.text())
            self._lowresToValue=float(parent._summary.x_max_edit.text())
        else:
            self._peakFromValue=float(parent._summary.norm_peak_from_pixel.text())
            self._peakToValue=float(parent._summary.norm_peak_to_pixel.text())
            self._backFromValue=float(parent._summary.norm_background_from_pixel1.text())
            self._backToValue=float(parent._summary.norm_background_to_pixel1.text())
            self._lowresFromValue=float(parent._summary.norm_x_min_edit.text())
            self._lowresToValue=float(parent._summary.norm_x_max_edit.text())

        #initialization of the superclass
        super(DesignerMainWindow, self).__init__(parent)
        self.parent = parent

        self.setParent(parent)
        self.setWindowTitle('Peak and Background 1D data selection')

        self.create_menu()
        self.create_main_frame()

        #peak text fields
        QtCore.QObject.connect(self.peakFrom, QtCore.SIGNAL("returnPressed()"), self.update_plot)
        QtCore.QObject.connect(self.peakTo, QtCore.SIGNAL("returnPressed()"), self.update_plot)

        #back text fields
        QtCore.QObject.connect(self.backFrom, QtCore.SIGNAL("returnPressed()"), self.update_plot)
        QtCore.QObject.connect(self.backTo, QtCore.SIGNAL("returnPressed()"), self.update_plot)

        #lowres text fields
        QtCore.QObject.connect(self.lowresFrom, QtCore.SIGNAL("returnPressed()"), self.update_plot)
        QtCore.QObject.connect(self.lowresTo, QtCore.SIGNAL("returnPressed()"), self.update_plot)

        #linear and log axis
        QtCore.QObject.connect(self.linear, QtCore.SIGNAL("clicked()"), self.update_linear_selection_mode)
        QtCore.QObject.connect(self.log, QtCore.SIGNAL("clicked()"), self.update_log_selection_mode)

        self.tmp_plot_data()

        self.peakFrom.setStyleSheet(CSS_PEAK)
        self.peakTo.setStyleSheet(CSS_PEAK)
        self.backFrom.setStyleSheet(CSS_BACK)
        self.backTo.setStyleSheet(CSS_BACK)
        self.lowresFrom.setStyleSheet(CSS_LOWRES)
        self.lowresTo.setStyleSheet(CSS_LOWRES)

    def create_main_frame(self):

        self.resize(932,661)
        self.setUnifiedTitleAndToolBarOnMac(False)

        #selection mode and axis type
        self.topHorizontalLayout = QtGui.QHBoxLayout()
        #peak and background input text fields
        self.topHorizontalLayoutPeak = QtGui.QHBoxLayout()
        self.label = QtGui.QLabel("Peak")
        self.peakFrom = QtGui.QLineEdit("{0:.2f}".format(self._peakFromValue))
        self.peakFrom.setMinimumSize(QtCore.QSize(60,0))
        self.peakFrom.setMaximumSize(QtCore.QSize(60,16777215))
        self.peakFrom.setObjectName("peakFrom")
        self.peakTo = QtGui.QLineEdit("{0:.2f}".format(self._peakToValue))
        self.peakTo.setMinimumSize(QtCore.QSize(60,0))
        self.peakTo.setMaximumSize(QtCore.QSize(60,16777215))
        self.peakTo.setObjectName("peakTo")
        self.topHorizontalLayoutPeak.addWidget(self.label)
        self.topHorizontalLayoutPeak.addWidget(self.peakFrom)
        self.topHorizontalLayoutPeak.addWidget(self.peakTo)

        #spacer
        spacerItem = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)

        #back
        self.topHorizontalLayoutBack = QtGui.QHBoxLayout()
        self.label = QtGui.QLabel("Background")
        self.backFrom = QtGui.QLineEdit("{0:.2f}".format(self._backFromValue))
        self.backFrom.setMinimumSize(QtCore.QSize(60,0))
        self.backFrom.setMaximumSize(QtCore.QSize(60,16777215))
        self.backFrom.setObjectName("backFrom")
        self.backTo = QtGui.QLineEdit("{0:.2f}".format(self._backToValue))
        self.backTo.setMinimumSize(QtCore.QSize(60,0))
        self.backTo.setMaximumSize(QtCore.QSize(60,16777215))
        self.backTo.setObjectName("backTo")
        self.topHorizontalLayoutBack.addWidget(self.label)
        self.topHorizontalLayoutBack.addWidget(self.backFrom)
        self.topHorizontalLayoutBack.addWidget(self.backTo)

        #spacer
        spacerItem = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)

        #low resolution
        self.topHorizontalLayoutLowres = QtGui.QHBoxLayout()
        self.label = QtGui.QLabel("Low resolution axis")
        self.lowresFrom = QtGui.QLineEdit("{0:.2f}".format(self._lowresFromValue))
        self.lowresFrom.setMinimumSize(QtCore.QSize(60,0))
        self.lowresFrom.setMaximumSize(QtCore.QSize(60,16777215))
        self.lowresFrom.setObjectName("lowresFrom")
        self.lowresTo = QtGui.QLineEdit("{0:.2f}".format(self._lowresToValue))
        self.lowresTo.setMinimumSize(QtCore.QSize(60,0))
        self.lowresTo.setMaximumSize(QtCore.QSize(60,16777215))
        self.lowresTo.setObjectName("lowresTo")
        self.topHorizontalLayoutLowres.addWidget(self.label)
        self.topHorizontalLayoutLowres.addWidget(self.lowresFrom)
        self.topHorizontalLayoutLowres.addWidget(self.lowresTo)

        #spacer
        spacerItem = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)

        #linear and log
        self.topHorizontalLayoutRight = QtGui.QHBoxLayout()
        self.linear = QtGui.QRadioButton("Linear")
        self.linear.setChecked(True)
        self.linear.setAutoExclusive(False)
        self.log = QtGui.QRadioButton("Log")
        self.log.setChecked(False)
        self.log.setAutoExclusive(False)
        self.topHorizontalLayoutRight.addWidget(self.linear)
        self.topHorizontalLayoutRight.addWidget(self.log)

        self.topHorizontalLayout.addLayout(self.topHorizontalLayoutPeak)
        self.topHorizontalLayout.addItem(spacerItem)
        self.topHorizontalLayout.addLayout(self.topHorizontalLayoutBack)
        self.topHorizontalLayout.addItem(spacerItem)
        self.topHorizontalLayout.addLayout(self.topHorizontalLayoutLowres)
        self.topHorizontalLayout.addItem(spacerItem)
        self.topHorizontalLayout.addLayout(self.topHorizontalLayoutRight)

        #Plot and toolbar
        self.main_frame = QtGui.QWidget(self)

        self.dpi = 100
        self.fig = Figure((6.0, 4.0), dpi=self.dpi)

        self.canvas = FigureCanvas(self.fig)
        self.canvas.setParent(self.main_frame)

#        self.axes = self.fig.add_subplot(111)
        self.mpl_toolbar = NavigationToolbar(self.canvas, self.main_frame)

        plot_vbox = QtGui.QVBoxLayout()
        plot_vbox.addWidget(self.canvas)
        plot_vbox.addWidget(self.mpl_toolbar)

        #peak and background input
        self.bottomHorizontalLayout = QtGui.QHBoxLayout()

        vbox = QtGui.QVBoxLayout()
        vbox.addLayout(self.topHorizontalLayout)
        vbox.addLayout(plot_vbox)

        self.main_frame.setLayout(vbox)
        self.setCentralWidget(self.main_frame)

    def create_action(self, text, slot=None, shortcut=None,
                      icon=None, tip=None, checkable=False,
                      signal="triggered()"):
        action = QtGui.QAction(text, self)
        if icon is not None:
            action.setIcon(QIcon(":/%s.png" % icon))
        if shortcut is not None:
            action.setShortcut(shortcut)
        if tip is not None:
            action.setToolTip(tip)
            action.setStatusTip(tip)
        if slot is not None:
            self.connect(action, QtCore.SIGNAL(signal), slot)
        if checkable:
            action.setCheckable(True)
        return action

    def add_actions(self, target, actions):
        for action in actions:
            if action is None:
                target.addSeparator()
            else:
                target.addAction(action)

    def create_menu(self):
        self._file_menu = self.menuBar().addMenu("&File")
        close_action = self.create_action("&Close", shortcut="Ctrl+Q", slot=self.close, tip="Close the application")
        self.add_actions(self._file_menu, (close_action,))

    def peak_input_changed(self):
        self.update_graph()


    def update_peak_back_selection_mode(self, bPeak):
        """Make sure that only 1 peak/back selection is activated at the same time"""
        self.peakSwitch.setChecked(bPeak)
        self.backSwitch.setChecked(not(bPeak))

        if bPeak:
#            class_name = self.from_peak_input.__class__.__name__
            self.peakFrom.setStyleSheet(CSS_ACTIVATED)
            self.peakTo.setStyleSheet(CSS_ACTIVATED)
            self.backFrom.setStyleSheet(CSS_DESACTIVATED)
            self.backTo.setStyleSheet(CSS_DESACTIVATED)
        else:
            self.peakFrom.setStyleSheet(CSS_DESACTIVATED)
            self.peakTo.setStyleSheet(CSS_DESACTIVATED)
            self.backFrom.setStyleSheet(CSS_ACTIVATED)
            self.backTo.setStyleSheet(CSS_ACTIVATED)

    def update_peak_selection_mode(self):
        self.update_peak_back_selection_mode(True)

    def update_back_selection_mode(self):
        self.update_peak_back_selection_mode(False)

    def update_linear_log_selection_mode(self, bLinear):
        """Make sure that only linear or log are selected at the same time"""
        self.linear.setChecked(bLinear)
        self.log.setChecked(not(bLinear))

    def update_linear_selection_mode(self):
        self.update_linear_log_selection_mode(True)
        self.fig.clear()
        self.tmp_plot_data()

    def update_log_selection_mode(self):
        self._peakFromValue = float(self.peakFrom.text())
        self._peakToValue = float(self.peakTo.text())
        self._backFromValue = float(self.backFrom.text())
        self._backToValue = float(self.backTo.text())
        self._lowresFromValue = float(self.lowresFrom.text())
        self._lowresToValue = float(self.lowresTo.text())
        self.update_linear_log_selection_mode(False)
        self.fig.clear()
        self.tmp_plot_data()

    def select_file(self):
        """ opens a file select dialog """
        # open the dialog and get the selected file
        file = QtGui.QFileDialog.getOpenFileName()
        # if a file is selected
        if file:
            # update the lineEdit text with the selected filename
            self.mpllineEdit.setText(file)

    def parse_file(self, filename):
        """ parse a text file to extract letters frequencies """
        # lower-case letter ordinal numbers
        for i in range(97,122+1):
            letters[chr(i)] = 0

        # parse the input file
        with open(filename) as f:
            for line in f:
                for char in line:
                    #counts only letters
                    if ord(char.lower()) in range(97, 122+1):
                        letters[char.lower()] += 1

        # compute the ordered list of keys and relative values
        k = sorted(letters.keys())
        v = [letters[ki] for ki in k]

        return k,v

    def update_plot(self):
        """ retrieve the value of the peak/back/lowres and save it
        before refreshing plot """

        self._peakFromValue = float(self.peakFrom.text())
        self._peakToValue = float(self.peakTo.text())
        self._backFromValue = float(self.backFrom.text())
        self._backToValue = float(self.backTo.text())
        self._lowresFromValue = float(self.lowresFrom.text())
        self._lowresToValue = float(self.lowresTo.text())
        self.tmp_plot_data(True)

    def tmp_plot_data(self, refresh=False):
        """ just for testing, display random data """

        if refresh:
            for rect in self.drs:
                rect._refresh()
            return

        class DraggableRectangle:

            def __init__(self, rect, type, super, parent):
                self.rect = rect
                self.xpress = None
                self.type = type #peak_from, peak_to, back_from, back_to...
                self.super = super
                self.parent = parent

            def connect(self):
                'connect to all the events we need'
                self.cidpress = self.rect.figure.canvas.mpl_connect(
                    'button_press_event', self.on_press)
                self.cidrelease = self.rect.figure.canvas.mpl_connect(
                    'button_release_event', self.on_release)
                self.cidmotion = self.rect.figure.canvas.mpl_connect(
                    'motion_notify_event', self.on_motion)

            def on_press(self, event):
                'on button press we will see if the mouse is over us and store some data'
                if event.inaxes != self.rect.axes: return

                contains, attrd = self.rect.contains(event)
                if not contains: return
                self.xpress = event.xdata

            def on_motion(self, event):
                'on motion we will move the rect if the mouse is over us'
                if self.xpress is None: return
                if event.inaxes != self.rect.axes: return
                x0 = event.xdata
                _x0_format = '{0:.2f}'.format(x0)
                _x0_parent_format = str(int(round(x0)))

                if self.super.dataType == 'data':
                    parent_data_peak_from = self.parent._summary.data_peak_from_pixel
                    parent_data_peak_to = self.parent._summary.data_peak_to_pixel
                    parent_back_peak_from = self.parent._summary.data_background_from_pixel1
                    parent_back_peak_to = self.parent._summary.data_background_to_pixel1
                    parent_x_min = self.parent._summary.x_min_edit
                    parent_x_max = self.parent._summary.x_max_edit
                else:
                    parent_data_peak_from = self.parent._summary.norm_peak_from_pixel
                    parent_data_peak_to = self.parent._summary.norm_peak_to_pixel
                    parent_back_peak_from = self.parent._summary.norm_background_from_pixel1
                    parent_back_peak_to = self.parent._summary.norm_background_to_pixel1
                    parent_x_min = self.parent._summary.norm_x_min_edit
                    parent_x_max = self.parent._summary.norm_x_max_edit

                if self.type == 'peak_from':
                    self.super.peakFrom.setText(_x0_format)
                    parent_data_peak_from.setText(_x0_parent_format)
                if self.type == 'peak_to':
                    self.super.peakTo.setText(_x0_format)
                    parent_data_peak_to.setText(_x0_parent_format)
                if self.type == 'back_from':
                    self.super.backFrom.setText(_x0_format)
                    parent_back_peak_from.setText(_x0_parent_format)
                if self.type == 'back_to':
                    self.super.backTo.setText(_x0_format)
                    parent_back_peak_to.setText(_x0_parent_format)
                if self.type == 'lowres_from':
                    self.super.lowresFrom.setText(_x0_format)
                    parent_x_min.setText(_x0_parent_format)
                if self.type == 'lowres_to':
                    self.super.lowresTo.setText(_x0_format)
                    parent_x_max.setText(_x0_parent_format)

                self.rect.set_xdata(x0)
                self.rect.figure.canvas.draw()

            def _refresh(self):
                if self.type == 'peak_from':
                    x0 = self.super.peakFrom.text()
                if self.type == 'peak_to':
                    x0 = self.super.peakTo.text()
                if self.type == 'back_from':
                    x0 = self.super.backFrom.text()
                if self.type == 'back_to':
                    x0 = self.super.backTo.text()
                if self.type == 'lowres_from':
                    x0 = self.super.lowresFrom.text()
                if self.type == 'lowres_to':
                    x0 = self.super.lowresTo.text()
                self.rect.set_xdata(x0)
                self.rect.figure.canvas.draw()

            def on_release(self, event):
                'on release we reset the press data'
                self.xpress = None
                self.rect.figure.canvas.draw()

            def disconnect(self):
                'disconnect all the stored connection ids'
                self.rect.figure.canvas.mpl_disconnect(self.cidpress)
                self.rect.figure.canvas.mpl_disconnect(self.cidrelease)
                self.rect.figure.canvas.mpl_disconnect(self.cidmotion)

        parent = self.parent

        #top plot will be for peak and background selection
        axes = self.fig.add_subplot(211)

        x = self.x1
        y = self.y1

        bLinear = self.linear.isChecked()

        if bLinear:
            line, = axes.plot(x,y,color='black')
        else:
            line, = axes.semilogy(x,y,color='black')

        ymax=max(y)

        if bLinear:
            ymin=0
        else:
            ymin = 1

        #peak cursor
        peakx1 = self._peakFromValue
        self.peakFrom.setText(str(peakx1))
        peakx2 = self._peakToValue
        self.peakTo.setText(str(peakx2))
        if bLinear:
            rectpeakx1 = axes.plot([peakx1,peakx1],[ymin,ymax], '--', color='blue')
        else:
            rectpeakx1 = axes.semilogy([peakx1,peakx1],[ymin,ymax], '--', color='blue')

        for rect in rectpeakx1:
            dr = DraggableRectangle(rect, 'peak_from', self, parent)
            dr.connect()
            self.drs.append(dr)

        if bLinear:
            rectpeakx2 = axes.plot([peakx2,peakx2],[ymin,ymax], '--', color='blue')
        else:
            rectpeakx2 = axes.semilogy([peakx2,peakx2],[ymin,ymax], '--', color='blue')

        for rect in rectpeakx2:
            dr = DraggableRectangle(rect, 'peak_to', self, parent)
            dr.connect()
            self.drs.append(dr)

        #back cursor
        backx1 = self._backFromValue
        self.backFrom.setText(str(backx1))
        backx2 = self._backToValue
        self.backTo.setText(str(backx2))
        if bLinear:
            rectbackx1 = axes.plot([backx1,backx1],[ymin,ymax], '--', color='red')
        else:
            rectbackx1 = axes.semilogy([backx1,backx1],[ymin,ymax], '--', color='red')

        for rect in rectbackx1:
            dr = DraggableRectangle(rect, 'back_from', self, parent)
            dr.connect()
            self.drs.append(dr)

        if bLinear:
            rectbackx2 = axes.plot([backx2,backx2],[ymin,ymax], '--', color='red')
        else:
            rectbackx2 = axes.semilogy([backx2,backx2],[ymin,ymax], '--', color='red')

        for rect in rectbackx2:
            dr = DraggableRectangle(rect, 'back_to', self, parent)
            dr.connect()
            self.drs.append(dr)

        #bottom plot will be for low resolution selection
        axes2 = self.fig.add_subplot(212)

        x = self.x2
        y = self.y2

        if bLinear:
            line2, = axes2.plot(x,y,color='black')
        else:
            line2, = axes2.semilogy(x,y,color='black')

        ymax=max(y)
        if bLinear:
            ymin=0
        else:
            ymin = 1

        #low resolution range
        peakx1 = self._lowresFromValue
        self.lowresFrom.setText(str(peakx1))
        peakx2 = self._lowresToValue
        self.lowresTo.setText(str(peakx2))
        if bLinear:
            rectpeakx1 = axes2.plot([peakx1,peakx1],[ymin,ymax], '--', color='green')
            rectpeakx2 = axes2.plot([peakx2,peakx2],[ymin,ymax], '--', color='green')
        else:
            rectpeakx1 = axes2.semilogy([peakx1,peakx1],[ymin,ymax], '--', color='green')
            rectpeakx2 = axes2.semilogy([peakx2,peakx2],[ymin,ymax], '--', color='green')

#        self.drs2 = []
        for rect in rectpeakx1:
            dr = DraggableRectangle(rect, 'lowres_from', self, parent)
            dr.connect()
            self.drs.append(dr)

        for rect in rectpeakx2:
            dr = DraggableRectangle(rect, 'lowres_to', self, parent)
            dr.connect()
            self.drs.append(dr)

        self.canvas.draw()

        #set the x-axes range visible
        axes.set_xlim(self.peak_back_xlim)
        axes2.set_xlim(self.low_res_xlim)

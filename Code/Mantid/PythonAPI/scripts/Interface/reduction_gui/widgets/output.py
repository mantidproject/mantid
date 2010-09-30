from PyQt4 import QtGui, uic, QtCore
from reduction_gui.settings.application_settings import GeneralSettings
from base_widget import BaseWidget

try:
    from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
    from matplotlib.figure import Figure
    HAS_MPL = True
except:
    HAS_MPL = False

class OutputWidget(BaseWidget):    
    """
        Widget that presents the transmission options to the user
    """
    _plot = None
    
    def __init__(self, parent=None, state=None, settings=None):
        BaseWidget.__init__(self, parent=parent, state=state, settings=settings)

        f = QtCore.QFile(":hfir_output.ui")
        f.open(QtCore.QIODevice.ReadOnly)
        uic.loadUi(f, self._content)
        self.initialize_content()
        
    def set_log(self, log_text):
        self._content.output_text_edit.setText(QtCore.QString(log_text))
    
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """

        # Clear data list
        self._content.output_text_edit.clear()
        
        if HAS_MPL:
            self.main_frame = QtGui.QWidget() 
        
            self.dpi = 100
            self.fig = Figure((5.0, 4.0), dpi=self.dpi)
            self.canvas = FigureCanvas(self.fig)
            self.canvas.setParent(self.main_frame)
            
            self.axes = self.fig.add_subplot(111)
            
            vbox = QtGui.QVBoxLayout()
            vbox.addWidget(self.canvas)
            
            self.main_frame.setLayout(vbox)
            
            self._content.plot_area_layout.addWidget(self.main_frame)
                        
    def plot_data(self, data):
        if HAS_MPL:
            self.axes.cla()
            self.axes.plot(data.x, data.y, 'r')
            self.axes.set_xlabel("Q [Angstrom^{-1}]")
            self.axes.set_ylabel("I(Q) [a.u.]")
            #self.axes.set_xscale("log")
            #self.axes.set_yscale("log")
            self.axes.grid(True)
            self.canvas.draw()
            
            
            
    
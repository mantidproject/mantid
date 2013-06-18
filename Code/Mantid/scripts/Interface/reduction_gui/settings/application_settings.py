from PyQt4 import QtGui, QtCore

class GeneralSettings(QtCore.QObject):
    """
        Settings class that will be passed from the main application
        to the control widgets.
    """
    data_path = '.'
    debug = False
    advanced = True
    last_data_ws = ''
    last_file = ''
    instrument_name = ''
    facility_name = ''
    data_output_dir = None
    
    # Mantid Python API version
    api2 = True
    
    data_updated = QtCore.pyqtSignal('PyQt_PyObject','PyQt_PyObject')
    progress = QtCore.pyqtSignal(int)
    
    def __init__(self, settings=None):
        """
            Initialization.
            @param settings: QSettings object passed by the main application
        """
        super(GeneralSettings, self).__init__() 
        if settings is not None:
            self.from_settings(settings)
            
    def emit_key_value(self, key, value):
        """
            Emit a signal to alert listeners of key/value update
        """
        self.data_updated.emit(key, value)
        
    def to_settings(self, settings):
        """
            Write the current settings to a QSettings object
            @param settings: QSettings object
        """
        last_dir = QtCore.QVariant(QtCore.QString(self.data_path))
        settings.setValue("general_data_path", last_dir)
        debug_mode = QtCore.QVariant(self.debug)
        settings.setValue("debug_mode", debug_mode)
        advanced_mode = QtCore.QVariant(self.advanced)
        settings.setValue("advanced_mode", advanced_mode)
        instr_name = QtCore.QVariant(QtCore.QString(self.instrument_name))
        settings.setValue("instrument_name", instr_name)
        facility_name = QtCore.QVariant(QtCore.QString(self.facility_name))
        settings.setValue("facility_name", facility_name)
        
    def from_settings(self, settings):
        """
            Get the settings from a QSettings object
            @param settings: QSettings object
        """
        self.data_path = unicode(settings.value("general_data_path", QtCore.QVariant('.')).toString())
        self.debug = settings.value("debug_mode", QtCore.QVariant('false')).toBool()
        self.advanced = settings.value("advanced_mode", QtCore.QVariant('true')).toBool()
        self.instrument_name = unicode(settings.value("instrument_name", QtCore.QVariant('')).toString())
        self.facility_name = unicode(settings.value("facility_name", QtCore.QVariant('')).toString())
        
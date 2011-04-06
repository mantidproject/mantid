from PyQt4 import QtGui, QtCore

class GeneralSettings(object):
    """
        Settings class that will be passed from the main application
        to the control widgets.
    """
    data_path = '.'
    debug = False
    last_data_ws = ''
    last_file = ''
    instrument_name = ''
    
    def __init__(self, settings=None):
        """
            Initialization.
            @param settings: QSettings object passed by the main application
        """
        if settings is not None:
            self.from_settings(settings)
            
    def to_settings(self, settings):
        """
            Write the current settings to a QSettings object
            @param settings: QSettings object
        """
        last_dir = QtCore.QVariant(QtCore.QString(self.data_path))
        settings.setValue("general_data_path", last_dir)
        debug_mode = QtCore.QVariant(self.debug)
        settings.setValue("debug_mode", debug_mode)
        
    def from_settings(self, settings):
        """
            Get the settings from a QSettings object
            @param settings: QSettings object
        """
        self.data_path = unicode(settings.value("general_data_path", QtCore.QVariant('.')).toString())
        self.debug = settings.value("debug_mode", QtCore.QVariant('false')).toBool()
        self.instrument_name = unicode(settings.value("instrument_name", QtCore.QVariant('.')).toString())
        
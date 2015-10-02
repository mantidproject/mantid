from PyQt4 import QtGui, QtCore
import threading
import time

# Only way that I have found to use the logger from both the command line and mantiplot
try:
    import mantidplot
    from mantid.kernel import logger
except:
    import logging
    logging.basicConfig(level=logging.DEBUG)
    logger = logging.getLogger("AppSettings")

'''
Notes:

From the command line:
$ cd ~/git/mantid/Code/Mantid/scripts/Interface 
$ python reduction_application.py
It uses:
.config/Mantid/Mantid Reduction.conf
From mantidplot:
.config/Mantid/MantidPlot.conf

'''

class GeneralSettings(QtCore.QObject):
    """
        Settings class that will be passed from the main application
        to the control widgets.
    """
    
    # Properties to be saved in the QSettings file
    _data_path = '.'
    _debug = False
    _advanced = True
    _last_data_ws = ''
    _last_file = ''
    _instrument_name = ''
    _facility_name = ''
    _data_output_dir = None

    # User information for remote submission
    cluster_user = None
    cluster_pass = None
    compute_resource = "Fermi"

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
            self._settings = settings
        else:
            self._settings = QtCore.QSettings()
        

    def emit_key_value(self, key, value):
        """
            Emit a signal to alert listeners of key/value update
        """
        logger.debug("emit_key_value: %s : %s"%(key,value))
        self.data_updated.emit(key, value)

    @property
    def data_path(self):
        self._data_path = unicode(self._settings.value("general_data_path", '.'))
        logger.debug("Getter data_path = %s"%str(self._data_path))
        return self._data_path
    
    @data_path.setter
    def data_path(self, val):
        val = str(val)
        logger.debug("Setter data_path = %s"%val)
        self._settings.setValue("general_data_path", val)
        self._data_path = val

    @property
    def debug(self):
        self._debug = unicode(self._settings.value("debug_mode", 'false')).lower()=='true'
        logger.debug("Getter debug = %s"%str(self._debug))
        return self._debug
    
    @debug.setter
    def debug(self, val):
        val = str(val)
        logger.debug("Setter debug = %s"%val)
        self._settings.setValue("debug_mode", val)
        self._debug = val


    @property
    def advanced(self):
        self._advanced = unicode(self._settings.value("advanced_mode", 'true')).lower()=='true'
        logger.debug("Getter advanced = %s"%str(self._advanced))
        return self._advanced
    
    @advanced.setter
    def advanced(self, val):
        val = str(val)
        logger.debug("Setter advanced = %s"%val)
        self._settings.setValue("advanced_mode", val)
        self._advanced = val

    @property
    def instrument_name(self):
        self._instrument_name = unicode(self._settings.value("instrument_name", 'true')).lower()=='true'
        logger.debug("Getter instrument_name = %s"%str(self._instrument_name))
        return self._instrument_name
    
    @instrument_name.setter
    def instrument_name(self, val):
        val = str(val)
        logger.debug("Setter instrument_name = %s"%val)
        self._settings.setValue("instrument_name", val)
        self._instrument_name = val

    @property
    def facility_name(self):        
        self._facility_name = unicode(self._settings.value("facility_name", 'true')).lower()=='true'
        logger.debug("Getter facility_name = %s"%str(self._facility_name))
        return self._facility_name
    
    @facility_name.setter
    def facility_name(self, val):
        val = str(val)
        logger.debug("Setter facility_name = %s"%val)
        self._settings.setValue("facility_name", val)
        self._facility_name = val
    

    @property
    def data_output_dir(self):        
        self._data_output_dir = unicode(self._settings.value("data_output_dir", 'true')).lower()=='true'
        logger.debug("Getter data_output_dir = %s"%self._data_output_dir)
        return self._data_output_dir
    
    @data_output_dir.setter
    def data_output_dir(self, val):
        val = str(val)
        logger.debug("Setter data_output_dir = %s"%val)
        self._settings.setValue("data_output_dir", val)
        self._data_output_dir = val
    
    def _log_dump_settings(self,settings):
        keys = settings.allKeys();
        for k in keys:
            try:
                v = str(settings.value(k))
            except:
                v = "ERROR READING"
            logger.debug("   %s : %s"%(str(k),v))

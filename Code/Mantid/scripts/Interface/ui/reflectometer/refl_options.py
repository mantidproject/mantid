import refl_options_window
from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class ReflOptions(QtGui.QDialog, refl_options_window.Ui_OptionsDialog):


    """
    Member variables
    """
    __frequency = 0
    __method = 0
    __method_list = ["Add","Replace","Append"]
    __ads_get = False
    __icat_search = False
    __icat_download = False

    def __init__(self, def_method, def_freq, def_ads_get, def_alg_use, def_icat_search, def_icat_download):
        """
        Initialise the interface
        """
        super(QtGui.QDialog, self).__init__()
        
        # Initialize member variables
        self.__ads_get = def_ads_get
        self.__alg_use = def_alg_use
        self.__method = def_method
        self.__frequency = def_freq
        self.__icat_search = def_icat_search
        self.__icat_download = def_icat_download
        
        
        self.setupUi(self)
        
        # Setup UI controls
        self.comboAccMethod.addItems(self.__method_list)
        if def_method in self.__method_list:
            self.comboAccMethod.setCurrentIndex(self.__method_list.index(def_method))
        else:
            self.comboAccMethod.setCurrentIndex(0)

        self.dspinFrequency.setValue(def_freq)
        self.checkADS.setChecked(def_ads_get)
        self.checkAlg.setChecked(def_alg_use)
        self.checkICATSearch.setChecked(def_icat_search)
        self.checkICATDownload.setChecked(def_icat_download)
        self.__sync_download_option(def_icat_search)

        
        #connect update signals to functions
        self.dspinFrequency.valueChanged.connect(self.__update_frequency)
        self.comboAccMethod.activated.connect(self.__update_method)
        self.checkADS.clicked.connect(self.__update_ADS_get)
        self.checkAlg.clicked.connect(self.__update_Alg_use)
        self.checkICATSearch.clicked.connect(self.__update_search_method)
        self.checkICATDownload.clicked.connect(self.__update_download_method)

    def __update_Alg_use(self, checked):
        self.__alg_use = checked

    def __update_ADS_get(self, checked):
        self.__ads_get = checked

    def __update_frequency(self, freq):
        self.__frequency = freq

    def __update_method(self, meth):
        self.__method = meth
        
    def __sync_download_option(self, do_icat_search):
        self.checkICATDownload.setEnabled(do_icat_search)
        if not do_icat_search:
            self.checkICATDownload.setChecked(False)
        
    
    def __update_search_method(self, checked):
        self.__icat_search = checked
        # Download options should only be enabled and checked if the search option is enabled
        self.__sync_download_option(do_icat_search = checked)
            
    def __update_download_method(self, checked):
        self.__icat_download = checked
        
    def icatSearch(self):
        return self.__icat_search
    
    def icatDownload(self):
        return (self.__icat_search and self.__icat_download)
    
    def frequency(self):
        return self.__frequency
    
    def useADS(self):
        return self.__ads_get
    
    def useAlg(self):
        return self.__alg_use
    
    def method(self):
        return self.__method
    
   
    

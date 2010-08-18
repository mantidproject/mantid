"""Excitations analysis common module
"""
import glob
import os.path
from mantidsimple import mtd, LoadEmptyInstrument

#----------- Conversion base class -------------
class EnergyConversion(object):
    '''
    Convert to energy interface
    '''
    
    def __init__(self, file_prefix):
        if self.__class__.__base__ == object:
            raise NotImplementedError('Cannot create object of type "%s". Use one of the different types of energy converter' % self.__class__.__name__)

        self._to_stdout = True
        self._log_to_mantid = False

        self.initialise(file_prefix)

    def initialise(self, file_prefix):
        '''
        Initialise the attributes of the class
        '''
        self.file_prefix = file_prefix
        self.file_ext = '.raw'
        self.save_formats = ['.nxs']
       
        self.setup_mtd_instrument()
        # Call possibly function in inheriting classes
        self.init_params()

    def init_params(self):
        '''
        Attach parameters as attributes of class. Base class does nothing
        '''
        pass

    def init_idf_params(self):
        '''
        Override to initialise parameters from an instrument file using the self.instrument attribute
        '''
        pass
        
    def setup_mtd_instrument(self, workspace = None):
        if workspace != None:
            self.instrument = workspace.getInstrument()
        else:
            # Load an empty instrument
            idf_dir = mtd.getConfigProperty('instrumentDefinition.directory')
            instr_pattern = os.path.join(idf_dir,self.file_prefix + '*_Definition.xml')
            idf_files = glob.glob(instr_pattern)
            if len(idf_files) > 0:
                tmp_ws_name = '_tmp_empty_instr'
                LoadEmptyInstrument(idf_files[0],tmp_ws_name)
                self.instrument = mtd[tmp_ws_name].getInstrument()
                # Instrument is cached so this is fine
                mtd.deleteWorkspace(tmp_ws_name)
            else:
                self.instrument = None
                raise RuntimeError('Cannot load instrument for prefix "%s"' % self.file_prefix)
        # Initialise IDF parameters
        self.init_idf_params()

    def log(self, msg):
        """Send a log message to the location defined
        """
        if self._to_stdout:
            print msg
        if self._log_to_mantid:
            mtd.sendLogMessage(msg)
            
    def convert_to_energy(self):
        raise NotImplementedError('Object does not define a "convert_to_energy" function, cannot continue.')
  
      
#--------------------------------------------------------------

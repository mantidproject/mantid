import MantidFramework
import mantidsimple

def instrument_factory(name):
    """
        Returns an instance of the instrument with the given class name
        @param name: name of the instrument class to instantiate
    """
    if name in globals():
        return globals()[name]()
    else:
        raise RuntimeError, "Instrument %s doesn't exist\n  %s" % (name, sys.exc_value)

class Instrument(object):
    def __init__(self, instr_filen=None):
        """
            Reads the instrument definition xml file
            @param instr_filen: the name of the instrument definition file to read 
            @raise IndexError: if any parameters (e.g. 'default-incident-monitor-spectrum') aren't in the xml definition
        """
        if instr_filen is None:
            instr_filen = self._NAME+'_Definition.xml'

        self._definition_file = MantidFramework.mtd.getConfigProperty(
            'instrumentDefinition.directory')+'/'+instr_filen
                
        self.definition = self.load_instrument() 

    def load_instrument(self):
        """
            Runs LoadInstrument get the parameters for the instrument
            @return the instrument parameter data
        """
        wrksp = '__'+self._NAME+'instrument_definition'
        if not MantidFramework.mtd.workspaceExists(wrksp):
          mantidsimple.CreateWorkspace(wrksp,"1","1","1")
          #read the information about the instrument that stored in its xml
          mantidsimple.LoadInstrument(wrksp, InstrumentName=self._NAME)

        return MantidFramework.mtd[wrksp].getInstrument()  

    def name(self):
        """
            Return the name of the instrument
        """
        return self._NAME
    
    def get_default_beam_center(self):
        """
            Returns the default beam center position, or the pixel location
            of real-space coordinates (0,0).
        """
        return [0, 0]

    def view(self, workspace_name = None):
        """
            Opens Mantidplot's InstrumentView displaying the current instrument. This
            empty instrument created contained in the named workspace (a default name
            is generated if this the argument is left blank) unless the workspace already
            exists and then it's contents are displayed
            @param workspace_name: the name of the workspace to create and/or display
        """
        if workspace_name is None:
            workspace_name = self._NAME+'_instrument_view'
            self.load_empty(workspace_name)
        elif not MantidFramework.mtd.workspaceExists(workspace_name):
            self.load_empty(workspace_name)

        instrument_win = mantidsimple.qti.app.mantidUI.getInstrumentView(workspace_name)
        instrument_win.showWindow()

        return workspace_name

    def load_empty(self, workspace_name = None):
        """
            Loads the instrument definition file into a workspace with the given name.
            If no name is given a hidden workspace is used
            @param workspace_name: the name of the workspace to create and/or display
            @return the name of the workspace that was created
        """
        if workspace_name is None:
            workspace_name = '__'+self._NAME+'_empty'

        mantidsimple.LoadEmptyInstrument(self._definition_file, workspace_name)

        return workspace_name
   
    def get_detector_from_pixel(self, pixel_list):
        """
            Returns a list of detector IDs from a list of [x,y] pixels,
            where the pixel coordinates are in pixel units.
        """
        return []
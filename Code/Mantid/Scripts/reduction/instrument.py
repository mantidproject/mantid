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
    def __init__(self, wrksp_name=None, instr_filen=None):
        """
            Reads the instrument definition xml file
            @param wrksp_name: Create a workspace with this containing the empty instrument, if it not set the instrument workspace is deleted afterwards
            @param instr_filen: the name of the instrument definition file to read 
            @raise IndexError: if any parameters (e.g. 'default-incident-monitor-spectrum') aren't in the xml definition
        """
        if instr_filen is None:
            instr_filen = self._NAME+'_Definition.xml'

        self._definition_file = MantidFramework.mtd.getConfigProperty(
            'instrumentDefinition.directory')+'/'+instr_filen
        
        if not wrksp_name is None:
            wrksp = wrksp_name
        else:
            wrksp = '_'+self._NAME+'instrument_definition'
        
        # Read instrument description
        wrksp = self.load_empty(wrksp_name)
        definitionWS = MantidFramework.mtd[wrksp]  
        self.definition = definitionWS.getInstrument()

        if wrksp_name is None:
            #we haven't been asked to leave the empty instrument workspace so remove it
            MantidFramework.mtd.deleteWorkspace(wrksp)

    def load_empty(self, wrksp_name=None):
        """
            Runs LoadEmptyInstrument to create an empty instrument in the named
            workspace (a default name is created if none is given)
            @param workspace_name: The empty instrument data will be contained in the named workspace
        """
        if not wrksp_name is None:
            wrksp = wrksp_name
        else:
            wrksp = '_'+self._NAME+'instrument_definition'
        #read the information about the instrument that stored in it's xml
        mantidsimple.LoadEmptyInstrument(self._definition_file, wrksp)

        return wrksp

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
            mantidsimple.LoadEmptyInstrument(self._definition_file, workspace_name)
        elif not MantidFramework.mtd.workspaceExists(workspace_name):
            mantidsimple.LoadEmptyInstrument(self._definition_file, workspace_name)
        

        instrument_win = mantidsimple.qti.app.mantidUI.getInstrumentView(workspace_name)
        instrument_win.showWindow()

        return workspace_name
    
    def get_detector_from_pixel(self, pixel_list):
        """
            Returns a list of detector IDs from a list of [x,y] pixels,
            where the pixel coordinates are in pixel units.
        """
        return []
import mantid.simpleapi as api
from mantid.kernel import *
from mantid.api import *

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

        config = ConfigService.Instance()
        self._definition_file = config["instrumentDefinition.directory"]+'/'+instr_filen

        self.definition = self.load_instrument()

    def get_default_instrument(self):
        instr_filen = self._NAME+'_Definition.xml'
        config = ConfigService.Instance()
        self._definition_file =config["instrumentDefinition.directory"]+'/'+instr_filen
        return self.load_instrument()

    def load_instrument(self):
        """
            Runs LoadInstrument get the parameters for the instrument
            @return the instrument parameter data
        """
        wrksp = '__'+self._NAME+'instrument_definition'
        if not AnalysisDataService.doesExist(wrksp):
            api.CreateWorkspace(OutputWorkspace=wrksp,DataX="1",DataY="1",DataE="1")
          #read the information about the instrument that stored in its xml
            api.LoadInstrument(Workspace=wrksp, InstrumentName=self._NAME)

        return AnalysisDataService.retrieve(wrksp).getInstrument()

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
        elif not AnalysisDataService.doesExist(workspace_name):
            self.load_empty(workspace_name)

        import mantidplot
        instrument_win = mantidplot.getInstrumentView(workspace_name)
        instrument_win.show()

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

        api.LoadEmptyInstrument(Filename=self._definition_file, OutputWorkspace=workspace_name)

        return workspace_name

    def get_detector_from_pixel(self, pixel_list, workspace=None):
        """
            Returns a list of detector IDs from a list of [x,y] pixels,
            where the pixel coordinates are in pixel units.
        """
        return []

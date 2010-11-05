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
    def __init__(self, wrksp_name=None):
        """
            Reads the instrument definition xml file
            @param wrksp_name: Create a workspace with this containing the empty instrument, if it not set the instrument workspace is deleted afterwards
            @raise IndexError: if any parameters (e.g. 'default-incident-monitor-spectrum') aren't in the xml definition
        """ 
    
        self._definition_file = \
            MantidFramework.mtd.getConfigProperty('instrumentDefinition.directory')+'/'+self._NAME+'_Definition.xml'
        
        if not wrksp_name is None:
            wrksp = wrksp_name
        else:
            wrksp = '_'+self._NAME+'instrument_definition'
        
        # Read instrument description
        mantidsimple.LoadEmptyInstrument(self._definition_file, wrksp)
        definitionWS = MantidFramework.mtd[wrksp]  
        self.definition = definitionWS.getInstrument()

        if wrksp_name is None:
            #we haven't been asked to leave the empty instrument workspace so remove it
            MantidFramework.mtd.deleteWorkspace(wrksp)
        
        
    def name(self):
        """
            Return the name of the instrument
        """
        return self._NAME
    

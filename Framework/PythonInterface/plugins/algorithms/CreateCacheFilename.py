#pylint: disable=no-init,invalid-name,bare-except
from mantid.api import *
from mantid.kernel import *
import mantid


# See ticket #14716

class CreateCacheFilename(PythonAlgorithm):
    """ Create cache filename
    """
    def category(self):
        """
        """
        return "Utils"

    def name(self):
        """
        """
        return "CreateCacheFilename"

    def summary(self):
        """ Return summary
        """
        return "Create cache filename"

    def require(self):
        return

    def PyInit(self):
        """ Declare properties
        """
        # this is the requirement of using this plugin
        # is there a place to register that?
        self.require()

        self.declareProperty("PropertyManager", "", "name of a property manager from which properties are extracted from")

        self.declareProperty(
            StringArrayProperty("Properties", "", Direction.Input),
            "A list of property names to be included")

        self.declareProperty(
            StringArrayProperty("OtherProperties", "", Direction.Input),
            "A list of key=value strings for other properties not in the property manager")

        self.declareProperty(
            StringProperty("Prefix", "", Direction.Input),
            "prefix to the output hash name")

        self.declareProperty(
            StringProperty("CacheDir", "", Direction.Input),
            "the directory in which the cache file will be created")
        return

    def PyExec(self):
        """ Main Execution Body
        """
        # Inputs
        prop_manager = self.getPropertyValue("PropertyManager")
        other_props = self.getPropertyValue("OtherProperties")
        if not prop_manager and not other_props:
            raise ValueError("Either PropertyManager or OtherProperties should be supplied")
        prop_manager = mantid.PropertyManagerDataService.retrieve(prop_manager)

        props = self.getPropertyValue("Properties")
        # default to all properties in the manager
        if not props:
            props = prop_manager.keys()

        prefix = self.getPropertyValue("Prefix")
        cache_dir = self.getPropertyValue("CacheDir")
        if not cache_dir:
            cache_dir = 
        
        # Generate Json file
        self._save(inputws, outfilename, plotname)
        return

# Register algorithm with Mantid
AlgorithmFactory.subscribe(CreateCacheFilename)


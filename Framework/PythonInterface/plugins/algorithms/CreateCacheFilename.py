#pylint: disable=no-init,invalid-name,bare-except
from mantid.api import *
from mantid.kernel import *
import mantid, os


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
        return """Create cache filename

The purpose of this algorithm is to create a unique
filename for a cache so that a workflow can reuse
results from previous computations.

The algorithm will accept a prefix, PropertyManager, list of properties 
to use from the property manager (empty is use all), and 
a string array (or List) of other properties to use, and 
a directory for cache files to exist in (default described below).

The list of property names will be used to select which of the properties 
in the PropertyManager will be used to calculate the hash 
and will be interpreted as globbing.

The string array of other_properties will be key/value pairs of properties 
that should be considered, but are not in the provided PropertyManager.

If a directory is not specified, cache files will go into a cache 
subdirectory of ConfigService::getUserPropertiesDir().
On unix this will be ~/.mantid/cache.

The algorithm will convert all properties to strings as 
"%s=%s" % (property.name, property.valueAsStr), sort the list, 
then convert it to a sha1.

A filename with the form <location>/<prefix>_<sha1>.nxs 
will be returned as the output property.
If no prefix is specified then file result will be <location>/<sha1>.nxs.

property_manager: an instance of PropertyManager from which property values
        can be retrieved. None means we don't care about property manager 
        -- all properties will come from other_properties
properties: a list of strings. each string is a property managed by the 
        given property_manager, or it can be glob pattern to match prop
        names too. but empty list means taking all properties 
        from the property_manager
other_properties: a list of strings. each string is in the form of
        "key=value" for one property not managed by the property_manager.
        no globbing here.
prefix: prefix to the output hash name. when it is empty, just the hash.
        when it is not empty, it will be <prefix>_<sha1>
cache_dir: the directory in which the cach file will be created. 
        empty string means default as described above
"""

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
            StringArrayProperty("Properties", Direction.Input),
            "A list of property names to be included")

        self.declareProperty(
            StringArrayProperty("OtherProperties", Direction.Input),
            "A list of key=value strings for other properties not in the property manager")

        self.declareProperty(
            "Prefix", "", "prefix to the output hash name")

        self.declareProperty(
            "CacheDir", "",
            "the directory in which the cache file will be created")

        self.declareProperty(
            "OutputFilename", "",
            "output filename")
        return

    def PyExec(self):
        """ Main Execution Body
        """
        # Inputs
        prop_manager = self.getPropertyValue("PropertyManager")
        other_props = self.getProperty("OtherProperties").value
        if not prop_manager and not other_props:
            raise ValueError("Either PropertyManager or OtherProperties should be supplied")
        prop_manager = mantid.PropertyManagerDataService.retrieve(prop_manager)\
                       if prop_manager else None        
        # default to all properties in the manager
        props = self.getProperty("Properties").value
        if not props and prop_manager:
            props = prop_manager.keys()
        # output settings
        prefix = self.getPropertyValue("Prefix")
        cache_dir = self.getPropertyValue("CacheDir")
        if not cache_dir:
            cache_dir = os.path.join(
                ConfigService.getUserPropertiesDir(),
                "cache"
                )
        # calculate
        fn = self._calculate(
            prop_manager, props, other_props, prefix, cache_dir)
        self.setProperty("OutputFilename", fn)
        return

    def _calculate(self, prop_manager, props, other_props, prefix, cache_dir):
        # get matched properties
        if prop_manager:
            props = matched(prop_manager.keys(), props)
            # create the list of key=value strings
            kvpairs = [ 
                '%s=%s' % (prop, prop_manager.getPropertyValue(prop))
                for prop in props
            ]
        else:
            kvpairs = []
        kvpairs += other_props
        # sort
        kvpairs.sort()
        # one string out of the list
        s = ','.join(kvpairs)
        # hash
        h = hash(s)
        # prefix
        if prefix:
            h = "%s_%s" % (prefix, h)
        # filename
        fn = "%s.nxs" % h
        return os.path.join(cache_dir, fn)


def hash(s):
    import hashlib
    return hashlib.sha1(s).hexdigest()


def matched(keys, patterns):
    "return keys that match any of the given patterns"
    import fnmatch
    filtered = []
    for pat in patterns:
        filtered += fnmatch.filter(keys, pat)
        continue
    return set(filtered)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CreateCacheFilename)


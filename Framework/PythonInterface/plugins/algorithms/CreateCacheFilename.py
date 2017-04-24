#pylint: disable=no-init,invalid-name,bare-except,too-many-arguments
from __future__ import (absolute_import, division, print_function)

from mantid.api import *
from mantid.kernel import *
import mantid
import os


# See ticket #14716

class CreateCacheFilename(PythonAlgorithm):
    """ Create cache filename
    """

    def category(self):
        """
        """
        return "Workflow\\DataHandling"

    def name(self):
        """
        """
        return "CreateCacheFilename"

    def summary(self):
        """ Return summary
        """
        return """Create cache filename"""

    def require(self):
        return

    def PyInit(self):
        """ Declare properties
        """
        # this is the requirement of using this plugin
        # is there a place to register that?
        self.require()

        self.declareProperty("PropertyManager", "", "Name of a property manager from which properties are extracted from")

        self.declareProperty(
            StringArrayProperty("Properties", Direction.Input),
            "A list of property names to be included")

        self.declareProperty(
            StringArrayProperty("OtherProperties", Direction.Input),
            "A list of key=value strings for other properties not in the property manager")

        self.declareProperty(
            "Prefix", "", "prefix for the output file name")

        self.declareProperty(
            "CacheDir", "",
            "the directory in which the cache file will be created")

        self.declareProperty("OutputFilename", "", "Full path of output file name", Direction.Output)

        self.declareProperty("OutputSignature", "", "Calculated sha1 hash", Direction.Output)
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
            props = list(prop_manager.keys())
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

    def _get_signature(self, prop_manager, props, other_props):
        # get matched properties
        if prop_manager:
            props = matched(list(prop_manager.keys()), props)
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
        self.setProperty("OutputSignature", s)
        return s

    def _calculate(self, prop_manager, props, other_props, prefix, cache_dir):
        s = self._get_signature(prop_manager, props, other_props)
        # hash
        h = _hash(s)
        # prefix
        if prefix:
            h = "%s_%s" % (prefix, h)
        # filename
        fn = "%s.nxs" % h
        return os.path.join(cache_dir, fn)


def _hash(s):
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

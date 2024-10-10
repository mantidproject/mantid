# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,bare-except,too-many-arguments
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm
from mantid.kernel import ConfigService, Direction, StringArrayProperty
import mantid
import os


# See ticket #14716


class CreateCacheFilename(PythonAlgorithm):
    """Create cache filename"""

    def category(self):
        """ """
        return "Workflow\\DataHandling"

    def name(self):
        """ """
        return "CreateCacheFilename"

    def summary(self):
        """Return summary"""
        return """Create cache filename"""

    def PyInit(self):
        """Declare properties"""
        # this is the requirement of using this plugin
        # is there a place to register that?
        self.declareProperty("PropertyManager", "", "Name of a property manager from which properties are extracted from")
        self.declareProperty(StringArrayProperty("Properties", Direction.Input), "A list of property names to be included")
        self.declareProperty(
            StringArrayProperty("OtherProperties", Direction.Input),
            "A list of key=value strings for other properties not in the property manager",
        )
        self.declareProperty("Prefix", "", "prefix for the output file name")
        self.declareProperty(
            FileProperty(name="CacheDir", defaultValue="", action=FileAction.OptionalDirectory),
            doc="Directory storing cache files for reuse, in-lieu of repetitive, time-consuming calculations",
        )
        self.declareProperty("OutputFilename", "", "Full path of output file name", Direction.Output)
        self.declareProperty("OutputSignature", "", "sha1 string, 40 characters long", Direction.Output)
        return

    def validateInputs(self):
        issues = dict()

        manager = self.getPropertyValue("PropertyManager").strip()
        if len(manager) > 0 and manager not in mantid.PropertyManagerDataService:
            issues["PropertyManager"] = "Does not exist"
        if len(manager) <= 0 and not self.getProperty("OtherProperties").value:
            message = "Either PropertyManager or OtherProperties should be supplied"
            issues["PropertyManager"] = message
            issues["OtherProperties"] = message

        return issues

    def PyExec(self):
        """Main Execution Body"""
        # Inputs
        prop_manager = self.getPropertyValue("PropertyManager").strip()
        if prop_manager in mantid.PropertyManagerDataService:
            prop_manager = mantid.PropertyManagerDataService[prop_manager]
        else:
            prop_manager = None

        other_props = self.getProperty("OtherProperties").value

        if not prop_manager and not other_props:
            raise ValueError("Either PropertyManager or OtherProperties should be supplied")

        # default to all properties in the manager
        props = self.getProperty("Properties").value
        if not props and prop_manager:
            props = list(prop_manager.keys())
        # output settings
        prefix = self.getPropertyValue("Prefix")
        cache_dir = self.getPropertyValue("CacheDir")
        if not cache_dir:
            cache_dir = os.path.join(ConfigService.getUserPropertiesDir(), "cache")
        # calculate
        file_name, sha1_hash = self._calculate(prop_manager, props, other_props, prefix, cache_dir)
        self.setProperty("OutputFilename", file_name)
        self.setProperty("OutputSignature", sha1_hash)
        return

    def _get_signature(self, prop_manager, props, other_props):
        # get matched properties
        if prop_manager:
            props = matched(list(prop_manager.keys()), props)
            # create the list of key=value strings
            kvpairs = ["%s=%s" % (prop, prop_manager.getPropertyValue(prop)) for prop in props]
        else:
            kvpairs = []
        kvpairs += other_props
        # sort
        kvpairs.sort()
        # one string out of the list
        s = ",".join(kvpairs)
        return s

    def _calculate(self, prop_manager, props, other_props, prefix, cache_dir):
        s = self._get_signature(prop_manager, props, other_props)
        h = _hash(s)  # sha1 hash
        if prefix:
            fn = "%s_%s.nxs" % (prefix, h)  # filename
        else:
            fn = "%s.nxs" % h
        return os.path.join(cache_dir, fn), h


def _hash(s):
    import hashlib

    return hashlib.sha1(str(s).encode("utf-8")).hexdigest()


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

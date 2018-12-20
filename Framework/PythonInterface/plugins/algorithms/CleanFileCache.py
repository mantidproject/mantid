#pylint: disable=no-init,invalid-name,bare-except,too-many-arguments,multiple-statements
from __future__ import (absolute_import, division, print_function)

from mantid.api import *
from mantid.kernel import *
import os


# See ticket #14716

class CleanFileCache(PythonAlgorithm):
    """ Remove cache files from the cache directory
    """

    def category(self):
        """
        """
        return "Workflow\\DataHandling"

    def seeAlso(self):
        return [ "ClearCache" ]

    def name(self):
        """
        """
        return "CleanFileCache"

    def summary(self):
        """ Return summary
        """
        return """Remove cache files"""

    def require(self):
        return

    def PyInit(self):
        """ Declare properties
        """
        # this is the requirement of using this plugin
        # is there a place to register that?
        self.require()

        self.declareProperty(
            "CacheDir", "",
            "the directory in which the cache file will be created. If nothing is given, default location for cache files will be used",
            Direction.Input)

        self.declareProperty(
            "AgeInDays", 14,
            "If any file is more than this many days old, it will be deleted. 0 means remove everything",
            Direction.Input)
        return

    def PyExec(self):
        """ Main Execution Body
        """
        # Inputs
        cache_dir = self.getPropertyValue("CacheDir")
        if not cache_dir:
            cache_dir = os.path.join(
                ConfigService.getUserPropertiesDir(),
                "cache"
                )
        age = int(self.getPropertyValue("AgeInDays"))
        #
        _run(cache_dir, age)
        return


def _run(cache_dir, days):
    import glob
    import re
    import time
    from datetime import timedelta, date
    rm_date = date.today() - timedelta(days = days)
    rm_date = time.mktime(rm_date.timetuple()) + 24*60*60
    for f in glob.glob(os.path.join(cache_dir, "*.nxs")):
        # skip over non-files
        if not os.path.isfile(f):
            continue
        # skip over new files
        # print os.stat(f).st_mtime, rm_date
        if os.stat(f).st_mtime > rm_date:
            continue
        # check filename pattern
        base = os.path.basename(f)
        if re.match(".*_[0-9a-f]{40}.nxs", base):
            os.remove(f)
            continue
        if re.match("[0-9a-f]{40}.nxs", base):
            os.remove(f)
            continue
        continue
    return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CleanFileCache)

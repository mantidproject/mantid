# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,bare-except,too-many-arguments,multiple-statements
from mantid.api import AlgorithmFactory, PythonAlgorithm
from mantid.kernel import ConfigService, Direction
import os


def _print_while_testing(msg: str) -> None:
    """If we are testing then print out extra information"""
    # This is currently keyed to the Jenkins names that are known
    if "JOB_NAME" in os.environ:
        print(msg, flush=True)


class CleanFileCache(PythonAlgorithm):
    """Remove cache files from the cache directory"""

    def category(self):
        """ """
        return "Workflow\\DataHandling"

    def seeAlso(self):
        return ["ClearCache"]

    def name(self):
        """ """
        return "CleanFileCache"

    def summary(self):
        """Return summary"""
        return """Remove cache files"""

    def require(self):
        return

    def PyInit(self):
        """Declare properties"""
        # this is the requirement of using this plugin
        # is there a place to register that?
        self.require()

        self.declareProperty(
            "CacheDir",
            "",
            "the directory in which the cache file will be created. If nothing is given, default location for cache files will be used",
            Direction.Input,
        )

        self.declareProperty(
            "AgeInDays", 14, "If any file is more than this many days old, it will be deleted. 0 means remove everything", Direction.Input
        )
        return

    def PyExec(self):
        """Main Execution Body"""
        # Inputs
        cache_dir = self.getPropertyValue("CacheDir")
        if not cache_dir:
            cache_dir = os.path.join(ConfigService.getUserPropertiesDir(), "cache")
        age = int(self.getPropertyValue("AgeInDays"))
        #
        _run(cache_dir, age)
        return


def _run(cache_dir, days):
    import glob
    import re
    import time
    from datetime import timedelta, date

    rm_date = date.today() - timedelta(days=days)
    rm_date = time.mktime(rm_date.timetuple()) + 24 * 60 * 60
    for f in glob.glob(os.path.join(cache_dir, "*.nxs")):
        # skip over non-files
        if not os.path.isfile(f):
            continue
        # skip over new files
        mtime = os.stat(f).st_mtime
        _print_while_testing(f"{f}: mtime={mtime}, rm_date={rm_date}")
        if mtime > rm_date:
            _print_while_testing(f"  skipping {f}")
            continue
        # check filename pattern
        base = os.path.basename(f)
        if re.match(".*_[0-9a-f]{40}.nxs", base):
            _print_while_testing(f"  removing {f}")
            os.remove(f)
            continue
        if re.match("[0-9a-f]{40}.nxs", base):
            _print_while_testing(f"  removing {f}")
            os.remove(f)
            continue
        continue
    return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CleanFileCache)

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=eval-used
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *


class SelectNexusFilesByMetadata(PythonAlgorithm):
    _criteria_splitted = []

    def category(self):
        return "DataHandling\\Nexus"

    def summary(self):
        return "Filters nexus files by metadata criteria"

    def validateInputs(self):
        issues = dict()
        criteria = self.getPropertyValue("NexusCriteria")
        # at least one nexus entry should be specified
        dollars = criteria.count("$")
        if dollars % 2 != 0 or dollars < 2:
            issues["NexusCriteria"] = "Make sure the nexus entry name is enclosed with $ sybmols"
        else:
            # check if the syntax of criteria is valid by replacing the nexus entries with dummy values
            self._criteria_splitted = criteria.split("$")
            toeval = ""
            for i, item in enumerate(self._criteria_splitted):
                if i % 2 == 1:  # at odd indices will always be the nexus entry names
                    # replace nexus entry names by 0
                    toeval += "0"
                else:
                    # keep other portions intact
                    toeval += item
            try:
                eval(toeval)
            except (NameError, ValueError, SyntaxError):
                issues["NexusCriteria"] = "Invalid syntax, check NexusCriteria."

        return issues

    def PyInit(self):
        self.declareProperty(MultipleFileProperty("FileList", extensions=["nxs", "hdf"]), doc="List of input files")
        self.declareProperty(
            name="NexusCriteria",
            defaultValue="",
            doc="Logical expresion for metadata criteria using python syntax. "
            "Provide full absolute names for nexus entries enclosed with $ symbol from both sides.",
        )
        self.declareProperty(
            name="Result",
            defaultValue="",
            direction=Direction.Output,
            doc="Comma separated list of the fully resolved file names satisfying the given criteria.",
        )

    def PyExec(self):
        # run only if h5py is present
        try:
            import h5py
        except ImportError:
            raise RuntimeError("This algorithm requires h5py package. See https://pypi.python.org/pypi/h5py")

        outputfiles = ""
        # first split by ,
        for runs in self.getPropertyValue("FileList").split(","):
            filestosum = ""
            # then split each by +
            for run in runs.split("+"):
                with h5py.File(run, "r") as nexusfile:
                    if self.checkCriteria(run, nexusfile):
                        filestosum += run + "+"

            if filestosum:
                # trim the last +
                filestosum = filestosum[:-1]
                outputfiles += filestosum + ","

        # trim the last ,
        if outputfiles:
            outputfiles = outputfiles[:-1]
        else:
            self.log().notice("No files where found to satisfy the criteria, check the FileList and/or NexusCriteria")

        self.setPropertyValue("Result", outputfiles)

    def checkCriteria(self, run, nexusfile):
        toeval = ""
        item = None  # for pylint
        for i, item in enumerate(self._criteria_splitted):
            if i % 2 == 1:  # at odd indices will always be the nexus entry names
                try:
                    # try to get the entry from the file
                    entry = nexusfile.get(item)

                    if len(entry.shape) > 1 or len(entry) > 1:
                        self.log().warning(
                            "Nexus entry %s has more than one dimension or more than one element"
                            "in file %s. Skipping the file." % (item, run)
                        )
                        return False

                    # replace entry name by it's value
                    value = entry[0]

                    if str(value.dtype).startswith("|S"):
                        # string value, need to quote for eval
                        value = value.decode()
                        toeval += '"' + value + '"'
                    else:
                        toeval += str(value)

                except (TypeError, AttributeError):
                    self.log().warning("Nexus entry %s does not exist in file %s. Skipping the file." % (item, run))
                    return False
            else:
                # keep other portions intact
                toeval += item
        self.log().debug("Expression to be evaluated for file %s :\n %s" % (run, toeval))
        try:
            return eval(toeval)
        except (NameError, ValueError, SyntaxError):
            # even if syntax is validated, eval can still throw, since
            # the nexus entry value itself can be spurious for a given file
            self.log().warning("Invalid value for the nexus entry %s in file %s. Skipping the file." % (item, run))
            return False


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SelectNexusFilesByMetadata)

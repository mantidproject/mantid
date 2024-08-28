# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm


class MergeCalFiles(PythonAlgorithm):
    def category(self):
        return "DataHandling\\Text;Diffraction\\DataHandling\\CalFiles"

    def seeAlso(self):
        return [
            "ReadGroupsFromFile",
            "CreateDummyCalFile",
            "CreateCalFileByNames",
            "DiffractionFocussing",
            "LoadCalFile",
            "SaveCalFile",
        ]

    def name(self):
        return "MergeCalFiles"

    def summary(self):
        return "Combines the data from two Cal Files."

    def PyInit(self):
        self.declareProperty(
            FileProperty("UpdateFile", "", FileAction.Load, ["cal"]), doc="The cal file containing the updates to merge into another file."
        )
        self.declareProperty(
            FileProperty("MasterFile", "", FileAction.Load, ["cal"]), doc="The master file to be altered, the file must be sorted by UDET"
        )
        self.declareProperty(FileProperty("OutputFile", "", FileAction.Save, ["cal"]), doc="The file to contain the results")

        self.declareProperty(
            "MergeOffsets", False, doc="If True, the offsets from file1 will be merged " + "to the master file. Default: False"
        )
        self.declareProperty(
            "MergeSelections", False, doc="If True, the selections from file1 will be merged " + "to the master file. Default: False"
        )
        self.declareProperty(
            "MergeGroups", False, doc="If True, the Groups from file1 will be merged to " + "the master file. Default: False"
        )

    # pylint: disable=too-many-branches
    def PyExec(self):
        # extract settings
        mergeOffsets = self.getProperty("MergeOffsets").value
        mergeSelections = self.getProperty("MergeSelections").value
        mergeGroups = self.getProperty("MergeGroups").value
        updateFileName = self.getPropertyValue("UpdateFile")
        masterFileName = self.getPropertyValue("MasterFile")
        outputFileName = self.getPropertyValue("OutputFile")

        if masterFileName == outputFileName:
            raise RuntimeError("The output file must be different to the master file.")

        self.DisplayMessage(mergeOffsets, mergeSelections, mergeGroups, updateFileName, masterFileName)

        updateFile = open(updateFileName, "r")
        updateDict = dict()
        lastNumber = 0
        linesUpdated = 0
        linesUntouched = 0
        linesAdded = 0
        for line in updateFile:
            if not self.IsComment(line):
                # process line
                try:
                    (number, UDET, offset, select, group) = self.ProcessLine(line)
                except ValueError:
                    pass
                # remeber all of the values
                updateDict[UDET] = (offset, select, group)

        updateFile.close()
        self.log().information(str(len(updateDict)) + " updates found in " + updateFileName)

        masterFile = open(masterFileName, "r")
        outputFile = open(outputFileName, "w")

        for line in masterFile:
            if self.IsComment(line):
                # copy the comment over
                outputFile.write(line)
            else:
                # process line
                try:
                    (number, UDET, masterOffset, masterSelect, masterGroup) = self.ProcessLine(line)
                    lastNumber = number
                    # If line to be updated
                    if UDET in updateDict:
                        (offset, select, group) = updateDict.pop(UDET)
                        linesUpdated += 1
                        if mergeOffsets:
                            masterOffset = offset
                        if mergeSelections:
                            masterSelect = select
                        if mergeGroups:
                            masterGroup = group
                    else:
                        linesUntouched += 1
                    outputFile.write(self.FormatLine(number, UDET, masterOffset, masterSelect, masterGroup))
                except ValueError:
                    # invalid line - ignore it
                    #          linesInvalid += 1
                    pass

        # add any lines at the end
        for UDET in updateDict.keys():
            (offset, select, group) = updateDict[UDET]
            lastNumber += 1
            outputFile.write(self.FormatLine(lastNumber, UDET, offset, select, group))
            linesAdded += 1

        self.log().information("{0} lines Updated, {1} lines added, {2} lines untouched".format(linesUpdated, linesAdded, linesUntouched))
        # close the files
        masterFile.close()
        outputFile.close()

    # pylint: disable=too-many-arguments
    def DisplayMessage(self, mergeOffsets, mergeSelections, mergeGroups, fileName1, fileName2):
        # Log the settings string
        outputString = "Merging "
        if mergeOffsets:
            outputString += "offsets, "
        if mergeSelections:
            outputString += "selections, "
        if mergeGroups:
            outputString += "groups, "
        # strip the final comma
        outputString = outputString[0 : len(outputString) - 2]
        outputString += " from file " + fileName1 + " into " + fileName2
        self.log().information(outputString)

    def IsComment(self, line):
        return line.startswith("#")

    def ProcessLine(self, line):
        try:
            elements = line.split()
            number = int(elements[0])
            UDET = int(elements[1])
            offset = float(elements[2])
            select = int(elements[3])
            group = int(elements[4])
        except:
            raise ValueError("invalid line: " + line)
        return (number, UDET, offset, select, group)

    # pylint: disable=too-many-arguments
    def FormatLine(self, number, UDET, offset, select, group):
        line = "{0:9d}{1:16d}{2:16.7f}{3:9d}{4:9d}\n".format(number, UDET, offset, select, group)
        return line


#############################################################################################
AlgorithmFactory.subscribe(MergeCalFiles())

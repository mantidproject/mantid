# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import mantid
import mantid.api
import mantid.simpleapi
import mantid.kernel
import numpy


class GenerateGroupingSNSInelastic(mantid.api.PythonAlgorithm):
    """Class to generate grouping file"""

    def category(self):
        """Mantid required"""
        return "Inelastic\\Utility;Transforms\\Grouping"

    def seeAlso(self):
        return ["GroupWorkspaces"]

    def name(self):
        """Mantid required"""
        return "GenerateGroupingSNSInelastic"

    def summary(self):
        """Mantid required"""
        return "Generate grouping files for ARCS, CNCS, HYSPEC, and SEQUOIA."

    def PyInit(self):
        """Python initialization:  Define input parameters"""
        py = ["1", "2", "4", "8", "16", "32", "64", "128"]
        px = ["1", "2", "4", "8"]
        instrument = ["ARCS", "CNCS", "HYSPEC", "SEQUOIA", "InstrumentDefinitionFile"]

        self.declareProperty("AlongTubes", "1", mantid.kernel.StringListValidator(py), "Number of pixels across tubes to be grouped")
        self.declareProperty("AcrossTubes", "1", mantid.kernel.StringListValidator(px), "Number of pixels across tubes to be grouped")
        self.declareProperty(
            "Instrument", instrument[0], mantid.kernel.StringListValidator(instrument), "The instrument for wich to create grouping"
        )

        IDF_enabled = mantid.kernel.VisibleWhenProperty("Instrument", mantid.kernel.PropertyCriterion.IsEqualTo, "InstrumentDefinitionFile")
        f_in = mantid.api.FileProperty("InstrumentDefinitionFile", "", mantid.api.FileAction.OptionalLoad, ".xml")
        self.declareProperty(f_in, doc="Select the instrument definition file from only one of ARCS, SEQUOIA, CNCS, HYSPEC")
        self.setPropertySettings("InstrumentDefinitionFile", IDF_enabled)

        f = mantid.api.FileProperty("Filename", "", mantid.api.FileAction.Save, ".xml")

        self.declareProperty(f, "Output filename.")

        return

    def validateInputs(self):
        errors = dict()

        # only one can be set
        if (self.getProperty("Instrument").value == "InstrumentDefinitionFile") and (
            not self.getProperty("InstrumentDefinitionFile").value.strip()
        ):
            errors["InstrumentDefinitionFile"] = "You must select an instrument or instrument definition file"

        return errors

    def PyExec(self):
        """Main execution body"""
        # 1. Get input
        pixelsy = int(self.getProperty("AlongTubes").value)
        pixelsx = int(self.getProperty("AcrossTubes").value)
        instrument = self.getProperty("Instrument").value
        filename = self.getProperty("Filename").value
        IDF = self.getProperty("InstrumentDefinitionFile").value

        ###
        __w = None
        if instrument != "InstrumentDefinitionFile":
            IDF_instrument = mantid.api.ExperimentInfo.getInstrumentFilename(instrument)
            __w = mantid.simpleapi.LoadEmptyInstrument(Filename=IDF_instrument)

        if IDF:
            if __w:
                mantid.kernel.logger.warning("Instrument definition file will be ignored if instrument is selected")
            else:
                __w = mantid.simpleapi.LoadEmptyInstrument(Filename=IDF)
                # checks the instrument from the loaded IDF belongs to the DGS/SNS suite.
                if __w.getInstrument().getName() not in ["ARCS", "CNCS", "HYSPEC", "SEQUOIA"]:
                    raise ValueError("Select the instrument definition file from only one of ARCS, SEQUOIA, CNCS, HYSPEC")
                # set instrument to instrument name from the loaded IDF instead of drop down menu
                instrument = __w.getInstrument().getName()

        i = 0
        spectrumInfo = __w.spectrumInfo()
        while spectrumInfo.isMonitor(i):
            i += 1
        # i is the index of the first true detector
        # now, crop the workspace of the monitors
        __w = mantid.simpleapi.CropWorkspace(__w, StartWorkspaceIndex=i)

        # get number of detectors (not including monitors)
        y = __w.extractY()
        numdet = (y[y == 1]).size

        spectra = numpy.arange(numdet).reshape(-1, 8, 128)

        banks = numdet // 8 // 128

        f = open(filename, "w")

        f.write('<?xml version="1.0" encoding="UTF-8" ?>\n<detector-grouping instrument="' + instrument + '">\n')

        groupnum = 0
        for i in range(banks):
            for j in range(0, 8, pixelsx):
                for k in range(0, 128, pixelsy):

                    groupname = str(groupnum)
                    ids = spectra[i, j : j + pixelsx, k : k + pixelsy].reshape(-1)
                    detids = []
                    for l in ids:
                        detids.append(__w.getDetector(int(l)).getID())

                    detids = str(detids).replace("[", "").replace("]", "")

                    f.write('<group name="' + groupname + '">\n   <detids val="' + detids + '"/> \n</group>\n')
                    groupnum += 1
        f.write("</detector-grouping>")
        f.close()
        mantid.simpleapi.DeleteWorkspace(__w.name())
        return


mantid.api.AlgorithmFactory.subscribe(GenerateGroupingSNSInelastic)

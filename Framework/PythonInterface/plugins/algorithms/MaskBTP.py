# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi
import mantid.api
from mantid.kernel import Direction, IntArrayProperty, StringListValidator
import numpy
from collections import defaultdict


class MaskBTP(mantid.api.PythonAlgorithm):
    """ Class to generate grouping file
    """

    # list of supported instruments
    INSTRUMENT_LIST = ['ARCS','CNCS','CORELLI','HYSPEC','MANDI','NOMAD','POWGEN','REF_M','SEQUOIA','SNAP','SXD','TOPAZ','WAND','WISH']

    instname = None
    instrument = None
    bankmin = None
    bankmax = None

    def category(self):
        """ Mantid required
        """
        return "Transforms\\Masking;Inelastic\\Utility"

    def seeAlso(self):
        return [ "MaskDetectors","MaskInstrument" ]

    def name(self):
        """ Mantid required
        """
        return "MaskBTP"

    def summary(self):
        """ Mantid required
        """
        return "Algorithm to mask detectors in particular banks, tube, or pixels."

    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty("Workspace", "",direction=Direction.InOut,
                                                          optional = mantid.api.PropertyMode.Optional), "Input workspace (optional)")
        allowedInstrumentList=StringListValidator(['']+self.INSTRUMENT_LIST)
        self.declareProperty("Instrument","",validator=allowedInstrumentList,doc="One of the following instruments: "
                             + ', '.join(self.INSTRUMENT_LIST))
        self.declareProperty(IntArrayProperty(name="Bank", values=[]),
                             doc="Bank(s) to be masked. If empty, will apply to all banks")
        self.declareProperty("Tube","",doc="Tube(s) to be masked. If empty, will apply to all tubes")
        self.declareProperty("Pixel","",doc="Pixel(s) to be masked. If empty, will apply to all pixels")
        self.declareProperty(IntArrayProperty(name="MaskedDetectors", direction=Direction.Output),
                             doc="List of  masked detectors")

    #pylint: disable=too-many-branches
    def PyExec(self):
        ws = self.getProperty("Workspace").value
        self.instrument=None
        self.instname = self.getProperty("Instrument").value

        if ws is not None:
            self.instrument = ws.getInstrument()
            self.instname = self.instrument.getName()

        # special cases are defined, default value is in front
        self.bankmin=defaultdict(lambda: 1, {"MANDI":1,"SEQUOIA":23,"TOPAZ":10})
        self.bankmax={"ARCS":115,"CNCS":50,"CORELLI":91,"HYSPEC":20,"MANDI":59,"NOMAD":99,"POWGEN":300,"REF_M":1,
                      "SEQUOIA":150,"SNAP":64,"SXD":11,"TOPAZ":59,"WAND":8,"WISH":10}
        tubemin=defaultdict(int, {"ARCS":1,"CNCS":1,"CORELLI":1,"HYSPEC":1,"NOMAD":1,"SEQUOIA":1,"WAND":1,"WISH":1})
        tubemax=defaultdict(lambda: 8, {"CORELLI":16,"MANDI":255,"POWGEN":153,"REF_M":303,"SNAP":255,"SXD":63,"TOPAZ":255,
                                        "WAND":480,"WISH":152})
        pixmin=defaultdict(int, {"ARCS":1,"CNCS":1,"CORELLI":1,"HYSPEC":1,"NOMAD":1,"SEQUOIA":1,"WAND":1,"WISH":1})
        pixmax=defaultdict(lambda: 128, {"CORELLI":256,"MANDI":255,"POWGEN":6,"REF_M":255,"SNAP":255,"SXD":63,"TOPAZ":255,
                                         "WAND":512,"WISH":512})

        if self.instname not in self.INSTRUMENT_LIST:
            raise ValueError("Instrument '"+self.instname+"' not in the allowed list")

        if self.instrument is None:
            IDF=mantid.api.ExperimentInfo.getInstrumentFilename(self.instname)
            ws=mantid.simpleapi.LoadEmptyInstrument(IDF, OutputWorkspace=self.instname+"MaskBTP")
            self.instrument=ws.getInstrument()

        # get the ranges for banks, tubes and pixels - adding the minimum back in
        banks = self.getProperty("Bank").value
        if len(banks) == 0:
            banks = numpy.arange(self.bankmin[self.instname], self.bankmax[self.instname] + 1)

        tubeString = self.getProperty("Tube").value
        if tubeString.lower() == "edges":
            tubes=[0, -1]
        else:
            tubes=self._parseBTPlist(tubeString, tubemin[self.instname], tubemax[self.instname])

        pixelString = self.getProperty("Pixel").value
        if pixelString.lower() == "edges":
            pixels=[0, -1]
        else:
            pixels=self._parseBTPlist(pixelString, pixmin[self.instname], pixmax[self.instname])

        # convert bank numbers into names and remove ones that couldn't be named
        banks = [self._getName(bank) for bank in banks]
        banks = [bank for bank in banks if bank]

        compInfo = ws.componentInfo()
        bankIndices = self._getBankIndices(compInfo, banks)

        # generate the list of detector identifiers to mask
        detlist=[]
        fullDetectorIdList = ws.detectorInfo().detectorIDs()
        for bankIndex in bankIndices:
            tubeIndices = compInfo.children(bankIndex)
            if len(tubeIndices) == 1:  # go down one level
                tubeIndices = compInfo.children(int(tubeIndices[0]))

            if len(tubes):  # use specific tubes if requested
                tubeIndices = tubeIndices[tubes]

            for tubeIndex in tubeIndices:
                pixelIndices = compInfo.children(int(tubeIndex))
                if len(pixels):  # use specific pixels if requested
                    pixelIndices = pixelIndices[pixels]

                for pixelIndex in pixelIndices:
                    detlist.append(fullDetectorIdList[pixelIndex])

        # mask the detectors
        detlist = numpy.array(detlist)
        if detlist.size > 0:
            mantid.simpleapi.MaskDetectors(Workspace=ws, DetectorList=detlist, EnableLogging=False)
        else:
            self.log().information("no detectors within this range")

        # set the outputs
        self.setProperty("Workspace",ws.name())
        self.setProperty("MaskedDetectors", detlist)

    def _parseBTPlist(self, value, min_value, max_value):
        if len(value) == 0:
            return list()  # empty list means use everything
        else:
            # let IntArrayProperty do the work and make sure that the result is valid
            prop = IntArrayProperty(name='temp', values=value)
            validationMsg = prop.isValid
            if validationMsg:
                raise RuntimeError(validationMsg)
            result = prop.value
            result = result[result >= min_value]
            result = result[result <= max_value]
            if len(result) == 0:
                raise RuntimeError('Could not generate values from "{}"'.format(value))
            return result - min_value

    def _getName(self, banknum):
        banknum=int(banknum)
        if not (self.bankmin[self.instname] <= banknum <= self.bankmax[self.instname]):
            raise ValueError("Out of range index={} for {} instrument bank numbers".format(banknum, self.instname))

        if self.instname == "WISH":
            return "panel" + "%02d" % banknum
        elif self.instname == "REF_M":
            return "detector" + "%1d" % banknum
        else:
            return "bank" + str(banknum)

    def _getBankIndices(self, compInfo, bankNames):
        '''This removes banks that don't exist'''
        bankIndices = []
        known_banks = {}  # name: index
        for bank in bankNames:
            if bank in known_banks:  # use the known index
                bankIndices.append(known_banks[bank])
                continue
            try:
                bankIndex = int(compInfo.indexOfAny(bank))
                bankIndices.append(bankIndex)

                # get all of the bank's parent's children
                parentIndex = int(compInfo.parent(bankIndex))
                for index in compInfo.children(parentIndex):
                    index = int(index)
                    known_banks[compInfo.name(index)] = index
            except ValueError:
                continue  # bank wasn't found

        self.log().information('While determining banks, filtered from {} banks to {}'.format(len(bankNames), len(bankIndices)))
        return bankIndices


mantid.api.AlgorithmFactory.subscribe(MaskBTP)

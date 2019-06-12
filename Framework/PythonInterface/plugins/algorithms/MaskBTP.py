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
            ws=mantid.simpleapi.LoadEmptyInstrument(IDF,OutputWorkspace=self.instname+"MaskBTP")
            self.instrument=ws.getInstrument()

        # get the ranges for banks, tubes and pixels
        banks=self._parseBTPlist(self.getProperty("Bank").value, self.bankmin[self.instname], self.bankmax[self.instname])

        tubeString = self.getProperty("Tube").value
        if tubeString.lower() == "edges":
            tubes=[tubemin[self.instname], tubemax[self.instname]]
        else:
            tubes=self._parseBTPlist(tubeString, tubemin[self.instname], tubemax[self.instname])

        pixelString = self.getProperty("Pixel").value
        if pixelString.lower() == "edges":
            pixels=[pixmin[self.instname], pixmax[self.instname]]
        else:
            pixels=self._parseBTPlist(pixelString, pixmin[self.instname], pixmax[self.instname])
        self.log().warning('TUBES:{} PIXELS:{}'.format(tubes, pixels))

        # generate the list of detector identifiers to mask
        detlist=[]
        for bank in banks:
            component = self._getEightPackHandle(bank)
            if component is not None:
                for tube in tubes:
                    if (tube<tubemin[self.instname]) or (tube>tubemax[self.instname]):
                        raise ValueError("Out of range index for tube number")
                    else:
                        for p in pixels:
                            if (p<pixmin[self.instname]) or (p>pixmax[self.instname]):
                                raise ValueError("Out of range index for pixel number")
                            else:
                                try:
                                    pid=component[int(tube-tubemin[self.instname])][int(p-pixmin[self.instname])].getID()
                                except:
                                    raise RuntimeError("Problem finding pixel in bank={}, tube={}, pixel={}".format(bank,
                                                                                                                    tube - tubemin[self.instname],
                                                                                                                    p - pixmin[self.instname]))
                                detlist.append(pid)

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
            return numpy.arange(min_value, max_value + 1)
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
            return result

    #pylint: disable=too-many-branches,too-many-return-statements
    def _getEightPackHandle(self, banknum):
        """
        Helper function to return the handle to a given eightpack
        """
        banknum=int(banknum)
        if not (self.bankmin[self.instname] <= banknum <= self.bankmax[self.instname]):
            raise ValueError("Out of range index={} for {} instrument bank numbers".format(banknum, self.instname))

        if self.instname=="ARCS":
            if self.bankmin[self.instname]<=banknum<= 38:
                return self.instrument.getComponentByName("B row")[banknum-1][0]
            elif 39<=banknum<= 77:
                return self.instrument.getComponentByName("M row")[banknum-39][0]
            elif 78<=banknum<=self.bankmax[self.instname]:
                return self.instrument.getComponentByName("T row")[banknum-78][0]
            else:
                raise ValueError("Out of range index for ARCS instrument bank numbers")
        elif self.instname=="SEQUOIA":
            # there are only banks 23-26 in A row
            if self.bankmin[self.instname]<=banknum<= 37:
                A_row = self.instrument.getComponentByName("A row")
                if A_row is None or banknum>26:
                    return None
                return A_row[banknum-23][0]
            if 38<=banknum<= 74:
                return self.instrument.getComponentByName("B row")[banknum-38][0]
            elif 75<=banknum<= 113:
                return self.instrument.getComponentByName("C row")[banknum-75][0]
            elif 114<=banknum<=self.bankmax[self.instname]:
                return self.instrument.getComponentByName("D row")[banknum-114][0]
            else:
                raise ValueError("Out of range index for SEQUOIA instrument bank numbers: %s" % (banknum,))
        elif self.instname in ["CNCS", "CORELLI", "HYSPEC", "WAND"]:
                return self.instrument.getComponentByName("bank"+str(banknum))[0]
        elif self.instname=="WISH":
            try:
                return self.instrument.getComponentByName("panel"+"%02d" % banknum)[0]
            except TypeError: #if not found, the return is None, so None[0] is a TypeError
                return None
        elif self.instname=="REF_M":
            return self.instrument.getComponentByName("detector"+"%1d" % banknum)
        else:
            return self.instrument.getComponentByName("bank"+str(banknum))


mantid.api.AlgorithmFactory.subscribe(MaskBTP)

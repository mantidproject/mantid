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
import mantid.kernel
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
        self.declareProperty(mantid.api.WorkspaceProperty("Workspace", "",direction=mantid.kernel.Direction.InOut,
                                                          optional = mantid.api.PropertyMode.Optional), "Input workspace (optional)")
        allowedInstrumentList=mantid.kernel.StringListValidator(['']+self.INSTRUMENT_LIST)
        self.declareProperty("Instrument","",validator=allowedInstrumentList,doc="One of the following instruments: "
                             + ', '.join(self.INSTRUMENT_LIST))
        self.declareProperty("Bank","",doc="Bank(s) to be masked. If empty, will apply to all banks")
        self.declareProperty("Tube","",doc="Tube(s) to be masked. If empty, will apply to all tubes")
        self.declareProperty("Pixel","",doc="Pixel(s) to be masked. If empty, will apply to all pixels")
        self.declareProperty(mantid.kernel.IntArrayProperty(name="MaskedDetectors", direction=mantid.kernel.Direction.Output),
                             doc="List of  masked detectors")

    #pylint: disable=too-many-branches
    def PyExec(self):
        ws = self.getProperty("Workspace").value
        self.instrument=None
        self.instname = self.getProperty("Instrument").value
        bankString = self.getProperty("Bank").value
        tubeString = self.getProperty("Tube").value
        pixelString = self.getProperty("Pixel").value

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

        if bankString == "":
            banks=numpy.arange(self.bankmax[self.instname]-self.bankmin[self.instname]+1)+self.bankmin[self.instname]
        else:
            banks=self._parseBTPlist(bankString)

        if tubeString == "":
            tubes=numpy.arange(tubemax[self.instname]-tubemin[self.instname]+1)+tubemin[self.instname]
        elif tubeString == "edges":
            tubes=[tubemin[self.instname],tubemax[self.instname]]
        else:
            tubes=self._parseBTPlist(tubeString)

        if pixelString == "":
            pixels=numpy.arange(pixmax[self.instname]-pixmin[self.instname]+1)+pixmin[self.instname]
        elif pixelString == "edges":
            pixels=[pixmin[self.instname],pixmax[self.instname]]
        else:
            pixels=self._parseBTPlist(pixelString)

        detlist=[]
        for b in banks:
            ep=self._getEightPackHandle(b)
            if ep is not None:
                for t in tubes:
                    if (t<tubemin[self.instname]) or (t>tubemax[self.instname]):
                        raise ValueError("Out of range index for tube number")
                    else:
                        for p in pixels:
                            if (p<pixmin[self.instname]) or (p>pixmax[self.instname]):
                                raise ValueError("Out of range index for pixel number")
                            else:
                                try:
                                    pid=ep[int(t-tubemin[self.instname])][int(p-pixmin[self.instname])].getID()
                                except:
                                    raise RuntimeError("Problem finding pixel in bank="+str(b)+
                                                       ", tube="+str(t-tubemin[self.instname])+", pixel="+str(p-pixmin[self.instname]))
                                detlist.append(pid)
        detlist = numpy.array(detlist)
        if detlist.size > 0:
            mantid.simpleapi.MaskDetectors(Workspace=ws,DetectorList=detlist, EnableLogging=False)
        else:
            self.log().information("no detectors within this range")
        self.setProperty("Workspace",ws.name())
        self.setProperty("MaskedDetectors", detlist)

    def _parseBTPlist(self,value):
        """
        Helper function to transform a string into a list of integers
        For example "1,2-4,8-10" will become [1,2,3,4,8,9,10]
        It will deal with lists as well, so range(1,4) will still be [1,2,3]
        """
        parsed = []
        #split the commas
        parts = str(value).strip(']').strip('[').split(',')
        #now deal with the hyphens
        for p in parts:
            if len(p) > 0:
                elem = p.split("-")
            if len(elem) == 1:
                parsed.append(int(elem[0]))
            if len(elem) == 2:
                startelem = int(elem[0])
                endelem = int(elem[1])
                if endelem < startelem:
                    raise ValueError("The element after the hyphen needs to be greater or equal than the first element")
                elemlist = list(range(startelem,endelem+1))
                parsed.extend(elemlist)
        return parsed

    #pylint: disable=too-many-branches,too-many-return-statements
    def _getEightPackHandle(self,banknum):
        """
        Helper function to return the handle to a given eightpack
        """
        banknum=int(banknum)
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
            if self.bankmin[self.instname]<=banknum<= 23+4-1:
                A_row = self.instrument.getComponentByName("A row")
                if A_row is None: return None
                return A_row[banknum-23][0]
            if 27<=banknum<=37:
                # these banks are still missing
                return
            if 38<=banknum<= 74:
                return self.instrument.getComponentByName("B row")[banknum-38][0]
            elif 75<=banknum<= 113:
                return self.instrument.getComponentByName("C row")[banknum-75][0]
            elif 114<=banknum<=self.bankmax[self.instname]:
                return self.instrument.getComponentByName("D row")[banknum-114][0]
            else:
                raise ValueError("Out of range index for SEQUOIA instrument bank numbers: %s" % (banknum,))
        elif self.instname in ["CNCS", "CORELLI","HYSPEC", "WAND"]:
            if self.bankmin[self.instname]<=banknum<= self.bankmax[self.instname]:
                return self.instrument.getComponentByName("bank"+str(banknum))[0]
            else:
                raise ValueError("Out of range index for "+str(self.instname)+" instrument bank numbers")
        elif self.instname=="WISH":
            if self.bankmin[self.instname]<=banknum<= self.bankmax[self.instname]:
                try:
                    return self.instrument.getComponentByName("panel"+"%02d" % banknum)[0]
                except TypeError: #if not found, the return is None, so None[0] is a TypeError
                    return None
            else:
                raise ValueError("Out of range index for "+str(self.instname)+" instrument bank numbers")
        elif self.instname=="REF_M":
            if self.bankmin[self.instname]<=banknum<= self.bankmax[self.instname]:
                return self.instrument.getComponentByName("detector"+"%1d" % banknum)
            else:
                raise ValueError("Out of range index for "+str(self.instname)+" instrument bank numbers")

        else:
            if self.bankmin[self.instname]<=banknum<= self.bankmax[self.instname]:
                return self.instrument.getComponentByName("bank"+str(banknum))
            else:
                raise ValueError("Out of range index for "+str(self.instname)+" instrument bank numbers")


mantid.api.AlgorithmFactory.subscribe(MaskBTP)

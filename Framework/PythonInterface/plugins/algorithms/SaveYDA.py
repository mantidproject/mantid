from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceUnitValidator,\
    InstrumentValidator, FileProperty, FileAction, mtd
from mantid.kernel import Direction, CompositeValidator
from mantid.dataobjects import Workspace2D

import ruamel.yaml
from ruamel.yaml.comments import CommentedMap, CommentedSeq

import math


class SaveYDA(PythonAlgorithm):

    def getBinCenters(self, ax):

        bin = []

        for i in range(1, ax.size):
                bin.append((ax[i]+ax[i-1])/2)

        return bin

    def category(self):
        return 'DataHandling'

    def PyInit(self):

        wsValidators = CompositeValidator()
        wsValidators.add(WorkspaceUnitValidator("DeltaE"))
        wsValidators.add(InstrumentValidator())

        self.declareProperty(MatrixWorkspaceProperty(name="InputWorkspace", defaultValue="",
                             direction=Direction.Input, validator=wsValidators), doc="Workspace name for input")
        self.declareProperty(FileProperty(name="Filename", defaultValue="", action=FileAction.Save,
                             extensions=""),"The name to use when writing the file")

    def validateInputs(self):

        issues = dict()

        allowUn = 'MomentumTransfer'
        ws      = self.getProperty("InputWorkspace").value
        ax      = ws.getAxis(1)

        self.log().debug(str(ax.getUnit().unitID()))

        if not ax.isSpectra() and ax.getUnit().unitID() != allowUn:
            issues["InputWorkspace"] = "Y axis is not 'Spectrum Axis' or 'Momentum Transfer'"

        if not isinstance(ws, Workspace2D):
            issues["InputWorkspace"] = "Input Workspace is not a Workspace2D"

        self.log().debug(str(ax.isSpectra()))

        return issues

    def PyExec(self):

        ws       = mtd[self.getPropertyValue('InputWorkspace')]
        filename = self.getProperty("Filename").value

        if ws is None:
            raise NotImplementedError("InputWorkspace does not exist")

        run   = ws.getRun()
        ax    = ws.getAxis(1)
        nHist = ws.getNumberHistograms()

        if len(run.getLogData()) == 0:
            raise NotImplementedError("No sample log data exist in workspace: "
                                      + self.getPropertyValue('InputWorkspace'))

        metadata = CommentedMap()

        metadata["format"] =  "yaml/frida 2.0"
        metadata["type"]   = "gerneric tabular data"

        hist = []

        if run.hasProperty('proposal_number'):
            propn ="Proposal number " + run.getLogData("proposal_number").value
            hist.append(propn)
        else:
            self.log().warning("no proposal number found")

        if run.hasProperty('proposal_title'):
            propt = run.getLogData("proposal_title").value
            hist.append(propt)
        else:
            self.log().warning("no proposal title found")

        if run.hasProperty('experiment_team'):
            expt = run.getLogData("experiment_team").value
            hist.append(expt)
        else:
            self.log().warning("no experiment team found")

        hist.append("data reduced with mantid")

        rpar = []

        if run.hasProperty('temperature'):
            temperature = float(run.getLogData("temperature").value)

            temp = CommentedMap()
            temp["name"] = "T"
            temp["unit"] = "K"
            temp["val"]  = temperature
            temp["stdv"] = 0

            rpar.append(temp)
        else:
            self.log().warning("no temperature found")

        if run.hasProperty('Ei'):
            eimeV = float(run.getLogData("Ei").value)

            ei = CommentedMap()
            ei["name"] = "Ei"
            ei["unit"] = "meV"
            ei["val"]  = eimeV
            ei["stdv"] = 0

            rpar.append(ei)
        else:
            self.log().warning("no Ei found")

        coord = CommentedMap()

        x = CommentedMap()

        x["name"] = "w"
        x["unit"] = "meV"

        coord["x"] = x
        coord['x'].fa.set_flow_style()

        y = CommentedMap()

        y["name"] = "S(q,w)"
        y["unit"] = "meV-1"

        coord["y"] = y
        coord["y"].fa.set_flow_style()

        z = CommentedMap()

        if ax.isSpectra():
            zname = "2th"
            zunit = "deg"
        else:
            zname = "q"
            zunit = "A-1"

        z["name"] = zname
        z["unit"] = zunit

        coord["z"] = z
        coord["z"].fa.set_flow_style()

        slices = []

        bin = []

        if ax.isSpectra:
            samplePos = ws.getInstrument().getSample().getPos()
            sourcePos = ws.getInstrument().getSource().getPos()
            beamPos = samplePos - sourcePos
            for i in range(nHist):
                detector = ws.getDetector(i)
                twoTheta = detector.getTwoTheta(samplePos, beamPos)*180/math.pi
                bin.append(twoTheta)
        elif ax.length() == nHist:
            for i in range(ax.length()):
                bin.append(ax.getValue())
        else:
            bin = self.getBinCenters(ax)

        for i in range(nHist):

            ys = ws.dataY(0)
            yv = []
            for j in range(ys.size):
                yv.append(ys[j])

            xax = ws.readX(i)
            xcenters = self.getBinCenters(xax)

            slicethis = CommentedMap()

            slicethis['j'] = i

            val        = CommentedMap()
            val['val'] = bin[i]
            value      = [val]

            slicethis['z'] = CommentedSeq(value)
            slicethis['z'].fa.set_flow_style()

            xx = [float(i) for i in xcenters]
            slicethis['x'] = CommentedSeq(xx)
            slicethis['x'].fa.set_flow_style()

            yy = [float(i) for i in yv]
            slicethis['y'] = CommentedSeq(yy)
            slicethis['y'].fa.set_flow_style()

            slices.append(slicethis)

        data = CommentedMap()

        data['Meta']    = metadata
        data['History'] = hist
        data['Coord']   = coord
        data['RPar']    = rpar
        data['Slices']  = slices

        try:
            with open(filename,'w') as outfile:
                ruamel.yaml.round_trip_dump(data, outfile, block_seq_indent=2, indent=4)
                outfile.close()
        except:
            raise RuntimeError("Can't write in File" + filename)

#---------------------------------------------------------------------------------------------------------------------#

AlgorithmFactory.subscribe(SaveYDA)

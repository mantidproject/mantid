# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    PythonAlgorithm,
    AlgorithmFactory,
    MatrixWorkspaceProperty,
    WorkspaceUnitValidator,
    InstrumentValidator,
    FileProperty,
    FileAction,
)
from mantid.kernel import Direction, CompositeValidator
from mantid.dataobjects import Workspace2D

import yaml
from yaml import Dumper

from collections import OrderedDict

import math


class SaveYDA(PythonAlgorithm):
    """Save data in yaml/frida 2.0 format from a Workspace2D."""

    def category(self):
        """Return category"""
        return "DataHandling\\Text"

    def name(self):
        """Return name"""
        return "SaveYDA"

    def summary(self):
        """Return summary"""
        return "Save Workspace to a Frida 2.0 yaml format"

    def PyInit(self):
        """Declare properties"""
        wsValidators = CompositeValidator()
        # X axis must be a NumericAxis in energy transfer units.
        wsValidators.add(WorkspaceUnitValidator("DeltaE"))
        # Workspace must have an Instrument
        wsValidators.add(InstrumentValidator())

        self.declareProperty(
            MatrixWorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input, validator=wsValidators),
            doc="Workspace name for input",
        )
        self.declareProperty(
            FileProperty(name="Filename", defaultValue="", action=FileAction.Save, extensions=""),
            doc="The name to use when writing the file",
        )

    def validateInputs(self):
        """Basic validation for inputs.
        :return: issues with not valid Inputs in dictionary
        """
        issues = dict()
        # Only MomentumTransfer is allowed
        allowUn = "MomentumTransfer"
        ws = self.getProperty("InputWorkspace").value
        # Y axis must be either a SpectrumAxis or a NumericAxis in q units.
        # workspace must be a Workspace2D
        if ws:
            ax = ws.getAxis(1)

            if not ax.isSpectra() and ax.getUnit().unitID() != allowUn:
                issues["InputWorkspace"] = "Y axis is not 'Spectrum Axis' or 'Momentum Transfer'"

            if not isinstance(ws, Workspace2D):
                issues["InputWorkspace"] = "Input Workspace is not a Workspace2D"

        return issues

    def PyExec(self):
        """Main execution body"""
        # Properties
        ws = self.getProperty("InputWorkspace").value
        filename = self.getProperty("Filename").value

        run = ws.getRun()
        ax = ws.getAxis(1)
        nHist = ws.getNumberHistograms()

        # check sample logs exists
        if len(run.getLogData()) == 0:
            raise NotImplementedError("No sample log data exist in workspace: " + self.getPropertyValue("InputWorkspace"))

        # save sample log data in lists, commented sequences an commented maps
        # commented sequences and maps are used to keep Data in the order they get inserted
        # if a log does not exist a warning is written on the log and the data is not saved in the file

        metadata = OrderedDict()

        metadata["format"] = "yaml/frida 2.0"
        metadata["type"] = "generic tabular data"

        hist = []

        if run.hasProperty("proposal_number"):
            propn = "Proposal number " + run.getLogData("proposal_number").value
            hist.append(propn)
        else:
            self.log().warning("no proposal number found")

        if run.hasProperty("proposal_title"):
            propt = run.getLogData("proposal_title").value
            hist.append(propt)
        else:
            self.log().warning("no proposal title found")

        if run.hasProperty("experiment_team"):
            expt = run.getLogData("experiment_team").value
            hist.append(expt)
        else:
            self.log().warning("no experiment team found")

        hist.append("data reduced with mantid")

        rpar = []

        if run.hasProperty("temperature"):
            temperature = float(run.getLogData("temperature").value)

            temp = OrderedDict()
            temp["name"] = "T"
            temp["unit"] = "K"
            temp["val"] = round(temperature, 14)
            temp["stdv"] = 0

            rpar.append(temp)
        else:
            self.log().warning("no temperature found")

        if run.hasProperty("Ei"):
            eimeV = float(run.getLogData("Ei").value)

            ei = OrderedDict()
            ei["name"] = "Ei"
            ei["unit"] = "meV"
            ei["val"] = round(eimeV, 14)
            ei["stdv"] = 0

            rpar.append(ei)
        else:
            self.log().warning("no Ei found")

        coord = OrderedDict()

        x = FlowOrderedDict()

        x["name"] = "w"
        x["unit"] = "meV"

        coord["x"] = x

        y = FlowOrderedDict()

        y["name"] = "S(q,w)"
        y["unit"] = "meV-1"

        coord["y"] = y

        z = FlowOrderedDict()

        if ax.isSpectra():
            zname = "2th"
            zunit = "deg"
        else:
            zname = "q"
            zunit = "A-1"

        z["name"] = zname
        z["unit"] = zunit

        coord["z"] = FlowList()
        coord["z"].append(z)

        slices = []

        bin = []

        # if y axis is SpectrumAxis
        if ax.isSpectra:
            samplePos = ws.getInstrument().getSample().getPos()
            sourcePos = ws.getInstrument().getSource().getPos()
            beamPos = samplePos - sourcePos
            for i in range(nHist):
                detector = ws.getDetector(i)
                # convert radians to degrees
                twoTheta = detector.getTwoTheta(samplePos, beamPos) * 180 / math.pi
                twoTheta = round(twoTheta, 14)
                bin.append(twoTheta)
        elif ax.length() == nHist:
            # if y axis contains bin centers
            for i in range(ax.length()):
                xval = round(ax.getValue(), 14)
                bin.append(xval)
        else:
            # get the bin centers not the bin edges
            bin = self._get_bin_centers(ax)

        for i in range(nHist):
            slicethis = OrderedDict()

            # add j to slices, j = counts
            slicethis["j"] = i

            # save in list and commented Map to keep format
            val = FlowOrderedDict()
            val["val"] = bin[i]
            # z is bin centers of y axis, SpectrumAxis or NumericAxis in q units
            slicethis["z"] = FlowList()
            slicethis["z"].append(val)

            xax = ws.readX(i)
            # get the bin centers not the bin edges
            xcenters = self._get_bin_centers(xax)
            # x axis is NumericAxis in energy transfer units
            xx = [float(j) for j in xcenters]
            slicethis["x"] = FlowList(xx)

            ys = ws.dataY(i)
            # y is dataY of the workspace
            yy = [float(round(j, 14)) for j in ys]
            slicethis["y"] = FlowList(yy)

            slices.append(slicethis)

        data = OrderedDict()

        data["Meta"] = metadata
        data["History"] = hist
        data["Coord"] = coord
        data["RPar"] = rpar
        data["Slices"] = slices
        data["Slices"] = slices

        # create yaml file
        try:
            with open(filename, "w") as outfile:
                yaml.dump(data, outfile, default_flow_style=False, canonical=False, Dumper=MyDumper)
                outfile.close()
        except:
            raise RuntimeError("Can't write in File" + filename)

    def _get_bin_centers(self, ax):
        """calculates the bin centers from the bin edges
        :param ax: bin center axis
        :return: list of bin centers
        """
        bin = []

        for i in range(1, ax.size):
            axval = round((ax[i] + ax[i - 1]) / 2, 14)
            bin.append(axval)

        return bin


class MyDumper(Dumper):
    """regulates the indent for yaml Dumper"""

    def increase_indent(self, flow=False, indentless=False):
        return super(MyDumper, self).increase_indent(flow, False)


class FlowOrderedDict(OrderedDict):
    """Helper class to switch between flow style and no flow style

    Equal to OrderedDict class but other yaml representer
    """

    pass


class FlowList(list):
    """Helper class to switch between flow style and no flow style

    Equal to list class but other yaml representer
    """

    pass


def _flow_list_rep(dumper, data):
    """Yaml representer for list in flow style"""
    return dumper.represent_sequence("tag:yaml.org,2002:seq", data, flow_style=True)


def _flow_ord_dic_rep(dumper, data):
    """Yaml representer for OrderedDict in flow style"""
    return dumper.represent_mapping("tag:yaml.org,2002:map", data, flow_style=True)


def _represent_ordered_dict(dumper, data):
    """Yaml representer for OrderedDict

    regulates dumping for class OrderedDict
    """
    value = []

    for item_key, item_value in data.items():
        node_key = dumper.represent_data(item_key)
        node_value = dumper.represent_data(item_value)

        value.append((node_key, node_value))

    return yaml.nodes.MappingNode("tag:yaml.org,2002:map", value)


# Adding representers to yaml
yaml.add_representer(OrderedDict, _represent_ordered_dict)
yaml.add_representer(FlowList, _flow_list_rep)
yaml.add_representer(FlowOrderedDict, _flow_ord_dic_rep)

# ---------------------------------------------------------------------------------------------------------------------#

AlgorithmFactory.subscribe(SaveYDA)

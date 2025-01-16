# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,redefined-builtin
import mantid
from mantid.kernel import Direction, IntArrayProperty, StringArrayProperty, StringListValidator
import importlib


class SavePlot1D(mantid.api.PythonAlgorithm):
    _wksp = None

    def category(self):
        """Category"""
        return "DataHandling\\Plots"

    def seeAlso(self):
        return ["SavePlot1DAsJson", "StringToPng"]

    def name(self):
        """Algorithm name"""
        return "SavePlot1D"

    def summary(self):
        return "Save 1D plots to a file"

    def checkGroups(self):
        return False

    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty("InputWorkspace", "", mantid.kernel.Direction.Input), "Workspace to plot")
        self.declareProperty(
            mantid.api.FileProperty("OutputFilename", "", action=mantid.api.FileAction.OptionalSave, extensions=[".png"]),
            doc="Name of the image file to savefile.",
        )
        have_plotly = importlib.util.find_spec("plotly") is not None
        if have_plotly:
            outputTypes = ["image", "plotly", "plotly-full"]
        else:
            outputTypes = ["image"]
        self.declareProperty("OutputType", "image", StringListValidator(outputTypes), "Method for rendering plot")
        self.declareProperty("XLabel", "", "Label on the X axis. If empty, it will be taken from workspace")
        self.declareProperty("YLabel", "", "Label on the Y axis. If empty, it will be taken from workspace")
        self.declareProperty(IntArrayProperty("SpectraList", [], direction=Direction.Input), "Which spectra to plot")
        self.declareProperty(StringArrayProperty("SpectraNames", [], direction=Direction.Input), "Override with custom names for spectra")
        self.declareProperty("Result", "", Direction.Output)

        self.declareProperty("PopCanvas", False, "If true, a Matplotlib canvas will be popped out , which contains the saved plot.")

    def validateInputs(self):
        messages = {}
        outputType = self.getProperty("OutputType").value
        if outputType != "plotly":
            filename = self.getProperty("OutputFilename").value
            if len(filename.strip()) <= 0:
                messages["OutputFilename"] = "Required for OutputType != plotly"

        return messages

    def PyExec(self):
        self._wksp = self.getProperty("InputWorkspace").value
        outputType = self.getProperty("OutputType").value

        self.visibleSpectra = self.getProperty("SpectraList").value

        if outputType == "image":
            result = self.saveImage()
        else:
            result = self.savePlotly(outputType == "plotly-full")

        self.setProperty("Result", result)

    def getData(self, ws, wkspIndex, label=""):
        x = ws.readX(wkspIndex)
        y = ws.readY(wkspIndex)
        if x.size == y.size + 1:
            x = (x[:-1] + x[1:]) * 0.5

        # use suggested label
        if len(label.strip()) > 0:
            return (x, y, label)

        # determine the label from the data
        ax = ws.getAxis(1)
        if ax.isSpectra():
            label = ax.label(wkspIndex)
        else:
            LHS = ax.title()
            if LHS == "":
                LHS = ax.getUnit().caption()
            label = LHS + " = " + str(float(ax.label(wkspIndex)))

        return (x, y, label)

    def showSpectrum(self, ws, wkspIndex):
        spectraNum = ws.getSpectrum(wkspIndex).getSpectrumNo()

        # user not specifying which spectra means show them all
        if len(self.visibleSpectra) <= 0:
            return True

        return spectraNum in self.visibleSpectra

    def getAxesLabels(self, ws, utf8=False):
        xlabel = self.getProperty("XLabel").value
        if xlabel == "":
            xaxis = ws.getAxis(0)
            if utf8:
                unitLabel = xaxis.getUnit().symbol().utf8()
            else:  # latex markup
                unitLabel = "$" + xaxis.getUnit().symbol().latex() + "$"
            xlabel = xaxis.getUnit().caption() + " (" + unitLabel + ")"

        ylabel = self.getProperty("YLabel").value
        if ylabel == "":
            ylabel = ws.YUnit()
            if ylabel == "":
                ylabel = ws.YUnitLabel()

        return (xlabel, ylabel)

    def savePlotly(self, fullPage):
        from plotly import tools as toolsly
        from plotly.offline import plot
        import plotly.graph_objs as go

        spectraNames = self.getProperty("SpectraNames").value

        if isinstance(self._wksp, mantid.api.WorkspaceGroup):
            fig = toolsly.make_subplots(rows=self._wksp.getNumberOfEntries())

            for i in range(self._wksp.getNumberOfEntries()):
                wksp = self._wksp.getItem(i)
                (traces, xlabel, ylabel) = self.toScatterAndLabels(wksp, spectraNames)
                for spectrum in traces:
                    fig.append_trace(spectrum, i + 1, 1)
                fig["layout"]["xaxis%d" % (i + 1)].update(title={"text": xlabel})
                fig["layout"]["yaxis%d" % (i + 1)].update(title={"text": ylabel})
                if len(spectraNames) > 0:  # remove the used spectra names
                    spectraNames = spectraNames[len(traces) :]
            fig["layout"].update(margin={"r": 0, "t": 0})
        else:
            (traces, xlabel, ylabel) = self.toScatterAndLabels(self._wksp, spectraNames)

            # plotly seems to change the way to set the axes labels
            # randomly and within a version. Just give up and try both.
            try:
                layout = go.Layout(yaxis={"title": ylabel}, xaxis={"title": xlabel}, margin={"l": 40, "r": 0, "t": 0, "b": 40})
            except:  # try different call when any exception happens
                layout = go.Layout(
                    yaxis={"title": {"text": ylabel}}, xaxis={"title": {"text": xlabel}}, margin={"l": 40, "r": 0, "t": 0, "b": 40}
                )

            fig = go.Figure(data=traces, layout=layout)

        # extra arguments for div vs full page
        if fullPage:
            filename = self.getProperty("OutputFilename").value
            plotly_args = {"filename": filename}
        else:  # just the div
            plotly_args = {"output_type": "div", "include_plotlyjs": False}

        # render the plot
        div = plot(fig, show_link=False, **plotly_args)

        # decide what to return
        if fullPage:
            return filename
        else:
            return str(div)

    def toScatterAndLabels(self, wksp, spectraNames):
        import plotly.graph_objs as go

        data = []
        for i in range(wksp.getNumberHistograms()):
            if len(spectraNames) > i:
                (x, y, label) = self.getData(wksp, i, spectraNames[i])
            else:
                (x, y, label) = self.getData(wksp, i)

            visible = True
            if not self.showSpectrum(wksp, i):
                visible = "legendonly"

            data.append(go.Scatter(x=x, y=y, name=label, visible=visible))

        (xlabel, ylabel) = self.getAxesLabels(wksp, utf8=True)

        return (data, xlabel, ylabel)

    def saveImage(self):
        import matplotlib.pyplot as plt

        if isinstance(self._wksp, mantid.api.WorkspaceGroup):
            num_subplots = self._wksp.getNumberOfEntries()
            fig, axarr = plt.subplots(num_subplots)
            for i in range(self._wksp.getNumberOfEntries()):
                self.doPlotImage(axarr[i], self._wksp.getItem(i))
        else:
            fig, ax = plt.subplots()
            self.doPlotImage(ax, self._wksp)

        # get the flag to pop out canvas or not
        pop_canvas = self.getProperty("PopCanvas").value

        plt.tight_layout()
        if pop_canvas:
            plt.show()
        filename = self.getProperty("OutputFilename").value
        fig.savefig(filename, bbox_inches="tight")

        return filename

    def doPlotImage(self, ax, ws):
        spectra = ws.getNumberHistograms()
        number_of_lines = spectra if len(self.visibleSpectra) <= 0 else len(self.visibleSpectra)
        if number_of_lines > 10:
            mantid.kernel.logger.warning("more than 10 spectra to plot")
        prog_reporter = mantid.api.Progress(self, start=0.0, end=1.0, nreports=spectra)

        for j in range(spectra):
            if not self.showSpectrum(ws, j):
                continue

            (x, y, plotlabel) = self.getData(ws, j)

            ax.plot(x, y, label=plotlabel)

            (xlabel, ylabel) = self.getAxesLabels(ws)
            ax.set_xlabel(xlabel)
            ax.set_ylabel(ylabel)
            prog_reporter.report("Processing")

        if 1 < number_of_lines <= 10:
            ax.legend()


mantid.api.AlgorithmFactory.subscribe(SavePlot1D)

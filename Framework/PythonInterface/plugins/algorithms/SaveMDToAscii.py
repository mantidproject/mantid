# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from mantid.api import AlgorithmFactory, FileAction, FileProperty, IMDHistoWorkspaceProperty, MDNormalization, PropertyMode, PythonAlgorithm
from mantid.kernel import Direction, EnabledWhenProperty, IntBoundedValidator, Property, PropertyCriterion, StringListValidator

_SEPARATORS = {"CSV": ",", "Tab": "\t", "Space": " ", "Colon": ":", "SemiColon": ";"}
_DEFAULT_PRECISION = 6


def _dim2array(d):
    """Bin-centre coordinates along dimension d, as a 1D numpy array of length d.getNBins()."""
    dmin = d.getMinimum()
    dmax = d.getMaximum()
    dstep = d.getX(1) - d.getX(0)
    return np.arange(dmin + dstep / 2, dmax, dstep)


class SaveMDToAscii(PythonAlgorithm):
    def category(self):
        return "MDAlgorithms\\DataHandling"

    def summary(self):
        return (
            "Save an MDHistoWorkspace to a plain ASCII file, one row per bin, "
            "with dimension coordinate columns followed by intensity and error."
        )

    def seeAlso(self):
        return ["SaveAscii", "SaveMD", "SaveMDHistoToVTK"]

    def PyInit(self):
        self.declareProperty(
            IMDHistoWorkspaceProperty("InputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="Input MDHistoWorkspace to save.",
        )
        self.declareProperty(
            FileProperty("Filename", "", action=FileAction.Save, extensions=[".dat", ".txt"], direction=Direction.Input),
            doc="Output filename.",
        )
        self.declareProperty(
            name="ExcludeIntegratedDimensions",
            defaultValue=True,
            direction=Direction.Input,
            doc="If True (default), integrated dimensions (those with a single bin) are excluded from the "
            "output columns. If False, all dimensions are written as columns.",
        )
        self.declareProperty(
            name="Normalization",
            defaultValue="FromWorkspace",
            validator=StringListValidator(["FromWorkspace", "NoNormalization", "VolumeNormalization", "NumEventsNormalization"]),
            direction=Direction.Input,
            doc="Normalization to apply to the signal and error before writing. 'FromWorkspace' (default) uses "
            "the workspace's own displayNormalizationHisto() setting. The other choices force that "
            "normalization regardless of what the workspace reports.",
        )
        self.declareProperty(
            name="Separator",
            defaultValue="Space",
            validator=StringListValidator(["CSV", "Tab", "Space", "Colon", "SemiColon", "UserDefined"]),
            direction=Direction.Input,
            doc="Column delimiter to use in the output file. 'UserDefined' requires CustomSeparator to be set.",
        )
        self.declareProperty(
            name="CustomSeparator",
            defaultValue="",
            direction=Direction.Input,
            doc="Used as the column delimiter when Separator is set to 'UserDefined'.",
        )
        self.setPropertySettings("CustomSeparator", EnabledWhenProperty("Separator", PropertyCriterion.IsEqualTo, "UserDefined"))
        self.declareProperty(
            name="Precision",
            defaultValue=Property.EMPTY_INT,
            validator=IntBoundedValidator(lower=1),
            direction=Direction.Input,
            doc="Number of significant digits for the numeric columns. If not set, a default of 6 is used.",
        )

    def validateInputs(self):
        issues = dict()

        separator = self.getPropertyValue("Separator")
        custom_separator = self.getPropertyValue("CustomSeparator")
        if separator == "UserDefined" and not custom_separator:
            issues["CustomSeparator"] = "CustomSeparator must be set when Separator is 'UserDefined'."

        ws = self.getProperty("InputWorkspace").value
        if ws is not None and self.getProperty("ExcludeIntegratedDimensions").value:
            if len(ws.getNonIntegratedDimensions()) == 0:
                issues["ExcludeIntegratedDimensions"] = "All dimensions of the workspace are integrated; there are no columns to write."

        return issues

    def PyExec(self):
        ws = self.getProperty("InputWorkspace").value
        filename = self.getPropertyValue("Filename")
        exclude_integrated = self.getProperty("ExcludeIntegratedDimensions").value

        if exclude_integrated:
            dims = ws.getNonIntegratedDimensions()
        else:
            dims = [ws.getDimension(i) for i in range(ws.getNumDims())]

        dim_arrays = [_dim2array(d) for d in dims]
        if len(dim_arrays) > 1:
            broadcast_arrays = np.meshgrid(*dim_arrays, indexing="ij")
        else:
            broadcast_arrays = dim_arrays

        signal = ws.getSignalArray() * 1.0
        err2 = ws.getErrorSquaredArray() * 1.0

        normalization = self._resolve_normalization(ws)
        if normalization == MDNormalization.NumEventsNormalization:
            nev = ws.getNumEventsArray()
            signal = signal / nev
            err2 = err2 / (nev * nev)
        error = np.sqrt(err2)

        if signal.ndim != len(dims):
            signal = np.squeeze(signal)
            error = np.squeeze(error)

        columns = [d.flatten() for d in broadcast_arrays] + [signal.flatten(), error.flatten()]
        data_to_write = np.column_stack(columns)

        header = " ".join(d.name for d in dims) + " Intensity Error"
        header += "\nshape: " + "x".join(str(d.getNBins()) for d in dims)

        separator = self._resolve_separator()
        precision = self.getProperty("Precision").value
        if precision == Property.EMPTY_INT:
            precision = _DEFAULT_PRECISION
        fmt = f"%.{precision}e"

        np.savetxt(filename, data_to_write, fmt=fmt, delimiter=separator, header=header)

    def _resolve_normalization(self, ws):
        """Resolve the MDNormalization to apply. Mirrors the override pattern of
        get_md_normalization/get_md_data (mantid.plots.datafunctions) without importing that module,
        which pulls in matplotlib and is inappropriate for a headless Save algorithm."""
        choice = self.getPropertyValue("Normalization")
        if choice == "FromWorkspace":
            return ws.displayNormalizationHisto()
        return getattr(MDNormalization, choice)

    def _resolve_separator(self):
        choice = self.getPropertyValue("Separator")
        if choice == "UserDefined":
            return self.getPropertyValue("CustomSeparator")
        return _SEPARATORS[choice]


AlgorithmFactory.subscribe(SaveMDToAscii)

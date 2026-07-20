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
    boundaries = np.array([d.getX(i) for i in range(d.getNBins() + 1)])
    return 0.5 * (boundaries[:-1] + boundaries[1:])


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
            name="ExtraHeader",
            defaultValue="",
            direction=Direction.Input,
            doc="If not empty, this text is written at the top of the file's header, above the column names "
            "and shape lines. Each line is prefixed with '#', the same as the rest of the header; if this "
            "text spans multiple lines, every one of them is prefixed individually.",
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
            "normalization regardless of what the workspace reports: 'NumEventsNormalization' divides by the "
            "number of events in each bin, and 'VolumeNormalization' multiplies by the workspace's inverse "
            "bin volume.",
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
            doc="Number of digits after the decimal point in scientific notation for the numeric columns. "
            "If not set, a default of 6 is used.",
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
        elif normalization == MDNormalization.VolumeNormalization:
            inverse_volume = ws.getInverseVolume()
            signal = signal * inverse_volume
            err2 = err2 * (inverse_volume * inverse_volume)
        error = np.sqrt(err2)

        if signal.ndim != len(dims):
            signal = np.squeeze(signal)
            error = np.squeeze(error)

        columns = [d.ravel() for d in broadcast_arrays] + [signal.ravel(), error.ravel()]
        data_to_write = np.column_stack(columns)

        separator = self._resolve_separator()

        header_lines = []
        extra_header = self.getPropertyValue("ExtraHeader")
        if extra_header:  # in case `\n` was interpreted as a literal string
            header_lines.append(extra_header.replace("\\n", "\n").replace("\\t", "\t"))
        header_lines.append(separator.join([*(d.name for d in dims), "Intensity", "Error"]))
        header_lines.append("shape: " + "x".join(str(d.getNBins()) for d in dims))
        header = "\n".join(header_lines)

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

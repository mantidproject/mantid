# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from scipy.special import modstruve, i0, i1

from mantid.api import AlgorithmFactory, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import Direction, Property, StringListValidator, FloatBoundedValidator, FloatMandatoryValidator, CompositeValidator
from mantid.simpleapi import CreateWorkspace, CreateSingleValuedWorkspace
from mantid.geometry import GeometryShape


def bilinear_interpolate(x, y, x_grid, y_grid, Z):
    """
    Bilinear interpolation of a 2D table Z over grids x_grid (len M), y_grid (len N).
    Returns Z(x,y).
    Assumes x_grid and y_grid are strictly increasing.
    """
    # Raise only for points strictly outside the tabulated domain.
    # Exact matches on the first/last grid points are valid and should
    # interpolate on the edge cells.
    if x < x_grid[0] or x > x_grid[-1] or y < y_grid[0] or y > y_grid[-1]:
        raise ValueError(f"Interpolation point (x={x}, y={y}) is out of bounds of the grid.")
    # Find bracketing indices. Use side="right" so exact matches on grid
    # points choose the cell to their left, then clamp to the valid cell
    # range so the first and last grid points remain in bounds.
    ix = np.searchsorted(x_grid, x, side="right") - 1
    iy = np.searchsorted(y_grid, y, side="right") - 1
    ix = min(max(ix, 0), len(x_grid) - 2)
    iy = min(max(iy, 0), len(y_grid) - 2)

    x1, x2 = x_grid[ix], x_grid[ix + 1]
    y1, y2 = y_grid[iy], y_grid[iy + 1]
    Q11 = Z[iy, ix]
    Q21 = Z[iy, ix + 1]
    Q12 = Z[iy + 1, ix]
    Q22 = Z[iy + 1, ix + 1]

    if None in [Q11, Q21, Q12, Q22]:
        raise ValueError(f"Cannot interpolate at (x={x}, y={y}) due to missing data in the grid cell.")

    # Linear weights
    tx = 0.0 if x2 == x1 else (x - x1) / (x2 - x1)
    ty = 0.0 if y2 == y1 else (y - y1) / (y2 - y1)

    # Bilinear blend
    return (1 - tx) * (1 - ty) * Q11 + tx * (1 - ty) * Q21 + (1 - tx) * ty * Q12 + tx * ty * Q22


# This is table 1 from Blech and Averbach 1964
δ_grid = np.array(
    [  # μR = 0
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        # μR = 0.1
        [
            0.1049,
            0.1023,
            0.1001,
            0.0981,
            0.0963,
            0.0946,
            0.0931,
            0.0916,
            0.0902,
            0.0888,
            0.0895,
            0.0819,
            0.0768,
            0.0615,
            0.0425,
            0.0333,
            0.0277,
            0.0237,
        ],
        # μR = 0.2
        [
            0.1922,
            0.1878,
            0.1841,
            0.1809,
            0.178,
            0.1752,
            0.1726,
            0.1701,
            0.1677,
            0.1654,
            0.1631,
            0.1536,
            0.1445,
            0.1174,
            0.0818,
            0.0644,
            0.0535,
            0.046,
        ],
        # μR = 0.3
        [
            0.2657,
            0.26,
            0.2553,
            0.2512,
            0.2475,
            0.244,
            0.2407,
            0.2376,
            0.2345,
            0.2316,
            0.2287,
            0.2165,
            0.2044,
            0.1682,
            0.1181,
            0.0933,
            0.0778,
            0.067,
        ],
        # μR = 0.4
        [
            0.3286,
            0.3212,
            0.3157,
            0.311,
            0.3067,
            0.3028,
            0.2991,
            0.2955,
            0.2921,
            0.2887,
            0.2854,
            0.2715,
            0.2573,
            0.2143,
            0.1516,
            0.1203,
            0.1005,
            0.0867,
        ],
        # μR = 0.5
        [
            None,
            0.3742,
            0.367,
            0.3616,
            0.357,
            0.3527,
            0.3488,
            0.345,
            0.3413,
            0.3377,
            0.3342,
            0.3193,
            0.3036,
            0.256,
            0.1825,
            0.1454,
            0.1218,
            0.1053,
        ],
        # μR = 0.6
        [
            None,
            None,
            0.4118,
            0.4046,
            0.3933,
            0.3947,
            0.3905,
            0.3866,
            0.3829,
            0.3792,
            0.3756,
            0.3605,
            0.3439,
            0.2937,
            0.211,
            0.1687,
            0.1418,
            0.1227,
        ],
        # μR = 0.7
        [
            None,
            None,
            None,
            0.4422,
            0.4349,
            0.4295,
            0.425,
            0.421,
            0.4172,
            0.4136,
            0.41,
            0.3953,
            0.3785,
            0.3273,
            0.237,
            0.1904,
            0.1604,
            0.1391,
        ],
        # μR = 0.8
        [
            None,
            None,
            None,
            None,
            0.4661,
            0.4585,
            0.453,
            0.4486,
            0.4447,
            0.4411,
            0.4376,
            0.4239,
            0.4073,
            0.3571,
            0.2607,
            0.2103,
            0.1776,
            0.1543,
        ],
        # μR = 0.9
        [
            None,
            None,
            None,
            None,
            None,
            None,
            0.4751,
            0.4701,
            0.4657,
            0.4619,
            0.4586,
            0.4461,
            0.4303,
            0.383,
            0.282,
            0.2286,
            0.1936,
            0.1685,
        ],
    ]
)


class CylinderAbsorptionCW(PythonAlgorithm):
    def category(self):
        return "CorrectionFunctions\\AbsorptionCorrection"

    def seeAlso(self):
        return ["CylinderAbsorption", "AbsorptionCorrection", "MultipleScatteringCorrection"]

    def name(self):
        return "CylinderAbsorptionCW"

    def summary(self):
        return (
            "Absorption and multiple scattering calculation for cylindrical samples "
            "with constant wavelength and assuming in-plane scattering only."
        )

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", direction=Direction.Input),
            "Input workspace",
        )
        float_greater_than_zero_validator = FloatBoundedValidator(lower=0.0)
        float_greater_than_zero_validator.setLowerExclusive(True)

        self.declareProperty(
            "Wavelength",
            Property.EMPTY_DBL,
            CompositeValidator([float_greater_than_zero_validator, FloatMandatoryValidator()]),
            doc="Wavelength in Angstroms.",
        )
        self.declareProperty(
            "Radius",
            Property.EMPTY_DBL,
            float_greater_than_zero_validator,
            doc="Radius of the cylinder in cm. If not provided, it will be inferred from the workspace sample shape if it is a cylinder.",
        )
        self.declareProperty(
            "Height",
            Property.EMPTY_DBL,
            float_greater_than_zero_validator,
            doc="Height of the cylinder in cm. Only used for multiple scattering calculation. If not provided, it will be inferred from "
            "the workspace sample shape if it is a cylinder.",
        )
        self.declareProperty(
            "AttenuationXSection",
            Property.EMPTY_DBL,
            float_greater_than_zero_validator,
            doc="Attenuation cross-section in barn at 1.7982 Å. If not provided, it will be inferred from the workspace sample material.",
        )
        self.declareProperty(
            "ScatteringXSection",
            Property.EMPTY_DBL,
            float_greater_than_zero_validator,
            doc="Scattering cross-section in barn. If not provided, it will be inferred from the workspace sample material.",
        )
        self.declareProperty(
            "SampleNumberDensity",
            Property.EMPTY_DBL,
            float_greater_than_zero_validator,
            doc="Number density of the material in atoms/Å^3. If not provided, it will be inferred from the workspace sample material.",
        )
        self.declareProperty(
            "AbsorptionCorrectionMethod",
            "Sears",
            doc="Method to calculate absorption correction.",
            validator=StringListValidator(["Sears", "Sabine"]),
        )
        self.declareProperty("MultipleScattering", True, doc="Calculate multiple scattering in addition to absorption correction.")
        self.declareProperty(
            WorkspaceProperty("AbsorptionWorkspace", "", direction=Direction.Output), doc="Absorption correction output workspace name"
        )
        self.declareProperty(
            WorkspaceProperty("MultipleScatteringWorkspace", "", direction=Direction.Output),
            doc="Multiple scattering output workspace name",
        )

    def validateInputs(self):
        issues = dict()

        ws = self.getProperty("InputWorkspace").value

        if self.getProperty("Radius").isDefault and self.getProperty("Height").isDefault:
            shape = ws.sample().getShape()
            if not shape.hasValidShape():
                issues["InputWorkspace"] = (
                    "Input workspace sample shape is not valid. Please provide radius and height properties "
                    "or use a workspace with a valid cylinder shape."
                )
            else:
                shape_info = shape.shapeInfo()
                if shape_info.shape() != GeometryShape.CYLINDER:
                    issues["InputWorkspace"] = (
                        "Input workspace sample shape is not a cylinder. Please provide radius and height properties "
                        "or use a workspace with a cylinder sample shape."
                    )
        elif not self.getProperty("MultipleScattering").value:
            if self.getProperty("Radius").isDefault:
                issues["Radius"] = "Radius is required, either provide it or use a workspace with a cylinder sample shape."
        elif self.getProperty("Radius").isDefault or self.getProperty("Height").isDefault:
            issues["Radius"] = "Radius is required, either provide it or use a workspace with a cylinder sample shape."
            issues["Height"] = "Height is required, either provide it or use a workspace with a cylinder sample shape."

        if (
            self.getProperty("AttenuationXSection").isDefault
            and self.getProperty("ScatteringXSection").isDefault
            and self.getProperty("SampleNumberDensity").isDefault
        ):
            material = ws.sample().getMaterial()
            if (
                material.absorbXSection(self.getProperty("Wavelength").value) == 0
                or material.totalScatterXSection() == 0
                or material.numberDensity == 0
            ):
                msg = (
                    "Attenuation cross-section, scattering cross-section, and number density are not provided and cannot be inferred from "
                    "the workspace sample material. Please provide these properties or ensure the sample material has valid values."
                )
                issues["InputWorkspace"] = msg
                issues["AttenuationXSection"] = msg
                issues["ScatteringXSection"] = msg
                issues["SampleNumberDensity"] = msg
        elif (
            self.getProperty("AttenuationXSection").isDefault
            or self.getProperty("ScatteringXSection").isDefault
            or self.getProperty("SampleNumberDensity").isDefault
        ):
            issues["AttenuationXSection"] = "Attenuation and scattering cross-section properties must be provided together."
            issues["ScatteringXSection"] = "Attenuation and scattering cross-section properties must be provided together."
            issues["SampleNumberDensity"] = "Attenuation and scattering cross-section properties must be provided together."

        return issues

    def PyExec(self):
        ws = self.getProperty("InputWorkspace").value

        wavelength = self.getProperty("Wavelength").value
        method = self.getProperty("AbsorptionCorrectionMethod").value
        multiple_scattering = self.getProperty("MultipleScattering").value

        if (
            self.getProperty("AttenuationXSection").isDefault
            and self.getProperty("ScatteringXSection").isDefault
            and self.getProperty("SampleNumberDensity").isDefault
        ):
            material = ws.sample().getMaterial()
            absorbXSection = material.absorbXSection(wavelength)
            totalScatterXSection = material.totalScatterXSection()
            totalXSection = absorbXSection + totalScatterXSection
            numberDensity = material.numberDensity
        else:
            absorbXSection = self.getProperty("AttenuationXSection").value
            totalScatterXSection = self.getProperty("ScatteringXSection").value
            totalXSection = absorbXSection + totalScatterXSection
            numberDensity = self.getProperty("SampleNumberDensity").value

        if self.getProperty("Radius").isDefault and self.getProperty("Height").isDefault:
            shape_info = ws.sample().getShape().shapeInfo()
            radius = shape_info.radius() * 100  # convert from m to cm
            height = shape_info.height() * 100  # convert from m to cm
        else:
            radius = self.getProperty("Radius").value
            height = self.getProperty("Height").value

        μ = numberDensity * totalXSection
        μR = μ * radius

        self.log().information(
            f"Using absorption method: {method} with wavelength {wavelength:.4f} Å, radius {radius:.4f} cm, height {height:.4f} cm"
        )
        self.log().information(
            f"Calculated absorption coefficient μ: {μ:.4f} cm^-1 from σ_A {absorbXSection:.4f} barn, σ_S {totalScatterXSection:.4f} barn,"
            f" and numberDensity {numberDensity:.4e} atoms/Å^3"
        )

        spectrumInfo = ws.spectrumInfo()

        thetas = np.array([spectrumInfo.twoTheta(i) for i in range(spectrumInfo.size())]) / 2

        if method == "Sears":
            if μR > 0.9:
                raise ValueError(
                    "μR > 0.9. Absorption correction cannot be calculated to ~1% accuracy using Sears method. Please select Sabine."
                )
            a1 = 1.6977
            a2 = -0.0590
            b2 = -0.5
            A = np.exp(-a1 * μR - (a2 + b2 * np.sin(thetas) ** 2) * μR**2)
        else:  # Sabine
            z = 2 * μR
            if z == 0:
                A = np.ones_like(thetas, dtype=float)
            else:
                A_L = 2 * (i0(z) - modstruve(0, z) - (i1(z) - modstruve(1, z)) / z)
                A_B = (i1(2 * z) - modstruve(1, 2 * z)) / z
                A = A_L * np.cos(thetas) ** 2 + A_B * np.sin(thetas) ** 2

        # Create output absorption correction workspace
        output_abs_ws = CreateWorkspace(
            DataX=[0, 1],
            DataY=A,
            NSpec=len(A),
            ParentWorkspace=ws,
            OutputWorkspace=self.getProperty("AbsorptionWorkspace").value,
            EnableLogging=False,
        )

        self.setProperty("AbsorptionWorkspace", output_abs_ws)

        multiple_scattering_delta = 0

        if multiple_scattering:
            R_over_h_grid = np.array(
                [0.10, 0.12, 0.14, 0.16, 0.18, 0.20, 0.22, 0.24, 0.26, 0.28, 0.30, 0.40, 0.50, 1.00, 2.00, 3.00, 4.00, 5.00]
            )
            μR_grid = np.array([0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9])

            R_over_h = radius / height

            try:
                δ = bilinear_interpolate(R_over_h, μR, R_over_h_grid, μR_grid, δ_grid)
            except ValueError:
                self.log().error("Multiple scattering correction out of lookup range for this sample and set to zero")
                δ = 0

            multiple_scattering_delta = δ * totalScatterXSection / totalXSection
            self.log().information(
                f"Calculated multiple scattering delta: {multiple_scattering_delta:.4f} using R/h {R_over_h:.4f} and μR {μR:.4f}"
            )

        output_ms_ws = CreateSingleValuedWorkspace(
            DataValue=multiple_scattering_delta, OutputWorkspace=self.getProperty("MultipleScatteringWorkspace").value, EnableLogging=False
        )
        self.setProperty("MultipleScatteringWorkspace", output_ms_ws)


AlgorithmFactory.subscribe(CylinderAbsorptionCW)

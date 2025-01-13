# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import mtd, AlgorithmFactory, NumericAxis, PropertyMode, Progress, PythonAlgorithm, WorkspaceGroupProperty, WorkspaceGroup
from mantid.kernel import (
    Direction,
    EnabledWhenProperty,
    FloatArrayProperty,
    FloatBoundedValidator,
    PropertyCriterion,
    PropertyManagerProperty,
    RebinParamsValidator,
    StringListValidator,
)
from mantid.simpleapi import (
    AppendSpectra,
    ConvertAxisByFormula,
    ConvertSpectrumAxis,
    CloneWorkspace,
    CreateSingleValuedWorkspace,
    CreateWorkspace,
    DeleteWorkspace,
    DeleteWorkspaces,
    Divide,
    GroupWorkspaces,
    Minus,
    Multiply,
    RenameWorkspace,
    ReplaceSpecialValues,
    SofQWNormalisedPolygon,
    SumOverlappingTubes,
    Transpose,
    UnGroupWorkspace,
    WeightedMean,
)

from scipy.constants import physical_constants
import numpy as np
import math


class D7AbsoluteCrossSections(PythonAlgorithm):
    _sampleAndEnvironmentProperties = None
    _mode = None
    _debug = None

    @staticmethod
    def _max_value_per_detector(ws, one_per_detector=True):
        """Returns maximum value either one per detector or a global maximum."""
        if isinstance(mtd[ws], WorkspaceGroup):
            max_values = np.zeros(shape=(mtd[ws][0].getNumberHistograms(), mtd[ws].getNumberOfEntries()))
            err_values = np.zeros(shape=(mtd[ws][0].getNumberHistograms(), mtd[ws].getNumberOfEntries()))
            for entry_no, entry in enumerate(mtd[ws]):
                max_values[:, entry_no] = entry.extractY().T
                err_values[:, entry_no] = entry.extractE().T
        else:
            max_values = mtd[ws].extractY().T
            err_values = mtd[ws].extractE().T
        max_values = max_values.flatten()
        err_values = err_values.flatten()
        if one_per_detector:
            indices = np.argmax(max_values, axis=1)
            values = np.zeros(shape=len(indices))
            errors = np.zeros(shape=len(indices))
            for index_no, index in enumerate(indices):
                values[index_no] = max_values[index]
                errors[index_no] = err_values[index]
        else:
            index = np.argmax(max_values)
            values = max_values[index]
            errors = err_values[index]
        return values, errors

    @staticmethod
    def _extract_numor(name):
        """Returns a numor contained in the workspace name, assuming the first number in the name is the run number.
        Otherwise it returns the full name to ensure a unique workspace name."""
        word_list = name.split("_")
        numor = name  # fail case to ensure unique name
        for word in word_list:
            if word.isnumeric():
                numor = word
        return numor

    @staticmethod
    def _find_matching_twoTheta(sample_ws, det_eff_ws):
        """Finds a workspace in the detector efficiency WorkspaceGroup det_eff_ws that matches the twoTheta
        SampleLog of the sample_ws workspace."""
        sample_twoTheta = str(mtd[sample_ws].getRun().getLogData("2theta.requested").value)
        matched_no = 0
        for entry_no, entry in enumerate(mtd[det_eff_ws]):
            det_eff_twoTheta = str(entry.getRun().getLogData("2theta.requested").value)
            if sample_twoTheta == det_eff_twoTheta:
                matched_no = entry_no
                break
        return matched_no

    @staticmethod
    def _calculate_uniaxial_separation(cr_section):
        """Separates measured uniaxial (Z-only) cross-sections into total, nuclear coherent/isotope-incoherent,
        and spin-incoherent components and returns them separately. Based on DOI:10.1063/1.4819739, Eq. 9,
        where magnetic contribution is assumed to be 0.
        Keyword arguments:
        cr_section -- dictionary with measured cross-sections
        """
        total = cr_section["z_nsf"] + cr_section["z_sf"]
        nuclear = cr_section["z_nsf"] - 0.5 * cr_section["z_sf"]
        incoherent = 1.5 * cr_section["z_sf"]
        return total, nuclear, incoherent

    @staticmethod
    def _find_min_max_q(ws):
        """Estimate the start and end q bins for a S(Q, w) workspace."""
        h = physical_constants["Planck constant"][0]  # in m^2 kg / s
        hbar = h / (2.0 * np.pi)  # in m^2 kg / s
        neutron_mass = physical_constants["neutron mass"][0]  # in0 kg
        wavelength = ws.getRun().getLogData("monochromator.wavelength").value * 1e-10  # in m
        Ei = math.pow(h / wavelength, 2) / (2 * neutron_mass)  # in Joules
        minW = ws.readX(0)[0] * 1e-3 * physical_constants["elementary charge"][0]  # in Joules
        maxEf = Ei - minW
        # In Ångströms
        maxQ = 1e-10 * np.sqrt((2.0 * neutron_mass / (hbar**2)) * (Ei + maxEf - 2 * np.sqrt(Ei * maxEf) * -1.0))
        minQ = 0.0
        return minQ, maxQ

    @staticmethod
    def _median_delta_two_theta(ws):
        """Calculate the median theta spacing for a S(Q, w) workspace."""
        tmp_ws = "{}_tmp".format(ws.name())
        ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=tmp_ws, Target="SignedTheta", OrderAxis=False)
        Transpose(InputWorkspace=tmp_ws, OutputWorkspace=tmp_ws)
        thetas = mtd[tmp_ws].getAxis(0).extractValues() * np.pi / 180.0  # in rad
        dThetas = np.abs(np.diff(thetas))
        DeleteWorkspace(Workspace=tmp_ws)
        return np.median(dThetas)

    def category(self):
        return "ILL\\Diffraction"

    def summary(self):
        return (
            "Separates magnetic, nuclear coherent, and incoherent components for diffraction and spectroscopy data,"
            "and corrects the sample data for detector efficiency and normalises it to the chosen standard."
        )

    def seeAlso(self):
        return ["D7YIGPositionCalibration", "PolDiffILLReduction"]

    def name(self):
        return "D7AbsoluteCrossSections"

    def validateInputs(self):
        issues = dict()

        normalisation_method = self.getPropertyValue("NormalisationMethod")

        if normalisation_method == "Vanadium" and self.getProperty("VanadiumInputWorkspace").isDefault:
            issues["VanadiumInputWorkspace"] = (
                'Vanadium input workspace is mandatory for when detector efficiency calibration is "Vanadium".'
            )

        if normalisation_method in ["Incoherent", "Paramagnetic"]:
            if self.getProperty("CrossSectionSeparationMethod").isDefault:
                issues["NormalisationMethod"] = "Chosen sample normalisation requires input from the cross-section separation."
                issues["CrossSectionSeparationMethod"] = "Chosen sample normalisation requires input from the cross-section separation."

            if normalisation_method == "Paramagnetic" and self.getPropertyValue("CrossSectionSeparationMethod") == "Z":
                issues["NormalisationMethod"] = "Paramagnetic normalisation is not compatible with Z-only measurement."
                issues["CrossSectionSeparationMethod"] = "Paramagnetic normalisation is not compatible with Z-only measurement."

        if normalisation_method != "None" or self.getPropertyValue("CrossSectionSeparationMethod") == "10p":
            sampleAndEnvironmentProperties = self.getProperty("SampleAndEnvironmentProperties").value
            if len(sampleAndEnvironmentProperties) == 0:
                issues["SampleAndEnvironmentProperties"] = "Sample parameters need to be defined."
            else:
                required_keys = []
                if normalisation_method == "Incoherent":
                    required_keys = ["SampleMass", "FormulaUnitMass"]
                elif normalisation_method == "Incoherent" and self.getProperty("AbsoluteUnitsNormalisation").value:
                    required_keys.append("IncoherentCrossSection")
                elif normalisation_method == "Paramagnetic":
                    required_keys.append("SampleSpin")

                if self.getPropertyValue("CrossSectionSeparationMethod") == "10p":
                    required_keys.append("ThetaOffset")

                if self.getPropertyValue("MeasurementTechnique") == "SingleCrystal":
                    required_keys.extend(["KiXAngle", "OmegaShift"])

                for key in required_keys:
                    if key not in sampleAndEnvironmentProperties:
                        issues["SampleAndEnvironmentProperties"] = "{} needs to be defined.".format(key)

        return issues

    def PyInit(self):
        self.declareProperty(
            WorkspaceGroupProperty("InputWorkspace", "", direction=Direction.Input),
            doc="The input workspace with spin-flip and non-spin-flip data.",
        )

        self.declareProperty(
            WorkspaceGroupProperty("RotatedXYZWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The workspace used in 10p method when data is taken as two XYZ measurements rotated by 45 degress.",
        )

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The output workspace.",
        )

        self.declareProperty(
            name="CrossSectionSeparationMethod",
            defaultValue="None",
            validator=StringListValidator(["None", "Z", "XYZ", "10p"]),
            direction=Direction.Input,
            doc="What type of cross-section separation to perform.",
        )

        self.declareProperty(
            name="OutputUnits",
            defaultValue="Default",
            validator=StringListValidator(["Default", "TwoTheta", "Q", "Qxy", "Qw", "Input"]),
            direction=Direction.Input,
            doc="The choice to display the output as a function of detector twoTheta,"
            " the momentum exchange, the 2D momentum exchange, or as a function of momentum"
            " and energy exchange. Default will provide output appropriate to the technique used.",
        )

        self.declareProperty(
            name="NormalisationMethod",
            defaultValue="None",
            validator=StringListValidator(["None", "Vanadium", "Incoherent", "Paramagnetic"]),
            direction=Direction.Input,
            doc="Method to correct detector efficiency and normalise data.",
        )

        self.declareProperty(
            name="OutputTreatment",
            defaultValue="Individual",
            validator=StringListValidator(["Individual", "Merge"]),
            direction=Direction.Input,
            doc="Which treatment of the provided scan should be used to create output.",
        )

        self.declareProperty(
            name="MeasurementTechnique",
            defaultValue="Powder",
            validator=StringListValidator(["Powder", "SingleCrystal", "TOF"]),
            direction=Direction.Input,
            doc="What type of measurement technique has been used to collect the data.",
        )

        self.declareProperty(
            PropertyManagerProperty("SampleAndEnvironmentProperties", dict()),
            doc="Dictionary for the information about sample and its environment.",
        )

        self.declareProperty(
            name="ScatteringAngleBinSize",
            defaultValue=0.5,
            validator=FloatBoundedValidator(lower=0),
            direction=Direction.Input,
            doc="Scattering angle bin size in degrees used for expressing scan data on a single TwoTheta axis.",
        )

        self.setPropertySettings("ScatteringAngleBinSize", EnabledWhenProperty("OutputTreatment", PropertyCriterion.IsEqualTo, "Merge"))

        self.declareProperty(
            WorkspaceGroupProperty("VanadiumInputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the vanadium workspace.",
        )

        self.setPropertySettings(
            "VanadiumInputWorkspace", EnabledWhenProperty("NormalisationMethod", PropertyCriterion.IsEqualTo, "Vanadium")
        )

        validRebinParams = RebinParamsValidator(AllowEmpty=True)
        self.declareProperty(FloatArrayProperty(name="QBinning", validator=validRebinParams), doc="Manual Q-binning parameters.")

        self.setPropertySettings("QBinning", EnabledWhenProperty("MeasurementTechnique", PropertyCriterion.IsEqualTo, "TOF"))

        self.declareProperty("AbsoluteUnitsNormalisation", True, doc="Whether or not express the output in absolute units.")

        self.declareProperty(
            "IsotropicMagnetism", True, doc="Whether the paramagnetism is isotropic (Steward, Ehlers) or anisotropic (Schweika)."
        )

        self.declareProperty("ClearCache", True, doc="Whether or not to delete intermediate workspaces.")

        self.declareProperty(
            "DebugMode", defaultValue=False, doc="Whether to create and show all intermediate workspaces at each correction step."
        )

    def _data_structure_helper(self, ws):
        """Returns the number of polarisation orientations present in the data and checks against the chosen
        cross-section separation method if it is feasible with the given data structure."""
        user_method = self.getPropertyValue("CrossSectionSeparationMethod")
        measurements = set()
        for name in mtd[ws].getNames():  # ws name suffix is now: '_POL_FLIPPER_STATE'
            slast_underscore = name.rfind("_", 0, name.rfind("_"))
            measurements.add(name[slast_underscore + 1 :])
        nMeasurements = len(measurements)
        error_msg = "The provided data cannot support {} measurement cross-section separation."
        if nMeasurements == 10:
            nComponents = 6  # Total, Nuclear Coherent, Spin-incoherent, SF Magnetic, NSF Magnetic, Average Magnetic
        elif nMeasurements == 6:
            nComponents = 6  # Total, Nuclear Coherent, Spin-incoherent, SF Magnetic, NSF Magnetic, Average Magnetic
            if user_method == "10p" and self.getProperty("RotatedXYZWorkspace").isDefault:
                raise RuntimeError(error_msg.format(user_method))
        elif nMeasurements == 2:
            nComponents = 3  # Total, Nuclear Coherent, Spin-incoherent
            if user_method == "10p":
                raise RuntimeError(error_msg.format(user_method))
            if user_method == "XYZ":
                raise RuntimeError(error_msg.format(user_method))
        if nMeasurements not in [2, 6, 10]:
            raise RuntimeError(
                "The analysis options are: Z, XYZ, and 10p. " + "The provided input does not fit in any of these measurement types."
            )
        return nMeasurements, nComponents

    def _read_experiment_properties(self, ws):
        """Reads the user-provided dictionary that contains sample geometry (type, dimensions) and experimental conditions,
        such as the beam size and calculates derived parameters."""
        self._sampleAndEnvironmentProperties = self.getProperty("SampleAndEnvironmentProperties").value
        if "InitialEnergy" not in self._sampleAndEnvironmentProperties:
            h = physical_constants["Planck constant"][0]  # in m^2 kg / s
            neutron_mass = physical_constants["neutron mass"][0]  # in0 kg
            wavelength = mtd[ws][0].getRun().getLogData("monochromator.wavelength").value * 1e-10  # in m
            joules_to_mev = 1e3 / physical_constants["electron volt"][0]
            self._sampleAndEnvironmentProperties["InitialEnergy"] = joules_to_mev * math.pow(h / wavelength, 2) / (2 * neutron_mass)

        if self.getPropertyValue("NormalisationMethod") != "None" and "NMoles" not in self._sampleAndEnvironmentProperties:
            sample_mass = self._sampleAndEnvironmentProperties["SampleMass"].value
            formula_unit_mass = self._sampleAndEnvironmentProperties["FormulaUnitMass"].value
            self._sampleAndEnvironmentProperties["NMoles"] = sample_mass / formula_unit_mass

    def _create_angle_dists(self, ws):
        """Calculates sin^2 (alpha) and cos^2 (alpha) for all detectors, needed by Schweika's anisotropic cross-section
        separation. Alpha angle is the Scharpf angle. Returns a name of the workspace containing a difference between
        the calculated cos^2 (alpha) and sin^2 (alpha)."""
        ws_to_transpose = mtd[ws][0].name()
        angle_ws = mtd[ws][0].name() + "_tmp_angle"
        conv_to_theta = 0.5  # conversion to half of the scattering angle
        if self._mode != "SingleCrystal":
            # for single crystal, the spectrum axis is already converted
            ConvertSpectrumAxis(InputWorkspace=mtd[ws][0], OutputWorkspace=angle_ws, Target="SignedTheta", OrderAxis=False)
            ws_to_transpose = angle_ws
            conv_to_theta *= -1.0  # the sign needs to be flipped
        Transpose(InputWorkspace=ws_to_transpose, OutputWorkspace=angle_ws)
        theta = conv_to_theta * mtd[angle_ws].extractX()[0]
        alpha = (theta - self._sampleAndEnvironmentProperties["KiXAngle"].value) * np.pi / 180.0
        cos2_alpha_arr = np.power(np.cos(alpha), 2)
        sin2_alpha_arr = np.power(np.sin(alpha), 2)
        cos2_m_sin2_alpha_name = "cos2_m_sin2_alpha"
        CreateWorkspace(
            OutputWorkspace=cos2_m_sin2_alpha_name, DataX=np.arange(1), DataY=np.subtract(cos2_alpha_arr, sin2_alpha_arr), NSpec=len(alpha)
        )
        DeleteWorkspace(Workspace=angle_ws)
        return cos2_m_sin2_alpha_name

    def _calculate_XYZ_separation(self, cr_section, cos2_m_sin2_alpha):
        """Separates measured 6-point (XYZ) cross-sections into total, nuclear coherent/isotope-incoherent,
        spin-incoherent, and magnetic components and returns them separately. Based on DOI:10.1063/1.4819739, Eq. 9-12.
        Keyword arguments:
        cr_section -- dictionary with measured cross-sections
        cos2_m_sin2_alpha - cos^2 (alpha) - sin^2 (alpha), where alpha is Scharpf angle for each detector
        """
        isotropic_magnetism = self.getProperty("IsotropicMagnetism").value
        # Total cross-section:
        data_total = (
            cr_section["z_nsf"] + cr_section["x_nsf"] + cr_section["y_nsf"] + cr_section["z_sf"] + cr_section["x_sf"] + cr_section["y_sf"]
        ) / 3.0
        if isotropic_magnetism:  # Steward's isotropic cross-section separation
            # Magnetic component
            # spin-flip magnetic component:
            magnetic_1_cs = 2.0 * (-2.0 * cr_section["z_sf"] + cr_section["x_sf"] + cr_section["y_sf"])
            # non-spin-flip component:
            magnetic_2_cs = 2.0 * (2.0 * cr_section["z_nsf"] - cr_section["x_nsf"] - cr_section["y_nsf"])
            data_average_magnetic = WeightedMean(InputWorkspace1=magnetic_1_cs, InputWorkspace2=magnetic_2_cs)
            # Nuclear coherent component
            data_nuclear = (
                2.0 * (cr_section["x_nsf"] + cr_section["y_nsf"] + cr_section["z_nsf"])
                - (cr_section["x_sf"] + cr_section["y_sf"] + cr_section["z_sf"])
            ) / 6.0
            # Incoherent component
            data_incoherent = 0.5 * (cr_section["x_sf"] + cr_section["y_sf"] + cr_section["z_sf"]) - data_average_magnetic
        else:  # anisotropic, Schweika's cross-section separation
            # Nuclear coherent component
            data_nuclear = 0.5 * (cr_section["x_nsf"] + cr_section["y_nsf"] - cr_section["z_sf"])
            # Incoherent component
            data_incoherent = 1.5 * ((cr_section["x_nsf"] - cr_section["y_nsf"]) / mtd[cos2_m_sin2_alpha] + cr_section["z_sf"])
            # Magnetic components
            magnetic_1_cs = cr_section["z_sf"] - (2.0 / 3.0) * data_incoherent  # perpendicular Y
            magnetic_2_cs = cr_section["z_nsf"] - data_incoherent / 3.0 - data_nuclear  # perpendicular Z
            data_average_magnetic = WeightedMean(InputWorkspace1=magnetic_1_cs, InputWorkspace2=magnetic_2_cs)
        return data_total, data_nuclear, data_incoherent, magnetic_1_cs, magnetic_2_cs, data_average_magnetic

    def _create_auxiliary_workspaces(self, parent_ws, n_detectors, to_clean):
        """Returns auxiliary intermediate workspaces, needed to calculate separated cross-sections. Based on
        definitions from DOI:10.1063/1.4819739. Eq. 22 and 23.
        Keyword arguments:
        parent_ws -- workspace used to provide dimensionality and units for the output
        n_detectors -- number of detectors in the instrument
        to_clean -- list with intermediate workspace names to be deleted at later stage
        """
        DEG_2_RAD = np.pi / 180.0
        theta_0 = DEG_2_RAD * self._sampleAndEnvironmentProperties["ThetaOffset"].value
        theta_value = np.zeros(n_detectors)
        for det_no in range(n_detectors):
            theta_value[det_no] = mtd[parent_ws].detectorInfo().twoTheta(det_no)
        alpha_value = theta_value - 0.5 * np.pi - theta_0
        cos_alpha_value = np.cos(alpha_value)
        c0_value = cos_alpha_value**2
        c4_value = (cos_alpha_value - np.pi / 4.0) ** 2

        x_axis = mtd[parent_ws].readX(0)
        c0_t2_m4 = CreateWorkspace(DataX=x_axis, DataY=2 * c0_value - 4, NSPec=n_detectors, ParentWorkspace=parent_ws)
        to_clean.add("c0_t2_m4")
        c0_t2_p2 = CreateWorkspace(DataX=x_axis, DataY=2 * c0_value + 2, NSPec=n_detectors, ParentWorkspace=parent_ws)
        to_clean.add("c0_t2_p2")
        mc0_t4_p2 = CreateWorkspace(DataX=x_axis, DataY=-4 * c0_value + 2, NSPec=n_detectors, ParentWorkspace=parent_ws)
        to_clean.add("mc0_t4_p2")
        c4_t2_m4 = CreateWorkspace(DataX=x_axis, DataY=2 * c4_value - 4, NSPec=n_detectors, ParentWorkspace=parent_ws)
        to_clean.add("c4_t2_m4")
        c4_t2_p2 = CreateWorkspace(DataX=x_axis, DataY=2 * c4_value + 2, NSPec=n_detectors, ParentWorkspace=parent_ws)
        to_clean.add("c4_t2_p2")
        mc4_t4_p2 = CreateWorkspace(DataX=x_axis, DataY=-4 * c4_value + 2, NSPec=n_detectors, ParentWorkspace=parent_ws)
        to_clean.add("mc4_t4_p2")
        cos_2alpha = CreateWorkspace(DataX=x_axis, DataY=np.cos(2 * alpha_value), NSPec=n_detectors, ParentWorkspace=parent_ws)
        to_clean.add("cos_2alpha")
        sin_2alpha = CreateWorkspace(DataX=x_axis, DataY=np.cos(2 * alpha_value), NSPec=n_detectors, ParentWorkspace=parent_ws)
        to_clean.add("sin_2alpha")

        return c0_t2_m4, c0_t2_p2, mc0_t4_p2, c4_t2_m4, c4_t2_p2, mc4_t4_p2, cos_2alpha, sin_2alpha, to_clean

    def _calculate_10p_separation(self, parent_ws, n_detectors, to_clean, cr_section):
        """Separates measured 10-point or two measurements of 6-point cross-sections into total,
        nuclear coherent/isotope-incoherent, spin-incoherent, and magnetic components and returns them separately.
        Based on DOI:10.1063/1.4819739, Eq. 22-24.
        Keyword arguments:
        parent_ws -- workspace used to provide dimensionality and units for the auxiliary workspaces
        n_detectors -- number of detectors in the instrument
        to_clean -- list with intermediate workspace names to be deleted at later stage
        cr_section -- dictionary with measured cross-sections
        """

        total = (
            cr_section["z_nsf"]
            + cr_section["x_nsf"]
            + cr_section["y_nsf"]
            + cr_section["xmy_nsf"]
            + cr_section["xpy_nsf"]
            + cr_section["z_sf"]
            + cr_section["x_sf"]
            + cr_section["y_sf"]
            + cr_section["xpy_sf"]
            + cr_section["xpy_sf"]
        ) / 5.0

        c0_t2_m4, c0_t2_p2, mc0_t4_p2, c4_t2_m4, c4_t2_p2, mc4_t4_p2, cos_2alpha, sin_2alpha, to_clean = self._create_auxiliary_workspaces(
            parent_ws, n_detectors, to_clean
        )

        # Magnetic component
        magnetic_nsf_cos2alpha = c0_t2_m4 * cr_section["x_nsf"] + c0_t2_p2 * cr_section["y_nsf"] + mc0_t4_p2 * cr_section["z_nsf"]
        magnetic_sf_cos2alpha = (-1) * c0_t2_m4 * cr_section["x_sf"] - c0_t2_p2 * cr_section["y_sf"] - mc0_t4_p2 * cr_section["z_sf"]
        magnetic_nsf_sin2alpha = c4_t2_m4 * cr_section["xpy_sf"] + c4_t2_p2 * cr_section["xmy_nsf"] + mc4_t4_p2 * cr_section["z_nsf"]
        magnetic_sf_sin2alpha = (-1) * c4_t2_m4 * cr_section["xpy_sf"] - c4_t2_p2 * cr_section["xpy_sf"] - mc4_t4_p2 * cr_section["z_sf"]
        to_clean.add("magnetic_sf_cos2alpha")
        to_clean.add("magnetic_nsf_cos2alpha")
        to_clean.add("magnetic_sf_sin2alpha")
        to_clean.add("magnetic_nsf_sin2alpha")

        nsf_magnetic = magnetic_nsf_cos2alpha * cos_2alpha + magnetic_nsf_sin2alpha * sin_2alpha
        nsf_magnetic.getAxis(0).setUnit(mtd[parent_ws].getAxis(0).getUnit().unitID())
        nsf_magnetic.getAxis(1).setUnit(mtd[parent_ws].getAxis(1).getUnit().unitID())

        sf_magnetic = magnetic_sf_cos2alpha * cos_2alpha + magnetic_sf_sin2alpha * sin_2alpha
        sf_magnetic.getAxis(0).setUnit(mtd[parent_ws].getAxis(0).getUnit().unitID())
        sf_magnetic.getAxis(1).setUnit(mtd[parent_ws].getAxis(1).getUnit().unitID())

        average_magnetic = WeightedMean(InputWorkspace1=sf_magnetic, InputWorkspace2=nsf_magnetic)

        # Nuclear coherent component
        nuclear = (
            2.0 * (cr_section["x_nsf"] + cr_section["y_nsf"] + 2 * cr_section["z_nsf"] + cr_section["xpy_nsf"] + cr_section["xmy_nsf"])
            - (cr_section["x_sf"] + cr_section["y_sf"] + 2 * cr_section["z_sf"] + cr_section["xpy_sf"] + cr_section["xmy_sf"])
        ) / 12.0
        # Incoherent component
        incoherent = 0.25 * (cr_section["x_sf"] + cr_section["y_sf"] + 2 * cr_section["z_sf"] + cr_section["xpy_sf"] + cr_section["xmy_sf"])
        Minus(LHSWOrkspace=incoherent, RHSWorkspace=average_magnetic, OutputWorkspace=incoherent)

        return total, nuclear, incoherent, sf_magnetic, nsf_magnetic, average_magnetic, to_clean

    def _cross_section_separation(self, ws, nMeasurements):
        """Separates coherent, incoherent, and magnetic components based on spin-flip and non-spin-flip intensities of
        the current sample. The method used is based on either the user's choice or the provided data structure."""
        user_method = self.getPropertyValue("CrossSectionSeparationMethod")
        isotropic_magnetism = self.getProperty("IsotropicMagnetism").value
        to_clean = set()
        cross_sections = dict()
        cos2_m_sin2_alpha = ""
        if user_method == "XYZ" and not isotropic_magnetism:
            cos2_m_sin2_alpha = self._create_angle_dists(ws)
            to_clean.add(cos2_m_sin2_alpha)
        n_detectors = mtd[ws][0].getNumberHistograms()
        double_xyz_method = False
        if not self.getProperty("RotatedXYZWorkspace").isDefault:
            double_xyz_method = True
            second_xyz_ws = self.getPropertyValue("RotatedXYZWorkspace")
        separated_cs = []
        for entry_no in range(0, mtd[ws].getNumberOfEntries(), nMeasurements):
            cross_sections["z_sf"] = mtd[ws][entry_no]
            cross_sections["z_nsf"] = mtd[ws][entry_no + 1]
            numor = self._extract_numor(mtd[ws][entry_no].name())
            total_cs = numor + "_Total"
            nuclear_cs = numor + "_Coherent"
            incoherent_cs = numor + "_Incoherent"
            if user_method == "Z":
                data_total, data_nuclear, data_incoherent = self._calculate_uniaxial_separation(cross_sections)
                RenameWorkspace(InputWorkspace=data_total, OutputWorkspace=total_cs)
                separated_cs.append(total_cs)
                RenameWorkspace(InputWorkspace=data_nuclear, OutputWorkspace=nuclear_cs)
                separated_cs.append(nuclear_cs)
                RenameWorkspace(InputWorkspace=data_incoherent, OutputWorkspace=incoherent_cs)
                separated_cs.append(incoherent_cs)
            elif nMeasurements == 6 or nMeasurements == 10:
                cross_sections["x_sf"] = mtd[ws][entry_no + 2]
                cross_sections["x_nsf"] = mtd[ws][entry_no + 3]
                cross_sections["y_sf"] = mtd[ws][entry_no + 4]
                cross_sections["y_nsf"] = mtd[ws][entry_no + 5]
                if any(["Y" in ws_name for ws_name in [cross_sections["x_sf"].name(), cross_sections["x_nsf"].name()]]):
                    # if the order of X and Y polarisations is different than assumed, swap
                    cross_sections["x_sf"], cross_sections["y_sf"] = cross_sections["y_sf"], cross_sections["x_sf"]
                    cross_sections["x_nsf"], cross_sections["y_nsf"] = cross_sections["y_nsf"], cross_sections["x_nsf"]
                average_magnetic_cs = numor + "_AverageMagnetic"
                if isotropic_magnetism:
                    magnetic_name_1 = "_SFMagnetic"
                    magnetic_name_2 = "_NSFMagnetic"
                else:
                    magnetic_name_1 = "_PerpMagnetic_Y"
                    magnetic_name_2 = "_PerpMagnetic_Z"
                magnetic_1_cs = numor + magnetic_name_1
                magnetic_2_cs = numor + magnetic_name_2
                if nMeasurements == 6 and user_method == "XYZ":
                    output_data = self._calculate_XYZ_separation(cr_section=cross_sections, cos2_m_sin2_alpha=cos2_m_sin2_alpha)
                    RenameWorkspace(InputWorkspace=output_data[0], OutputWorkspace=total_cs)
                    separated_cs.append(total_cs)
                    RenameWorkspace(InputWorkspace=output_data[1], OutputWorkspace=nuclear_cs)
                    separated_cs.append(nuclear_cs)
                    RenameWorkspace(InputWorkspace=output_data[2], OutputWorkspace=incoherent_cs)
                    separated_cs.append(incoherent_cs)
                    RenameWorkspace(InputWorkspace=output_data[5], OutputWorkspace=average_magnetic_cs)
                    separated_cs.append(average_magnetic_cs)
                    RenameWorkspace(InputWorkspace=output_data[3], OutputWorkspace=magnetic_1_cs)
                    separated_cs.append(magnetic_1_cs)
                    RenameWorkspace(InputWorkspace=output_data[4], OutputWorkspace=magnetic_2_cs)
                    separated_cs.append(magnetic_2_cs)
                else:
                    if not double_xyz_method:
                        cross_sections["xmy_sf"] = mtd[ws][entry_no + 6]
                        cross_sections["xmy_nsf"] = mtd[ws][entry_no + 7]
                        cross_sections["xpy_sf"] = mtd[ws][entry_no + 8]
                        cross_sections["xpy_nsf"] = mtd[ws][entry_no + 9]
                    else:
                        # assumed is averaging of twice measured Z-axis:
                        cross_sections["z_sf"] = 0.5 * (cross_sections["z_sf"] + mtd[second_xyz_ws][entry_no])
                        cross_sections["z_nsf"] = 0.5 * (cross_sections["z_nsf"] + mtd[second_xyz_ws][entry_no + 1])
                        cross_sections["xmy_sf"] = mtd[second_xyz_ws][entry_no + 2]
                        cross_sections["xmy_nsf"] = mtd[second_xyz_ws][entry_no + 3]
                        cross_sections["xpy_sf"] = mtd[second_xyz_ws][entry_no + 4]
                        cross_sections["xpy_nsf"] = mtd[second_xyz_ws][entry_no + 5]
                    output_data = self._calculate_10p_separation(
                        parent_ws=mtd[ws][entry_no].name(), n_detectors=n_detectors, to_clean=to_clean, cr_section=cross_sections
                    )
                    to_clean = output_data[-1]
                    RenameWorkspace(InputWorkspace=output_data[0], OutputWorkspace=total_cs)
                    separated_cs.append(total_cs)
                    RenameWorkspace(InputWorkspace=output_data[1], OutputWorkspace=nuclear_cs)
                    separated_cs.append(nuclear_cs)
                    RenameWorkspace(InputWorkspace=output_data[2], OutputWorkspace=incoherent_cs)
                    separated_cs.append(incoherent_cs)
                    RenameWorkspace(InputWorkspace=output_data[3], OutputWorkspace=average_magnetic_cs)
                    separated_cs.append(average_magnetic_cs)
                    RenameWorkspace(InputWorkspace=output_data[4], OutputWorkspace=magnetic_1_cs)
                    separated_cs.append(magnetic_1_cs)
                    RenameWorkspace(InputWorkspace=output_data[5], OutputWorkspace=magnetic_2_cs)
                    separated_cs.append(magnetic_2_cs)

        if self.getProperty("ClearCache").value and to_clean != set():  # clean only when non-empty
            DeleteWorkspaces(WorkspaceList=list(to_clean))
        output_name = ws + "_separated_cs"
        GroupWorkspaces(InputWorkspaces=separated_cs, OutputWorkspace=output_name)
        if self._debug:
            clone_name = "{}_separated_cs".format(output_name)
            CloneWorkspace(InputWorkspace=output_name, OutputWorkspace=clone_name)
        return output_name

    def _detector_efficiency_correction(self, cross_section_ws):
        """Calculates detector efficiency using either vanadium data, incoherent,
        or paramagnetic scattering cross-sections."""

        calibrationType = self.getPropertyValue("NormalisationMethod")
        normaliseToAbsoluteUnits = self.getProperty("AbsoluteUnitsNormalisation").value
        det_efficiency_ws = cross_section_ws + "_det_efficiency"
        norm_ws = "normalisation_ws"
        tmp_name = "det_eff"
        tmp_names = []
        to_clean = []
        if calibrationType == "Vanadium":
            if normaliseToAbsoluteUnits:
                normFactor = self._sampleAndEnvironmentProperties["NMoles"].value
                CreateSingleValuedWorkspace(DataValue=normFactor, OutputWorkspace=norm_ws)
            else:
                CreateSingleValuedWorkspace(DataValue=1.0, OutputWorkspace=norm_ws)
            to_clean.append(norm_ws)
            Multiply(LHSWorkspace=cross_section_ws, RHSWorkspace=norm_ws, OutputWorkspace=det_efficiency_ws)
        elif calibrationType in ["Paramagnetic", "Incoherent"]:
            if calibrationType == "Paramagnetic":
                if self._mode == "TOF":
                    raise RuntimeError("Paramagnetic calibration is not valid in the TOF mode.")
                spin = self._sampleAndEnvironmentProperties["SampleSpin"].value
                for entry_no, entry in enumerate(mtd[cross_section_ws]):
                    ws_name = "{0}_{1}".format(tmp_name, entry_no)
                    tmp_names.append(ws_name)
                    const = (2.0 / 3.0) * math.pow(
                        physical_constants["neutron gyromag. ratio"][0] * physical_constants["classical electron radius"][0], 2
                    )
                    paramagneticComponent = mtd[cross_section_ws][3]
                    normalisation_name = "normalisation_{}".format(ws_name)
                    to_clean.append(normalisation_name)
                    CreateSingleValuedWorkspace(DataValue=const * spin * (spin + 1), OutputWorkspace=normalisation_name)
                    Divide(LHSWorkspace=paramagneticComponent, RHSWorkspace=normalisation_name, OutputWorkspace=ws_name)
            else:  # Incoherent
                if self._mode == "TOF":
                    raise RuntimeError("Incoherent calibration is not valid in the TOF mode.")
                for spectrum_no in range(mtd[cross_section_ws][2].getNumberHistograms()):
                    if normaliseToAbsoluteUnits:
                        normFactor = self._sampleAndEnvironmentProperties["IncoherentCrossSection"].value
                        CreateSingleValuedWorkspace(DataValue=normFactor, OutputWorkspace=norm_ws)
                    else:
                        normalisationFactors, dataE = self._max_value_per_detector(mtd[cross_section_ws].name(), one_per_detector=False)
                        if isinstance(normalisationFactors, float):
                            CreateSingleValuedWorkspace(DataValue=normalisationFactors, ErrorValue=dataE, OutputWorkspace=norm_ws)
                        else:
                            CreateWorkspace(
                                dataX=mtd[cross_section_ws][1].readX(0),
                                dataY=normalisationFactors,
                                dataE=dataE,
                                NSpec=mtd[cross_section_ws][1].getNumberHistograms(),
                                OutputWorkspace=norm_ws,
                            )
                    ws_name = "{0}_{1}".format(tmp_name, spectrum_no)
                    tmp_names.append(ws_name)
                    Divide(LHSWorkspace=mtd[cross_section_ws][2], RHSWorkspace=norm_ws, OutputWorkspace=ws_name)
                    to_clean.append(norm_ws)

            GroupWorkspaces(InputWorkspaces=tmp_names, OutputWorkspace=det_efficiency_ws)

        if self.getProperty("ClearCache").value and len(to_clean) != 0:
            DeleteWorkspaces(to_clean)
        if self._debug:
            clone_name = "{}_det_efficiency".format(det_efficiency_ws)
            CloneWorkspace(InputWorkspace=det_efficiency_ws, OutputWorkspace=clone_name)
        return det_efficiency_ws

    def _normalise_sample_data(self, sample_ws, det_efficiency_ws, nMeasurements, nComponents):
        """Normalises the sample data using the detector efficiency calibration workspace."""
        normalisation_method = self.getPropertyValue("NormalisationMethod")
        if (
            normalisation_method == "Vanadium"
            and self._mode == "SingleCrystal"
            and mtd[sample_ws][0].getNumberHistograms() == 2 * mtd[det_efficiency_ws][0].getNumberHistograms()
        ):
            # the length of the spectrum axis is twice the size of Vanadium, as data comes from two omega scans
            AppendSpectra(InputWorkspace1=det_efficiency_ws, InputWorkspace2=det_efficiency_ws, OutputWorkspace=det_efficiency_ws)

        single_eff_per_numor = False
        single_eff = False
        eff_entries = mtd[det_efficiency_ws].getNumberOfEntries()
        sample_entries = mtd[sample_ws].getNumberOfEntries()
        single_eff_per_twoTheta = False
        if eff_entries == 1:
            single_eff = True
        elif eff_entries != mtd[sample_ws].getNumberOfEntries() and eff_entries == (sample_entries / nComponents):
            single_eff_per_numor = True
        elif (sample_entries / nComponents) % eff_entries == 0:
            single_eff_per_twoTheta = True
        tmp_names = []

        for entry_no, entry in enumerate(mtd[sample_ws]):
            det_eff_no = entry_no
            if single_eff:
                det_eff_no = 0
            elif single_eff_per_numor:
                det_eff_no = entry_no % nMeasurements
            elif single_eff_per_twoTheta:
                det_eff_no = self._find_matching_twoTheta(entry.name(), det_efficiency_ws)
            elif det_eff_no >= eff_entries:
                det_eff_no = det_eff_no % eff_entries
            ws_name = entry.name() + "_normalised"
            tmp_names.append(ws_name)
            Divide(LHSWorkspace=entry, RHSWorkspace=mtd[det_efficiency_ws][det_eff_no], OutputWorkspace=ws_name)
        output_ws = self.getPropertyValue("OutputWorkspace")
        GroupWorkspaces(InputWorkspaces=tmp_names, Outputworkspace=output_ws)
        if self._debug:
            clone_name = "{}_absolute_scale".format(output_ws)
            CloneWorkspace(InputWorkspace=output_ws, OutputWorkspace=clone_name)
        return output_ws

    def _qxy_rebin(self, ws):
        """
        Rebins the single crystal omega scan measurement output onto 2D Qx-Qy grid.
        :param ws: Output of the cross-section separation and/or normalisation.
        :return: WorkspaceGroup containing 2D histograms on a Qx-Qy grid.
        """
        DEG_2_RAD = np.pi / 180.0
        fld = self._sampleAndEnvironmentProperties["fld"].value if "fld" in self._sampleAndEnvironmentProperties else 1
        nQ = self._sampleAndEnvironmentProperties["nQ"].value if "nQ" in self._sampleAndEnvironmentProperties else 80
        omega_shift = (
            self._sampleAndEnvironmentProperties["OmegaShift"].value if "OmegaShift" in self._sampleAndEnvironmentProperties else 0
        )
        wavelength = mtd[ws][0].getRun().getLogData("monochromator.wavelength").value
        ki = 2 * np.pi / wavelength
        dE = 0.0  # monochromatic data
        const_val = 2.07194  # hbar^2/2m
        kf = np.sqrt(ki * ki - dE / const_val)
        twoTheta = -mtd[ws][0].getAxis(1).extractValues() * DEG_2_RAD  # detector positions in radians
        omega = mtd[ws][0].getAxis(0).extractValues() * DEG_2_RAD  # omega scan angle in radians
        ntheta = len(twoTheta)
        nomega = len(omega)
        omega = np.matrix(omega) + omega_shift * DEG_2_RAD
        Qmag = np.sqrt(ki * ki + kf * kf - 2 * ki * kf * np.cos(twoTheta))
        # beta is the angle between ki and Q
        beta = (twoTheta / np.abs(twoTheta)) * np.arccos((ki * ki - kf * kf + Qmag * Qmag) / (2 * ki * Qmag))
        alpha = -np.pi / 2 + omega.T * np.ones(shape=(1, ntheta)) + np.ones(shape=(nomega, 1)) * beta
        Qx = np.multiply((np.ones(shape=(nomega, 1)) * Qmag), np.cos(alpha)).T
        Qy = np.multiply((np.ones(shape=(nomega, 1)) * Qmag), np.sin(alpha)).T
        Qmax = 1.1 * np.max(Qmag)
        dQ = Qmax / nQ
        output_names = []
        for entry in mtd[ws]:
            w_out = np.zeros(shape=((fld + 1) * nQ, (fld + 1) * nQ))
            e_out = np.zeros(shape=((fld + 1) * nQ, (fld + 1) * nQ))
            n_out = np.zeros(shape=((fld + 1) * nQ, (fld + 1) * nQ))
            w_in = entry.extractY()
            e_in = entry.extractE()
            for theta in range(ntheta):
                for omega in range(nomega):
                    if fld == 1:
                        ix = int(((Qx[theta, omega] + dQ / 2.0) / dQ) + nQ)
                        iy = int(((Qy[theta, omega] + dQ / 2.0) / dQ) + nQ)
                        if Qx[theta, omega] > 0.99 * Qmax or Qy[theta, omega] > 0.99 * Qmax:
                            continue
                    else:
                        ix = int(abs((Qx[theta, omega]) + dQ / 2.0) / dQ)
                        iy = int(abs((Qy[theta, omega]) + dQ / 2.0) / dQ)
                    w_out[ix, iy] += w_in[theta, omega]
                    e_out[ix, iy] += e_in[theta, omega] ** 2
                    n_out[ix, iy] += 1.0
            w_out /= n_out
            e_out = np.sqrt(e_out / n_out)
            w_out_name = entry.name() + "_qxqy"
            output_names.append(w_out_name)
            data_x = [(val - (fld * nQ)) * dQ for val in range((fld + 1) * nQ)]
            y_axis = NumericAxis.create(int((fld + 1) * nQ))
            for q_index in range(int((fld + 1) * nQ)):
                y_axis.setValue(q_index, (q_index - (fld * nQ)) * dQ)
            CreateWorkspace(DataX=data_x, DataY=w_out, DataE=e_out, NSpec=int((fld + 1) * nQ), OutputWorkspace=w_out_name)
            mtd[w_out_name].replaceAxis(1, y_axis)
            mtd[w_out_name].getAxis(0).setUnit("Label").setLabel("Qx", r"\AA^{-1}")
            mtd[w_out_name].getAxis(1).setUnit("Label").setLabel("Qy", r"\AA^{-1}")
            ReplaceSpecialValues(
                InputWorkspace=w_out_name, OutputWorkspace=w_out_name, NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0
            )
        DeleteWorkspace(Workspace=ws)
        GroupWorkspaces(InputWorkspaces=output_names, OutputWorkspace=ws)
        return ws

    def _delta_q(self, ws):
        """Estimate a q bin width for a S(Q, w) workspace."""
        deltaTheta = self._median_delta_two_theta(ws)
        wavelength = ws.run().getLogData("monochromator.wavelength").value
        return 2.0 * np.pi / wavelength * deltaTheta

    def _get_q_binning(self, ws):
        """Returns either a user-provided or automatic binning in momentum exchange."""
        if self.getProperty("QBinning").isDefault:
            qMin, qMax = self._find_min_max_q(mtd[ws][0])
            dq = self._delta_q(mtd[ws][0])
            e = np.ceil(-np.log10(dq)) + 1
            dq = (5.0 * ((dq * 10**e) // 5 + 1.0)) * 10**-e
            q_binning = [qMin, dq, qMax]
        else:
            q_binning = self.getProperty("QBinning").value
            if len(q_binning) == 1:
                qMin, qMax = self._find_min_max_q(mtd[ws][0])
                q_binning = [qMin, q_binning[0], qMax]
        return q_binning

    @staticmethod
    def _convert_to_2theta(ws_in, ws_out, merged_data):
        """Converts the relevant axis unit to 2theta."""
        if merged_data:
            ConvertAxisByFormula(InputWorkspace=ws_in, OutputWorkspace=ws_out, Axis="X", Formula="-x")
        else:
            ConvertSpectrumAxis(InputWorkspace=ws_in, OutputWorkspace=ws_out, Target="SignedTheta", OrderAxis=False)
            ConvertAxisByFormula(InputWorkspace=ws_out, OutputWorkspace=ws_out, Axis="Y", Formula="-y")
            Transpose(InputWorkspace=ws_out, OutputWorkspace=ws_out)
        return ws_out

    def _convert_to_q(self, ws_in, ws_out, merged_data):
        """Converts the relevant axis unit to momentum exchange."""
        if merged_data:
            wavelength = mtd[ws_in][0].getRun().getLogData("monochromator.wavelength").value  # in Angstrom
            # flips axis sign and converts detector 2theta to momentum exchange
            formula = "4*pi*sin(-0.5*pi*x/180.0)/{}".format(wavelength)
            ConvertAxisByFormula(InputWorkspace=ws_in, OutputWorkspace=ws_out, Axis="X", Formula=formula)
            # manually set the correct x-axis unit
            for entry in mtd[ws_out]:
                entry.getAxis(0).setUnit("MomentumTransfer")
        else:
            ConvertSpectrumAxis(
                InputWorkspace=ws_in,
                OutputWorkspace=ws_out,
                Target="ElasticQ",
                EFixed=self._sampleAndEnvironmentProperties["InitialEnergy"].value,
                OrderAxis=False,
            )
            Transpose(InputWorkspace=ws_out, OutputWorkspace=ws_out)
        return ws_out

    def _convert_to_sofqw(self, ws_in, ws_out):
        """Converts the input workspace and appends a '_qw' suffix to all workspace names."""
        q_binning = self._get_q_binning(ws_in)
        tmp_name = "{}_{}".format(ws_in, "tmp")
        SofQWNormalisedPolygon(
            InputWorkspace=ws_in,
            OutputWorkspace=tmp_name,
            EMode="Direct",
            EFixed=self._sampleAndEnvironmentProperties["InitialEnergy"].value,
            QAxisBinning=q_binning,
        )
        Transpose(InputWorkspace=tmp_name, OutputWorkspace=tmp_name)  # users prefer omega to be the vertical axis
        original_names = mtd[ws_in].getNames()
        current_names = mtd[tmp_name].getNames()
        new_names = []
        for original_name, current_name in zip(original_names, current_names):
            new_name = "{}_{}".format(original_name, "qw")
            new_names.append(new_name)
            RenameWorkspace(InputWorkspace=current_name, OutputWorkspace=new_name)
        UnGroupWorkspace(InputWorkspace=tmp_name)
        GroupWorkspaces(InputWorkspaces=new_names, OutputWorkspace=ws_out)
        return ws_out

    def _set_units(self, ws, nMeasurements):
        """Sets units for the output workspace."""
        measurement_technique = self.getPropertyValue("MeasurementTechnique")
        separation_method = self.getPropertyValue("CrossSectionSeparationMethod")
        output_unit = self.getPropertyValue("OutputUnits")
        if (
            self.getPropertyValue("NormalisationMethod") in ["Incoherent", "Paramagnetic"]
            or self.getPropertyValue("NormalisationMethod") == "Vanadium"
            and separation_method == "None"
        ):
            unit = "Normalized intensity"
            unit_symbol = ""
        elif self.getPropertyValue("NormalisationMethod") == "Vanadium":
            unit_symbol = "barn / sr / formula unit"
            unit = r"d$\sigma$/d$\Omega$"
            if measurement_technique == "TOF":
                unit_symbol_sofqw = "{} / meV".format(unit_symbol)
                unit_sofqw = r"{}/dE".format(unit)
        else:  # NormalisationMethod == None and OutputUnits == Input
            unit = "Corrected intensity"
            unit_symbol = ""

        perform_merge = mtd[ws].getNumberOfEntries() / nMeasurements > 1 and self.getPropertyValue("OutputTreatment") == "Merge"
        if perform_merge:
            self._merge_data(ws)

        if output_unit == "TwoTheta":
            self._convert_to_2theta(ws_in=ws, ws_out=ws, merged_data=perform_merge)
        elif output_unit == "Q" or (output_unit == "Default" and measurement_technique == "Powder"):
            self._convert_to_q(ws_in=ws, ws_out=ws, merged_data=perform_merge)
        elif output_unit == "Qxy" or (output_unit == "Default" and measurement_technique == "SingleCrystal"):
            ws = self._qxy_rebin(ws)
        elif output_unit == "Qw" or (output_unit == "Default" and measurement_technique == "TOF"):
            group_list = []
            if output_unit == "Default":  # provide SofQW, and distributions as a function of Q and 2theta
                twoTheta_distribution = self._convert_to_2theta(ws_in=ws, ws_out="tthw", merged_data=perform_merge)
                ws_sofqw = self._convert_to_sofqw(ws_in=ws, ws_out=ws + "_qw")
                # transpose the results as a function of spectrum number to remain consistent with other workspaces
                Transpose(InputWorkspace=ws, OutputWorkspace=ws)
                # interleave output names so that SofQW, 2theta, and q distributions for the same input are together:
                group_list = [
                    ws_name
                    for name_tuple in zip(mtd[ws].getNames(), mtd[twoTheta_distribution].getNames(), mtd[ws_sofqw].getNames())
                    for ws_name in name_tuple
                ]
            else:
                ws = self._convert_to_sofqw(ws_in=ws, ws_out=ws)

            if len(group_list) > 0:
                UnGroupWorkspace(InputWorkspace="tthw")
                UnGroupWorkspace(InputWorkspace=ws + "_qw")
                GroupWorkspaces(InputWorkspaces=group_list, OutputWorkspace=ws)

        if isinstance(mtd[ws], WorkspaceGroup):
            for entry in mtd[ws]:
                if measurement_technique == "TOF" and (
                    self.getPropertyValue("NormalisationMethod") != "None"
                    and self.getPropertyValue("CrossSectionSeparationMethod") != "None"
                    and "_qw" in entry.name()
                ):
                    entry.setYUnitLabel("{} ({})".format(unit_sofqw, unit_symbol_sofqw))
                else:
                    entry.setYUnitLabel("{} ({})".format(unit, unit_symbol))
        else:
            mtd[ws].setYUnitLabel("{} ({})".format(unit, unit_symbol))
        return ws

    def _merge_data(self, ws):
        """Averages data belonging to the same polarisation direction and flipper state, or cross-section type."""
        # assumed naming scheme: numor_pol-dir_flipper-state_cross-section(optional)_normalised(optional)
        cs_present = not self.getProperty("CrossSectionSeparationMethod").isDefault
        possible_polarisations = ["ZPO", "YPO", "XPO", "XMYPO", "XPYPO"]  # 10p is approximate, no data atm
        flipper_states = ["_ON", "_OFF"]
        merging_keys = dict()
        for name in mtd[ws].getNames():
            if cs_present:
                non_numor_info = name[name.find("_") + 1 :]
                next_underscore = non_numor_info.find("_")
                if next_underscore < 1:
                    key = non_numor_info
                else:
                    key = non_numor_info[:next_underscore]
            else:
                key = ""
                for pol in possible_polarisations:
                    if pol in name:
                        key = pol
                        break
                if flipper_states[0] in name:
                    key += flipper_states[0]
                else:
                    key += flipper_states[1]
            if key not in merging_keys:
                merging_keys[key] = list()
            merging_keys[key].append(name)
        if len(merging_keys) > 1:
            names_list = list()
            for key in merging_keys:
                name = "{0}_{1}".format(ws, key)
                self._call_sum_data(input_name=merging_keys[key], output_name=name)
                names_list.append(name)
            DeleteWorkspaces(WorkspaceList=ws)
            GroupWorkspaces(InputWorkspaces=names_list, OutputWorkspace=ws)
        return ws

    def _call_sum_data(self, input_name, output_name=""):
        """Wrapper around SumOverlappingTubes algorithm."""
        if output_name == "":
            output_name = input_name
        SumOverlappingTubes(
            InputWorkspaces=input_name,
            OutputWorkspace=output_name,
            OutputType="1D",
            ScatteringAngleBinning=self.getProperty("ScatteringAngleBinSize").value,
            Normalise=True,
            HeightAxis="-0.1,0.1",
        )
        return output_name

    def _get_number_reports(self):
        nreports = 4
        if self.getPropertyValue("CrossSectionSeparationMethod") != "None":
            nreports += 1
        return nreports

    def _set_output_names(self, output_ws):
        """Renames output workspaces with unique names based on the provided output workspace name
        and the input name."""
        input_ws = self.getPropertyValue("InputWorkspace")
        output_treatment = self.getPropertyValue("OutputTreatment")
        separation_method = self.getPropertyValue("CrossSectionSeparationMethod")
        possible_polarisations = ["XPO", "YPO", "ZPO"]
        old_names = []
        new_names = []
        for entry_no, entry in enumerate(mtd[output_ws]):  # renames individual ws to contain the output name
            entry_name = entry.name()
            old_names.append(entry_name)
            if entry_name[:2] == "__":
                entry_name = entry_name[2:]
            if input_ws in entry_name:
                entry_name = entry_name[len(input_ws) + 1 :]  # assuming the verbose input name is at the beginning
            pol_present = [polarisation in entry_name for polarisation in possible_polarisations]
            if any(pol_present) and (output_treatment == "Merge" or separation_method != "None"):
                pol_length = len(max(possible_polarisations, key=len))
                if "ON" in entry_name:
                    pol_length += 4  # length of '_ON_'
                else:  # == 'OFF'
                    pol_length += 5  # length of '_OFF_'
                pol_type = np.array(possible_polarisations)[pol_present]
                pol_pos = entry_name.find(pol_type[0])
                entry_name = "{}{}".format(entry_name[:pol_pos], entry_name[pol_pos + pol_length :])
            output_name = self.getPropertyValue("OutputWorkspace")
            if output_name not in entry_name:
                new_name = "{}_{}".format(output_name, entry_name)
            else:
                new_name = "{}_{}".format(entry_name, str(entry_no))
            new_names.append(new_name)

        for old_name, new_name in zip(old_names, new_names):
            if old_name != new_name:
                RenameWorkspace(InputWorkspace=old_name, OutputWorkspace=new_name)

    def PyExec(self):
        progress = Progress(self, start=0.0, end=1.0, nreports=self._get_number_reports())
        input_ws = self.getPropertyValue("InputWorkspace")
        output_ws = self.getPropertyValue("OutputWorkspace")
        self._mode = self.getPropertyValue("MeasurementTechnique")
        progress.report("Loading experiment properties")
        self._read_experiment_properties(input_ws)
        nMeasurements, nComponents = self._data_structure_helper(input_ws)
        self._debug = self.getProperty("DebugMode").value
        to_clean = []
        normalisation_method = self.getPropertyValue("NormalisationMethod")
        if self.getPropertyValue("CrossSectionSeparationMethod") == "None":
            if normalisation_method == "Vanadium":
                det_efficiency_input = self.getPropertyValue("VanadiumInputWorkspace")
                progress.report("Calculating detector efficiency correction")
                det_efficiency_ws = self._detector_efficiency_correction(det_efficiency_input)
                progress.report("Normalising sample data")
                output_ws = self._normalise_sample_data(input_ws, det_efficiency_ws, nMeasurements, nComponents)
                to_clean.append(det_efficiency_ws)
            else:
                CloneWorkspace(InputWorkspace=input_ws, OutputWorkspace=output_ws)
        else:
            progress.report("Separating cross-sections")
            component_ws = self._cross_section_separation(input_ws, nMeasurements)
            if normalisation_method != "None":
                if normalisation_method == "Vanadium":
                    det_efficiency_input = self.getPropertyValue("VanadiumInputWorkspace")
                else:
                    det_efficiency_input = component_ws
                progress.report("Calculating detector efficiency correction")
                det_efficiency_ws = self._detector_efficiency_correction(det_efficiency_input)
                progress.report("Normalising sample data")
                output_ws = self._normalise_sample_data(component_ws, det_efficiency_ws, nMeasurements, nComponents)
                to_clean += [component_ws, det_efficiency_ws]
            else:
                RenameWorkspace(InputWorkspace=component_ws, OutputWorkspace=output_ws)
        progress.report("Setting units")
        output_ws = self._set_units(output_ws, nMeasurements)
        self._set_output_names(output_ws)
        self.setProperty("OutputWorkspace", mtd[output_ws])
        if self.getProperty("ClearCache").value and len(to_clean) != 0:
            DeleteWorkspaces(WorkspaceList=to_clean)


AlgorithmFactory.subscribe(D7AbsoluteCrossSections)

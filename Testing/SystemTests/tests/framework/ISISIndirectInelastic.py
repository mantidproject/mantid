# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,attribute-defined-outside-init,too-many-lines
# pylint: disable=too-many-instance-attributes,non-parent-init-called,abstract-method,too-few-public-methods
# non-parent-init-called is disabled to remove false positives from a bug in pyLint < 1.4
# abstract-method checking seems to ignore the fact some classes are declared abstract using abc

"""
- TOSCA only supported by "Reduction" (the Energy Transfer tab of C2E).
- OSIRIS/IRIS supported by all tabs / interfaces.
- VESUVIO is not supported by any interface as of yet.

For diagrams on the intended work flow of the IDR and IDA interfaces see:

- Indirect_DataReduction.rst
- Indirect_DataAnalysis.rst

System test class hierarchy as shown below:

systemtesting.MantidSystemTest
 |
 +--ISISIndirectInelasticBase
     |
     +--ISISIndirectInelasticReduction
     |   |
     |   +--TOSCAReduction
     |   +--IRISReduction
     |   +--OSIRISReduction
     |
     +--ISISIndirectInelasticCalibratrion
     |   |
     |   +--IRISCalibratrion
     |   +--OSIRISCalibratrion
     |
     +--ISISIndirectInelasticResolution
     |   |
     |   +--IRISResolution
     |   +--OSIRISResolution
     |
     +--ISISIndirectInelasticDiagnostics
     |   |
     |   +--IRISDiagnostics
     |   +--OSIRISDiagnostics
     |
     +--ISISIndirectInelasticMoments
     |   |
     |   +--IRISMoments
     |   +--OSIRISMoments
     |
     +--ISISIndirectInelasticElwinAndMSDFit
     |   |
     |   +--IRISElwinAndMSDFit
     |   +--OSIRISElwinAndMSDFit
     |
     +--ISISIndirectInelasticIqtAndIqtFit
     |   |
     |   +--IRISIqtAndIqtFit
     |   +--OSIRISIqtAndIqtFit
     |
     +--ISISIndirectInelasticIqtAndIqtFitMulti
     |   |
     |   +--IRISIqtAndIqtFitMulti
     |   +--OSIRISIqtAndIqtFitMulti
     |
     +--ISISIndirectInelasticConvFit
     |   |
     |   +--IRISConvFit
     |   +--OSIRISConvFit
     |
"""

from abc import ABCMeta, abstractmethod

from mantid.simpleapi import *

# For debugging only.
from mantid.api import FileFinder
from systemtesting import MantidSystemTest, using_gsl_v1


class ISISIndirectInelasticBase(MantidSystemTest, metaclass=ABCMeta):
    """
    A common base class for the ISISIndirectInelastic* base classes.
    """

    @abstractmethod
    def get_reference_files(self):
        """Returns the name of the reference files to compare against."""
        raise NotImplementedError("Implmenent get_reference_files to return " "the names of the files to compare against.")

    @abstractmethod
    def _run(self):
        raise NotImplementedError("Implement _run.")

    def validate_results_and_references(self):
        num_ref_files = len(self.get_reference_files())
        num_results = len(self.result_names)

        if not isinstance(self.get_reference_files(), list):
            raise RuntimeError("The reference file(s) should be in a list")
        if not isinstance(self.result_names, list):
            raise RuntimeError("The result workspace(s) should be in a list")
        if num_ref_files != num_results:
            raise RuntimeError(
                "The number of result workspaces (%d) does not match" " the number of reference files (%d)." % (num_ref_files, num_results)
            )
        if num_ref_files < 1 or num_results < 1:
            raise RuntimeError("There needs to be a least one result and " "reference.")

    @abstractmethod
    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""
        raise NotImplementedError("Implmenent _validate_properties.")

    def runTest(self):
        self._validate_properties()
        self._run()
        self.validate_results_and_references()

    def validate(self):
        """Performs the validation for the generalised case of multiple results
        and multiple reference files.
        """

        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Axes")

        for reference_file, result in zip(self.get_reference_files(), self.result_names):
            wsName = "RefFile"
            if reference_file.endswith(".nxs"):
                LoadNexus(Filename=reference_file, OutputWorkspace=wsName)
            else:
                raise RuntimeError("Should supply a NeXus file: %s" % reference_file)

            if not self.validateWorkspaces([result, wsName]):
                print(str([reference_file, result]) + " do not match.")
                return False

        return True

    def get_temp_dir_path(self, filename):
        """Given a filename, prepends the system test temporary directory
        and returns the full path."""
        return os.path.join(config["defaultsave.directory"], filename)


# ==============================================================================


class ISISIndirectInelasticReduction(ISISIndirectInelasticBase):
    """A base class for the ISIS indirect inelastic reduction tests

    The workflow is defined in the _run() method, simply
    define an __init__ method and set the following properties
    on the object
        - instr_name: A string giving the instrument name for the test
        - detector_range: A list containing the lower and upper bounds of the
                          range of detectors to use
        - data_file: A string giving the data file to use
        - rebin_string: A comma separated string giving the rebin params
        - save_formats: A list containing the file extensions of the formats
                        to save to.
    """

    sum_files = False

    def _run(self):
        """Defines the workflow for the test"""
        self.tolerance = 1e-7

        kwargs = {}

        if self.rebin_string is not None:
            kwargs["RebinString"] = self.rebin_string

        reductions = ISISIndirectEnergyTransfer(
            Instrument=self.instr_name,
            Analyser="graphite",
            Reflection="002",
            InputFiles=self.data_files,
            SumFiles=self.sum_files,
            SpectraRange=self.detector_range,
            **kwargs
        )

        self.result_names = sorted(reductions.getNames())

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""
        if not isinstance(self.instr_name, str):
            raise RuntimeError("instr_name property should be a string")
        if not isinstance(self.detector_range, list) and len(self.detector_range) != 2:
            raise RuntimeError("detector_range should be a list of exactly 2 " "values")
        if not isinstance(self.data_files, list):
            raise RuntimeError("data_file property should be a string")
        if self.rebin_string is not None and not isinstance(self.rebin_string, str):
            raise RuntimeError("rebin_string property should be a string")
        if self.sum_files is not None and not isinstance(self.sum_files, bool):
            raise RuntimeError("sum_files property should be a bool")


# ------------------------- TOSCA tests ----------------------------------------


class TOSCAReduction(ISISIndirectInelasticReduction):
    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "TOSCA"
        self.detector_range = [1, 140]
        self.data_files = ["TSC15352.raw"]
        self.rebin_string = "-2.5,0.015,3,-0.005,1000"

    def get_reference_files(self):
        return ["II.TOSCAReductionFromFile.nxs"]


class TOSCASimpleReductionUsingOldRunNumber(ISISIndirectInelasticReduction):
    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "TOSCA"
        self.detector_range = [1, 140]
        self.data_files = ["TSC04970.raw"]
        self.rebin_string = "-2.5,0.015,3,-0.005,1000"

    def get_reference_files(self):
        return ["II.TOSCAReductionOfOldRunNumber.nxs"]


class TOSCAMultiFileReduction(ISISIndirectInelasticReduction):
    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "TOSCA"
        self.detector_range = [1, 140]
        self.data_files = ["TSC15352.raw", "TSC15353.raw", "TSC15354.raw"]
        self.rebin_string = "-2.5,0.015,3,-0.005,1000"

    def get_reference_files(self):
        # note that the same run for single reduction is used.
        # as they should be the same
        return ["II.TOSCAReductionFromFile.nxs", "II.TOSCAMultiFileReduction1.nxs", "II.TOSCAMultiFileReduction2.nxs"]


class TOSCAMultiFileSummedReduction(ISISIndirectInelasticReduction):
    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "TOSCA"
        self.detector_range = [1, 140]
        self.data_files = ["TSC15352.raw", "TSC15353.raw", "TSC15354.raw"]
        self.rebin_string = "-2.5,0.015,3,-0.005,1000"
        self.sum_files = True

    def get_reference_files(self):
        return ["II.TOSCAMultiFileSummedReduction.nxs"]


class TOSCAMultiFileSummedReductionWithDifferentMaskedDetectors(ISISIndirectInelasticReduction):
    """
    This test was created in response to the bug in issue #25072.
    """

    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "TOSCA"
        self.detector_range = [1, 140]
        self.data_files = ["TSC22841.raw", "TSC22842.raw", "TSC22843.raw"]
        self.rebin_string = "-2.5,0.015,3,-0.005,1000"
        self.sum_files = True

    def get_reference_files(self):
        return ["II.TOSCAMultiFileSummedReduction2.nxs"]


class TOSCAMultiFileReductionWithDifferentMaskedDetectors(ISISIndirectInelasticReduction):
    """
    This test was created in response to the bug in issue #25061.
    """

    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "TOSCA"
        self.detector_range = [1, 140]
        self.data_files = ["TSC22797.raw", "TSC22798.raw", "TSC22799.raw"]
        self.rebin_string = "-2.5,0.015,3,-0.005,1000"
        self.sum_files = False

    def get_reference_files(self):
        return ["II.TOSCAMultiFileReduction22797.nxs", "II.TOSCAMultiFileReduction22798.nxs", "II.TOSCAMultiFileReduction22799.nxs"]


# ------------------------- OSIRIS tests ---------------------------------------


class OSIRISReduction(ISISIndirectInelasticReduction):
    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "OSIRIS"
        self.detector_range = [963, 1004]
        self.data_files = ["OSIRIS00106550.raw"]
        self.rebin_string = None

    def get_reference_files(self):
        return ["II.OSIRISReductionFromFile.nxs"]


class OSIRISMultiFileReduction(ISISIndirectInelasticReduction):
    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "OSIRIS"
        self.detector_range = [963, 1004]
        self.data_files = ["OSIRIS00106550.raw", "OSIRIS00106551.raw"]
        self.rebin_string = None

    def get_reference_files(self):
        # note that the same run for single reduction is used.
        # as they should be the same
        return ["II.OSIRISReductionFromFile.nxs", "II.OSIRISMultiFileReduction1.nxs"]


class OSIRISMultiFileSummedReduction(ISISIndirectInelasticReduction):
    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "OSIRIS"
        self.detector_range = [963, 1004]
        self.data_files = ["OSIRIS00106550.raw", "OSIRIS00106551.raw"]
        self.rebin_string = None
        self.sum_files = True

    def get_reference_files(self):
        return ["II.OSIRISMultiFileSummedReduction.nxs"]


# ------------------------- IRIS tests -----------------------------------------


class IRISReduction(ISISIndirectInelasticReduction):
    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "IRIS"
        self.detector_range = [3, 53]
        self.data_files = ["IRS21360.raw"]
        self.rebin_string = None

    def get_reference_files(self):
        return ["II.IRISReductionFromFile.nxs"]


class IRISMultiFileReduction(ISISIndirectInelasticReduction):
    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "IRIS"
        self.detector_range = [3, 53]
        self.data_files = ["IRS21360.raw", "IRS53664.raw"]
        self.rebin_string = None

    def get_reference_files(self):
        return ["II.IRISReductionFromFile.nxs", "II.IRISMultiFileReduction1.nxs"]


class IRISMultiFileSummedReduction(ISISIndirectInelasticReduction):
    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = "IRIS"
        self.detector_range = [3, 53]
        self.data_files = ["IRS21360.raw", "IRS53664.raw"]
        self.sum_files = True
        self.rebin_string = None

    def get_reference_files(self):
        # note that the same run for single reduction is used.
        # as they should be the same
        return ["II.IRISMultiFileSummedReduction.nxs"]


# ==============================================================================


class ISISIndirectInelasticCalibration(ISISIndirectInelasticBase):
    """A base class for the ISIS indirect inelastic calibration tests

    The workflow is defined in the _run() method, simply
    define an __init__ method and set the following properties
    on the object
        - self.data_file: a string giving the name of the data file
        - self.detector_range: a list of two ints, giving the lower and
                               upper bounds of the detector range
        - self.parameters: a list containing four doubles, each a parameter.
        - self.analyser: a string giving the name of the analyser to use
        - self.reflection: a string giving the reflection to use
    """

    def _run(self):
        """Defines the workflow for the test"""
        self.tolerance = 1e-7

        self.result_names = ["IndirectCalibration_Output"]

        IndirectCalibration(
            InputFiles=self.data_file,
            OutputWorkspace="IndirectCalibration_Output",
            DetectorRange=self.detector_range,
            PeakRange=self.peak,
            BackgroundRange=self.back,
        )

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if not isinstance(self.data_file, str):
            raise RuntimeError("data_file property should be a string")
        if not isinstance(self.detector_range, list) and len(self.detector_range) != 2:
            raise RuntimeError("detector_range should be a list of exactly 2 values")
        if not isinstance(self.peak, list) and len(self.peak) != 2:
            raise RuntimeError("peak should be a list of exactly 2 values")
        if not isinstance(self.back, list) and len(self.back) != 2:
            raise RuntimeError("back should be a list of exactly 2 values")


# ------------------------- OSIRIS tests ---------------------------------------


class OSIRISCalibration(ISISIndirectInelasticCalibration):
    def __init__(self):
        ISISIndirectInelasticCalibration.__init__(self)
        self.data_file = "OSI97935.raw"
        self.detector_range = [963, 1004]
        self.back = [68000.00, 70000.00]
        self.peak = [59000.00, 61000.00]

    def get_reference_files(self):
        return ["II.OSIRISCalibration.nxs"]


# ------------------------- IRIS tests ---------------------------------------


class IRISCalibration(ISISIndirectInelasticCalibration):
    def __init__(self):
        ISISIndirectInelasticCalibration.__init__(self)
        self.data_file = "IRS53664.raw"
        self.detector_range = [3, 53]
        self.back = [59000.00, 61500.00]
        self.peak = [62500.00, 65000.00]

    def get_reference_files(self):
        return ["II.IRISCalibration.nxs"]


# ==============================================================================


class ISISIndirectInelasticResolution(ISISIndirectInelasticBase):
    """A base class for the ISIS indirect inelastic resolution tests

    The workflow is defined in the _run() method, simply
    define an __init__ method and set the following properties
    on the object
        - self.instrument: a string giving the instrument name
        - self.analyser: a string giving the name of the analyser
        - self.reflection: a string giving the name of the reflection
        - self.detector_range: a list of two integers, giving the range of detectors
        - self.background: a list of two doubles, giving the background params
        - self.rebin_params: a comma separated string containing the rebin params
        - self.files: a list of strings containing filenames
    """

    def _run(self):
        """Defines the workflow for the test"""
        self.tolerance = 1e-7

        IndirectResolution(
            InputFiles=self.files,
            OutputWorkspace="__IndirectResolution_Test",
            Instrument=self.instrument,
            Analyser=self.analyser,
            Reflection=self.reflection,
            DetectorRange=self.detector_range,
            BackgroundRange=self.background,
            RebinParam=self.rebin_params,
        )

        self.result_names = ["__IndirectResolution_Test"]

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if not isinstance(self.instrument, str):
            raise RuntimeError("instrument property should be a string")
        if not isinstance(self.analyser, str):
            raise RuntimeError("analyser property should be a string")
        if not isinstance(self.reflection, str):
            raise RuntimeError("reflection property should be a string")
        if not isinstance(self.detector_range, list) and len(self.detector_range) != 2:
            raise RuntimeError("detector_range should be a list of exactly 2 values")
        if not isinstance(self.background, list) and len(self.background) != 2:
            raise RuntimeError("background should be a list of exactly 2 values")
        if not isinstance(self.rebin_params, str):
            raise RuntimeError("rebin_params property should be a string")
        # Have this as just one file for now.
        if not isinstance(self.files, list) and len(self.files) != 1:
            raise RuntimeError("files should be a list of exactly 1 value")


# ------------------------- OSIRIS tests ---------------------------------------


class OSIRISResolution(ISISIndirectInelasticResolution):
    def __init__(self):
        ISISIndirectInelasticResolution.__init__(self)
        self.instrument = "OSIRIS"
        self.analyser = "graphite"
        self.reflection = "002"
        self.detector_range = [963, 1004]
        self.background = [-0.563032, 0.605636]
        self.rebin_params = "-0.2,0.002,0.2"
        self.files = ["OSI97935.raw"]

    def get_reference_files(self):
        return ["II.OSIRISResolution.nxs"]


# ------------------------- IRIS tests -----------------------------------------


class IRISResolution(ISISIndirectInelasticResolution):
    def __init__(self):
        ISISIndirectInelasticResolution.__init__(self)
        self.instrument = "IRIS"
        self.analyser = "graphite"
        self.reflection = "002"
        self.detector_range = [3, 53]
        self.background = [-0.54, 0.54]
        self.rebin_params = "-0.2,0.002,0.2"
        self.files = ["IRS53664.raw"]

    def get_reference_files(self):
        return ["II.IRISResolution.nxs"]


# ==============================================================================


class ISISIndirectInelasticDiagnostics(ISISIndirectInelasticBase):
    """A base class for the ISIS indirect inelastic diagnostic tests

    The workflow is defined in the _run() method, simply
    define an __init__ method and set the following properties
    on the object
    """

    def _run(self):
        """Defines the workflow for the test"""

        self.tolerance = 1e-7

        TimeSlice(
            InputFiles=self.rawfiles,
            OutputNameSuffix=self.suffix,
            OutputWorkspace="__IndirectInelasticDiagnostics_out_group",
            PeakRange=self.peak,
            SpectraRange=self.spectra,
        )

        # Construct the result ws name.
        Load(Filename=self.rawfiles[0], OutputWorkspace="__temp")
        resultWs = mtd["__temp"]
        inst_name = resultWs.getInstrument().getFullName().lower()
        run_number = resultWs.run().getProperty("run_number").value
        self.result_names = [inst_name + run_number + self.suffix]

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if not isinstance(self.rawfiles, list) and len(self.rawfiles) != 1:
            raise RuntimeError("rawfiles should be a list of exactly 1 value")
        if not isinstance(self.peak, list) and len(self.peak) != 2:
            raise RuntimeError("peak should be a list of exactly 2 values")
        if not isinstance(self.spectra, list) and len(self.spectra) != 2:
            raise RuntimeError("spectra should be a list of exactly 2 values")
        if not isinstance(self.suffix, str):
            raise RuntimeError("suffix property should be a string")


# ------------------------- IRIS tests -----------------------------------------


class IRISDiagnostics(ISISIndirectInelasticDiagnostics):
    def __init__(self):
        ISISIndirectInelasticDiagnostics.__init__(self)

        self.peak = [62500, 65000]
        self.rawfiles = ["IRS53664.raw"]
        self.spectra = [3, 53]
        self.suffix = "_graphite002_slice"

    def get_reference_files(self):
        return ["II.IRISDiagnostics.nxs"]


# ------------------------- OSIRIS tests ---------------------------------------


class OSIRISDiagnostics(ISISIndirectInelasticDiagnostics):
    def __init__(self):
        ISISIndirectInelasticDiagnostics.__init__(self)

        self.peak = [59000, 61000]
        self.rawfiles = ["OSI97935.raw"]
        self.spectra = [963, 1004]
        self.suffix = "_graphite002_slice"

    def get_reference_files(self):
        return ["II.OSIRISDiagnostics.nxs"]


# ==============================================================================


class ISISIndirectInelasticMoments(ISISIndirectInelasticBase):
    """A base class for the ISIS indirect inelastic TransformToIqt/TransformToIqtFit tests

    The output of Elwin is usually used with MSDFit and so we plug one into
    the other in this test.
    """

    def _run(self):
        """Defines the workflow for the test"""

        LoadNexus(self.input_workspace, OutputWorkspace=self.input_workspace)

        SofQWMoments(
            InputWorkspace=self.input_workspace,
            EnergyMin=self.e_min,
            EnergyMax=self.e_max,
            Scale=self.scale,
            OutputWorkspace=self.input_workspace + "_Moments",
        )

        self.result_names = [self.input_workspace + "_Moments"]

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if not isinstance(self.input_workspace, str):
            raise RuntimeError("Input workspace should be a string.")
        if not isinstance(self.e_min, float):
            raise RuntimeError("Energy min should be a float")
        if not isinstance(self.e_max, float):
            raise RuntimeError("Energy max should be a float")
        if not isinstance(self.scale, float):
            raise RuntimeError("Scale should be a float")


# ------------------------- OSIRIS tests ---------------------------------------


class OSIRISMoments(ISISIndirectInelasticMoments):
    def __init__(self):
        ISISIndirectInelasticMoments.__init__(self)
        self.input_workspace = "osi97935_graphite002_sqw.nxs"
        self.e_min = -0.4
        self.e_max = 0.4
        self.scale = 1.0

    def get_reference_files(self):
        return ["II.OSIRISMoments.nxs"]


# ------------------------- IRIS tests -----------------------------------------


class IRISMoments(ISISIndirectInelasticMoments):
    def __init__(self):
        ISISIndirectInelasticMoments.__init__(self)
        self.input_workspace = "irs53664_graphite002_sqw.nxs"
        self.e_min = -0.4
        self.e_max = 0.4
        self.scale = 1.0

    def get_reference_files(self):
        return ["II.IRISMoments.nxs"]


# ==============================================================================


class ISISIndirectInelasticElwinAndMSDFit(ISISIndirectInelasticBase):
    """A base class for the ISIS indirect inelastic Elwin/MSD Fit tests

    The output of Elwin is usually used with MSDFit and so we plug one into
    the other in this test.
    """

    def __init__(self):
        super(ISISIndirectInelasticElwinAndMSDFit, self).__init__()
        self.tolerance = 1e-7

    def _run(self):
        """Defines the workflow for the test"""
        elwin_input = "__ElWinMult_in"
        elwin_results = ["__ElWinMult_q", "__ElWinMult_q2", "__ElWinMult_elf"]

        # Load files and create workspace group
        for filename in self.files:
            Load(Filename=filename, OutputWorkspace=filename)
        GroupWorkspaces(InputWorkspaces=self.files, OutputWorkspace=elwin_input)

        ElasticWindowMultiple(
            InputWorkspaces=elwin_input,
            IntegrationRangeStart=self.eRange[0],
            IntegrationRangeEnd=self.eRange[1],
            OutputInQ=elwin_results[0],
            OutputInQSquared=elwin_results[1],
            OutputELF=elwin_results[2],
        )

        int_files = [self.get_temp_dir_path(filename) + ".nxs" for filename in elwin_results]

        # Save the EQ1 & EQ2 results from Elwin to put into MSDFit.
        for ws, filename in zip(elwin_results, int_files):
            SaveNexusProcessed(Filename=filename, InputWorkspace=ws)

        eq_file = elwin_results[0]
        msdfit_result = MSDFit(InputWorkspace=eq_file, XStart=self.startX, XEnd=self.endX, SpecMax=1)

        # Clean up the intermediate files.
        for filename in int_files:
            os.remove(filename)

        # We're interested in the intermediate Elwin results as well as the
        # final MSDFit result.
        self.result_names = [elwin_results[0], elwin_results[1], msdfit_result[2].name()]  # EQ1  # EQ2  # Fit workspace

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if not isinstance(self.files, list) or len(self.files) != 2:
            raise RuntimeError("files should be a list of exactly 2 strings")
        if not isinstance(self.eRange, list) or len(self.eRange) != 2:
            raise RuntimeError("eRange should be a list of exactly 2 values")
        if not isinstance(self.startX, float):
            raise RuntimeError("startX should be a float")
        if not isinstance(self.endX, float):
            raise RuntimeError("endX should be a float")


# ------------------------- OSIRIS tests ---------------------------------------


class OSIRISElwinAndMSDFit(ISISIndirectInelasticElwinAndMSDFit):
    def __init__(self):
        ISISIndirectInelasticElwinAndMSDFit.__init__(self)
        self.tolerance = 5e-6
        self.files = ["osi97935_graphite002_red.nxs", "osi97936_graphite002_red.nxs"]
        self.eRange = [-0.02, 0.02]
        self.startX = 0.189077
        self.endX = 1.8141

    def get_reference_files(self):
        return ["II.OSIRISElwinEQ1.nxs", "II.OSIRISElwinEQ2.nxs", "II.OSIRISMSDFit.nxs"]


# ------------------------- IRIS tests -----------------------------------------


class IRISElwinAndMSDFit(ISISIndirectInelasticElwinAndMSDFit):
    def __init__(self):
        ISISIndirectInelasticElwinAndMSDFit.__init__(self)
        self.files = ["irs53664_graphite002_red.nxs", "irs53665_graphite002_red.nxs"]
        self.eRange = [-0.02, 0.02]
        self.startX = 0.441682
        self.endX = 1.85378

    def get_reference_files(self):
        return ["II.IRISElwinEQ1.nxs", "II.IRISElwinEQ2.nxs", "II.IRISMSDFit.nxs"]


# ==============================================================================


class ISISIndirectInelasticIqtAndIqtFit(ISISIndirectInelasticBase):
    """
    A base class for the ISIS indirect inelastic Iqt/IqtFit tests

    The output of TransformToIqt is usually used with IqtFit and so we plug one into
    the other in this test.
    """

    def _run(self):
        """Defines the workflow for the test"""
        self.tolerance = 1e-3
        self.samples = [sample[:-4] for sample in self.samples]

        # Load files into Mantid
        for sample in self.samples:
            LoadNexus(sample, OutputWorkspace=sample)
        LoadNexus(FileFinder.getFullPath(self.resolution), OutputWorkspace=self.resolution)

        _, iqt_ws = TransformToIqt(
            SampleWorkspace=self.samples[0],
            ResolutionWorkspace=self.resolution,
            EnergyMin=self.e_min,
            EnergyMax=self.e_max,
            BinReductionFactor=self.num_bins,
            DryRun=False,
            NumberOfIterations=200,
        )

        # Test IqtFit Sequential
        iqtfitSeq_ws, params, fit_group = IqtFitSequential(
            InputWorkspace=iqt_ws, Function=self.func, StartX=self.startx, EndX=self.endx, SpecMin=0, SpecMax=self.spec_max
        )

        self.result_names = [iqt_ws.name(), iqtfitSeq_ws[0].name()]

        # Remove workspaces from Mantid
        for sample in self.samples:
            DeleteWorkspace(sample)
        DeleteWorkspace(params)
        DeleteWorkspace(fit_group)
        DeleteWorkspace(self.resolution)

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if not isinstance(self.samples, list):
            raise RuntimeError("Samples should be a list of strings.")
        if not isinstance(self.resolution, str):
            raise RuntimeError("Resolution should be a string.")
        if not isinstance(self.e_min, float):
            raise RuntimeError("e_min should be a float")
        if not isinstance(self.e_max, float):
            raise RuntimeError("e_max should be a float")
        if not isinstance(self.num_bins, int):
            raise RuntimeError("num_bins should be an int")
        if not isinstance(self.func, str):
            raise RuntimeError("Function should be a string.")
        if not isinstance(self.startx, float):
            raise RuntimeError("startx should be a float")
        if not isinstance(self.endx, float):
            raise RuntimeError("endx should be a float")


# ------------------------- OSIRIS tests ---------------------------------------


class OSIRISIqtAndIqtFit(ISISIndirectInelasticIqtAndIqtFit):
    def __init__(self):
        ISISIndirectInelasticIqtAndIqtFit.__init__(self)

        # TransformToIqt
        self.samples = ["osi97935_graphite002_red.nxs"]
        self.resolution = "osi97935_graphite002_res.nxs"
        self.e_min = -0.4
        self.e_max = 0.4
        self.num_bins = 4

        # Iqt Seq Fit
        self.func = (
            r"composite=CompositeFunction,NumDeriv=1;name=LinearBackground,A0=0,A1=0,ties=(A1=0);"
            "name=UserFunction,Formula=Intensity*exp(-(x/Tau)),"
            "Intensity=0.304185,Tau=100;ties=(f1.Intensity=1-f0.A0)"
        )
        self.spec_max = 41
        self.startx = 0.0
        self.endx = 0.118877

    def get_reference_files(self):
        # Relative tolerance is used because the calculation of Monte Carlo errors means the Iqt errors are randomized
        # within a set amount. Also, gsl v2 gives a slightly different result than v1 for II.OSIRISFuryFitSeq.
        self.tolerance = 5.0
        self.tolerance_is_rel_err = True
        return ["II.OSIRISFury.nxs", "II.OSIRISFuryFitSeq.nxs"]


# ------------------------- IRIS tests -----------------------------------------


class IRISIqtAndIqtFit(ISISIndirectInelasticIqtAndIqtFit):
    def __init__(self):
        ISISIndirectInelasticIqtAndIqtFit.__init__(self)

        # TransformToIqt
        self.samples = ["irs53664_graphite002_red.nxs"]
        self.resolution = "irs53664_graphite002_res.nxs"
        self.e_min = -0.4
        self.e_max = 0.4
        self.num_bins = 4

        # Iqt Seq Fit
        self.func = (
            r"composite=CompositeFunction,NumDeriv=1;name=LinearBackground,A0=0,A1=0,ties=(A1=0);"
            "name=UserFunction,Formula=Intensity*exp(-(x/Tau)),"
            "Intensity=0.355286,Tau=100;ties=(f1.Intensity=1-f0.A0)"
        )
        self.spec_max = 50
        self.startx = 0.0
        self.endx = 0.169171

    def get_reference_files(self):
        # Relative tolerance is used because the calculation of Monte Carlo errors means the Iqt errors are randomized
        # within a set amount. Also, gsl v2 gives a slightly different result than v1 for II.IRISFuryFitSeq.
        self.tolerance = 5.0
        self.tolerance_is_rel_err = True
        return ["II.IRISFury.nxs", "II.IRISFuryFitSeq.nxs"]


# ==============================================================================


class ISISIndirectInelasticIqtAndIqtFitMulti(ISISIndirectInelasticBase):
    """A base class for the ISIS indirect inelastic Iqt/IqtFit tests

    The output of Elwin is usually used with MSDFit and so we plug one into
    the other in this test.
    """

    def _run(self):
        """Defines the workflow for the test"""
        self.tolerance = 1e-2
        self.samples = [sample[:-4] for sample in self.samples]

        # load files into mantid
        for sample in self.samples:
            LoadNexus(sample, OutputWorkspace=sample)
        LoadNexus(FileFinder.getFullPath(self.resolution), OutputWorkspace=self.resolution)

        _, iqt_ws = TransformToIqt(
            SampleWorkspace=self.samples[0],
            ResolutionWorkspace=self.resolution,
            EnergyMin=self.e_min,
            EnergyMax=self.e_max,
            BinReductionFactor=self.num_bins,
            DryRun=False,
            NumberOfIterations=200,
        )

        # Test IqtFitMultiple
        iqtfitSeq_ws, params, fit_group = IqtFitMultiple(
            iqt_ws.name(), self.func, self.ftype, self.startx, self.endx, self.spec_min, self.spec_max
        )

        self.result_names = [iqt_ws.name(), iqtfitSeq_ws.name()]

        # remove workspaces from mantid
        for sample in self.samples:
            DeleteWorkspace(sample)
        DeleteWorkspace(params)
        DeleteWorkspace(fit_group)
        DeleteWorkspace(self.resolution)

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if not isinstance(self.samples, list):
            raise RuntimeError("Samples should be a list of strings.")
        if not isinstance(self.resolution, str):
            raise RuntimeError("Resolution should be a string.")
        if not isinstance(self.e_min, float):
            raise RuntimeError("e_min should be a float")
        if not isinstance(self.e_max, float):
            raise RuntimeError("e_max should be a float")
        if not isinstance(self.num_bins, int):
            raise RuntimeError("num_bins should be an int")
        if not isinstance(self.func, str):
            raise RuntimeError("Function should be a string.")
        if not isinstance(self.ftype, str):
            raise RuntimeError("Function type should be a string.")
        if not isinstance(self.startx, float):
            raise RuntimeError("startx should be a float")
        if not isinstance(self.endx, float):
            raise RuntimeError("endx should be a float")


# ------------------------- OSIRIS tests ---------------------------------------


class OSIRISIqtAndIqtFitMulti(ISISIndirectInelasticIqtAndIqtFitMulti):
    def __init__(self):
        ISISIndirectInelasticIqtAndIqtFitMulti.__init__(self)

        # TransformToIqt
        self.samples = ["osiris97944_graphite002_red.nxs"]
        self.resolution = "osi97935_graphite002_res.nxs"
        self.e_min = -0.4
        self.e_max = 0.4
        self.num_bins = 4

        # Iqt Fit
        self.func = (
            r"name=LinearBackground,A0=0.213439,A1=0,ties=(A1=0);name=StretchExp,"
            "Height=0.786561,Lifetime=0.0247894,"
            "Stretching=1;ties=(f1.Height=1-f0.A0)"
        )
        self.ftype = "1E_s"
        self.startx = 0.0
        self.endx = 0.12
        self.spec_min = 0
        self.spec_max = 41

    def skipTests(self):
        # In its current state this test is not fit for purpose. There is a large amount of randomness present such
        # that a very very wide relative tolerance is required to pass reliably (50+). This was discovered after
        # a change to CompareWorkspaces that made the comparison actually relative. Prior, this test was actually
        # using an absolute tolerance of 5.0. A redesign is needed to remove some of the randomness or determine
        # the actual required tolerance.
        return True

    def get_reference_files(self):
        # Relative tolerance is used because the calculation of Monte Carlo errors means the Iqt errors are randomized
        # within a set amount
        self.tolerance = 5.0
        self.tolerance_is_rel_err = True
        return ["II.OSIRISIqt.nxs", "II.OSIRISIqtFitMulti.nxs"]


# ------------------------- IRIS tests -----------------------------------------


class IRISIqtAndIqtFitMulti(ISISIndirectInelasticIqtAndIqtFitMulti):
    def __init__(self):
        ISISIndirectInelasticIqtAndIqtFitMulti.__init__(self)

        # TransformToIqt
        self.samples = ["irs53664_graphite002_red.nxs"]
        self.resolution = "irs53664_graphite002_res.nxs"
        self.e_min = -0.4
        self.e_max = 0.4
        self.num_bins = 4
        self.spec_min = 0
        self.spec_max = 50

        # Iqt Fit
        self.func = (
            r"name=LinearBackground,A0=0.584488,A1=0,ties=(A1=0);name=StretchExp,"
            "Height=0.415512,Stretching=0.022653;ties=(f1.Height=1-f0.A0,f1.Lifetime=0.05)"
        )
        self.ftype = "1S_s"
        self.startx = 0.0
        self.endx = 0.156250

    def get_reference_files(self):
        # Relative tolerance is used because the calculation of Monte Carlo errors means the Iqt errors are randomized
        # within a set amount
        self.tolerance = 5.0
        self.tolerance_is_rel_err = True
        return ["II.IRISFury.nxs", "II.IRISFuryFitMulti.nxs"]


# ==============================================================================


class ISISIndirectInelasticConvFit(ISISIndirectInelasticBase):
    """A base class for the ISIS indirect inelastic ConvFit tests

    The workflow is defined in the _run() method, simply
    define an __init__ method and set the following properties
    on the object
    """

    def _run(self):
        """Defines the workflow for the test"""
        self.tolerance = 1e-4
        LoadNexus(self.sample, OutputWorkspace=self.sample)
        LoadNexus(FileFinder.getFullPath(self.resolution), OutputWorkspace=self.resolution)

        convfitSeq_ws, params, fit_group = ConvolutionFitSequential(
            InputWorkspace=self.sample,
            Function=self.func,
            PassWSIndexToFunction=self.passWSIndexToFunction,
            StartX=self.startx,
            EndX=self.endx,
            SpecMin=self.spectra_min,
            SpecMax=self.spectra_max,
            PeakRadius=5,
        )

        self.result_names = [convfitSeq_ws[0].name()]

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if not isinstance(self.sample, str):
            raise RuntimeError("Sample should be a string.")
        if not isinstance(self.resolution, str):
            raise RuntimeError("Resolution should be a string.")
        if not isinstance(self.func, str):
            raise RuntimeError("Function should be a string.")
        if not isinstance(self.startx, float):
            raise RuntimeError("startx should be a float")
        if not isinstance(self.endx, float):
            raise RuntimeError("endx should be a float")
        if not isinstance(self.spectra_min, int):
            raise RuntimeError("Min spectrum should be a int")
        if not isinstance(self.spectra_max, int):
            raise RuntimeError("Max spectrum should be a int")
        if not isinstance(self.ties, bool):
            raise RuntimeError("ties should be a boolean.")


# ------------------------- OSIRIS tests ---------------------------------------


class OSIRISConvFit(ISISIndirectInelasticConvFit):
    def __init__(self):
        ISISIndirectInelasticConvFit.__init__(self)
        self.sample = "osi97935_graphite002_red.nxs"
        self.resolution = "osi97935_graphite002_res.nxs"
        # ConvFit fit function
        self.func = (
            "name=LinearBackground,A0=0,A1=0;(composite=Convolution,FixResolution=true,NumDeriv=true;"
            'name=Resolution,Workspace="%s";name=Lorentzian,Amplitude=2,FWHM=0.002,ties=(PeakCentre=0)'
            ",constraints=(FWHM>0.002))" % self.resolution
        )
        self.passWSIndexToFunction = False  # osi97935_graphite002_res is single histogram
        self.startx = -0.2
        self.endx = 0.2
        self.spectra_min = 0
        self.spectra_max = 41
        self.ties = False

        self.result_names = ["osi97935_graphite002_conv_1LFitL_s0_to_41_Result_1"]

    def get_reference_files(self):
        self.tolerance = 0.3
        # gsl v2 gives a slightly different result than v1
        return ["II.OSIRISConvFitSeq.nxs"] if using_gsl_v1() else ["II.OSIRISConvFitSeq_gslv2.nxs"]


# ------------------------- IRIS tests -----------------------------------------


class IRISConvFit(ISISIndirectInelasticConvFit):
    def __init__(self):
        ISISIndirectInelasticConvFit.__init__(self)
        self.sample = "irs53664_graphite002_red.nxs"
        self.resolution = "irs53664_graphite002_res.nxs"
        # ConvFit fit function
        self.func = (
            "name=LinearBackground,A0=0.060623,A1=0.001343;"
            "(composite=Convolution,FixResolution=true,NumDeriv=true;"
            'name=Resolution,Workspace="%s";name=Lorentzian,Amplitude=1.033150,FWHM=0.001576,'
            "ties=(PeakCentre=0.0),constraints=(FWHM>0.001))" % self.resolution
        )
        self.passWSIndexToFunction = False  # irs53664_graphite002_res is single histogram
        self.startx = -0.2
        self.endx = 0.2
        self.spectra_min = 0
        self.spectra_max = 50
        self.ties = False

        self.result_names = ["irs53664_graphite002_conv_1LFitL_s0_to_50_Result_1"]

    def get_reference_files(self):
        self.tolerance = 0.2
        # gsl v2 gives a slightly different result than v1
        return ["II.IRISConvFitSeq.nxs"] if using_gsl_v1() else ["II.IRISConvFitSeq_gslv2.nxs"]


# ==============================================================================
# Transmission Monitor Test


class ISISIndirectInelasticTransmissionMonitor(ISISIndirectInelasticBase):
    # Mark as an abstract class
    def _run(self):
        """Defines the workflow for the test"""

        self.tolerance = 1e-4
        Load(self.sample, OutputWorkspace=self.sample)
        Load(self.can, OutputWorkspace=self.can)

        IndirectTransmissionMonitor(SampleWorkspace=self.sample, CanWorkspace=self.can, OutputWorkspace="IRISTransmissionMonitorTest")

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if not isinstance(self.sample, str):
            raise RuntimeError("Sample should be a string.")
        if not isinstance(self.can, str):
            raise RuntimeError("Can should be a string.")


# ------------------------- IRIS tests -----------------------------------------


class IRISTransmissionMonitor(ISISIndirectInelasticTransmissionMonitor):
    def __init__(self):
        ISISIndirectInelasticTransmissionMonitor.__init__(self)
        self.sample = "IRS26176.RAW"
        self.can = "IRS26173.RAW"

        self.result_names = ["IRISTransmissionMonitorTest"]

    def get_reference_files(self):
        return ["II.IRISTransmissionMonitor.nxs"]

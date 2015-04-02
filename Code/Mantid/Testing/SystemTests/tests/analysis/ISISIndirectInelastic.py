#pylint: disable=no-init,invalid-name
import stresstesting
import os
import platform
from abc import ABCMeta, abstractmethod

from mantid.simpleapi import *

# For debugging only.
from mantid.api import FileFinder

# Import our workflows.
from IndirectDataAnalysis import furyfitSeq, furyfitMult, confitSeq, abscorFeeder

'''
- TOSCA only supported by "Reduction" (the Energy Transfer tab of C2E).
- OSIRIS/IRIS supported by all tabs / interfaces.
- VESUVIO is not supported by any interface as of yet.

For diagrams on the intended work flow of the IDA and Indirect parts of the
C2E interface, please see:

- http://www.mantidproject.org/IDA
- http://www.mantidproject.org/Indirect

System test class hierarchy as shown below:

stresstesting.MantidStressTest
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
     +--ISISIndirectInelasticFuryAndFuryFit
     |   |
     |   +--IRISFuryAndFuryFit
     |   +--OSIRISFuryAndFuryFit
     |
     +--ISISIndirectInelasticFuryAndFuryFitMulti
     |   |
     |   +--IRISFuryAndFuryFitMulti
     |   +--OSIRISFuryAndFuryFitMulti
     |
     +--ISISIndirectInelasticConvFit
     |   |
     |   +--IRISConvFit
     |   +--OSIRISConvFit
     |
'''


class ISISIndirectInelasticBase(stresstesting.MantidStressTest):
    '''A common base class for the ISISIndirectInelastic* base classes.
    '''

    __metaclass__ = ABCMeta  # Mark as an abstract class

    @abstractmethod
    def get_reference_files(self):
        '''Returns the name of the reference files to compare against.'''
        raise NotImplementedError("Implmenent get_reference_files to return "
                                  "the names of the files to compare against.")

    @abstractmethod
    def _run(self):
        raise NotImplementedError("Implement _run.")

    def validate_results_and_references(self):
        num_ref_files = len(self.get_reference_files())
        num_results = len(self.result_names)

        if type(self.get_reference_files()) != list:
            raise RuntimeError("The reference file(s) should be in a list")
        if type(self.result_names) != list:
            raise RuntimeError("The result workspace(s) should be in a list")
        if num_ref_files != num_results:
            raise RuntimeError("The number of result workspaces (%d) does not match"
                               " the number of reference files (%d)." % (
                               num_ref_files, num_results))
        if num_ref_files < 1 or num_results < 1:
            raise RuntimeError("There needs to be a least one result and "
                               "reference.")

    @abstractmethod
    def _validate_properties(self):
        '''Check the object properties are in an expected state to continue'''
        raise NotImplementedError("Implmenent _validate_properties.")

    def runTest(self):
        self._validate_properties()
        self._run()
        self.validate_results_and_references()

    def validate(self):
        '''Performs the validation for the generalised case of multiple results
        and multiple reference files.
        '''

        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Axes')

        for reference_file, result in zip(self.get_reference_files(),
                                          self.result_names):
            wsName = "RefFile"
            if reference_file.endswith('.nxs'):
                LoadNexus(Filename=reference_file, OutputWorkspace=wsName)
            else:
                raise RuntimeError("Should supply a NeXus file: %s" %
                                   reference_file)

            if not self.validateWorkspaces([result, wsName]):
                print str([reference_file, result]) + " do not match."
                return False

        return True

    def get_temp_dir_path(self, filename):
        '''Given a filename, prepends the system test temporary directory
        and returns the full path.'''
        return os.path.join(config['defaultsave.directory'], filename)


#==============================================================================
class ISISIndirectInelasticReduction(ISISIndirectInelasticBase):
    '''A base class for the ISIS indirect inelastic reduction tests

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
    '''

    __metaclass__ = ABCMeta  # Mark as an abstract class
    sum_files = False

    def _run(self):
        self.tolerance = 1e-7

        kwargs = {}

        if self.rebin_string is not None:
            kwargs['RebinString'] = self.rebin_string

        reductions = ISISIndirectEnergyTransfer(Instrument=self.instr_name,
                                                Analyser='graphite',
                                                Reflection='002',
                                                InputFiles=self.data_files,
                                                SumFiles=self.sum_files,
                                                SpectraRange=self.detector_range,
                                                **kwargs)

        self.result_names = sorted(reductions.getNames())

    def _validate_properties(self):
        '''Check the object properties are in an expected state to continue'''
        if type(self.instr_name) != str:
            raise RuntimeError("instr_name property should be a string")
        if type(self.detector_range) != list and len(self.detector_range) != 2:
            raise RuntimeError("detector_range should be a list of exactly 2 "
                               "values")
        if type(self.data_files) != list:
            raise RuntimeError("data_file property should be a string")
        if self.rebin_string is not None and type(self.rebin_string) != str:
            raise RuntimeError("rebin_string property should be a string")
        if self.sum_files is not None and type(self.sum_files) != bool:
            raise RuntimeError("sum_files property should be a bool")

#------------------------- TOSCA tests ----------------------------------------


class TOSCAReduction(ISISIndirectInelasticReduction):

    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = 'TOSCA'
        self.detector_range = [1, 140]
        self.data_files = ['TSC15352.raw']
        self.rebin_string = '-2.5,0.015,3,-0.005,1000'

    def get_reference_files(self):
        return ["II.TOSCAReductionFromFile.nxs"]

class TOSCAMultiFileReduction(ISISIndirectInelasticReduction):

    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = 'TOSCA'
        self.detector_range = [1, 140]
        self.data_files = ['TSC15352.raw', 'TSC15353.raw', 'TSC15354.raw']
        self.rebin_string = '-2.5,0.015,3,-0.005,1000'

    def get_reference_files(self):
        #note that the same run for single reduction is used.
        #as they should be the same
        return ['II.TOSCAReductionFromFile.nxs', 'II.TOSCAMultiFileReduction1.nxs', 'II.TOSCAMultiFileReduction2.nxs']

class TOSCAMultiFileSummedReduction(ISISIndirectInelasticReduction):

    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = 'TOSCA'
        self.detector_range = [1, 140]
        self.data_files = ['TSC15352.raw', 'TSC15353.raw','TSC15354.raw']
        self.rebin_string = '-2.5,0.015,3,-0.005,1000'
        self.sum_files = True

    def get_reference_files(self):
        return ['II.TOSCAMultiFileSummedReduction.nxs']


#------------------------- OSIRIS tests ---------------------------------------


class OSIRISReduction(ISISIndirectInelasticReduction):

    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = 'OSIRIS'
        self.detector_range = [963, 1004]
        self.data_files = ['OSIRIS00106550.raw']
        self.rebin_string = None

    def get_reference_files(self):
        return ["II.OSIRISReductionFromFile.nxs"]

class OSIRISMultiFileReduction(ISISIndirectInelasticReduction):

    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = 'OSIRIS'
        self.detector_range = [963, 1004]
        self.data_files = ['OSIRIS00106550.raw',' OSIRIS00106551.raw']
        self.rebin_string = None

    def get_reference_files(self):
        #note that the same run for single reduction is used.
        #as they should be the same
        return ['II.OSIRISReductionFromFile.nxs','II.OSIRISMultiFileReduction1.nxs']

class OSIRISMultiFileSummedReduction(ISISIndirectInelasticReduction):

    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = 'OSIRIS'
        self.detector_range = [963, 1004]
        self.data_files = ['OSIRIS00106550.raw', 'OSIRIS00106551.raw']
        self.rebin_string = None
        self.sum_files = True

    def get_reference_files(self):
        return ['II.OSIRISMultiFileSummedReduction.nxs']

#------------------------- IRIS tests -----------------------------------------

class IRISReduction(ISISIndirectInelasticReduction):

    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = 'IRIS'
        self.detector_range = [3, 53]
        self.data_files = ['IRS21360.raw']
        self.rebin_string = None

    def get_reference_files(self):
        return ["II.IRISReductionFromFile.nxs"]


class IRISMultiFileReduction(ISISIndirectInelasticReduction):

    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = 'IRIS'
        self.detector_range = [3, 53]
        self.data_files = ['IRS21360.raw', 'IRS53664.raw']
        self.rebin_string = None

    def get_reference_files(self):
        return ['II.IRISReductionFromFile.nxs', 'II.IRISMultiFileReduction1.nxs']


class IRISMultiFileSummedReduction(ISISIndirectInelasticReduction):

    def __init__(self):
        ISISIndirectInelasticReduction.__init__(self)
        self.instr_name = 'IRIS'
        self.detector_range = [3, 53]
        self.data_files = ['IRS21360.raw', 'IRS53664.raw']
        self.sum_files = True
        self.rebin_string = None

    def get_reference_files(self):
        #note that the same run for single reduction is used.
        #as they should be the same
        return ['II.IRISMultiFileSummedReduction.nxs']


#==============================================================================
class ISISIndirectInelasticCalibration(ISISIndirectInelasticBase):
    '''A base class for the ISIS indirect inelastic calibration tests

    The workflow is defined in the _run() method, simply
    define an __init__ method and set the following properties
    on the object
        - self.data_file: a string giving the name of the data file
        - self.detector_range: a list of two ints, giving the lower and
                               upper bounds of the detector range
        - self.parameters: a list containing four doubles, each a parameter.
        - self.analyser: a string giving the name of the analyser to use
        - self.reflection: a string giving the reflection to use
    '''

    __metaclass__ = ABCMeta  # Mark as an abstract class

    def _run(self):
        '''Defines the workflow for the test'''
        self.tolerance = 1e-7

        self.result_names = ['IndirectCalibration_Output']

        IndirectCalibration(InputFiles=self.data_file,
                            OutputWorkspace='IndirectCalibration_Output',
                            DetectorRange=self.detector_range,
                            PeakRange=self.peak,
                            BackgroundRange=self.back)

    def _validate_properties(self):
        '''Check the object properties are in an expected state to continue'''

        if type(self.data_file) != str:
            raise RuntimeError("data_file property should be a string")
        if type(self.detector_range) != list and len(self.detector_range) != 2:
            raise RuntimeError("detector_range should be a list of exactly 2 values")
        if type(self.peak) != list and len(self.peak) != 2:
            raise RuntimeError("peak should be a list of exactly 2 values")
        if type(self.back) != list and len(self.back) != 2:
            raise RuntimeError("back should be a list of exactly 2 values")

#------------------------- OSIRIS tests ---------------------------------------


class OSIRISCalibration(ISISIndirectInelasticCalibration):

    def __init__(self):
        ISISIndirectInelasticCalibration.__init__(self)
        self.data_file = 'OSI97935.raw'
        self.detector_range = [963, 1004]
        self.back = [68000.00, 70000.00]
        self.peak = [59000.00, 61000.00]

    def get_reference_files(self):
        return ["II.OSIRISCalibration.nxs"]

#------------------------- IRIS tests ---------------------------------------


class IRISCalibration(ISISIndirectInelasticCalibration):

    def __init__(self):
        ISISIndirectInelasticCalibration.__init__(self)
        self.data_file = 'IRS53664.raw'
        self.detector_range = [3, 53]
        self.back = [59000.00, 61500.00]
        self.peak = [62500.00, 65000.00]

    def get_reference_files(self):
        return ["II.IRISCalibration.nxs"]


#==============================================================================
class ISISIndirectInelasticResolution(ISISIndirectInelasticBase):
    '''A base class for the ISIS indirect inelastic resolution tests

    The workflow is defined in the _run() method, simply
    define an __init__ method and set the following properties
    on the object
        - self.instrument: a string giving the intrument name
        - self.analyser: a string giving the name of the analyser
        - self.reflection: a string giving the name of the reflection
        - self.detector_range: a list of two integers, giving the range of detectors
        - self.background: a list of two doubles, giving the background params
        - self.rebin_params: a comma separated string containing the rebin params
        - self.files: a list of strings containing filenames
    '''

    __metaclass__ = ABCMeta  # Mark as an abstract class

    def _run(self):
        self.tolerance = 1e-7
        '''Defines the workflow for the test'''

        IndirectResolution(InputFiles=self.files,
                           OutputWorkspace='__IndirectResolution_Test',
                           Instrument=self.instrument,
                           Analyser=self.analyser,
                           Reflection=self.reflection,
                           DetectorRange=self.detector_range,
                           BackgroundRange=self.background,
                           RebinParam=self.rebin_params,
                           Plot=False)

        self.result_names = ['__IndirectResolution_Test']

    def _validate_properties(self):
        '''Check the object properties are in an expected state to continue'''

        if type(self.instrument) != str:
            raise RuntimeError("instrument property should be a string")
        if type(self.analyser) != str:
            raise RuntimeError("analyser property should be a string")
        if type(self.reflection) != str:
            raise RuntimeError("reflection property should be a string")
        if type(self.detector_range) != list and len(self.detector_range) != 2:
            raise RuntimeError("detector_range should be a list of exactly 2 values")
        if type(self.background) != list and len(self.background) != 2:
            raise RuntimeError("background should be a list of exactly 2 values")
        if type(self.rebin_params) != str:
            raise RuntimeError("rebin_params property should be a string")
        # Have this as just one file for now.
        if type(self.files) != list and len(self.files) != 1:
            raise RuntimeError("files should be a list of exactly 1 value")

#------------------------- OSIRIS tests ---------------------------------------


class OSIRISResolution(ISISIndirectInelasticResolution):

    def __init__(self):
        ISISIndirectInelasticResolution.__init__(self)
        self.instrument = 'OSIRIS'
        self.analyser = 'graphite'
        self.reflection = '002'
        self.detector_range = [963, 1004]
        self.background = [-0.563032, 0.605636]
        self.rebin_params = '-0.2,0.002,0.2'
        self.files = ['OSI97935.raw']

    def get_reference_files(self):
        return ["II.OSIRISResolution.nxs"]

#------------------------- IRIS tests -----------------------------------------


class IRISResolution(ISISIndirectInelasticResolution):

    def __init__(self):
        ISISIndirectInelasticResolution.__init__(self)
        self.instrument = 'IRIS'
        self.analyser = 'graphite'
        self.reflection = '002'
        self.detector_range = [3, 53]
        self.background = [-0.54, 0.65]
        self.rebin_params = '-0.2,0.002,0.2'
        self.files = ['IRS53664.raw']

    def get_reference_files(self):
        return ["II.IRISResolution.nxs"]


#==============================================================================
class ISISIndirectInelasticDiagnostics(ISISIndirectInelasticBase):
    '''A base class for the ISIS indirect inelastic diagnostic tests

    The workflow is defined in the _run() method, simply
    define an __init__ method and set the following properties
    on the object
    '''

    __metaclass__ = ABCMeta  # Mark as an abstract class

    def _run(self):
        '''Defines the workflow for the test'''

        self.tolerance = 1e-7

        TimeSlice(InputFiles=self.rawfiles,
                  OutputNameSuffix=self.suffix,
                  OutputWorkspace='__IndirectInelasticDiagnostics_out_group',
                  PeakRange=self.peak,
                  SpectraRange=self.spectra,
                  Plot=False,
                  Save=False)

        # Construct the result ws name.
        self.result_names = [os.path.splitext(self.rawfiles[0])[0] + self.suffix]

    def _validate_properties(self):
        '''Check the object properties are in an expected state to continue'''

        if type(self.rawfiles) != list and len(self.rawfiles) != 1:
            raise RuntimeError("rawfiles should be a list of exactly 1 value")
        if type(self.peak) != list and len(self.peak) != 2:
            raise RuntimeError("peak should be a list of exactly 2 values")
        if type(self.spectra) != list and len(self.spectra) != 2:
            raise RuntimeError("spectra should be a list of exactly 2 values")
        if type(self.suffix) != str:
            raise RuntimeError("suffix property should be a string")


#------------------------- IRIS tests -----------------------------------------


class IRISDiagnostics(ISISIndirectInelasticDiagnostics):

    def __init__(self):
        ISISIndirectInelasticDiagnostics.__init__(self)

        self.peak = [62500, 65000]
        self.rawfiles = ['IRS53664.raw']
        self.spectra = [3, 53]
        self.suffix = '_graphite002_slice'

    def get_reference_files(self):
        return ["II.IRISDiagnostics.nxs"]


#------------------------- OSIRIS tests ---------------------------------------


class OSIRISDiagnostics(ISISIndirectInelasticDiagnostics):

    def __init__(self):
        ISISIndirectInelasticDiagnostics.__init__(self)

        self.peak = [59000, 61000]
        self.rawfiles = ['OSI97935.raw']
        self.spectra = [963, 1004]
        self.suffix = '_graphite002_slice'

    def get_reference_files(self):
        return ["II.OSIRISDiagnostics.nxs"]


#==============================================================================
class ISISIndirectInelasticMoments(ISISIndirectInelasticBase):
    '''A base class for the ISIS indirect inelastic TransformToIqt/TransformToIqtFit tests

    The output of Elwin is usually used with MSDFit and so we plug one into
    the other in this test.
    '''
    # Mark as an abstract class
    __metaclass__ = ABCMeta

    def _run(self):
        '''Defines the workflow for the test'''

        LoadNexus(self.input_workspace,
                  OutputWorkspace=self.input_workspace)

        SofQWMoments(Sample=self.input_workspace, EnergyMin=self.e_min,
                     EnergyMax=self.e_max, Scale=self.scale,
                     Plot=False, Save=False, OutputWorkspace=self.input_workspace + '_Moments')

        self.result_names = [self.input_workspace + '_Moments']

    def _validate_properties(self):
        '''Check the object properties are in an expected state to continue'''

        if type(self.input_workspace) != str:
            raise RuntimeError("Input workspace should be a string.")
        if type(self.e_min) != float:
            raise RuntimeError("Energy min should be a float")
        if type(self.e_max) != float:
            raise RuntimeError("Energy max should be a float")
        if type(self.scale) != float:
            raise RuntimeError("Scale should be a float")


#------------------------- OSIRIS tests ---------------------------------------
class OSIRISMoments(ISISIndirectInelasticMoments):

    def __init__(self):
        ISISIndirectInelasticMoments.__init__(self)
        self.input_workspace = 'osi97935_graphite002_sqw.nxs'
        self.e_min = -0.4
        self.e_max = 0.4
        self.scale = 1.0

    def get_reference_files(self):
        return ['II.OSIRISMoments.nxs']


#------------------------- IRIS tests -----------------------------------------
class IRISMoments(ISISIndirectInelasticMoments):

    def __init__(self):
        ISISIndirectInelasticMoments.__init__(self)
        self.input_workspace = 'irs53664_graphite002_sqw.nxs'
        self.e_min = -0.4
        self.e_max = 0.4
        self.scale = 1.0

    def get_reference_files(self):
        return ['II.IRISMoments.nxs']


#==============================================================================
class ISISIndirectInelasticElwinAndMSDFit(ISISIndirectInelasticBase):
    '''A base class for the ISIS indirect inelastic Elwin/MSD Fit tests

    The output of Elwin is usually used with MSDFit and so we plug one into
    the other in this test.
    '''

    __metaclass__ = ABCMeta  # Mark as an abstract class

    def _run(self):
        '''Defines the workflow for the test'''
        self.tolerance = 1e-7

        elwin_input = '__ElWinMult_in'
        elwin_results = ['__ElWinMult_q', '__ElWinMult_q2', '__ElWinMult_elf']

        # Load files and create workspace group
        for filename in self.files:
            Load(Filename=filename, OutputWorkspace=filename)
        GroupWorkspaces(InputWorkspaces=self.files, OutputWorkspace=elwin_input)

        ElasticWindowMultiple(InputWorkspaces=elwin_input, Plot=False,
                              Range1Start=self.eRange[0], Range1End=self.eRange[1],
                              OutputInQ=elwin_results[0], OutputInQSquared=elwin_results[1],
                              OutputELF=elwin_results[2])

        int_files = [self.get_temp_dir_path(filename) + ".nxs"
                     for filename in elwin_results]

        # Save the EQ1 & EQ2 results from Elwin to put into MSDFit.
        for ws, filename in zip(elwin_results, int_files):
            SaveNexusProcessed(Filename=filename,
                               InputWorkspace=ws)

        eq2_file = elwin_results[1]
        msdfit_result = MSDFit(InputWorkspace=eq2_file,
                               XStart=self.startX,
                               XEnd=self.endX,
                               SpecMax=1,
                               Plot=False)

        # Clean up the intermediate files.
        for filename in int_files:
            os.remove(filename)

        # We're interested in the intermediate Elwin results as well as the
        # final MSDFit result.
        self.result_names = [elwin_results[0],  # EQ1
                             elwin_results[1],  # EQ2
                             msdfit_result[2].name()]  # Fit workspace

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if type(self.files) != list or len(self.files) != 2:
            raise RuntimeError("files should be a list of exactly 2 strings")
        if type(self.eRange) != list or len(self.eRange) != 2:
            raise RuntimeError("eRange should be a list of exactly 2 values")
        if type(self.startX) != float:
            raise RuntimeError("startX should be a float")
        if type(self.endX) != float:
            raise RuntimeError("endX should be a float")

#------------------------- OSIRIS tests ---------------------------------------


class OSIRISElwinAndMSDFit(ISISIndirectInelasticElwinAndMSDFit):

    def __init__(self):
        ISISIndirectInelasticElwinAndMSDFit.__init__(self)
        self.files = ['osi97935_graphite002_red.nxs',
                      'osi97936_graphite002_red.nxs']
        self.eRange = [-0.02, 0.02]
        self.startX = 0.195082
        self.endX = 3.202128

    def get_reference_files(self):
        return ['II.OSIRISElwinEQ1.nxs',
                'II.OSIRISElwinEQ2.nxs',
                'II.OSIRISMSDFit.nxs']

#------------------------- IRIS tests -----------------------------------------


class IRISElwinAndMSDFit(ISISIndirectInelasticElwinAndMSDFit):

    def __init__(self):
        ISISIndirectInelasticElwinAndMSDFit.__init__(self)
        self.files = ['irs53664_graphite002_red.nxs',
                      'irs53665_graphite002_red.nxs']
        self.eRange = [-0.02, 0.02]
        self.startX = 0.313679
        self.endX = 3.285377

    def get_reference_files(self):
        return ['II.IRISElwinEQ1.nxs',
                'II.IRISElwinEQ2.nxs',
                'II.IRISMSDFit.nxs']


#==============================================================================
class ISISIndirectInelasticFuryAndFuryFit(ISISIndirectInelasticBase):
    '''
    A base class for the ISIS indirect inelastic Fury/FuryFit tests

    The output of TransformToIqt is usually used with FuryFit and so we plug one into
    the other in this test.
    '''

    __metaclass__ = ABCMeta  # Mark as an abstract class

    def _run(self):
        '''Defines the workflow for the test'''
        self.tolerance = 1e-7
        self.samples = [sample[:-4] for sample in self.samples]

        # Load files into Mantid
        for sample in self.samples:
            LoadNexus(sample, OutputWorkspace=sample)
        LoadNexus(self.resolution, OutputWorkspace=self.resolution)

        fury_props, fury_ws = TransformToIqt(SampleWorkspace=self.samples[0],
                                             ResolutionWorkspace=self.resolution,
                                             EnergyMin=self.e_min,
                                             EnergyMax=self.e_max,
                                             BinReductionFactor=self.num_bins,
                                             DryRun=False,
                                             Save=False,
                                             Plot=False)

        # Test FuryFit Sequential
        furyfitSeq_ws = furyfitSeq(fury_ws.getName(),
                                   self.func,
                                   self.ftype,
                                   self.startx,
                                   self.endx,
                                   Save=False,
                                   Plot='None')

        self.result_names = [fury_ws.getName(),
                             furyfitSeq_ws]

        # Remove workspaces from Mantid
        for sample in self.samples:
            DeleteWorkspace(sample)

        DeleteWorkspace(self.resolution)

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if type(self.samples) != list:
            raise RuntimeError("Samples should be a list of strings.")
        if type(self.resolution) != str:
            raise RuntimeError("Resolution should be a string.")
        if type(self.e_min) != float:
            raise RuntimeError("e_min should be a float")
        if type(self.e_max) != float:
            raise RuntimeError("e_max should be a float")
        if type(self.num_bins) != int:
            raise RuntimeError("num_bins should be an int")
        if type(self.func) != str:
            raise RuntimeError("Function should be a string.")
        if type(self.ftype) != str:
            raise RuntimeError("Function type should be a string.")
        if type(self.startx) != float:
            raise RuntimeError("startx should be a float")
        if type(self.endx) != float:
            raise RuntimeError("endx should be a float")

#------------------------- OSIRIS tests ---------------------------------------


class OSIRISFuryAndFuryFit(ISISIndirectInelasticFuryAndFuryFit):

    def __init__(self):
        ISISIndirectInelasticFuryAndFuryFit.__init__(self)

        # TransformToIqt
        self.samples = ['osi97935_graphite002_red.nxs']
        self.resolution = 'osi97935_graphite002_res.nxs'
        self.e_min = -0.4
        self.e_max = 0.4
        self.num_bins = 4

        # Fury Seq Fit
        self.func = r'name=LinearBackground,A0=0,A1=0,ties=(A1=0);name=UserFunction,Formula=Intensity*exp(-(x/Tau)),Intensity=0.304185,Tau=100;ties=(f1.Intensity=1-f0.A0)'
        self.ftype = '1E_s'
        self.startx = 0.022861
        self.endx = 0.118877

    def get_reference_files(self):
        return ['II.OSIRISFury.nxs',
                'II.OSIRISFuryFitSeq.nxs']

#------------------------- IRIS tests -----------------------------------------


class IRISFuryAndFuryFit(ISISIndirectInelasticFuryAndFuryFit):

    def __init__(self):
        ISISIndirectInelasticFuryAndFuryFit.__init__(self)

        # TransformToIqt
        self.samples = ['irs53664_graphite002_red.nxs']
        self.resolution = 'irs53664_graphite002_res.nxs'
        self.e_min = -0.4
        self.e_max = 0.4
        self.num_bins = 4

        # Fury Seq Fit
        self.func = r'name=LinearBackground,A0=0,A1=0,ties=(A1=0);name=UserFunction,Formula=Intensity*exp(-(x/Tau)),Intensity=0.355286,Tau=100;ties=(f1.Intensity=1-f0.A0)'
        self.ftype = '1E_s'
        self.startx = 0.013717
        self.endx = 0.169171

    def get_reference_files(self):
        return ['II.IRISFury.nxs',
                'II.IRISFuryFitSeq.nxs']

#==============================================================================


class ISISIndirectInelasticFuryAndFuryFitMulti(ISISIndirectInelasticBase):
    '''A base class for the ISIS indirect inelastic Fury/FuryFit tests

    The output of Elwin is usually used with MSDFit and so we plug one into
    the other in this test.
    '''

    __metaclass__ = ABCMeta  # Mark as an abstract class

    def _run(self):
        '''Defines the workflow for the test'''
        self.tolerance = 1e-6
        self.samples = [sample[:-4] for sample in self.samples]

        #load files into mantid
        for sample in self.samples:
            LoadNexus(sample, OutputWorkspace=sample)
        LoadNexus(self.resolution, OutputWorkspace=self.resolution)

        fury_props, fury_ws = TransformToIqt(SampleWorkspace=self.samples[0],
                                             ResolutionWorkspace=self.resolution,
                                             EnergyMin=self.e_min,
                                             EnergyMax=self.e_max,
                                             BinReductionFactor=self.num_bins,
                                             DryRun=False,
                                             Save=False,
                                             Plot=False)

        # Test FuryFit Sequential
        furyfitSeq_ws = furyfitMult(fury_ws.getName(),
                                    self.func,
                                    self.ftype,
                                    self.startx,
                                    self.endx,
                                    Save=False,
                                    Plot='None')

        self.result_names = [fury_ws.getName(),
                             furyfitSeq_ws]

        #remove workspaces from mantid
        for sample in self.samples:
            DeleteWorkspace(sample)
        DeleteWorkspace(self.resolution)

    def _validate_properties(self):
        """Check the object properties are in an expected state to continue"""

        if type(self.samples) != list:
            raise RuntimeError("Samples should be a list of strings.")
        if type(self.resolution) != str:
            raise RuntimeError("Resolution should be a string.")
        if type(self.e_min) != float:
            raise RuntimeError("e_min should be a float")
        if type(self.e_max) != float:
            raise RuntimeError("e_max should be a float")
        if type(self.num_bins) != int:
            raise RuntimeError("num_bins should be an int")
        if type(self.func) != str:
            raise RuntimeError("Function should be a string.")
        if type(self.ftype) != str:
            raise RuntimeError("Function type should be a string.")
        if type(self.startx) != float:
            raise RuntimeError("startx should be a float")
        if type(self.endx) != float:
            raise RuntimeError("endx should be a float")

#------------------------- OSIRIS tests ---------------------------------------


class OSIRISFuryAndFuryFitMulti(ISISIndirectInelasticFuryAndFuryFitMulti):

    def skipTests(self):
        return platform.system() == "Darwin"

    def __init__(self):
        ISISIndirectInelasticFuryAndFuryFitMulti.__init__(self)

        # TransformToIqt
        self.samples = ['osi97935_graphite002_red.nxs']
        self.resolution = 'osi97935_graphite002_res.nxs'
        self.e_min = -0.4
        self.e_max = 0.4
        self.num_bins = 4

        # Fury Seq Fit
        self.func = r'name=LinearBackground,A0=0.510595,A1=0,ties=(A1=0);name=UserFunction,Formula=Intensity*exp( -(x/Tau)^Beta),Intensity=0.489405,Tau=0.105559,Beta=1.61112e-14;ties=(f1.Intensity=1-f0.A0)'
        self.ftype = '1E_s'
        self.startx = 0.0
        self.endx = 0.119681

    def get_reference_files(self):
        return ['II.OSIRISFury.nxs',
                'II.OSIRISFuryFitMulti.nxs']

#------------------------- IRIS tests -----------------------------------------


class IRISFuryAndFuryFitMulti(ISISIndirectInelasticFuryAndFuryFitMulti):

    def __init__(self):
        ISISIndirectInelasticFuryAndFuryFitMulti.__init__(self)

        # TransformToIqt
        self.samples = ['irs53664_graphite002_red.nxs']
        self.resolution = 'irs53664_graphite002_res.nxs'
        self.e_min = -0.4
        self.e_max = 0.4
        self.num_bins = 4

        # Fury Seq Fit
        self.func = r'name=LinearBackground,A0=0.584488,A1=0,ties=(A1=0);name=UserFunction,Formula=Intensity*exp( -(x/Tau)^Beta),Intensity=0.415512,Tau=4.848013e-14,Beta=0.022653;ties=(f1.Intensity=1-f0.A0)'
        self.ftype = '1S_s'
        self.startx = 0.0
        self.endx = 0.156250

    def get_reference_files(self):
        return ['II.IRISFury.nxs',
                'II.IRISFuryFitMulti.nxs']

#==============================================================================


class ISISIndirectInelasticConvFit(ISISIndirectInelasticBase):
    '''A base class for the ISIS indirect inelastic ConvFit tests

    The workflow is defined in the _run() method, simply
    define an __init__ method and set the following properties
    on the object
    '''
    # Mark as an abstract class
    __metaclass__ = ABCMeta

    def _run(self):
        '''Defines the workflow for the test'''
        self.tolerance = 1e-4
        LoadNexus(self.sample, OutputWorkspace=self.sample)

        confitSeq(
            self.sample,
            self.func,
            self.startx,
            self.endx,
            self.ftype,
            self.bg,
            specMin=self.spectra_min,
            specMax=self.spectra_max,
            Plot='None',
            Save=False)

    def _validate_properties(self):
        '''Check the object properties are in an expected state to continue'''

        if type(self.sample) != str:
            raise RuntimeError("Sample should be a string.")
        if type(self.resolution) != str:
            raise RuntimeError("Resolution should be a string.")
        if not os.path.isfile(self.resolution):
            raise RuntimeError("Resolution must be a file that exists.")
        if type(self.func) != str:
            raise RuntimeError("Function should be a string.")
        if type(self.bg) != str:
            raise RuntimeError("Background type should be a string.")
        if type(self.ftype) != str:
            raise RuntimeError("Function type should be a string.")
        if type(self.startx) != float:
            raise RuntimeError("startx should be a float")
        if type(self.endx) != float:
            raise RuntimeError("endx should be a float")
        if type(self.spectra_min) != int:
            raise RuntimeError("Min spectrum should be a int")
        if type(self.spectra_max) != int:
            raise RuntimeError("Max spectrum should be a int")
        if type(self.ties) != bool:
            raise RuntimeError("ties should be a boolean.")

#------------------------- OSIRIS tests ---------------------------------------


class OSIRISConvFit(ISISIndirectInelasticConvFit):

    def __init__(self):
        ISISIndirectInelasticConvFit.__init__(self)
        self.sample = 'osi97935_graphite002_red.nxs'
        self.resolution = FileFinder.getFullPath('osi97935_graphite002_res.nxs')
        #ConvFit fit function
        self.func = 'name=LinearBackground,A0=0,A1=0;(composite=Convolution,FixResolution=true,NumDeriv=true;name=Resolution,FileName=\"%s\";name=Lorentzian,Amplitude=2,PeakCentre=0,FWHM=0.05)' % self.resolution
        self.ftype = '1L'
        self.startx = -0.2
        self.endx = 0.2
        self.bg = 'FitL_s'
        self.spectra_min = 0
        self.spectra_max = 41
        self.ties = False

        self.result_names = ['osi97935_graphite002_conv_1LFitL_s0_to_41_Result']

    def get_reference_files(self):
        return ['II.OSIRISConvFitSeq.nxs']


#------------------------- IRIS tests -----------------------------------------
class IRISConvFit(ISISIndirectInelasticConvFit):

    def __init__(self):
        ISISIndirectInelasticConvFit.__init__(self)
        self.sample = 'irs53664_graphite002_red.nxs'
        self.resolution = FileFinder.getFullPath('irs53664_graphite002_res.nxs')
        #ConvFit fit function
        self.func = 'name=LinearBackground,A0=0.060623,A1=0.001343;(composite=Convolution,FixResolution=true,NumDeriv=true;name=Resolution,FileName=\"%s\";name=Lorentzian,Amplitude=1.033150,PeakCentre=-0.000841,FWHM=0.001576)' % self.resolution
        self.ftype = '1L'
        self.startx = -0.2
        self.endx = 0.2
        self.bg = 'FitL_s'
        self.spectra_min = 0
        self.spectra_max = 50
        self.ties = False

        self.result_names = ['irs53664_graphite002_conv_1LFitL_s0_to_50_Result']

    def get_reference_files(self):
        return ['II.IRISConvFitSeq.nxs']

#==============================================================================


class ISISIndirectInelasticApplyCorrections(ISISIndirectInelasticBase):
    '''A base class for the ISIS indirect inelastic Apply Corrections tests

    The workflow is defined in the _run() method, simply
    define an __init__ method and set the following properties
    on the object
    '''
    # Mark as an abstract class
    __metaclass__ = ABCMeta

    def _run(self):
        '''Defines the workflow for the test'''
        self.tolerance = 1e-4

        LoadNexus(self._sample_workspace + '.nxs', OutputWorkspace=self._sample_workspace)
        if self._corrections_workspace != '':
            LoadNexus(self._corrections_workspace + '.nxs', OutputWorkspace=self._corrections_workspace)
        if self._can_workspace != '':
            LoadNexus(self._can_workspace + '.nxs', OutputWorkspace=self._can_workspace)

        output_workspaces = self._run_apply_corrections()
        self.result_names = [output_workspaces['reduced_workspace']]

    def _run_apply_corrections(self):
        abscorFeeder(self._sample_workspace, self._can_workspace, self._can_geometry,
                     self._using_corrections, self._corrections_workspace, **self._kwargs)
        return self._get_output_workspace_names()

    def _get_output_workspace_names(self):
        """
        abscorFeeder doesn't return anything, these names should exist in the ADS
        apply corrections uses the following naming convention:
        <instrument><sample number>_<analyser><reflection>_<mode>_<can number>
        """

        if self._can_workspace != '':
            can_run = mtd[self._can_workspace].getRun()
            can_run_number = can_run.getProperty('run_number').value

        mode = ''
        if self._corrections_workspace != '' and self._can_workspace != '':
            mode = 'Correct_%s' % can_run_number
        elif self._corrections_workspace != '':
            mode = 'Corrected'
        else:
            mode = 'Subtract_%s' % can_run_number

        workspace_name_stem = self._sample_workspace[:-3] + mode

        output_workspaces = {
            'reduced_workspace': workspace_name_stem + '_red',
            'rqw_workspace': workspace_name_stem + '_rqw',
        }

        if self._can_workspace != '':
            output_workspaces['result_workspace'] = workspace_name_stem + '_Result'

        return output_workspaces

    def _validate_properties(self):
        '''Check the object properties are in an expected state to continue'''

#------------------------- IRIS tests -----------------------------------------

class IRISApplyCorrectionsWithCan(ISISIndirectInelasticApplyCorrections):
    """ Test applying corrections with just a can workspace """

    def __init__(self):
        ISISIndirectInelasticApplyCorrections.__init__(self)

        self._sample_workspace = 'irs26176_graphite002_red'
        self._can_workspace = 'irs26173_graphite002_red'
        self._corrections_workspace = ''
        self._can_geometry = 'cyl'
        self._using_corrections = False

        self._kwargs = {'RebinCan':False, 'ScaleOrNotToScale':False,
                        'factor':1, 'Save':False, 'PlotResult':'None', 'PlotContrib':False}

    def get_reference_files(self):
        return ['II.IRISApplyCorrectionsWithCan.nxs']


class IRISApplyCorrectionsWithCorrectionsWS(ISISIndirectInelasticApplyCorrections):
    """ Test applying corrections with a corrections workspace """

    def __init__(self):
        ISISIndirectInelasticApplyCorrections.__init__(self)

        self._sample_workspace = 'irs26176_graphite002_red'
        self._can_workspace = ''
        self._corrections_workspace = 'irs26176_graphite002_cyl_Abs'
        self._can_geometry = 'cyl'
        self._using_corrections = True

        self._kwargs = {'RebinCan':False, 'ScaleOrNotToScale':False,
                        'factor':1, 'Save':False, 'PlotResult':'None', 'PlotContrib':False}

    def get_reference_files(self):
        return ['II.IRISApplyCorrectionsWithCorrectionsWS.nxs']

class IRISApplyCorrectionsWithBoth(ISISIndirectInelasticApplyCorrections):
    """ Test applying corrections with both a can and a corrections workspace """

    def __init__(self):
        ISISIndirectInelasticApplyCorrections.__init__(self)

        self._sample_workspace = 'irs26176_graphite002_red'
        self._can_workspace = 'irs26173_graphite002_red'
        self._corrections_workspace = 'irs26176_graphite002_cyl_Abs'
        self._can_geometry = 'cyl'
        self._using_corrections = True

        self._kwargs = {'RebinCan':False, 'ScaleOrNotToScale':False,
                        'factor':1, 'Save':False, 'PlotResult':'None', 'PlotContrib':False}

    def get_reference_files(self):
        return ['II.IRISApplyCorrections.nxs']

#==============================================================================
# Transmission Monitor Test

class ISISIndirectInelasticTransmissionMonitor(ISISIndirectInelasticBase):
    '''
    '''

    # Mark as an abstract class
    __metaclass__ = ABCMeta

    def _run(self):
        '''Defines the workflow for the test'''

        self.tolerance = 1e-4
        Load(self.sample, OutputWorkspace=self.sample)
        Load(self.can, OutputWorkspace=self.can)

        IndirectTransmissionMonitor(SampleWorkspace=self.sample, CanWorkspace=self.can,
                                    OutputWorkspace='IRISTransmissionMonitorTest')

    def _validate_properties(self):
        '''Check the object properties are in an expected state to continue'''

        if type(self.sample) != str:
            raise RuntimeError("Sample should be a string.")
        if type(self.can) != str:
            raise RuntimeError("Can should be a string.")


#------------------------- IRIS tests -----------------------------------------
class IRISTransmissionMonitor(ISISIndirectInelasticTransmissionMonitor):

    def __init__(self):
        ISISIndirectInelasticTransmissionMonitor.__init__(self)
        self.sample = 'IRS26176.RAW'
        self.can = 'IRS26173.RAW'

        self.result_names = ['IRISTransmissionMonitorTest']

    def get_reference_files(self):
        return ['II.IRISTransmissionMonitor.nxs']

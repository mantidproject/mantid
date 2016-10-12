# pylint: disable=no-init, too-many-instance-attributes
from __future__ import (absolute_import, division, print_function)
from six import iteritems

from mantid.kernel import *
from mantid.api import *
from vesuvio.base import VesuvioBase, TableWorkspaceDictionaryFacade
from vesuvio.fitting import parse_fit_options
import mantid.simpleapi as ms
import math

#----------------------------------------------------------------------------------------


def create_cuboid_xml(height, width, depth):
    """
    Create the XML string to describe a cuboid of the given dimensions

    @param height Height in metres (Y coordinate)
    @param width Width in metres (X coordinate)
    @param depth Depth in metres (Z coordinate)
    """
    half_height, half_width, half_thick = 0.5*height, 0.5*width, 0.5*depth
    xml_str = \
        " <cuboid id=\"sample-shape\"> " \
        + "<left-front-bottom-point " \
        + "x=\"%f\" y=\"%f\" z=\"%f\" /> " % (half_width, -half_height, half_thick) \
        + "<left-front-top-point " \
        + "x=\"%f\" y=\"%f\" z=\"%f\" /> " % (half_width, half_height, half_thick) \
        + "<left-back-bottom-point " \
        + "x=\"%f\" y=\"%f\" z=\"%f\" /> " % (half_width, -half_height, -half_thick) \
        + "<right-front-bottom-point " \
        + "x=\"%f\" y=\"%f\" z=\"%f\" /> " % (-half_width, -half_height, half_thick) \
        + "</cuboid>"
    return xml_str

#----------------------------------------------------------------------------------------


class VesuvioCorrections(VesuvioBase):

    _input_ws = None
    _output_ws = None
    _correction_workspaces = None
    _linear_fit_table = None
    _correction_wsg = None
    _corrected_wsg = None
    _container_ws = None
    _spec_idx = None

#------------------------------------------------------------------------------

    def summary(self):
        return "Apply post fitting steps to vesuvio data"

    def category(self):
        return "Inelastic\\Indirect\\Vesuvio"

#------------------------------------------------------------------------------

    # pylint: disable=too-many-locals
    def PyInit(self):
        # Inputs
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "",
                                                     direction=Direction.Input),
                             doc="Input TOF workspace")

        self.declareProperty("WorkspaceIndex", 0,
                             doc="Index of spectrum to calculate corrections for")

        self.declareProperty(ITableWorkspaceProperty("FitParameters", "",
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc="Table containing the calculated fit parameters"
                                 "for the data in the workspace")

        float_length_validator = FloatArrayLengthValidator()
        float_length_validator.setLengthMin(1)
        self.declareProperty(FloatArrayProperty("Masses", float_length_validator),
                             doc="Mass values for fitting")

        self.declareProperty("MassProfiles", "", StringMandatoryValidator(),
                             doc="Functions used to approximate mass profile. "
                                 "The format is "
                                 "function=Function1Name,param1=val1,param2=val2;"
                                 "function=Function2Name,param3=val3,param4=val4")

        self.declareProperty("IntensityConstraints", "",
                             doc="A semi-colon separated list of intensity "
                                 "constraints defined as lists e.g "
                                 "[0,1,0,-4];[1,0,-2,0]")

        # Container
        self.declareProperty(MatrixWorkspaceProperty("ContainerWorkspace", "",
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc="Container workspace in TOF")

        self.declareProperty("ContainerScale", 0.0,
                             doc="Scale factor to apply to container, set to 0 for "
                                 "automatic scale based on linear fit")

        # Gamma background
        self.declareProperty("GammaBackground", True, direction=Direction.Input,
                             doc="If true, correct for the gamma background")

        self.declareProperty("GammaBackgroundScale", 0.0,
                             doc="Scale factor to apply to gamma background, set to 0 "
                             "for automatic scale based on linear fit")

        # Multiple scattering
        self.declareProperty("MultipleScattering", True, direction=Direction.Input,
                             doc="If true, correct for the effects of multiple scattering")

        self.declareProperty("BeamRadius", 2.5,
                             doc="Radius of beam in cm")

        self.declareProperty("SampleHeight", 5.0,
                             doc="Height of sample in cm")

        self.declareProperty("SampleWidth", 5.0,
                             doc="Width of sample in cm")

        self.declareProperty("SampleDepth", 5.0,
                             doc="Depth of sample in cm")

        self.declareProperty("SampleDensity", 1.0,
                             doc="Sample density in g/cm^3")

        self.declareProperty("Seed", 123456789,
                             doc="")

        self.declareProperty("NumScatters", 3,
                             doc="")

        self.declareProperty("NumRuns", 10,
                             doc="")

        self.declareProperty("NumEvents", 50000,
                             doc="Number of neutron events")

        self.declareProperty("SmoothNeighbours", 3,
                             doc="")

        # Outputs
        self.declareProperty(WorkspaceGroupProperty("CorrectionWorkspaces", "",
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc="Workspace group containing correction intensities "
                             "for each correction")

        self.declareProperty(WorkspaceGroupProperty("CorrectedWorkspaces", "",
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc="Workspace group containing individual corrections "
                             "applied to raw data")

        self.declareProperty(ITableWorkspaceProperty("LinearFitResult", "",
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc="Table workspace containing the fit parameters used to"
                                 "linearly fit the corrections to the data")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "",
                                                     direction=Direction.Output),
                             doc="The name of the output workspace")

#------------------------------------------------------------------------------

    def validateInputs(self):
        self._get_properties()
        errors = dict()

        if self.getProperty("FitParameters").value is None:
            errors["FitParameters"] = "Corrections require a " \
                                      + "set of parameters from a fit of the data"

        return errors

#------------------------------------------------------------------------------

    def _get_properties(self):
        self._input_ws = self.getPropertyValue("InputWorkspace")
        self._container_ws = self.getPropertyValue("ContainerWorkspace")
        self._spec_idx = self.getProperty("WorkspaceIndex").value
        self._output_ws = self.getPropertyValue("OutputWorkspace")
        self._correction_wsg = self.getPropertyValue("CorrectionWorkspaces")
        self._corrected_wsg = self.getPropertyValue("CorrectedWorkspaces")
        self._linear_fit_table = self.getPropertyValue("LinearFitResult")

#------------------------------------------------------------------------------

    def PyExec(self):

        ms.ExtractSingleSpectrum(InputWorkspace=self._input_ws,
                                 OutputWorkspace=self._output_ws,
                                 WorkspaceIndex=self._spec_idx)

        # Performs corrections
        self._define_corrections()

        # The workspaces to fit for correction scale factors
        fit_corrections = [wks for wks in self._correction_workspaces if 'MultipleScattering' not in wks]

        # Perform fitting of corrections
        fixed_params = {}

        fixed_gamma_factor = self.getProperty("GammaBackgroundScale").value
        if fixed_gamma_factor != 0.0:
            fixed_params['GammaBackground'] = fixed_gamma_factor

        fixed_container_scale = self.getProperty("ContainerScale").value
        if fixed_container_scale != 0.0:
            fixed_params['Container'] = fixed_container_scale

        params_ws = self._fit_corrections(fit_corrections, self._linear_fit_table, **fixed_params)
        self.setProperty("LinearFitResult", params_ws)

        # Scale gamma background
        if self.getProperty("GammaBackground").value:
            gamma_correct_ws = self._get_correction_workspace('GammaBackground')[1]
            gamma_factor = self._get_correction_scale_factor('GammaBackground',
                                                             fit_corrections, params_ws)
            ms.Scale(InputWorkspace=gamma_correct_ws,
                     OutputWorkspace=gamma_correct_ws,
                     Factor=gamma_factor)

        # Scale multiple scattering
        if self.getProperty("MultipleScattering").value:
            # Use factor of total scattering as this includes single and multiple scattering
            multi_scatter_correct_ws = self._get_correction_workspace('MultipleScattering')[1]
            total_scatter_correct_ws = self._get_correction_workspace('TotalScattering')[1]
            total_scatter_factor = self._get_correction_scale_factor('TotalScattering',
                                                                     fit_corrections, params_ws)
            ms.Scale(InputWorkspace=multi_scatter_correct_ws,
                     OutputWorkspace=multi_scatter_correct_ws,
                     Factor=total_scatter_factor)
            ms.Scale(InputWorkspace=total_scatter_correct_ws,
                     OutputWorkspace=total_scatter_correct_ws,
                     Factor=total_scatter_factor)

        # Scale by container
        if self._container_ws != "":
            container_correct_ws = self._get_correction_workspace('Container')[1]
            container_factor = self._get_correction_scale_factor('Container',
                                                                 fit_corrections, params_ws)
            ms.Scale(InputWorkspace=container_correct_ws,
                     OutputWorkspace=container_correct_ws,
                     Factor=container_factor)

        # Calculate and output corrected workspaces as a WorkspaceGroup
        if self._corrected_wsg != "":
            corrected_workspaces = [ws_name.replace(self._correction_wsg, self._corrected_wsg) for ws_name in self._correction_workspaces]
            for corrected, correction in zip(corrected_workspaces, self._correction_workspaces):
                ms.Minus(LHSWorkspace=self._output_ws,
                         RHSWorkspace=correction,
                         OutputWorkspace=corrected)
            ms.GroupWorkspaces(InputWorkspaces=corrected_workspaces,
                               OutputWorkspace=self._corrected_wsg)
            self.setProperty("CorrectedWorkspaces", self._corrected_wsg)

        # Apply corrections
        for correction in self. _correction_workspaces:
            if 'TotalScattering' not in correction:
                ms.Minus(LHSWorkspace=self._output_ws,
                         RHSWorkspace=correction,
                         OutputWorkspace=self._output_ws)

        self.setProperty("OutputWorkspace", self._output_ws)

        # Remove correction workspaces if they are no longer required
        if self._correction_wsg == "":
            for wksp in self._correction_workspaces:
                ms.DeleteWorkspace(wksp)

#------------------------------------------------------------------------------

    def _define_corrections(self):
        """
        Defines all the corrections that are required
        """
        self._correction_workspaces = list()

        if self._container_ws != "":
            container_name = str(self._correction_wsg) + "_Container"
            self._container_ws = ms.ExtractSingleSpectrum(InputWorkspace=self._container_ws,
                                                          OutputWorkspace=container_name,
                                                          WorkspaceIndex=self._spec_idx)
            self._correction_workspaces.append(self._container_ws.name())

        # Do gamma correction
        if self.getProperty("GammaBackground").value:
            self._correction_workspaces.append(self._gamma_correction())

        # Do multiple scattering correction
        if self.getProperty("MultipleScattering").value:
            self._correction_workspaces.extend(self._ms_correction())

        # Output correction workspaces as a WorkspaceGroup
        if self._correction_wsg != "":
            ms.GroupWorkspaces(InputWorkspaces=self._correction_workspaces,
                               OutputWorkspace=self._correction_wsg)
            self.setProperty("CorrectionWorkspaces", self._correction_wsg)

#------------------------------------------------------------------------------

    def _fit_corrections(self, fit_workspaces, param_table_name, **fixed_parameters):
        functions = []

        for idx, wsn in enumerate(fit_workspaces):
            tie = ''
            for param, value in iteritems(fixed_parameters):
                if param in wsn:
                    tie = 'Scaling=%f,' % value
            function_str = "name=TabulatedFunction,Workspace=%s," % (wsn) \
                           + "ties=(%sShift=0,XScaling=1)," % (tie) \
                           + "constraints=(Scaling>=0.0)"
            functions.append(function_str)

            logger.notice('Corrections scale fit index %d is %s' % (idx, wsn))

        fit = AlgorithmManager.create("Fit")
        fit.initialize()
        fit.setChild(True)
        fit.setLogging(True)
        fit.setProperty("Function", ";".join(functions))
        fit.setProperty("InputWorkspace", self._output_ws)
        fit.setProperty("Output", param_table_name)
        fit.setProperty("CreateOutput", True)
        fit.execute()

        return fit.getProperty('OutputParameters').value

#------------------------------------------------------------------------------

    def _get_correction_workspace(self, correction_name, corrections=None):
        if corrections is None:
            corrections = self._correction_workspaces

        for idx, ws_name in enumerate(corrections):
            if correction_name in ws_name:
                return idx, ws_name

        return None, None

#------------------------------------------------------------------------------

    def _get_correction_scale_factor(self, correction_name, corrections, params_ws):
        index = self._get_correction_workspace(correction_name, corrections)[0]
        if index is None:
            raise RuntimeError('No workspace for given correction')

        params_dict = TableWorkspaceDictionaryFacade(params_ws)
        scale_param_name = 'f%d.Scaling' % index

        return params_dict[scale_param_name]

#------------------------------------------------------------------------------

    def _gamma_correction(self):
        correction_background_ws = str(self._correction_wsg) + "_GammaBackground"

        fit_opts = parse_fit_options(mass_values=self.getProperty("Masses").value,
                                     profile_strs=self.getProperty("MassProfiles").value,
                                     constraints_str=self.getProperty("IntensityConstraints").value)
        params_ws_name = self.getPropertyValue("FitParameters")
        params_dict = TableWorkspaceDictionaryFacade(mtd[params_ws_name])
        func_str = fit_opts.create_function_str(params_dict)

        ms.VesuvioCalculateGammaBackground(InputWorkspace=self._output_ws,
                                           ComptonFunction=func_str,
                                           BackgroundWorkspace=correction_background_ws,
                                           CorrectedWorkspace='__corrected_dummy')
        ms.DeleteWorkspace('__corrected_dummy')

        return correction_background_ws

#------------------------------------------------------------------------------

    def _ms_correction(self):
        """
        Calculates the contributions from multiple scattering
        on the input data from the set of given options
        """

        masses = self.getProperty("Masses").value
        params_ws_name = self.getPropertyValue("FitParameters")
        params_dict = TableWorkspaceDictionaryFacade(mtd[params_ws_name])

        atom_props = list()
        intensities = list()

        for i, mass in enumerate(masses):
            intentisty_prop = 'f%d.Intensity' % i
            c0_prop = 'f%d.C_0' % i

            if intentisty_prop in params_dict:
                intentisy = params_dict[intentisty_prop]
            elif c0_prop in params_dict:
                intentisy = params_dict[c0_prop]
            else:
                continue

            # The program DINSMS_BATCH uses those sample parameters together with the sigma divided
            # by the sum absolute of scattering intensities for each detector (detector bank),
            # sigma/int_sum
            # Thus:
            # intensity = intensity/intensity_sum

            # In the thin sample limit, 1-exp(-n*dens*sigma) ~ n*dens*sigma, effectively the same
            # scattering power (ratio of double to single scatt.)  is obtained either by using
            # relative intensities ( sigma/int_sum ) or density divided by the total intensity
            # However, in the realistic case of thick sample, the SampleDensity, dens,  must be
            # obtained by iterative numerical solution of the Eq:
            # 1-exp(-n*dens*sigma) = measured scattering power of the sample.
            # For this, a program like THICK must be used.
            # The program THICK also uses sigma/int_sum to be consistent with the prgram
            # DINSMS_BATCH

            width_prop = 'f%d.Width' % i
            sigma_x_prop = 'f%d.SigmaX' % i
            sigma_y_prop = 'f%d.SigmaY' % i
            sigma_z_prop = 'f%d.SigmaZ' % i

            if width_prop in params_dict:
                width = params_dict['f%d.Width' % i]
            elif sigma_x_prop in params_dict:
                sigma_x = float(params_dict[sigma_x_prop])
                sigma_y = float(params_dict[sigma_y_prop])
                sigma_z = float(params_dict[sigma_z_prop])
                width = math.sqrt((sigma_x**2 + sigma_y**2 + sigma_z**2) / 3.0)
            else:
                continue

            atom_props.append(mass)
            atom_props.append(intentisy)
            atom_props.append(width)
            intensities.append(intentisy)

        intensity_sum = sum(intensities)

        # Create the sample shape
        # Input dimensions are expected in CM
        ms.CreateSampleShape(InputWorkspace=self._output_ws,
                             ShapeXML=create_cuboid_xml(self.getProperty("SampleHeight").value/100.,
                                                        self.getProperty("SampleWidth").value/100.,
                                                        self.getProperty("SampleDepth").value/100.))

        # Massage options into how algorithm expects them
        total_scatter_correction = str(self._correction_wsg) + "_TotalScattering"
        multi_scatter_correction = str(self._correction_wsg) + "_MultipleScattering"

        # Calculation
        # In the thin sample limit, 1-exp(-n*dens*sigma) ~ n*dens*sigma, effectively the same
        # scattering power(ratio of double to single scatt.)  is obtained either by using relative
        # intensities ( sigma/int_sum )or density divided by the total intensity.
        # However, in the realistic case of thick sample, the SampleDensity, dens,  must be
        # obtained by iterative numerical solution of the Eq:
        # 1-exp(-n*dens*sigma) = measured scattering power of the sample.
        # For this, a program like THICK must be used.
        # The program THICK also uses sigma/int_sum to be consistent with the prgram DINSMS_BATCH
        # The algorithm VesuvioCalculateMs called by the algorithm VesuvioCorrections takes the
        # parameter AtomicProperties with the absolute intensities, contraty to DINSMS_BATCH which
        # takes in relative intensities.
        # To compensate for this, the thickness parameter, dens (SampleDensity),  is divided in by
        # the sum of absolute intensities in VesuvioCorrections before being passed to
        # VesuvioCalculateMs.
        # Then, for the modified VesuvioCorrection algorithm one can use the thickenss parameter is
        # as is from the THICK command, i.e. 43.20552
        # This works, however, only in the thin sample limit, contrary to the THICK program. Thus,
        # for some detectors (detector banks) the SampleDensiy parameter may be over(under)
        # estimated.

        ms.VesuvioCalculateMS(InputWorkspace=self._output_ws,
                              NoOfMasses=int(len(atom_props)/3),
                              SampleDensity=self.getProperty("SampleDensity").value/intensity_sum,
                              AtomicProperties=atom_props,
                              BeamRadius=self.getProperty("BeamRadius").value,
                              NumEventsPerRun=self.getProperty("NumEvents").value,
                              TotalScatteringWS=total_scatter_correction,
                              MultipleScatteringWS=multi_scatter_correction)

        # Smooth the output
        smooth_neighbours = self.getProperty("SmoothNeighbours").value
        ms.SmoothData(InputWorkspace=total_scatter_correction,
                      OutputWorkspace=total_scatter_correction,
                      NPoints=smooth_neighbours)
        ms.SmoothData(InputWorkspace=multi_scatter_correction,
                      OutputWorkspace=multi_scatter_correction,
                      NPoints=smooth_neighbours)

        return total_scatter_correction, multi_scatter_correction


# -----------------------------------------------------------------------------------------
AlgorithmFactory.subscribe(VesuvioCorrections)

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.kernel import Logger, Property, PropertyManager
from mantid.simpleapi import AbsorptionCorrection, DeleteWorkspace, Divide, Load, Multiply, \
    PaalmanPingsAbsorptionCorrection, PreprocessDetectorsToMD, SetSample, mtd
import numpy as np
import os

VAN_SAMPLE_DENSITY = 0.0721
_EXTENSIONS_NXS = ["_event.nxs", ".nxs.h5"]


def _getBasename(filename):
    """
    Helper function to get the filename without the path or extension
    """
    if type(filename) == list:
        filename = filename[0]
    name = os.path.split(filename)[-1]
    for extension in _EXTENSIONS_NXS:
        name = name.replace(extension, '')
    return name


def calculate_absorption_correction(filename, abs_method, props, sample_formula, mass_density,
                                    number_density=Property.EMPTY_DBL,
                                    container_shape="PAC06",
                                    num_wl_bins=1000,
                                    element_size=1,
                                    metaws=None):
    """The absorption correction is applied by (I_s - I_c*k*A_csc/A_cc)/A_ssc for pull Paalman-Ping

    If no cross-term then I_s/A_ss - I_c/A_cc

    Therefore this will return 2 workspace, one for correcting the
    sample (A_s) and one for the container (A_c) depending on the
    absorption method, that will be passed to _focusAndSum and
    therefore AlignAndFocusPowderFromFiles.

    If SampleOnly then

    A_s = A_ss
    A_c = None

    If SampleAndContainer then

    A_s = A_ss
    A_c = A_cc

    If FullPaalmanPings then
    A_s = A_ssc
    A_c = A_cc*A_ssc/A_csc

    This will then return (A_s, A_c)

    :param filename: File to be used for absorption correction
    :param abs_method: Type of absorption correction: None, SampleOnly, SampleAndContainer, FullPaalmanPings
    :param props: PropertyManager of run characterizations, obtained from PDDetermineCharacterizations
    :param sample_formula: Sample formula to specify the Material for absorption correction
    :param mass_density: Mass density of the sample to specify the Material for absorption correction
    :param number_density: Optional number density of sample to be added to the Material for absorption correction
    :param container_shape: Shape definition of container, such as PAC06.
    :param num_wl_bins: Number of bins for calculating wavelength
    :param element_size: Size of one side of the integration element cube in mm
    :param metaws: Optional workspace containing metadata to use instead of reading from filename
    :return:
    """
    if abs_method == "None":
        return None, None

    material = {"ChemicalFormula": sample_formula,
                "SampleMassDensity": mass_density}

    if number_density != Property.EMPTY_DBL:
        material["SampleNumberDensity"] = number_density

    environment = {'Name': 'InAir', 'Container': container_shape}

    donorWS = create_absorption_input(filename, props, num_wl_bins, material=material,
                                      environment=environment, metaws=metaws)

    absName = '__{}_abs_correction'.format(_getBasename(filename))

    return calc_absorption_corr_using_wksp(donorWS, abs_method, element_size, absName)


def calc_absorption_corr_using_wksp(donor_wksp, abs_method, element_size=1, prefix_name=''):
    """
    Calculates absorption correction on the specified donor workspace. See the documentation
    for the ``calculate_absorption_correction`` function above for more details.

    :param donor_wksp: Input workspace to compute absorption correction on
    :param abs_method: Type of absorption correction: None, SampleOnly, SampleAndContainer, FullPaalmanPings
    :param element_size: Size of one side of the integration element cube in mm
    :param prefix_name: Optional prefix of the output workspaces, default is the donor_wksp name.
    :return: Two workspaces (A_s, A_c), the first for the sample and the second for the container
    """

    if abs_method == "None":
        return None, None

    if isinstance(donor_wksp, str):
        if not mtd.doesExist(donor_wksp):
            raise RuntimeError("Specified donor workspace not found in the ADS")
        donor_wksp = mtd[donor_wksp]

    absName = donor_wksp.name()
    if prefix_name != '':
        absName = prefix_name

    if abs_method == "SampleOnly":
        AbsorptionCorrection(donor_wksp,
                             OutputWorkspace=absName + '_ass',
                             ScatterFrom='Sample',
                             ElementSize=element_size)
        return absName + '_ass', None
    elif abs_method == "SampleAndContainer":
        AbsorptionCorrection(donor_wksp,
                             OutputWorkspace=absName + '_ass',
                             ScatterFrom='Sample',
                             ElementSize=element_size)
        AbsorptionCorrection(donor_wksp,
                             OutputWorkspace=absName + '_acc',
                             ScatterFrom='Container',
                             ElementSize=element_size)
        return absName + '_ass', absName + '_acc'
    elif abs_method == "FullPaalmanPings":
        PaalmanPingsAbsorptionCorrection(donor_wksp,
                                         OutputWorkspace=absName,
                                         ElementSize=element_size)
        Multiply(LHSWorkspace=absName + '_acc',
                 RHSWorkspace=absName + '_assc',
                 OutputWorkspace=absName + '_ac')
        Divide(LHSWorkspace=absName + '_ac',
               RHSWorkspace=absName + '_acsc',
               OutputWorkspace=absName + '_ac')
        return absName + '_assc', absName + '_ac'
    else:
        raise RuntimeWarning("Unrecognized absorption correction method '{}'".format(abs_method))


def create_absorption_input(filename, props, num_wl_bins=1000,
                            material=None, geometry=None, environment=None,
                            opt_wl_min=0, opt_wl_max=Property.EMPTY_DBL,
                            metaws=None):
    """
    Create an input workspace for carpenter or other absorption corrections

    :param filename: Input file to retrieve properties from the sample log
    :param props: PropertyManager of run characterizations, obtained from PDDetermineCharacterizations
    :param num_wl_bins: The number of wavelength bins used for absorption correction
    :param material: Optional material to use in SetSample
    :param geometry: Optional geometry to use in SetSample
    :param environment: Optional environment to use in SetSample
    :param opt_wl_min: Optional minimum wavelength. If specified, this is used instead of from the props
    :param opt_wl_max: Optional maximum wavelength. If specified, this is used instead of from the props
    :param metaws: Optional workspace name with metadata to use for donor workspace instead of reading from filename
    :return: Name of the donor workspace created
    """
    if props is None:
        raise RuntimeError("props is required to create donor workspace, props is None")

    if not isinstance(props, PropertyManager):
        raise RuntimeError("props must be a PropertyManager object")

    log = Logger('CreateAbsorptionInput')

    # Load from file if no workspace with metadata has been given, otherwise avoid a duplicate load with the metaws
    absName = metaws
    if metaws is None:
        absName = '__{}_abs'.format(_getBasename(filename))
        Load(Filename=filename, OutputWorkspace=absName, MetaDataOnly=True)

    # first attempt to get the wavelength range from the properties file
    wl_min, wl_max = props['wavelength_min'].value, props['wavelength_max'].value
    # override that with what was given as parameters to the algorithm
    if opt_wl_min > 0.:
        wl_min = opt_wl_min
    if opt_wl_max != Property.EMPTY_DBL:
        wl_max = opt_wl_max

    # if it isn't found by this point, guess it from the time-of-flight range
    if (wl_min == wl_max == 0.):
        tof_min = props['tof_min'].value
        tof_max = props['tof_max'].value
        if tof_min >= 0. and tof_max > tof_min:
            log.information('TOF range is {} to {} microseconds'.format(tof_min, tof_max))

            # determine L1
            instr = mtd[absName].getInstrument()
            L1 = instr.getSource().getDistance(instr.getSample())
            # determine L2 range
            PreprocessDetectorsToMD(InputWorkspace=absName, OutputWorkspace=absName + '_dets', GetMaskState=False)
            L2 = mtd[absName + '_dets'].column('L2')
            Lmin = np.min(L2) + L1
            Lmax = np.max(L2) + L1
            DeleteWorkspace(Workspace=absName + '_dets')

            log.information('Distance range is {} to {} meters'.format(Lmin, Lmax))

            # wavelength is h*TOF / m_n * L  values copied from Kernel/PhysicalConstants.h
            usec_to_sec = 1.e-6
            meter_to_angstrom = 1.e10
            h_m_n = meter_to_angstrom * usec_to_sec * 6.62606896e-34 / 1.674927211e-27
            wl_min = h_m_n * tof_min / Lmax
            wl_max = h_m_n * tof_max / Lmin

    # there isn't a good way to guess it so error out
    if wl_max <= wl_min:
        DeleteWorkspace(Workspace=absName)  # no longer needed
        raise RuntimeError('Invalid wavelength range min={}A max={}A'.format(wl_min, wl_max))
    log.information('Using wavelength range min={}A max={}A'.format(wl_min, wl_max))

    absorptionWS = WorkspaceFactory.create(mtd[absName], NVectors=mtd[absName].getNumberHistograms(),
                                           XLength=num_wl_bins + 1, YLength=num_wl_bins)
    xaxis = np.arange(0., float(num_wl_bins + 1)) * (wl_max - wl_min) / (num_wl_bins) + wl_min
    for i in range(absorptionWS.getNumberHistograms()):
        absorptionWS.setX(i, xaxis)
    absorptionWS.getAxis(0).setUnit('Wavelength')

    # this effectively deletes the metadata only workspace
    AnalysisDataService.addOrReplace(absName, absorptionWS)

    # Set ChemicalFormula, and either SampleMassDensity or Mass, if SampleMassDensity not set
    if material is not None:
        if (not material['ChemicalFormula']) and ("SampleFormula" in absorptionWS.run()):
            material['ChemicalFormula'] = absorptionWS.run()['SampleFormula'].lastValue().strip()
        if ("SampleMassDensity" not in material or not material['SampleMassDensity']) and (
                "SampleDensity" in absorptionWS.run()):
            if (absorptionWS.run()['SampleDensity'].lastValue() != 1.0) and (
                    absorptionWS.run()['SampleDensity'].lastValue() != 0.0):
                material['SampleMassDensity'] = absorptionWS.run()['SampleDensity'].lastValue()
            else:
                material['Mass'] = absorptionWS.run()['SampleMass'].lastValue()

    # Set height for computing density if height not set
    if geometry is None:
        geometry = {}

    if geometry is not None:
        if "Height" not in geometry or not geometry['Height']:
            # Check units - SetSample expects cm
            if absorptionWS.run()['BL11A:CS:ITEMS:HeightInContainerUnits'].lastValue() == "mm":
                conversion = 0.1
            elif absorptionWS.run()['BL11A:CS:ITEMS:HeightInContainerUnits'].lastValue() == "cm":
                conversion = 1.0
            else:
                raise ValueError("HeightInContainerUnits expects cm or mm; specified units not recognized: ",
                                 absorptionWS.run()['BL11A:CS:ITEMS:HeightInContainerUnits'].lastValue())

            geometry['Height'] = absorptionWS.run()['BL11A:CS:ITEMS:HeightInContainer'].lastValue() * conversion

    # Set container if not set
    if environment is not None:
        if environment['Container'] == "":
            environment['Container'] = absorptionWS.run()['SampleContainer'].lastValue().replace(" ", "")

    # Make sure one is set before calling SetSample
    if material or geometry or environment is not None:
        setup_sample(absName, material, geometry, environment)

    return absName


def setup_sample(donor_ws, material, geometry, environment):
    """
    Calls SetSample with the associated sample and container material and geometry for use
    in creating an input workspace for an Absorption Correction algorithm
    :param donor_ws:
    :param material:
    :param geometry:
    :param environment:
    """

    # Set the material, geometry, and container info
    SetSample(InputWorkspace=donor_ws,
              Material=material,
              Geometry=geometry,
              Environment=environment)

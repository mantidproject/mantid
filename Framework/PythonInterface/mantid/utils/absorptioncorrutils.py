# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.kernel import Logger, Property, PropertyManager
from mantid.simpleapi import (AbsorptionCorrection, DeleteWorkspace, Divide, Load, Multiply,
                              PaalmanPingsAbsorptionCorrection, PreprocessDetectorsToMD,
                              RenameWorkspace, SetSample, SaveNexusProcessed, UnGroupWorkspace, mtd)
import mantid.simpleapi
import numpy as np
import os
from functools import wraps

VAN_SAMPLE_DENSITY = 0.0721
_EXTENSIONS_NXS = ["_event.nxs", ".nxs.h5"]


# ---------------------------- #
# ----- Helper functions ----- #
# ---------------------------- #
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


def __get_instrument_name(wksp):
    """
    Get the short name of given work space

    :param wksp: input workspace

    return instrument short name as string
    """
    if wksp in mtd:
        ws = mtd[wksp]
    else:
        raise ValueError(f"{wksp} cannot be found")
    return mantid.kernel.ConfigService.getInstrument(ws.getInstrument().getName()).shortName()


def __get_cache_name(meta_wksp_name, abs_method, cache_dir="", prefix_name=""):
    """generate cachefile name (full path) and sha1

    :param meta_wksp_name: name of workspace contains relevant meta data for hashing
    :param abs_method: method used to perform the absorption calculation
    :param cache_dir: cache directory to scan/load cache data
    :param prefix_name: prefix to add to wkspname for caching

    return cachefile_name: full path of the cache file
           sha1: MD5 value based on selected property
    """
    # grab the workspace
    if meta_wksp_name in mtd:
        ws = mtd[meta_wksp_name]
    else:
        raise ValueError(
            f"Cannot find workspace {meta_wksp_name} to extract meta data for hashing, aborting")

    # requires cache_dir
    if cache_dir == "":
        cache_filename, signature = "", ""
    else:
        # generate the property string for hashing
        property_string = [
            f"{key}={val}" for key, val in {
                'wavelength_min': ws.readX(0).min(),
                'wavelength_max': ws.readX(0).max(),
                "num_wl_bins": len(ws.readX(0)) - 1,
                "sample_formula": ws.run()['SampleFormula'].lastValue().strip(),
                "mass_density": ws.run()['SampleDensity'].lastValue(),
                "height_unit": ws.run()['BL11A:CS:ITEMS:HeightInContainerUnits'].lastValue(),
                "height": ws.run()['BL11A:CS:ITEMS:HeightInContainer'].lastValue(),
                "sample_container": ws.run()['SampleContainer'].lastValue().replace(" ", ""),
                "abs_method": abs_method,
            }.items()
        ]

        # use mantid build-in alg to generate the cache filename and sha1
        cache_filename, signature = mantid.simpleapi.CreateCacheFilename(
            Prefix=prefix_name,
            OtherProperties=property_string,
            CacheDir=cache_dir,
        )

    return cache_filename, signature


def __load_cached_data(cache_file_name, sha1, abs_method="", prefix_name=""):
    """try to load cached data from memory and disk

    :param abs_method: absorption calculation method
    :param sha1: SHA1 that identify cached workspace
    :param cache_file_name: cache file name to search
    :param prefix_name: prefix to add to wkspname for caching

    return  found_abs_wksp_sample, found_abs_wksp_container
            abs_wksp_sample, abs_wksp_container
    """
    # init
    abs_wksp_sample, abs_wksp_container = "", ""
    found_abs_wksp_sample, found_abs_wksp_container = False, False

    # step_0: depending on the abs_method, suffix will be different
    if abs_method == "SampleOnly":
        abs_wksp_sample = f"{prefix_name}_ass"
        found_abs_wksp_container = True
    elif abs_method == "SampleAndContainer":
        abs_wksp_sample = f"{prefix_name}_ass"
        abs_wksp_container = f"{prefix_name}_acc"
    elif abs_method == "FullPaalmanPings":
        abs_wksp_sample = f"{prefix_name}_assc"
        abs_wksp_container = f"{prefix_name}_ac"
    else:
        raise ValueError("Unrecognized absorption correction method '{}'".format(abs_method))

    # step_1: check memory
    if mtd.doesExist(abs_wksp_sample):
        found_abs_wksp_sample = mtd[abs_wksp_sample].run()["absSHA1"].value == sha1
    if mtd.doesExist(abs_wksp_container):
        found_abs_wksp_container = mtd[abs_wksp_container].run()["absSHA1"].value == sha1

    # step_2: load from disk if either is not found in memory
    if (not found_abs_wksp_sample) or (not found_abs_wksp_container):
        if os.path.exists(cache_file_name):
            wsntmp = f"tmpwsg"
            Load(Filename=cache_file_name, OutputWorkspace=wsntmp)
            wstype = mtd[wsntmp].id()
            if wstype == "Workspace2D":
                RenameWorkspace(InputWorkspace=wsntmp, OutputWorkspace=abs_wksp_sample)
            elif wstype == "WorkspaceGroup":
                UnGroupWorkspace(InputWorkspace=wsntmp)
            else:
                raise ValueError(f"Unsupported cached workspace type: {wstype}")

    # step_3: check memory again
    if mtd.doesExist(abs_wksp_sample):
        found_abs_wksp_sample = mtd[abs_wksp_sample].run()["absSHA1"].value == sha1
    if mtd.doesExist(abs_wksp_container):
        found_abs_wksp_container = mtd[abs_wksp_container].run()["absSHA1"].value == sha1

    return found_abs_wksp_sample, found_abs_wksp_container, abs_wksp_sample, abs_wksp_container


# NOTE:
#  In order to use the decorator, we must have consistent naming
#  or kwargs as this is probably the most reliable way to get
#  the desired data piped in multiple location
#  -- bare minimum signaure of the function
#    func(wksp_name: str, abs_method:str, cache_dir="")
def abs_cache(func):
    """decorator to make the caching process easier

    NOTE: this decorator should only be used on function calls where
          - the first positional arguments is the workspace name
          - the second positional arguments is the absorption calculation method

    WARNING: currently this decorator should only be used on
                calc_absorption_corr_using_wksp

    example:
    without caching:
        SNSPowderReduction successful, Duration 5 minutes 53.54 seconds
    with caching (disk):
        SNSPowderReduction successful, Duration 1 minutes 14.18 seconds
    Speedup by
        4.7660x
    """
    @wraps(func)
    def inner(*args, **kwargs):
        """
        How caching name works
        """
        # unpack key arguments
        wksp_name = args[0]
        abs_method = args[1]
        cache_dir = kwargs.get("cache_dir", "")
        prefix_name = kwargs.get("prefix_name", "")

        # prompt return if no cache_dir specified
        if cache_dir == "":
            return func(*args, **kwargs)

        # step_1: generate the SHA1 and cachefile name
        #         baseon given kwargs
        cache_prefix = __get_instrument_name(wksp_name)
        cache_filename, signature = __get_cache_name(wksp_name,
                                                     abs_method,
                                                     cache_dir=cache_dir,
                                                     prefix_name=cache_prefix)

        # step_2: try load the cached data from disk
        found_sample, found_container, abs_wksp_sample, abs_wksp_container = __load_cached_data(
            cache_filename,
            signature,
            abs_method=abs_method,
            prefix_name=prefix_name,
        )

        # step_3: calculation
        if (abs_method == "SampleOnly") and found_sample:
            return abs_wksp_sample, ""
        else:
            if found_sample and found_container:
                # cache is available in memory now, skip calculation
                return abs_wksp_sample, abs_wksp_container
            else:
                # no cache found, need calculation
                abs_wksp_sample, abs_wksp_container = func(*args, **kwargs)

                # set SHA1 to workspace
                mtd[abs_wksp_sample].mutableRun()["absSHA1"] = signature
                if abs_wksp_container != "":
                    mtd[abs_wksp_container].mutableRun()["absSHA1"] = signature

                # save to disk
                SaveNexusProcessed(InputWorkspace=abs_wksp_sample, Filename=cache_filename)
                if abs_wksp_container != "":
                    SaveNexusProcessed(InputWorkspace=abs_wksp_container,
                                       Filename=cache_filename,
                                       Append=True)

                return abs_wksp_sample, abs_wksp_container

    return inner


# ----------------------------- #
# ---- Core functionality ----- #
# ------------------------------#
def calculate_absorption_correction(
    filename,
    abs_method,
    props,
    sample_formula,
    mass_density,
    number_density=Property.EMPTY_DBL,
    container_shape="PAC06",
    num_wl_bins=1000,
    element_size=1,
    metaws=None,
    cache_dir="",
):
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
    :param cache_dir: cache directory for storing cached absorption correction workspace
    :param prefix: How the prefix of cache file is determined - FILENAME to use file, or SHA prefix

    :return:
        Two workspaces (A_s, A_c) names
    """
    if abs_method == "None":
        return None, None

    material = {"ChemicalFormula": sample_formula, "SampleMassDensity": mass_density}

    if number_density != Property.EMPTY_DBL:
        material["SampleNumberDensity"] = number_density

    environment = {'Name': 'InAir', 'Container': container_shape}

    donorWS = create_absorption_input(filename,
                                      props,
                                      num_wl_bins,
                                      material=material,
                                      environment=environment,
                                      metaws=metaws)

    # NOTE: Ideally we want to separate cache related task from calculation,
    #       but the fact that we are trying to determine the name based on
    #       caching types requires us to use part of the caching here.
    #       Not a clean design, but it is unavoidable given that we are
    #       mixing caching into the regular routine from start.
    # NOTE: Cache will always use sha in both workspace name and cache filename
    #       Examples
    #
    #       PG3_11111.nxs with cache_dir=/tmp and abs_method="SampleOnly"
    #       -------------------------------------------------------------
    #       absName = PG3_sha1_abs_correction
    #       cachefilename = /tmp/PG3_sha1.nxs
    #       sampleWorkspace = PG3_sha1_abs_correction_ass
    #
    #       PG3_11111.nxs with cache_dir="" and abs_method="SampleOnly"
    #       -----------------------------------------------------------
    #       absName = PG3_11111_abs_correction
    #       sampleWorkspace = PG3_11111_abs_correction_ass
    if cache_dir != "":
        cache_prefix = __get_instrument_name(donorWS)
        _, sha = __get_cache_name(donorWS, abs_method, cache_dir, cache_prefix)
        absName = f"{cache_prefix}_{sha}_abs_correction"
    else:
        absName = f"{_getBasename(filename)}_abs_correction"

    return calc_absorption_corr_using_wksp(donorWS,
                                           abs_method,
                                           element_size,
                                           prefix_name=absName,
                                           cache_dir=cache_dir)


@abs_cache
def calc_absorption_corr_using_wksp(
    donor_wksp,
    abs_method,
    element_size=1,
    prefix_name="",
    cache_dir="",
):
    """
    Calculates absorption correction on the specified donor workspace. See the documentation
    for the ``calculate_absorption_correction`` function above for more details.

    :param donor_wksp: Input workspace to compute absorption correction on
    :param abs_method: Type of absorption correction: None, SampleOnly, SampleAndContainer, FullPaalmanPings
    :param element_size: Size of one side of the integration element cube in mm
    :param prefix_name: Optional prefix of the output workspaces, default is the donor_wksp name.
    :param cache_dir: Cache directory to store cached abs workspace.

    :return: Two workspaces (A_s, A_c), the first for the sample and the second for the container
    """
    log = Logger('calc_absorption_corr_using_wksp')
    if cache_dir != "":
        log.information(f"Storing cached data in {cache_dir}")

    if abs_method == "None":
        return "", ""

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
        return absName + '_ass', ""
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
        raise ValueError("Unrecognized absorption correction method '{}'".format(abs_method))


def create_absorption_input(
    filename,
    props,
    num_wl_bins=1000,
    material=None,
    geometry=None,
    environment=None,
    opt_wl_min=0,
    opt_wl_max=Property.EMPTY_DBL,
    metaws=None,
):
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
        allowed_log = " ".join([
            'SampleFormula', 'SampleDensity', "BL11A:CS:ITEMS:HeightInContainerUnits",
            "SampleContainer"
        ])
        Load(Filename=filename, OutputWorkspace=absName, MetaDataOnly=True, AllowList=allowed_log)

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
            PreprocessDetectorsToMD(InputWorkspace=absName,
                                    OutputWorkspace=absName + '_dets',
                                    GetMaskState=False)
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

    absorptionWS = WorkspaceFactory.create(mtd[absName],
                                           NVectors=mtd[absName].getNumberHistograms(),
                                           XLength=num_wl_bins + 1,
                                           YLength=num_wl_bins)
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
        if ("SampleMassDensity" not in material
                or not material['SampleMassDensity']) and ("SampleDensity" in absorptionWS.run()):
            if (absorptionWS.run()['SampleDensity'].lastValue() !=
                    1.0) and (absorptionWS.run()['SampleDensity'].lastValue() != 0.0):
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
                raise ValueError(
                    "HeightInContainerUnits expects cm or mm; specified units not recognized: ",
                    absorptionWS.run()['BL11A:CS:ITEMS:HeightInContainerUnits'].lastValue())

            geometry['Height'] = absorptionWS.run()['BL11A:CS:ITEMS:HeightInContainer'].lastValue(
            ) * conversion

    # Set container if not set
    if environment is not None:
        if environment['Container'] == "":
            environment['Container'] = absorptionWS.run()['SampleContainer'].lastValue().replace(
                " ", "")

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

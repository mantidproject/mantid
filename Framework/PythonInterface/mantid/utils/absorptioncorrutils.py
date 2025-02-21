# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.kernel import Logger, Property, PropertyManager
from mantid.simpleapi import (
    AbsorptionCorrection,
    DefineGaugeVolume,
    DeleteWorkspace,
    Divide,
    Load,
    Multiply,
    MultipleScatteringCorrection,
    PaalmanPingsAbsorptionCorrection,
    PreprocessDetectorsToMD,
    RenameWorkspace,
    SaveNexusProcessed,
    UnGroupWorkspace,
    mtd,
)
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
    if isinstance(filename, list):
        filename = filename[0]
    name = os.path.split(filename)[-1]
    for extension in _EXTENSIONS_NXS:
        name = name.replace(extension, "")
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


def __get_cache_name(
    meta_wksp_name,
    abs_method,
    cache_dirs=[],
    prefix_name="",
    ms_method="",
):
    """generate candidate cachefile names (full paths) and associated ascii hash

    :param meta_wksp_name: name of workspace contains relevant meta data for hashing
    :param abs_method: method used to perform the absorption calculation (mandatory)
    :param cache_dirs: cache directories to scan/load cache data
    :param prefix_name: prefix to add to wkspname for caching
    :param ms_method: method used to perform multiple scattering correction (optional)

    return cache_filenames: full paths to candidate cache files
           ascii_hash: MD5 value based on selected property

    IMPORTANT:
        multiple scattering correction is optional whereas absorption
        correction mandatory.
    """
    # grab the workspace
    if meta_wksp_name in mtd:
        ws = mtd[meta_wksp_name]
    else:
        raise ValueError(f"Cannot find workspace {meta_wksp_name} to extract meta data for hashing, aborting")

    # requires cache_dir
    cache_filenames = []
    if cache_dirs:
        # generate the property string for hashing
        try:
            height_val_tmp = ws.run().getTimeAveragedValue("BL11A:CS:ITEMS:HeightInContainer")
            height_unit_tmp = ws.run()["BL11A:CS:ITEMS:HeightInContainerUnits"].lastValue()
        except RuntimeError as e:
            raise RuntimeError("Currently only configured for POWGEN") from e

        property_string = [
            f"{key}={val}"
            for key, val in {
                "wavelength_min": ws.readX(0).min(),
                "wavelength_max": ws.readX(0).max(),
                "num_wl_bins": len(ws.readX(0)) - 1,
                "sample_formula": ws.run()["SampleFormula"].lastValue().strip(),
                "mass_density": ws.run()["SampleDensity"].lastValue(),
                "height_unit": height_unit_tmp,
                "height": height_val_tmp,
                "sample_container": ws.run()["SampleContainer"].lastValue().replace(" ", ""),
                "abs_method": abs_method,
                "ms_method": ms_method,
            }.items()
        ]

        # use mantid build-in alg to generate the cache filename and sha1
        ascii_hash = ""
        for cache_dir in cache_dirs:
            ascii_name, ascii_hash = mantid.simpleapi.CreateCacheFilename(
                Prefix=prefix_name, OtherProperties=property_string, CacheDir=cache_dir
            )

            cache_filenames.append(ascii_name)

    return cache_filenames, ascii_hash


def __load_cached_data(cache_files, sha1, abs_method="", prefix_name=""):
    """try to load cached data from memory and disk

    :param abs_method: absorption calculation method
    :param sha1: SHA1 that identify cached workspace
    :param cache_files: list of cache file names to search
    :param prefix_name: prefix to add to wkspname for caching

    return  found_abs_wksp_sample, found_abs_wksp_container
            abs_wksp_sample, abs_wksp_container, cache_files[ 0 ]
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
        for candidate in cache_files:
            if os.path.exists(candidate):
                wsntmp = "tmpwsg"
                Load(Filename=candidate, OutputWorkspace=wsntmp)
                wstype = mtd[wsntmp].id()
                if wstype == "Workspace2D":
                    RenameWorkspace(InputWorkspace=wsntmp, OutputWorkspace=abs_wksp_sample)
                elif wstype == "WorkspaceGroup":
                    UnGroupWorkspace(InputWorkspace=wsntmp)
                else:
                    raise ValueError(f"Unsupported cached workspace type: {wstype}")
                break

    # step_3: check memory again
    if mtd.doesExist(abs_wksp_sample):
        found_abs_wksp_sample = mtd[abs_wksp_sample].run()["absSHA1"].value == sha1
    if mtd.doesExist(abs_wksp_container):
        found_abs_wksp_container = mtd[abs_wksp_container].run()["absSHA1"].value == sha1

    return found_abs_wksp_sample, found_abs_wksp_container, abs_wksp_sample, abs_wksp_container, cache_files[0]


# NOTE:
#  In order to use the decorator, we must have consistent naming
#  or kwargs as this is probably the most reliable way to get
#  the desired data piped in multiple location
#  -- bare minimum signature of the function
#    func(wksp_name: str, abs_method:str, cache_dir="", prefix_name="", ms_method="")
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
        cache_dirs = kwargs.get("cache_dirs", [])
        prefix_name = kwargs.get("prefix_name", "")
        ms_method = kwargs.get("ms_method", "")

        # prompt return if no cache_dirs specified
        if len(cache_dirs) == 0:
            return func(*args, **kwargs)

        # step_1: generate the SHA1 and cachefile name
        #         baseon given kwargs
        cache_prefix = __get_instrument_name(wksp_name)

        cache_filenames, ascii_hash = __get_cache_name(
            wksp_name,
            abs_method,
            cache_dirs=cache_dirs,
            prefix_name=cache_prefix,
            ms_method=ms_method,
        )

        # step_2: try load the cached data from disk
        found_sample, found_container, abs_wksp_sample, abs_wksp_container, cache_filename = __load_cached_data(
            cache_filenames,
            ascii_hash,
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
                log = Logger("calc_absorption_corr_using_wksp")
                if cache_filename:
                    log.information(f"Storing cached data in {cache_filename}")

                abs_wksp_sample, abs_wksp_container = func(*args, **kwargs)

                # set SHA1 to workspace
                mtd[abs_wksp_sample].mutableRun()["absSHA1"] = ascii_hash
                if abs_wksp_container != "":
                    mtd[abs_wksp_container].mutableRun()["absSHA1"] = ascii_hash

                # save to disk
                SaveNexusProcessed(InputWorkspace=abs_wksp_sample, Filename=cache_filename)
                if abs_wksp_container != "":
                    SaveNexusProcessed(InputWorkspace=abs_wksp_container, Filename=cache_filename, Append=True)

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
    sample_geometry={},
    can_geometry={},
    can_material={},
    gauge_vol="",
    beam_height=Property.EMPTY_DBL,
    number_density=Property.EMPTY_DBL,
    container_shape="PAC06",
    num_wl_bins=1000,
    element_size=1,
    metaws=None,
    cache_dirs=[],
    ms_method="",
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
    :param sample_geometry: Dictionary to specify the sample geometry for absorption correction
    :param can_geometry: Dictionary to specify the container geometry for absorption correction
    :param can_material: Dictionary to specify the container material for absorption correction
    :param gauge_vol: String in XML form to define the gauge volume, i.e., the sample portion visible to the beam
    :param beam_height: Optional beam height to use for absorption correction
    :param number_density: Optional number density of sample to be added to the Material for absorption correction
    :param container_shape: Shape definition of container, such as PAC06.
    :param num_wl_bins: Number of bins for calculating wavelength
    :param element_size: Size of one side of the integration element cube in mm
    :param metaws: Optional workspace containing metadata to use instead of reading from filename
    :param cache_dirs: list of cache directories for storing cached absorption correction workspace
    :param prefix: How the prefix of cache file is determined - FILENAME to use file, or SHA prefix
    :param ms_method: Method to use for multiple scattering correction

    :return:
        Two workspaces (A_s, A_c) names
    """
    if abs_method == "None":
        return None, None

    material = {"ChemicalFormula": sample_formula, "SampleMassDensity": mass_density}

    if number_density != Property.EMPTY_DBL:
        material["SampleNumberDensity"] = number_density

    environment = {}
    find_env = True
    if container_shape or (can_geometry and can_material):
        environment["Name"] = "InAir"
        find_env = False
        if not (can_geometry and can_material):
            environment["Container"] = container_shape

    donorWS = create_absorption_input(
        filename,
        props,
        num_wl_bins,
        material=material,
        geometry=sample_geometry,
        can_geometry=can_geometry,
        can_material=can_material,
        gauge_vol=gauge_vol,
        beam_height=beam_height,
        environment=environment,
        find_environment=find_env,
        metaws=metaws,
    )

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
    if cache_dirs:
        cache_prefix = __get_instrument_name(donorWS)
        _, sha = __get_cache_name(donorWS, abs_method, cache_dirs, cache_prefix)
        absName = f"{cache_prefix}_{sha}_abs_correction"
    else:
        absName = f"{_getBasename(filename)}_abs_correction"

    return calc_absorption_corr_using_wksp(
        donorWS,
        abs_method,
        element_size,
        prefix_name=absName,
        cache_dirs=cache_dirs,
        ms_method=ms_method,
    )


@abs_cache
def calc_absorption_corr_using_wksp(
    donor_wksp,
    abs_method,
    element_size=1,
    prefix_name="",
    cache_dirs=[],
    ms_method="",
):
    # warn about caching
    log = Logger("CalcAbsorptionCorrUsingWksp")
    if cache_dirs:
        log.warning("Empty cache dir found.")
    # 1. calculate first order absorption correction
    abs_s, abs_c = calc_1st_absorption_corr_using_wksp(donor_wksp, abs_method, element_size, prefix_name)
    # 2. calculate 2nd order absorption correction
    if ms_method in ["", None, "None"]:
        log.information("Skip multiple scattering correction as instructed.")
    else:
        MultipleScatteringCorrection(
            InputWorkspace=donor_wksp,
            ElementSize=element_size,
            method=ms_method,
            OutputWorkspace="ms_tmp",
        )
        if ms_method == "SampleOnly":
            ms_sampleOnly = mtd["ms_tmp_sampleOnly"]
            ms_sampleOnly = 1 - ms_sampleOnly
            # abs_s now point to the effective absorption correction
            # A = A / (1 - ms_s)
            Divide(
                LHSWorkspace=abs_s,  # str
                RHSWorkspace=ms_sampleOnly,  # workspace
                OutputWorkspace=abs_s,  # str
            )
            # nothing need to be done for container
            # cleanup
            mtd.remove("ms_tmp_sampleOnly")
        elif ms_method == "SampleAndContainer":
            ms_sampleAndContainer = mtd["ms_tmp_sampleAndContainer"]
            ms_sampleAndContainer = 1 - ms_sampleAndContainer
            Divide(
                LHSWorkspace=abs_s,  # str
                RHSWorkspace=ms_sampleAndContainer,  # workspace
                OutputWorkspace=abs_s,  # str
            )
            mtd.remove("ms_tmp_sampleAndContainer")
            ms_containerOnly = mtd["ms_tmp_containerOnly"]
            ms_containerOnly = 1 - ms_containerOnly
            Divide(
                LHSWorkspace=abs_c,  # str
                RHSWorkspace=ms_containerOnly,  # workspace
                OutputWorkspace=abs_c,  # str
            )
            mtd.remove("ms_tmp_containerOnly")
        else:
            log.warning(f"Multiple scattering method {ms_method} not supported, skipping.")

    return abs_s, abs_c


def calc_1st_absorption_corr_using_wksp(
    donor_wksp,
    abs_method,
    element_size=1,
    prefix_name="",
):
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
        return "", ""

    if isinstance(donor_wksp, str):
        if not mtd.doesExist(donor_wksp):
            raise RuntimeError("Specified donor workspace not found in the ADS")
        donor_wksp = mtd[donor_wksp]

    absName = donor_wksp.name()
    if prefix_name != "":
        absName = prefix_name

    if abs_method == "SampleOnly":
        AbsorptionCorrection(donor_wksp, OutputWorkspace=absName + "_ass", ScatterFrom="Sample", ElementSize=element_size)
        return absName + "_ass", ""
    elif abs_method == "SampleAndContainer":
        AbsorptionCorrection(donor_wksp, OutputWorkspace=absName + "_ass", ScatterFrom="Sample", ElementSize=element_size)
        AbsorptionCorrection(donor_wksp, OutputWorkspace=absName + "_acc", ScatterFrom="Container", ElementSize=element_size)
        return absName + "_ass", absName + "_acc"
    elif abs_method == "FullPaalmanPings":
        PaalmanPingsAbsorptionCorrection(donor_wksp, OutputWorkspace=absName, ElementSize=element_size)
        Multiply(LHSWorkspace=absName + "_acc", RHSWorkspace=absName + "_assc", OutputWorkspace=absName + "_ac")
        Divide(LHSWorkspace=absName + "_ac", RHSWorkspace=absName + "_acsc", OutputWorkspace=absName + "_ac")
        return absName + "_assc", absName + "_ac"
    else:
        raise ValueError("Unrecognized absorption correction method '{}'".format(abs_method))


def create_absorption_input(
    filename,
    props=None,
    num_wl_bins=1000,
    material={},
    geometry={},
    can_geometry={},
    can_material={},
    gauge_vol="",
    beam_height=Property.EMPTY_DBL,
    environment={},
    find_environment=True,
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
    :param can_geometry: Optional container geometry to use in SetSample
    :param can_material: Optional container material to use in SetSample
    :param gauge_vol: Optional gauge volume definition, i.e., sample portion visible to the beam.
    :param beam_height: Optional beam height to define gauge volume
    :param environment: Optional environment to use in SetSample
    :param find_environment: Optional find_environment to control whether to figure out environment automatically.
    :param opt_wl_min: Optional minimum wavelength. If specified, this is used instead of from the props
    :param opt_wl_max: Optional maximum wavelength. If specified, this is used instead of from the props
    :param metaws: Optional workspace name with metadata to use for donor workspace instead of reading from filename
    :return: Name of the donor workspace created
    """

    def confirmProps(props):
        """This function will throw an exception if the PropertyManager
        is not defined correctly. It should only be called if the value
        is needed."""
        if props is None:
            raise ValueError("props is required to create donor workspace, props is None")

        if not isinstance(props, PropertyManager):
            raise ValueError("props must be a PropertyManager object")

    log = Logger("CreateAbsorptionInput")

    # Load from file if no workspace with metadata has been given, otherwise avoid a duplicate load with the metaws
    absName = metaws
    if metaws is None:
        absName = "__{}_abs".format(_getBasename(filename))
        allowed_log = ",".join(["SampleFormula", "SampleDensity", "BL11A:CS:ITEMS:HeightInContainerUnits", "SampleContainer", "SampleMass"])
        Load(Filename=filename, OutputWorkspace=absName, MetaDataOnly=True, AllowList=allowed_log)

    # attempt to get the wavelength from the function parameters
    if opt_wl_min > 0.0:
        wl_min = opt_wl_min
    else:
        # or get it from the PropertyManager
        confirmProps(props)
        wl_min = props["wavelength_min"].value
    if opt_wl_max != Property.EMPTY_DBL:
        wl_max = opt_wl_max
    else:
        # or get it from the PropertyManager
        confirmProps(props)
        wl_max = props["wavelength_max"].value  # unset value is 0.

    # if it isn't found by this point, guess it from the time-of-flight range
    if wl_min == 0.0 or wl_max == 0.0:
        confirmProps(props)
        tof_min = props["tof_min"].value
        tof_max = props["tof_max"].value
        if tof_min >= 0.0 and tof_max > tof_min:
            log.information("TOF range is {} to {} microseconds".format(tof_min, tof_max))

            # determine L1
            instr = mtd[absName].getInstrument()
            L1 = instr.getSource().getDistance(instr.getSample())
            # determine L2 range
            PreprocessDetectorsToMD(InputWorkspace=absName, OutputWorkspace=absName + "_dets", GetMaskState=False)
            L2 = mtd[absName + "_dets"].column("L2")
            Lmin = np.min(L2) + L1
            Lmax = np.max(L2) + L1
            DeleteWorkspace(Workspace=absName + "_dets")

            log.information("Distance range is {} to {} meters".format(Lmin, Lmax))

            # wavelength is h*TOF / m_n * L  values copied from Kernel/PhysicalConstants.h
            usec_to_sec = 1.0e-6
            meter_to_angstrom = 1.0e10
            h_m_n = meter_to_angstrom * usec_to_sec * 6.62606896e-34 / 1.674927211e-27
            if wl_min == 0.0:
                wl_min = h_m_n * tof_min / Lmax
            if wl_max == 0.0:
                wl_max = h_m_n * tof_max / Lmin

    # there isn't a good way to guess it so error out
    if wl_max <= wl_min:
        DeleteWorkspace(Workspace=absName)  # no longer needed
        raise RuntimeError("Invalid wavelength range min={}A max={}A".format(wl_min, wl_max))
    log.information("Using wavelength range min={}A max={}A".format(wl_min, wl_max))

    absorptionWS = WorkspaceFactory.create(
        mtd[absName], NVectors=mtd[absName].getNumberHistograms(), XLength=num_wl_bins + 1, YLength=num_wl_bins
    )
    xaxis = np.arange(0.0, float(num_wl_bins + 1)) * (wl_max - wl_min) / (num_wl_bins) + wl_min
    for i in range(absorptionWS.getNumberHistograms()):
        absorptionWS.setX(i, xaxis)
    absorptionWS.getAxis(0).setUnit("Wavelength")

    # this effectively deletes the metadata only workspace
    AnalysisDataService.addOrReplace(absName, absorptionWS)

    # cleanup inputs before delegating work
    if not material:
        material = {}
    if not geometry:
        geometry = {}
    if not environment:
        environment = {}

    # Make sure one is set before calling SetSample
    if material or geometry or environment:
        mantid.simpleapi.SetSampleFromLogs(
            InputWorkspace=absName,
            Material=material,
            Geometry=geometry,
            ContainerGeometry=can_geometry,
            ContainerMaterial=can_material,
            Environment=environment,
            FindEnvironment=find_environment,
        )

    if beam_height != Property.EMPTY_DBL and not gauge_vol:
        # If the gauge volume is not defined, use the beam height to define it,
        # and we will be assuming a cylinder shape of the sample.
        gauge_vol = """<cylinder id="shape">
            <centre-of-bottom-base r="{0:4.2F}" t="90.0" p="270.0" />
            <axis x="0.0" y="0.2" z="0.0" />
            <radius val="{1:4.2F}" />
            <height val="{2:4.2F}" />
            </cylinder>"""
        gauge_vol = gauge_vol.format(beam_height / 2.0, geometry["Radius"], beam_height)

    if gauge_vol:
        DefineGaugeVolume(absName, gauge_vol)

    return absName

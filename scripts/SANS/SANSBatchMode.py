# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
#
# SANSBatchMode.py
#
# M. Gigg - Based on Steve King's example

#
# For batch-reduction of LOQ or SANS2D runs in Mantid using a SINGLE set of instrument parameters (Q, wavelength, radius, etc)
#
# Reads multi-line input from file of the form:
# sample_sans   54431   sample_trans    54435   direct_beam 54433   can_sans    54432   can_trans  --> continued below
# 54434   direct_beam 54433   background_sans     background_trans        direct_beam     output_as   script54435.txt
#
# Assumes the following have already been set by either the SANS GUI or directly from python using the AssignSample, LimitsR etc commands
# instrument
# data directory
# data format
# user file directory
# user file name
# reduction type
# detector bank
# whether to use default wavelength range for transmissions

# Allows over-riding of following from user (mask) file:
# radial integration limits
# wavelength limits, bin size and binning
# q limits, q bin size and binning
# qxy limits, qxy bin size and binning

# The save directory must currently be specified in the Mantid.user.properties file

# Make the reduction module available
from ISISCommandInterface import *
import SANSUtility as su
from mantid.simpleapi import *
from mantid.api import WorkspaceGroup
from mantid.kernel import Logger
import copy
import re
from reduction_settings import REDUCTION_SETTINGS_OBJ_NAME
from isis_reduction_steps import UserFile

sanslog = Logger("SANS")
# The allowed number of entries per row.
# The minimum is 4:  sample_sans, sample_sans_VALUE,
#                    output_as, output_as_VALUE
# The maximum is 22: sample_sans, sample_sans_VALUE,
#                    sample_trans, sample_trans_VALUE,
#                    sample_direct_beam, sample_directr_beam_VALUE,
#                    can_sans, can_sans_VALUE,
#                    can_trans, can_trans_VALUE,
#                    can_direct_beam, can_direct_beam_VALUE
#                    output_as, output_as_VALUE
#                    user_file, user_file_VALUE
#                    background_sans, background_sans_VALUE,         # CURRENTLY NOT SUPPORTED
#                    background_trans, background_trans_VALUE,       # CURRENTLY NOT SUPPORTED
#                    background_direct_beam, background_trans_VALUE  # CURRENTLY NOT SUPPORTED

ALLOWED_NUM_ENTRIES = set([22, 20, 16, 14, 8, 6, 4])

# Build a dictionary of possible input data  keys
IN_FORMAT = {}
IN_FORMAT["sample_sans"] = ""
IN_FORMAT["sample_trans"] = ""
IN_FORMAT["sample_direct_beam"] = ""
IN_FORMAT["can_sans"] = ""
IN_FORMAT["can_trans"] = ""
IN_FORMAT["can_direct_beam"] = ""
# Backgrounds not yet implemented
#    IN_FORMAT['background_sans'] = 0
#    IN_FORMAT['background_trans'] = 0
#    IN_FORMAT['background_direct_beam'] = 0
IN_FORMAT["output_as"] = ""
IN_FORMAT["user_file"] = ""

# maps the run types above to the Python interface command to use to load it
COMMAND = {}
COMMAND["sample_sans"] = "AssignSample("
COMMAND["can_sans"] = "AssignCan("
COMMAND["sample_trans"] = "TransmissionSample("
COMMAND["can_trans"] = "TransmissionCan("


class SkipEntry(RuntimeError):
    pass


class SkipReduction(RuntimeError):
    pass


def addRunToStore(parts, run_store):
    """Add a CSV line to the input data store (run_store)
    @param parts: the parts of a CSV line
    @param run_store: Append info about CSV line
    """
    if "MANTID_BATCH_FILE" in parts:
        return 0

    # Check logical structure of line
    nparts = len(parts)
    if nparts not in ALLOWED_NUM_ENTRIES:
        return 1

    inputdata = copy.deepcopy(IN_FORMAT)
    nparts -= 1
    # move through the file like sample_sans,99630,output_as,99630, ...
    for i in range(0, nparts, 2):
        role = parts[i]
        if role in list(inputdata.keys()):
            inputdata[parts[i]] = parts[i + 1]
        else:
            issueWarning("You seem to have specified an invalid key in the SANSBatch file. The key was " + str(role))
        if "background" in role:
            issueWarning("Background runs are not yet implemented in Mantid! Will process Sample & Can only")

    run_store.append(inputdata)
    return 0


def get_transmission_properties(workspace):
    transmission_properties = dict()
    for prop in ["Transmission", "TransmissionCan"]:
        if workspace.getRun().hasProperty(prop):
            ws_name = workspace.getRun().getLogData(prop).value
            if mtd.doesExist(ws_name):  # ensure the workspace has not been deleted
                transmission_properties[prop] = workspace.getRun().getLogData(prop).value
    return transmission_properties


def get_geometry_properties(reducer):
    """
    This extracts geometry properties from the ReductionSingleton. They are saved in the CanSAS1D format.

    @param reducer: a handle to the redcution singleton
    @return: a dict with geometry properties
    """

    def _add_property(key, element, props):
        if element is None:
            raise RuntimeError("Could not extract value for {0}.".format(key))
        props.update({key: element})

    geometry_properties = {}
    geometry = reducer.get_sample().geometry

    # Get the shape
    shape = geometry.shape
    if shape == "cylinder-axis-up":
        shape_to_save = "Cylinder"
    elif shape == "cuboid":
        shape_to_save = "Flat plate"
    elif shape == "cylinder-axis-along":
        shape_to_save = "Disc"
    else:
        raise RuntimeError("Unknown shape {0}. Cannot extract property".format(shape))
    geometry_properties.update({"Geometry": shape_to_save})

    # Get the Height
    _add_property("SampleHeight", geometry.height, geometry_properties)

    # Get the Width
    _add_property("SampleWidth", geometry.width, geometry_properties)

    # Get the thickness
    _add_property("SampleThickness", geometry.thickness, geometry_properties)
    return geometry_properties


def BatchReduce(  # noqa: C901
    filename,
    format,
    plotresults=False,
    saveAlgs={"SaveRKH": "txt"},
    verbose=False,
    centreit=False,
    reducer=None,
    combineDet=None,
    save_as_zero_error_free=False,
):
    """
    @param filename: the CSV file with the list of runs to analyse
    @param format: type of file to load, nxs for Nexus, etc.
    @param plotresults: if true and this function is run from Mantid a graph will be created for the results of each reduction
    @param saveAlgs: this named algorithm will be passed the name of the results workspace and filename (default = 'SaveRKH').
        Pass a tuple of strings to save to multiple file formats
    @param verbose: set to true to write more information to the log (default=False)
    @param centreit: do centre finding (default=False)
    @param reducer: if to use the command line (default) or GUI reducer object
    @param combineDet: that will be forward to WavRangeReduction (rear, front, both, merged, None)
    @param save_as_zero_error_free: Should the reduced workspaces contain zero errors or not
    @return final_setings: A dictionary with some values of the Reduction - Right Now:(scale, shift)
    """
    if not format.startswith("."):
        format = "." + format

    # Read CSV file and store information in runinfo using format IN_FORMAT
    file_handle = open(filename, "r")
    runinfo = []
    for line in file_handle:
        # See how many pieces of information have been provided;
        # brackets delineate the field separator (nothing for space-delimited, ',' for comma-seperated)
        parts = line.rstrip().split(",")
        if addRunToStore(parts, runinfo) > 0:
            issueWarning('Incorrect structure detected in input file "' + filename + '" at line \n"' + line + '"\nEntry skipped\n')
    file_handle.close()

    if reducer:
        ReductionSingleton().replace(reducer)
    ins_name = ReductionSingleton().instrument.name()
    # is used for SaveCanSAS1D go give the detectors names
    detnames = ", ".join(ReductionSingleton().instrument.listDetectors())

    # LARMOR has just one detector, but, it defines two because ISISInstrument is defined as two banks! #8395
    if ins_name == "LARMOR":
        detnames = ReductionSingleton().instrument.cur_detector().name()

    scale_shift = {"scale": 1.0000, "shift": 0.0000}
    # first copy the user settings in case running the reductionsteps can change it
    settings = copy.deepcopy(ReductionSingleton().reference())
    prop_man_settings = ReductionSingleton().settings.clone("TEMP_SETTINGS")

    # Make a note of the original user file, as we want to set it
    original_user_file = ReductionSingleton().user_settings.filename
    current_user_file = original_user_file

    # Store the original combineDet which was set either by the input. this should be used whenever we are using the
    # original user file
    original_combine_det = combineDet

    # Now loop over all the lines and do a reduction (hopefully) for each
    for run in runinfo:
        # Set the user file, if it is required
        try:
            current_user_file = setUserFileInBatchMode(
                new_user_file=run["user_file"],
                current_user_file=current_user_file,
                original_user_file=original_user_file,
                original_settings=settings,
                original_prop_man_settings=prop_man_settings,
            )

            if current_user_file == original_user_file:
                combineDet = original_combine_det
            else:
                # When we set a new user file, that means that the combineDet feature could be invalid,
                # ie if the detector under investigation changed in the user file. We need to change this
                # here too. But only if it is not None.
                if combineDet is not None:
                    new_combineDet = ReductionSingleton().instrument.get_detector_selection()
                    combineDet = su.get_correct_combinDet_setting(ins_name, new_combineDet)
        except (RuntimeError, ValueError) as e:
            raise RuntimeError(
                "Error in Batchmode user files: Could not reset the specified user file {0}. More info: {1}".format(
                    str(run["user_file"]), str(e)
                )
            )

        local_settings = copy.deepcopy(ReductionSingleton().reference())
        local_prop_man_settings = ReductionSingleton().settings.clone("TEMP_SETTINGS")

        raw_workspaces = []
        geometry_properties = {}
        try:
            # Load in the sample runs specified in the csv file
            raw_workspaces.append(read_run(run, "sample_sans", format))

            # Transmission runs to be applied to the sample
            raw_workspaces += read_trans_runs(run, "sample", format)

            # Can run
            raw_workspaces.append(read_run(run, "can_sans", format))

            # Transmission runs for the can
            raw_workspaces += read_trans_runs(run, "can", format)

            if centreit == 1:
                if verbose == 1:
                    FindBeamCentre(50.0, 170.0, 12)

            try:
                geometry_properties = get_geometry_properties(ReductionSingleton())
            except RuntimeError as e:
                message = "Could not extract geometry properties from the reducer: {0}".format(str(e))
                sanslog.warning(message)

            # WavRangeReduction runs the reduction for the specified
            # wavelength range where the final argument can either be
            # DefaultTrans or CalcTrans:
            reduced = WavRangeReduction(combineDet=combineDet, out_fit_settings=scale_shift)

        except SkipEntry as reason:
            # this means that a load step failed, the warning and the fact that the results aren't there is enough for the user
            issueWarning(str(reason) + ", skipping entry")
            continue
        except SkipReduction as reason:
            # this means that a load step failed, the warning and the fact that the results aren't there is enough for the user
            issueWarning(str(reason) + ", skipping reduction")
            continue
        except ValueError as reason:
            issueWarning("Cannot load file :" + str(reason))
            raise
        finally:
            delete_workspaces(raw_workspaces)

        if verbose:
            sanslog.notice(createColetteScript(run, format, reduced, centreit, plotresults, filename))
        # Rename the final workspace
        final_name = run["output_as"].strip()
        if final_name == "":
            final_name = reduced

        # Remove illegal characters
        final_name = sanitize_name(final_name)

        # convert the names from the default one, to the agreement
        # This caused a renaming with the following logic
        # | combinDet      |    Name HAB    |   Name LAB   | Name Merged  |
        # | rear           |    +_rear      |     -        |     -        |
        # | front          |      -         |    +_front   |     -        |
        # | both           |    +_rear      |    +_front   |     -        |
        # | merged         |    +_rear      |    +_front   |     +_merged |
        # This is not great since it uses SANS2D terminology for all instruments
        names = [final_name]
        if combineDet == "rear":
            new_name = su.rename_workspace_correctly(ins_name, su.ReducedType.LAB, final_name, reduced)
            names = [new_name]
        elif combineDet == "front":
            new_name = su.rename_workspace_correctly(ins_name, su.ReducedType.HAB, final_name, reduced)
            names = [new_name]
        elif combineDet == "both":
            if ins_name == "SANS2D":
                rear_reduced = reduced.replace("front", "rear")
            else:  # if ins_name == 'lOQ':
                rear_reduced = reduced.replace("HAB", "main")
            new_name_HAB = su.rename_workspace_correctly(ins_name, su.ReducedType.HAB, final_name, reduced)
            new_name_LAB = su.rename_workspace_correctly(ins_name, su.ReducedType.LAB, final_name, rear_reduced)
            names = [new_name_HAB, new_name_LAB]
        elif combineDet == "merged":
            if ins_name == "SANS2D":
                rear_reduced = reduced.replace("merged", "rear")
                front_reduced = reduced.replace("merged", "front")
            else:
                rear_reduced = reduced.replace("merged", "main")
                front_reduced = rear_reduced.replace("main", "HAB")
            new_name_Merged = su.rename_workspace_correctly(ins_name, su.ReducedType.Merged, final_name, reduced)
            new_name_LAB = su.rename_workspace_correctly(ins_name, su.ReducedType.LAB, final_name, rear_reduced)
            new_name_HAB = su.rename_workspace_correctly(ins_name, su.ReducedType.HAB, final_name, front_reduced)
            names = [new_name_Merged, new_name_LAB, new_name_HAB]
        else:
            RenameWorkspace(InputWorkspace=reduced, OutputWorkspace=final_name)

        file = run["output_as"]
        # saving if optional and doesn't happen if the result workspace is left blank. Is this feature used?
        if file:
            save_names = []
            for n in names:
                w = mtd[n]
                if isinstance(w, WorkspaceGroup):
                    save_names.extend(w.getNames())
                else:
                    save_names.append(n)

            # If we want to remove zero-errors, we map the original workspace to a cleaned workspace clone,
            # else we map it to itself.
            save_names_dict = get_mapped_workspaces(save_names, save_as_zero_error_free)

            for algor in list(saveAlgs.keys()):
                for workspace_name in save_names:
                    # add the file extension, important when saving different types of file so they don't over-write each other
                    ext = saveAlgs[algor]
                    if not ext.startswith("."):
                        ext = "." + ext
                    if algor == "SaveCanSAS1D":
                        # From v2, SaveCanSAS1D is able to save the Transmission workspaces related to the
                        # reduced data. The name of workspaces of the Transmission are available at the
                        # sample logs.
                        _ws = mtd[workspace_name]
                        transmission_properties = get_transmission_properties(_ws)

                        # Add the geometry properties if they exist
                        if geometry_properties:
                            transmission_properties.update(geometry_properties)

                        # Call the SaveCanSAS1D with the Transmission and TransmissionCan if they are
                        # available
                        SaveCanSAS1D(
                            save_names_dict[workspace_name], workspace_name + ext, DetectorNames=detnames, **transmission_properties
                        )
                    elif algor == "SaveNXcanSAS":
                        _ws = mtd[workspace_name]
                        transmission_properties = get_transmission_properties(_ws)
                        # Call the SaveNXcanSAS with the Transmission and TransmissionCan if they are
                        # available
                        SaveNXcanSAS(
                            save_names_dict[workspace_name], workspace_name + ext, DetectorNames=detnames, **transmission_properties
                        )
                    elif algor == "SaveRKH":
                        SaveRKH(save_names_dict[workspace_name], workspace_name + ext, Append=False)
                    else:
                        exec(algor + "('" + save_names_dict[workspace_name] + "', workspace_name+ext)")
            # If we performed a zero-error correction, then we should get rid of the cloned workspaces
            if save_as_zero_error_free:
                delete_cloned_workspaces(save_names_dict)

        if plotresults == 1:
            for final_name in names:
                PlotResult(final_name)

        # the call to WaveRang... killed the reducer so copy back over the settings
        ReductionSingleton().replace(copy.deepcopy(local_settings))
        ReductionSingleton().settings = local_prop_man_settings.clone(REDUCTION_SETTINGS_OBJ_NAME)

    # Set everything back to the initial state
    ReductionSingleton().replace(copy.deepcopy(settings))
    ReductionSingleton().settings = prop_man_settings.clone(REDUCTION_SETTINGS_OBJ_NAME)

    # end of reduction of all entries of batch file
    return scale_shift


def parse_run(run_num, ext):
    """
    Extracts an (optional) period specification from the run_num
    and then adds the extension
    @param run_num: run number with optional period specified a after the letter 'p'
    @param ext: file extension
    @return: run specification (number.extension), period
    """
    if not run_num:
        return "", -1
    parts = re.split("[pP]", run_num)
    if len(parts) > 2:
        raise RuntimeError('Problem reading run number "' + run_num + '"')
    run_spec = parts[0] + ext

    if len(parts) == 2:
        period = parts[1]
    else:
        period = -1

    return run_spec, period


def read_run(runs, run_role, format):
    """
    Load a run specified in the CSV file
    @param runs: a line from a CSV file
    @param run_role: type of run, e.g. sample
    @param format: extension to add to the end of the run number specification
    @return: name of the workspace that was loaded
    @throw SkipReduction: if there is a problem with the line that means a reduction is impossible
    @throw SkipEntry: if the sample is entry is empty
    """
    run_file = runs[run_role]
    if len(run_file) == 0:
        if run_role == "sample_sans":
            raise SkipEntry("Empty " + run_role + " run given")
        else:
            # only the sample is a required run
            return

    run_file, period = parse_run(run_file, format)
    run_ws = eval(COMMAND[run_role] + "run_file, period=period)")
    if not run_ws:
        raise SkipReduction("Cannot load " + run_role + ' run "' + run_file + '"')

    # AssignCan and AssignSample will change signature for: ws_name = AssignCan
    if isinstance(run_ws, tuple):
        return run_ws[0]
    return run_ws


def read_trans_runs(runs, sample_or_can, format):
    """
    Loads the transmission runs to either be applied to the sample or the can.
    There must be two runs
    @param runs: a line from a CSV file
    @param sample_or_can: a string with the name of the set of transmission runs to use, e.g. can
    @param format: extension to add to the end of the run number specifications
    @return: names of the two workspaces that were loaded
    @throw SkipReduction: if there is a problem with the line that means a reduction is impossible
    """
    role1 = sample_or_can + "_trans"
    role2 = sample_or_can + "_direct_beam"

    run_file1, p1 = parse_run(runs[role1], format)
    run_file2, p2 = parse_run(runs[role2], format)
    if (not run_file1) and (not run_file2):
        # it is OK for transmission files not to be present
        return []

    ws1, ws2 = eval(COMMAND[role1] + "run_file1, run_file2, period_t=p1, period_d=p2)")
    if len(run_file1) > 0 and len(ws1) == 0:
        raise SkipReduction("Cannot load " + role1 + ' run "' + run_file1 + '"')
    if len(run_file2) > 0 and len(ws2) == 0:
        raise SkipReduction("Cannot load " + role2 + ' run "' + run_file2 + '"')
    return [ws1, ws2]


def delete_workspaces(workspaces):
    """
    Delete the list of workspaces if possible but fail siliently if there is
    a problem
    @param workspaces: the list to delete
    """
    if not isinstance(workspaces, type(list())):
        if not isinstance(workspaces, type(tuple())):
            workspaces = [workspaces]

    for wksp in workspaces:
        if wksp and wksp in mtd:
            try:
                DeleteWorkspace(Workspace=wksp)
            except:
                # we're only deleting to save memory, if the workspace really won't delete leave it
                pass


def get_mapped_workspaces(save_names, save_as_zero_error_free):
    """
    Get a workspace name map, which maps from the original
    workspace to a zero-error-free cloned workspace if
    save_as_zero_error_free is checked otherwise the
    workspaces are mapped to themselves.
    @param save_names: a list of workspace names
    @param save_as_zero_error_free : if the user wants the zero-errors removed
    @returns a map of workspaces
    """
    workspace_dictionary = {}
    for name in save_names:
        if save_as_zero_error_free:
            cloned_name = name + "_cloned_temp"
            dummy_message, complete = su.create_zero_error_free_workspace(input_workspace_name=name, output_workspace_name=cloned_name)
            if complete:
                workspace_dictionary[name] = cloned_name
            else:
                workspace_dictionary[name] = name
        else:
            workspace_dictionary[name] = name
    return workspace_dictionary


def delete_cloned_workspaces(save_names_dict):
    """
    If there are cloned workspaces in the worksapce map, then they are deleted
    @param save_names_dict: a workspace name map
    """
    to_delete = []
    for key in save_names_dict:
        if key != save_names_dict[key]:
            to_delete.append(save_names_dict[key])
    for element in to_delete:
        su.delete_zero_error_free_workspace(input_workspace_name=element)


def setUserFileInBatchMode(new_user_file, current_user_file, original_user_file, original_settings, original_prop_man_settings):
    """
    Loads a specified user file. The file is loaded if
    new_user_file is different from  current_user_file. Else we just
    keep the user file loaded. If the new_user_file is empty then we default to
    original_user_file.
    @param new_user_file: The new user file. Note that this can be merely the filename (+ extension)
    @param current_user_file: The currently loaded user file
    @param original_user_file: The originally loaded user file. This is used as a default
    @param original_settings: The original reducer
    @param original_prop_man_settings: Original properties settings
    """
    user_file_to_set = ""

    if new_user_file == "":
        user_file_to_set = original_user_file
    else:
        # Try to find the user file in the default paths
        if not os.path.isfile(new_user_file):
            # Find the user file in the Mantid path. we make sure that the user file has a txt extension.
            user_file_names_with_extension = su.get_user_file_name_options_with_txt_extension(new_user_file)
            for user_file_name_with_extension in user_file_names_with_extension:
                user_file = FileFinder.getFullPath(user_file_name_with_extension)
                if user_file:
                    break

            if not os.path.isfile(user_file):
                message = "Error could not find specified user file {0}".format(new_user_file)
                raise RuntimeError(message)
            else:
                user_file_to_set = user_file
        else:
            user_file_to_set = new_user_file

    # Set the user file in the reducer and load the user file
    if os.path.normpath(user_file_to_set) != os.path.normpath(current_user_file):
        # Need to setup a clean reducer. If we are dealing with the original user file,
        # then we should take gui changes into account, ie reset to the original reducer
        if user_file_to_set == original_user_file:
            ReductionSingleton().replace(copy.deepcopy(original_settings))
            ReductionSingleton().settings = original_prop_man_settings.clone(REDUCTION_SETTINGS_OBJ_NAME)
        else:
            instrument = copy.deepcopy(ReductionSingleton().get_instrument())
            output_type = ReductionSingleton().to_Q.output_type
            ReductionSingleton().clean(isis_reducer.ISISReducer)
            ReductionSingleton().set_instrument(instrument)
            ReductionSingleton().user_settings = UserFile(user_file_to_set)
            ReductionSingleton().to_Q.output_type = output_type
            ReductionSingleton().user_settings.execute(ReductionSingleton())
        current_user_file = user_file_to_set
    return current_user_file


def sanitize_name(filename):
    # Users have used invalid characters for the output file in the past. We need to
    # catch them and remove them. We are fairly restrictive here, but this should
    # ensure that most user selections are supported
    keepcharacters = (" ", ".", "_")
    return "".join(c for c in filename if c.isalnum() or c in keepcharacters).rstrip()

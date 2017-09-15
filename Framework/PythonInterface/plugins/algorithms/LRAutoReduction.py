#pylint: disable=no-init, invalid-name, no-self-use, attribute-defined-outside-init
"""
    Top-level auto-reduction algorithm for the SNS Liquids Reflectometer
"""
from __future__ import (absolute_import, division, print_function)
import sys
import math
import re
import platform
import time
import numpy as np
import mantid
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *
from reduction_gui.reduction.reflectometer.refl_data_series import DataSeries
from reduction_gui.reduction.reflectometer.refl_data_script import DataSets
from six import string_types


class LRAutoReduction(PythonAlgorithm):

    def category(self):
        """ Return category """
        return "Reflectometry\\SNS"

    def name(self):
        """ Return name """
        return "LRAutoReduction"

    def version(self):
        """ Return version number """
        return 1

    def summary(self):
        """ Short description """
        return "Find reflectivity peak and return its pixel range."

    def PyInit(self):
        """ Property declarations """
        self.declareProperty(FileProperty("Filename", "", FileAction.OptionalLoad, ['.nxs']),
                             "Data file to reduce")
        self.declareProperty(WorkspaceProperty("InputWorkspace", "",
                                               Direction.Input, PropertyMode.Optional),
                             "Optionally, we can provide a workspace directly")
        self.declareProperty(FileProperty("TemplateFile", "", FileAction.OptionalLoad, ['.xml']),
                             "Template reduction file")

        # ------------ Properties that should be in the meta data -------------
        self.declareProperty("ScaleToUnity", True,
                             "If true, the reflectivity under the Q cutoff will be scaled to 1")
        self.declareProperty(IntArrayProperty("PrimaryFractionRange", [117, 197],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range to use for calculating the primary fraction correction.")
        self.declareProperty(IntArrayProperty("DirectBeamList", [], direction=Direction.Input),
                             "List of direct beam run numbers (integers)")
        self.declareProperty(FileProperty("ScalingFactorFile", "", FileAction.OptionalLoad,
                                          extensions=['.cfg', '.txt']), "Scaling factor file")
        self.declareProperty("IncidentMedium", "medium", "Name of the incident medium")
        # ---------------------------------------------------------------------

        self.declareProperty("ScalingFactorTOFStep", 200.0,
                             "Bin width in TOF for fitting scaling factors")
        self.declareProperty("WavelengthOffset", 0.0,
                             "Wavelength offset used for TOF range determination")
        self.declareProperty("ScalingWavelengthCutoff", 10.0,
                             "Wavelength above which the scaling factors are assumed to be one")
        self.declareProperty("FindPeaks", False,
                             "Find reflectivity peaks instead of using the template values")
        self.declareProperty("ReadSequenceFromFile", False,
                             "Read the run sequence information from the file, not the title")
        self.declareProperty("ForceSequenceNumber", 0,
                             "Force the sequence number value if it's not available")
        self.declareProperty("OrderDirectBeamsByRunNumber", False,
                             "Force the sequence of direct beam files to be ordered by run number")
        self.declareProperty(FileProperty('OutputFilename', '', action=FileAction.OptionalSave, extensions=["txt"]),
                             doc='Name of the reflectivity file output')
        self.declareProperty(FileProperty("OutputDirectory", "", FileAction.Directory))

        self.declareProperty(IntArrayProperty("SequenceInfo", [0, 0, 0], direction=Direction.Output),
                             "Run sequence information (run number, sequence ID, sequence number).")
        self.declareProperty("SlitTolerance", 0.02, doc="Tolerance for matching slit positions")

    def load_data(self):
        """
            Load the data. We can either load it from the specified
            run numbers, or use the input workspace if no runs are specified.
        """
        filename = self.getProperty("Filename").value
        ws_event_data = self.getProperty("InputWorkspace").value

        if len(filename) > 0:
            ws_event_data = LoadEventNexus(Filename=filename, MetaDataOnly=False)
        elif ws_event_data is None:
            raise RuntimeError("No input data was specified")
        return ws_event_data

    def _get_series_info(self):
        """
            Retrieve the information about the scan series so
            that we know how to put all the pieces together.

            At some point this should all be in the data logs.
            We can also pull some of the information from the title.
        """
        # Load meta data to decide what to do
        self.event_data = self.load_data()
        meta_data_run = self.event_data.getRun()
        run_number = self.event_data.getRunNumber()

        # Deal with a forced sequence number
        force_value = self.getProperty("ForceSequenceNumber").value
        read_sequence_from_file = self.getProperty("ReadSequenceFromFile").value
        if force_value > 0:
            sequence_number = force_value
            first_run_of_set = int(run_number) - int(sequence_number) + 1
            do_reduction = True
            is_direct_beam = False

        # Look for meta data information, available with the new DAS
        # If it's not available, parse the title.
        elif read_sequence_from_file is True \
                and meta_data_run.hasProperty("sequence_number") \
                and meta_data_run.hasProperty("sequence_id") \
                and meta_data_run.hasProperty("data_type"):
            sequence_number = meta_data_run.getProperty("sequence_number").value[0]
            first_run_of_set = meta_data_run.getProperty("sequence_id").value[0]
            data_type = meta_data_run.getProperty("data_type").value[0]
            # Normal sample data is type 0
            do_reduction = data_type == 0
            # Direct beams for scaling factors are type 1
            is_direct_beam = data_type == 1
            # Type 2 is zero-attenuator direct beams
            # Type 3 is data that we don't need to treat
        else:
            first_run_of_set, sequence_number, is_direct_beam = self._parse_title(meta_data_run, run_number)
            do_reduction = not is_direct_beam

        self.setProperty("SequenceInfo",
                         [int(run_number), int(first_run_of_set), int(sequence_number)])
        return run_number, first_run_of_set, sequence_number, do_reduction, is_direct_beam

    def _parse_title(self, meta_data_run, run_number):
        """
            Parse the title to get the first run number of the set and the sequence number
            @param meta_data_run: run object for the workspace
            @param run_number: run number
        """
        logger.notice("Parsing sequence ID and sequence number from title!")
        first_run_of_set = int(run_number)
        sequence_number = 1
        is_direct_beam = False
        title = meta_data_run.getProperty("run_title").value

        # Determine whether this is a direct beam run
        if "direct beam" in title.lower():
            logger.notice("Direct beam found in the title")
            is_direct_beam = True

        thi = meta_data_run.getProperty('thi').value[0]
        tthd = meta_data_run.getProperty('tthd').value[0]
        if math.fabs(thi - tthd) < 0.001:
            logger.notice("Angle appears to be zero: probably a direct beam run")
            is_direct_beam = True

        # Determine the sequence ID and sequence number
        #pylint: disable=bare-except
        try:
            m = re.search(r"Run:(\d+)-(\d+)\.", title)
            if m is not None:
                first_run_of_set = m.group(1)
                sequence_number = int(m.group(2))
            else:
                m = re.search(r"-(\d+)\.$", title)
                if m is not None:
                    sequence_number = int(m.group(1))
                    first_run_of_set = int(run_number) - int(sequence_number) + 1
                else:
                    sequence_number = -1
                    first_run_of_set = int(run_number) - int(sequence_number) + 1
        except:
            sequence_number = -1
            first_run_of_set = int(run_number) - int(sequence_number) + 1

        if sequence_number == -1:
            logger.notice("Title: %s" % title)
            msg = "Could not identify sequence number. "
            msg += "Make sure the run title ends with -n where 1 < n < 7"
            raise RuntimeError(msg)

        return first_run_of_set, sequence_number, is_direct_beam

    def _find_peaks(self, event_data):
        """
            Find reflectivity peak and low-resolution peak for a workspace
            @param event_data: data workspace
        """
        # Find peaks as needed
        nx = int(event_data.getInstrument().getNumberParameter("number-of-x-pixels")[0])
        ny = int(event_data.getInstrument().getNumberParameter("number-of-y-pixels")[0])
        tof_summed = Integration(InputWorkspace=event_data)

        # Reflectivity peak
        peak_data = RefRoi(InputWorkspace=tof_summed, IntegrateY=False,
                           NXPixel=nx, NYPixel=ny, ConvertToQ=False)
        peak_data = Transpose(InputWorkspace=peak_data)
        peak, _, _ = LRPeakSelection(InputWorkspace=peak_data, ComputePrimaryRange=False)

        # Low-resolution range
        peak_data = RefRoi(InputWorkspace=tof_summed, IntegrateY=True,
                           NXPixel=nx, NYPixel=ny, ConvertToQ=False)
        peak_data = Transpose(InputWorkspace=peak_data)
        _, low_res, _ = LRPeakSelection(InputWorkspace=peak_data, ComputePrimaryRange=False)

        AnalysisDataService.remove(str(tof_summed))
        AnalysisDataService.remove(str(peak_data))

        return [int(x) for x in peak], [int(x) for x in low_res]

    def _read_template(self, sequence_number):
        """
            Read template from file.
            @param sequence_number: the ID of the data set within the sequence of runs
        """
        template_file = self.getProperty("TemplateFile").value
        fd = open(template_file, "r")
        xml_str = fd.read()
        s = DataSeries()
        s.from_xml(xml_str)

        if len(s.data_sets) >= sequence_number:
            data_set = s.data_sets[sequence_number - 1]
        elif len(s.data_sets) > 0:
            data_set = s.data_sets[0]
        else:
            raise RuntimeError("Invalid reduction template")

        self.data_series_template = s

        return data_set

    def _get_template(self, run_number, first_run_of_set, sequence_number):
        """
            Get a template, either from file or creating one.
            @param run_number: run number according to the data file name
            @param first_run_of_set: first run in the sequence (sequence ID)
            @param sequence_number: the ID of the data set within the sequence of runs
        """
        # Check whether we need to read a template file
        filename = self.getProperty("TemplateFile").value
        # Keep track of the origin of the template so we know whether to force peak finding
        create_template = False

        # If a template was supplied, use it.
        if len(filename.strip()) > 0:
            data_set = self._read_template(sequence_number)
        # ... if not, create a new one using the meta-data information
        else:
            create_template = True
            logger.notice("No template supplied: one will be created - peaks will be found automatically")
            data_set = self._create_template(run_number, first_run_of_set, sequence_number)

        # Backward compatibility with early templates:
        #   Verify that the primary fraction is available
        if data_set.clocking_from is None and data_set.clocking_to is None:
            primary_range = self.getProperty("PrimaryFractionRange").value
            data_set.clocking_from = int(primary_range[0])
            data_set.clocking_to = int(primary_range[1])
            logger.notice("Template did not contain primary fraction range: using supplied default")

        # Get incident medium as a simple string
        _incident_medium_str = str(data_set.incident_medium_list[0])
        _list = _incident_medium_str.split(',')
        incident_medium = _list[data_set.incident_medium_index_selected]

        # If we have to find peaks, do it here
        find_peaks = self.getProperty("FindPeaks").value
        if find_peaks or create_template:
            # Find reflectivity peak
            self.reflectivity_peak, self.low_res = self._find_peaks(self.event_data)
            logger.notice("Using reflectivity peak %s (template was %s)" % (self.reflectivity_peak, data_set.DataPeakPixels))
            data_set.DataPeakPixels = self.reflectivity_peak
            data_set.DataBackgroundRoi = [self.reflectivity_peak[0] - 3, self.reflectivity_peak[1] + 3, 0, 0]
            logger.notice("Using low-res %s (template was %s)" % (self.low_res, data_set.data_x_range))
            data_set.data_x_range = self.low_res

        return data_set, incident_medium

    def _read_property(self, meta_data_run, key, default, is_string=False):
        """
            Read the value for the given key in the sample run logs
            @param meta_data_run: Run object from the Mantid workspace
            @param key: name of the property to read
            @param default: default value to return if we don't find the key
        """
        if meta_data_run.hasProperty(key):
            value = meta_data_run.getProperty(key).value[0]
        else:
            value = default
            logger.error("No %s value in the data logs: using %s=%s" % (key, key, default))
            return value
        if is_string and len(value.strip()) == 0:
            value = default
            logger.error("Empty %s value in the data logs: using %s=%s" % (key, key, default))
        return value

    #pylint: disable=too-many-locals
    def _create_template(self, run_number, first_run_of_set, sequence_number):
        """
            Create a new template according to the meta-data
            @param run_number: run number according to the data file name
            @param first_run_of_set: first run in the sequence (sequence ID)
            @param sequence_number: the ID of the data set within the sequence of runs
        """
        # If so, load it and only overwrite the part we are dealing with here.
        template_file = self._get_output_template_path(first_run_of_set)
        if os.path.isfile(template_file):
            logger.notice("Writing template: %s" % template_file)
            fd = open(template_file, "r")
            xml_str = fd.read()
            s = DataSeries()
            s.from_xml(xml_str)
        else:
            s = DataSeries()

        # Now we have an initial template
        self.data_series_template = s

        # Get the TOF range
        tof_range = self._get_tof_range()

        # Get information from meta-data
        meta_data_run = self.event_data.getRun()
        _incident_medium = self.getProperty("IncidentMedium").value
        incident_medium = self._read_property(meta_data_run, "incident_medium",
                                              _incident_medium, is_string=True)

        q_min = self._read_property(meta_data_run, "output_q_min", 0.001)
        q_step = -abs(self._read_property(meta_data_run, "output_q_step", 0.02))
        dQ_constant = self._read_property(meta_data_run, "dq_constant", 0.004)
        dQ_slope = self._read_property(meta_data_run, "dq_slope", 0.02)
        angle_offset = self._read_property(meta_data_run, "angle_offset", 0.016)
        angle_offset_err = self._read_property(meta_data_run, "angle_offset_error", 0.001)

        _primary_range = self.getProperty("PrimaryFractionRange").value
        _primary_min = int(_primary_range[0])
        _primary_max = int(_primary_range[1])
        # The DAS logs are all stored as floats, but we are expecting an integer
        primary_min = math.trunc(float(self._read_property(meta_data_run, "primary_range_min", _primary_min)))
        primary_max = math.trunc(float(self._read_property(meta_data_run, "primary_range_max", _primary_max)))

        _sf_file = self.getProperty("ScalingFactorFile").value
        sf_file = self._read_property(meta_data_run, "scaling_factor_file",
                                      _sf_file, is_string=True)

        def _new_data_set():
            d = DataSets()
            d.NormFlag = True
            d.DataBackgroundFlag = True
            d.data_x_range_flag = True
            d.norm_x_range_flag = True
            d.DataTofRange = tof_range
            d.NormBackgroundFlag = True
            d.slits_width_flag = True
            d.incident_medium_list = [incident_medium]
            d.incident_medium_index_selected = 0
            d.angle_offset = angle_offset
            d.angle_offset_error = angle_offset_err
            d.clocking_from = primary_min
            d.clocking_to = primary_max
            d.q_min = q_min
            d.q_step = q_step
            d.fourth_column_dq0 = dQ_constant
            d.fourth_column_dq_over_q = dQ_slope
            d.scaling_factor_file = sf_file
            return d

        # Copy over the existing series, up to the point we are at
        new_data_sets = []
        # First, copy over the entries in the existing template,
        # up to the point previous to the current point
        for i in range(min(int(run_number) - int(first_run_of_set), len(s.data_sets))):
            sequence_id = int(first_run_of_set) + i
            logger.information("Copying %s" % sequence_id)
            d = s.data_sets[i]
            d.data_files = [sequence_id]
            new_data_sets.append(d)

        running_id = len(new_data_sets)
        # Pad the items between what we have and the current point
        for i in range(running_id, int(run_number) - int(first_run_of_set) + 1):
            sequence_id = int(first_run_of_set) + i
            logger.information("Adding %s" % sequence_id)
            d = _new_data_set()
            d.data_files = [sequence_id]
            new_data_sets.append(d)

        self.data_series_template.data_sets = new_data_sets

        data_set = self.data_series_template.data_sets[sequence_number - 1]

        # Find direct beam peaks
        self._get_direct_beam(meta_data_run, data_set)

        return data_set

    def _get_tof_range(self):
        """
            Determine TOF range from the data
        """
        sample = self.event_data.getInstrument().getSample()
        source = self.event_data.getInstrument().getSource()
        source_sample_distance = sample.getDistance(source)
        detector = self.event_data.getDetector(0)
        sample_detector_distance = detector.getPos().getZ()
        source_detector_distance = source_sample_distance + sample_detector_distance
        h = 6.626e-34  # m^2 kg s^-1
        m = 1.675e-27  # kg
        wl = self.event_data.getRun().getProperty('LambdaRequest').value[0]
        chopper_speed = self.event_data.getRun().getProperty('SpeedRequest1').value[0]
        wl_offset = self.getProperty("WavelengthOffset").value
        cst = source_detector_distance / h * m
        tof_min = cst * (wl + wl_offset * 60.0 / chopper_speed - 1.7 * 60.0 / chopper_speed) * 1e-4
        tof_max = cst * (wl + wl_offset * 60.0 / chopper_speed + 1.7 * 60.0 / chopper_speed) * 1e-4
        return [tof_min, tof_max]

    def _get_direct_beam(self, meta_data_run, data_set):
        """
            Get the direct beam run information for the loaded data
            @param meta_data_run: Run object from the Mantid workspace
            @param data_set: DataSets object
        """
        # Wavelength of the data we are reducing
        data_wl = self.event_data.getRun().getProperty('LambdaRequest').value[0]
        data_thi = self.event_data.getRun().getProperty('thi').value[0]

        _direct_beam_runs = list(self.getProperty("DirectBeamList").value)
        direct_beam_runs_str = self._read_property(meta_data_run, "direct_beam_runs",
                                                   _direct_beam_runs, is_string=True)
        # The direct runs in the DAS logs are stored as a string
        if isinstance(direct_beam_runs_str, string_types):
            try:
                direct_beam_runs = [int(r.strip()) for r in direct_beam_runs_str.split(',')]
            except ValueError:
                direct_beam_runs = []
        else:
            direct_beam_runs = direct_beam_runs_str

        # For each run, load and compare the wavelength
        direct_beam_found = None
        for r in direct_beam_runs:
            direct_beam_data = LoadEventNexus(Filename="REF_L_%s" % r)
            # Only consider zero-attenuator runs
            att = direct_beam_data.getRun().getProperty('vAtt').value[0]-1
            if not att == 0:
                continue
            wl = direct_beam_data.getRun().getProperty('LambdaRequest').value[0]
            thi = direct_beam_data.getRun().getProperty('thi').value[0]
            if np.abs(data_wl - wl) < 0.01 and np.abs(data_thi - thi) < 0.015:
                direct_beam_found = r
                break

        # Raise an exception if we haven't found our direct beam run
        if direct_beam_found is None:
            msg = "Could not find a valid direct beam run for "
            msg += "wl=%s in %s" % (data_wl, str(direct_beam_runs))
            raise RuntimeError(msg)

        # Find the direct beam peak
        peak, low_res = self._find_peaks(direct_beam_data)
        data_set.norm_file = direct_beam_found
        data_set.NormPeakPixels = peak
        data_set.NormBackgroundRoi = [peak[0] - 3, peak[1] + 3]
        data_set.NormBackgroundFlag = True
        data_set.norm_x_range = low_res

    def _get_output_template_path(self, first_run_of_set):
        output_dir = self.getProperty("OutputDirectory").value
        return os.path.join(output_dir, "REF_L_%s_auto_template.xml" % first_run_of_set)

    def _write_template(self, data_set, run_number, first_run_of_set, sequence_number):
        """
            Write out a template using the reduction parameters that we have used.
            @param data_set: DataSets object
            @param run_number: run number according to the data file name
            @param first_run_of_set: first run in the sequence (sequence ID)
            @param sequence_number: the ID of the data set within the sequence of runs
        """
        # Write out a template for this run
        xml_str = "<Reduction>\n"
        xml_str += "  <instrument_name>REFL</instrument_name>\n"
        xml_str += "  <timestamp>%s</timestamp>\n" % time.ctime()
        xml_str += "  <python_version>%s</python_version>\n" % sys.version
        xml_str += "  <platform>%s</platform>\n" % platform.system()
        xml_str += "  <architecture>%s</architecture>\n" % str(platform.architecture())
        xml_str += "  <mantid_version>%s</mantid_version>\n" % mantid.__version__

        # Copy over the existing series, up to the point we are at
        new_data_sets = []
        for i in range(int(run_number) - int(first_run_of_set) + 1):
            if i >= len(self.data_series_template.data_sets):
                logger.warning("Sequence is corrupted: run=%s, first run of set=%s" % (str(run_number),
                                                                                       str(first_run_of_set)))
                break
            d = self.data_series_template.data_sets[i]
            d.data_files = [int(first_run_of_set) + i]
            new_data_sets.append(d)
        # Make copy over the parameters we actually used
        new_data_sets[sequence_number - 1] = data_set

        self.data_series_template.data_sets = new_data_sets

        xml_str += self.data_series_template.to_xml()
        xml_str += "</Reduction>\n"
        template_file = open(self._get_output_template_path(first_run_of_set), 'w')
        template_file.write(xml_str)
        template_file.close()

    def _save_partial_output(self, data_set, first_run_of_set, sequence_number, run_number):
        """
            Stitch and save the full reflectivity curve, or as much as we have at the moment.
            @param data_set: DataSets object
            @param run_number: run number according to the data file name
            @param first_run_of_set: first run in the sequence (sequence ID)
            @param sequence_number: the ID of the data set within the sequence of runs
        """
        output_dir = self.getProperty("OutputDirectory").value
        output_file = self.getProperty("OutputFilename").value
        if len(output_file.strip()) == 0:
            output_file = "REFL_%s_%s_%s_auto.nxs" % (first_run_of_set, sequence_number, run_number)
        # Save partial output
        n_ts = 0
        output_ws = None
        prefix = 'reflectivity_%s_%s_%s' % (first_run_of_set, sequence_number, run_number)
        for ws in AnalysisDataService.getObjectNames():
            if ws.endswith("ts") and ws.startswith(prefix):
                output_ws = ws
                n_ts += 1
        if n_ts > 1:
            logger.error("More than one reduced output for %s" % prefix)

        file_path = os.path.join(output_dir, output_file)
        SaveNexus(Filename=file_path, InputWorkspace=output_ws)

        # Put the reflectivity curve together
        for f in os.listdir(output_dir):
            if f.startswith("REFL_%s" % first_run_of_set) and f.endswith("auto.nxs"):
                ws_name = f.replace("_auto.nxs", "")
                ws_name = ws_name.replace("REFL_", "")
                LoadNexus(Filename=os.path.join(output_dir, f), OutputWorkspace="reflectivity_%s_auto_ts" % ws_name)

        ws_list = AnalysisDataService.getObjectNames()
        input_ws_list = []
        for ws in ws_list:
            if ws.endswith("auto_ts"):
                input_ws_list.append(ws)

        if len(input_ws_list) == 0:
            logger.notice("No data sets to stitch.")
            return
        input_ws_list = sorted(input_ws_list)

        default_file_name = 'REFL_%s_combined_data_auto.txt' % first_run_of_set
        file_path = os.path.join(output_dir, default_file_name)
        scale_to_unity = self.getProperty("ScaleToUnity").value
        wl_cutoff = self.getProperty("ScalingWavelengthCutoff").value

        # The following were the values used in the auto-reduction before 2016
        # output_binning = [0.005, -0.01, 2.0]
        output_binning = [data_set.q_min, -abs(data_set.q_step), 2.0]
        dQ_constant = data_set.fourth_column_dq0
        dQ_slope = data_set.fourth_column_dq_over_q

        LRReflectivityOutput(ReducedWorkspaces=input_ws_list, ScaleToUnity=scale_to_unity,
                             ScalingWavelengthCutoff=wl_cutoff, OutputBinning=output_binning,
                             DQConstant=dQ_constant, DQSlope=dQ_slope, OutputFilename=file_path)
        for ws in input_ws_list:
            AnalysisDataService.remove(str(ws))

        return file_path

    def _get_sequence_total(self, default=10):
        """
            Return the total number of runs in the current sequence.
            If reading sequence information from file was turned off,
            or if the information was not found, return the given default.

            For direct beams, a default of 10 is not efficient but is a
            good value to avoid processing runs we know will be processed later.
            That is because most direct beam run sets are either 13 (for 30 Hz)
            or 21 (for 60 Hz).

            @param default: default value for when the info is not available
        """
        meta_data_run = self.event_data.getRun()
        # Get the total number of direct beams in a set.
        # A default of 10 is not efficient but is a good default to
        # avoid processing runs we know will be processed later.
        read_sequence_from_file = self.getProperty("ReadSequenceFromFile").value
        if read_sequence_from_file:
            return self._read_property(meta_data_run, "sequence_total", [default])
        else:
            return default

    def PyExec(self):
        slit_tolerance = self.getProperty("SlitTolerance").value

        # Determine where we are in the scan
        run_number, first_run_of_set, sequence_number, do_reduction, is_direct_beam = self._get_series_info()
        logger.information("Run %s - Sequence %s [%s/%s]" % (run_number, first_run_of_set,
                                                             sequence_number,
                                                             self._get_sequence_total(default=-1)))

        # If we have a direct beam, compute the scaling factors
        if is_direct_beam:
            sequence_total = self._get_sequence_total(default=10)
            if sequence_number < sequence_total:
                logger.notice("Waiting for at least %s runs to compute scaling factors" % sequence_total)
                return
            logger.notice("Using automated scaling factor calculator")
            output_dir = self.getProperty("OutputDirectory").value
            sf_tof_step = self.getProperty("ScalingFactorTOFStep").value
            order_by_runs = self.getProperty("OrderDirectBeamsByRunNumber").value

            # The medium for these direct beam runs may not be what was set in the template,
            # so either use the medium in the data file or a default name
            meta_data_run = self.event_data.getRun()
            _incident_medium = self.getProperty("IncidentMedium").value
            incident_medium = self._read_property(meta_data_run, "incident_medium",
                                                  _incident_medium, is_string=True)
            file_id = incident_medium.replace("medium", "")
            LRDirectBeamSort(RunList=list(range(first_run_of_set, first_run_of_set + sequence_number)),
                             UseLowResCut=True, ComputeScalingFactors=True, TOFSteps=sf_tof_step,
                             IncidentMedium=incident_medium,
                             SlitTolerance=slit_tolerance,
                             OrderDirectBeamsByRunNumber=order_by_runs,
                             ScalingFactorFile=os.path.join(output_dir, "sf_%s_%s_auto.cfg" % (first_run_of_set, file_id)))
            return
        elif not do_reduction:
            logger.notice("The data is of a type that does not have to be reduced")
            return

        # Get the reduction parameters for this run
        data_set, incident_medium = self._get_template(run_number, first_run_of_set, sequence_number)

        # Write template before we start the computation
        self._write_template(data_set, run_number, first_run_of_set, sequence_number)

        # Execute the reduction
        LiquidsReflectometryReduction(#RunNumbers=[int(run_number)],
                                      InputWorkspace=self.event_data,
                                      NormalizationRunNumber=str(data_set.norm_file),
                                      SignalPeakPixelRange=data_set.DataPeakPixels,
                                      SubtractSignalBackground=data_set.DataBackgroundFlag,
                                      SignalBackgroundPixelRange=data_set.DataBackgroundRoi[:2],
                                      NormFlag=data_set.NormFlag,
                                      NormPeakPixelRange=data_set.NormPeakPixels,
                                      NormBackgroundPixelRange=data_set.NormBackgroundRoi,
                                      SubtractNormBackground=data_set.NormBackgroundFlag,
                                      LowResDataAxisPixelRangeFlag=data_set.data_x_range_flag,
                                      LowResDataAxisPixelRange=data_set.data_x_range,
                                      LowResNormAxisPixelRangeFlag=data_set.norm_x_range_flag,
                                      LowResNormAxisPixelRange=data_set.norm_x_range,
                                      TOFRange=data_set.DataTofRange,
                                      IncidentMediumSelected=incident_medium,
                                      GeometryCorrectionFlag=False,
                                      QMin=data_set.q_min,
                                      QStep=data_set.q_step,
                                      AngleOffset=data_set.angle_offset,
                                      AngleOffsetError=data_set.angle_offset_error,
                                      ScalingFactorFile=str(data_set.scaling_factor_file),
                                      SlitsWidthFlag=data_set.slits_width_flag,
                                      ApplyPrimaryFraction=True,
                                      SlitTolerance=slit_tolerance,
                                      PrimaryFractionRange=[data_set.clocking_from, data_set.clocking_to],
                                      OutputWorkspace='reflectivity_%s_%s_%s' % (first_run_of_set, sequence_number, run_number))

        # Put the reflectivity curve together
        self._save_partial_output(data_set, first_run_of_set, sequence_number, run_number)


AlgorithmFactory.subscribe(LRAutoReduction)

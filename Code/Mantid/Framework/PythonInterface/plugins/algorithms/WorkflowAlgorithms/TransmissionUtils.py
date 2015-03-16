#pylint: disable=invalid-name
from mantid.api import *
from mantid.kernel import *
import os
import sys

def simple_algorithm(algorithm_str, parameters):
    return _execute(algorithm_str, parameters)

def _execute(algorithm_str, parameters, is_name=True):
    if is_name:
        alg = AlgorithmManager.create(algorithm_str)
    else:
        alg = Algorithm.fromString(algorithm_str)
    alg.initialize()
    alg.setChild(True)
    for key, value in parameters.iteritems():
        if value is None:
            Logger("TransmissionUtils").error("Trying to set %s=None" % key)
        if alg.existsProperty(key):
            if type(value)==str:
                alg.setPropertyValue(key, value)
            else:
                alg.setProperty(key, value)
    try:
        alg.execute()
    except:
        Logger("TransmissionUtils").error("Error executing [%s]" % str(alg))
        Logger("TransmissionUtils").error(str(sys.exc_value))
    return alg

def load_monitors(self, property_manager):
    """
        Load files necessary to compute transmission.
        Return their names.
    """
    output_str = ""
    property_manager_name = self.getPropertyValue("ReductionProperties")
    # If we need to get a special beam center position, do it now
    beam_center_x = None
    beam_center_y = None
    beam_center_x_input = self.getProperty("BeamCenterX").value
    beam_center_y_input = self.getProperty("BeamCenterY").value
    if beam_center_x_input > 0 and beam_center_y_input > 0:
        beam_center_x = beam_center_x_input
        beam_center_y = beam_center_y_input

    # Get instrument to use with FileFinder
    instrument = ''
    if property_manager.existsProperty("InstrumentName"):
        instrument = property_manager.getProperty("InstrumentName").value

    # Get the data loader
    def _load_data(filename, output_ws):
        if not property_manager.existsProperty("LoadAlgorithm"):
            Logger("SANSDirectBeamTransmission").error("SANS reduction not set up properly: missing load algorithm")
            raise RuntimeError, "SANS reduction not set up properly: missing load algorithm"
        p=property_manager.getProperty("LoadAlgorithm")

        alg_props = {"Filename": filename,
                     "OutputWorkspace": output_ws,
                     "ReductionProperties": property_manager_name,\
                     }
        if beam_center_x is not None and beam_center_y is not None:
            alg_props["BeamCenterX"] = beam_center_x
            alg_props["BeamCenterY"] = beam_center_y

        alg = _execute(p.valueAsStr, alg_props, is_name=False)

        msg = 'Loaded %s\n' % filename
        if alg.existsProperty("OutputMessage"):
            msg += alg.getProperty("OutputMessage").value
        ws = alg.getProperty("OutputWorkspace").value

        return ws, msg.replace('\n', '\n|')

    # Load the data files
    sample_file = self.getPropertyValue("SampleDataFilename")
    empty_file = self.getPropertyValue("EmptyDataFilename")

    sample_ws_name = "__transmission_sample"
    sample_ws, l_text = _load_data(sample_file, sample_ws_name)
    tmp_str = "   Sample: %s\n%s\n" % (os.path.basename(sample_file), l_text)
    output_str += tmp_str.replace('\n', '\n      ')

    sample_ws, l_text = subtract_dark_current(self, sample_ws, property_manager)
    if len(l_text)>0:
        output_str += l_text.replace('\n', '\n      ')+'\n'

    empty_ws_name = "__transmission_empty"
    empty_ws, l_text = _load_data(empty_file, empty_ws_name)
    tmp_str = "   Empty: %s\n%s\n" % (os.path.basename(empty_file), l_text)
    output_str += tmp_str.replace('\n', '\n      ')

    empty_ws, l_text = subtract_dark_current(self, empty_ws, property_manager)
    if len(l_text)>0:
        output_str += l_text.replace('\n', '\n      ')+'\n'

    # Find which pixels to sum up as our "monitor". At this point we have moved the detector
    # so that the beam is at (0,0), so all we need is to sum the area around that point.
    #TODO: in IGOR, the error-weighted average is computed instead of simply summing up the pixels
    beam_radius = self.getProperty("BeamRadius").value
    pixel_size_x = sample_ws.getInstrument().getNumberParameter("x-pixel-size")[0]
    cylXML = '<infinite-cylinder id="transmission_monitor">' + \
               '<centre x="0.0" y="0.0" z="0.0" />' + \
               '<axis x="0.0" y="0.0" z="1.0" />' + \
               '<radius val="%12.10f" />' % (beam_radius*pixel_size_x/1000.0) + \
             '</infinite-cylinder>\n'

    # Use the transmission workspaces to find the list of monitor pixels
    # since the beam center may be at a different location
    alg = _execute("FindDetectorsInShape",
                   {"Workspace": sample_ws,
                    "ShapeXML": cylXML\
                    })
    det_list = alg.getProperty("DetectorList").value
    first_det = det_list[0]

    #TODO: check that both workspaces have the same masked spectra

    # Get normalization for transmission calculation
    monitor_det_ID = None
    if property_manager.existsProperty("TransmissionNormalisation"):
        if property_manager.getProperty("TransmissionNormalisation").value=="Monitor":
            monitor_det_ID = int(sample_ws.getInstrument().getNumberParameter("default-incident-monitor-spectrum")[0])
        else:
            monitor_det_ID = int(sample_ws.getInstrument().getNumberParameter("default-incident-timer-spectrum")[0])
    elif property_manager.existsProperty("NormaliseAlgorithm"):
        def _normalise(workspace):
            p=property_manager.getProperty("NormaliseAlgorithm")
            alg = _execute(p.valueAsStr,
                           {"InputWorkspace": workspace,
                            "OutputWorkspace": workspace,
                            "ReductionProperties": property_manager_name\
                            },
                           is_name=False)
            msg = ''
            if alg.existsProperty("OutputMessage"):
                msg += alg.getProperty("OutputMessage").value+'\n'
            ws = alg.getProperty("OutputWorkspace").value
            return ws, msg
        empty_ws, norm_msg = _normalise(empty_ws)
        output_str += "   %s\n" % norm_msg.replace('\n', '   \n')
        sample_ws, norm_msg = _normalise(sample_ws)
        output_str += "   %s\n" % norm_msg.replace('\n', '   \n')

    empty_mon_ws_name = "__empty_mon"
    sample_mon_ws_name = "__sample_mon"

    det_list = [str(i) for i in det_list]
    det_list = ','.join(det_list)

    # Ensuring that the binning is uniform
    spec0 = empty_ws.dataX(0)
    spec_last = empty_ws.dataX(empty_ws.getNumberHistograms()-1)
    if abs(sum(spec0)-sum(spec_last))>0.000001:
        alg = _execute("ExtractSingleSpectrum",
                       {"InputWorkspace": empty_ws,
                        "OutputWorkspace": '__reference_binning',
                        "WorkspaceIndex": det_list[0]\
                        })
        reference_ws = alg.getProperty("OutputWorkspace").value
        alg = _execute("RebinToWorkspace",
                       {"WorkspaceToRebin": empty_ws,
                        "WorkspaceToMatch": reference_ws,
                        "OutputWorkspace": empty_ws_name\
                        })
        empty_ws = alg.getProperty("OutputWorkspace").value
        alg = _execute("RebinToWorkspace",
                       {"WorkspaceToRebin": sample_ws,
                        "WorkspaceToMatch": reference_ws,
                        "OutputWorkspace": sample_ws_name\
                        })
        sample_ws = alg.getProperty("OutputWorkspace").value

    alg = _execute("GroupDetectors",
                   {"InputWorkspace": empty_ws,
                    "OutputWorkspace": empty_mon_ws_name,
                    "DetectorList": det_list,
                    "KeepUngroupedSpectra": True\
                    })
    empty_mon_ws = alg.getProperty("OutputWorkspace").value

    alg = _execute("GroupDetectors",
                   {"InputWorkspace": sample_ws,
                    "OutputWorkspace": sample_mon_ws_name,
                    "DetectorList": det_list,
                    "KeepUngroupedSpectra": True\
                    })
    sample_mon_ws = alg.getProperty("OutputWorkspace").value

    alg = _execute("ConvertToMatrixWorkspace",
                   {"InputWorkspace": empty_mon_ws,
                    "OutputWorkspace": empty_mon_ws_name\
                    })
    empty_mon_ws = alg.getProperty("OutputWorkspace").value

    alg = _execute("ConvertToMatrixWorkspace",
                   {"InputWorkspace": sample_mon_ws,
                    "OutputWorkspace": sample_mon_ws_name\
                    })
    sample_mon_ws = alg.getProperty("OutputWorkspace").value

    alg = _execute("RebinToWorkspace",
                   {"WorkspaceToRebin": empty_mon_ws,
                    "WorkspaceToMatch": sample_mon_ws,
                    "OutputWorkspace": empty_mon_ws_name\
                    })
    empty_mon_ws = alg.getProperty("OutputWorkspace").value

    return sample_mon_ws, empty_mon_ws, first_det, output_str, monitor_det_ID

def calculate_transmission(self, sample_mon_ws, empty_mon_ws, first_det,
                           trans_output_workspace, monitor_det_ID=None):
    """
        Compute zero-angle transmission.

        Returns the fitted transmission workspace as well as the raw transmission workspace.

        @param sample_mon_ws: name of the sample monitor workspace
        @param empty_mon_ws: name of the empty monitor workspace
        @param first_det: ID of the first detector, so we know where to find the summed counts
        @param trans_output_workspace: name of the transmission workspace to create
        @param monitor_det_ID: ID of the monitor spectrum (for HFIR data only)
    """
    try:
        if monitor_det_ID is not None:
            alg = _execute("CalculateTransmission",
                           {"DirectRunWorkspace": empty_mon_ws,
                            "SampleRunWorkspace": sample_mon_ws,
                            "OutputWorkspace": trans_output_workspace,
                            "IncidentBeamMonitor": str(monitor_det_ID),
                            "TransmissionMonitor": str(first_det),
                            "OutputUnfittedData": True})
            output_ws = alg.getProperty("OutputWorkspace").value
        else:
            alg = _execute("CalculateTransmission",
                           {"DirectRunWorkspace": empty_mon_ws,
                            "SampleRunWorkspace": sample_mon_ws,
                            "OutputWorkspace": trans_output_workspace,
                            "TransmissionMonitor": str(first_det),
                            "OutputUnfittedData": True})
            output_ws = alg.getProperty("OutputWorkspace").value
        # Get the unfitted data
        raw_ws = None
        if alg.existsProperty("UnfittedData"):
            raw_ws = alg.getProperty("UnfittedData").value
        if raw_ws is None:
            Logger("TransmissionUtils").warning("Could not retrieve unfitted transmission for %s" % trans_output_workspace)
        return output_ws, raw_ws
    except:
        Logger("TransmissionUtils").error("Couldn't compute transmission. Is the beam center in the right place?\n%s" % sys.exc_value)
        return None, None

def apply_transmission(self, workspace, trans_workspace):
    """
        Apply transmission correction
        @param workspace: workspace to apply correction to
        @param trans_workspace: workspace name for of the transmission
    """
    # Sanity check
    if workspace is None:
        return None

    # Make sure the binning is compatible
    alg = _execute("RebinToWorkspace",
                   {"WorkspaceToRebin": trans_workspace,
                    "WorkspaceToMatch": workspace,
                    "OutputWorkspace": '__trans_rebin',
                    "PreserveEvents": False\
                    })
    rebinned_ws = alg.getProperty("OutputWorkspace").value

    # Apply angle-dependent transmission correction using the zero-angle transmission
    theta_dependent = self.getProperty("ThetaDependent").value

    alg = _execute("ApplyTransmissionCorrection",
                   {"InputWorkspace": workspace,
                    "TransmissionWorkspace": rebinned_ws,
                    "OutputWorkspace": '__corrected_output',
                    "ThetaDependent": theta_dependent\
                    })
    output_ws = alg.getProperty("OutputWorkspace").value
    return output_ws

def subtract_dark_current(self, workspace, property_manager):
    """
        Subtract the dark current
        @param workspace: workspace object to subtract from
        @param property_manager: property manager object
    """
    # Subtract dark current
    use_sample_dc = self.getProperty("UseSampleDarkCurrent").value
    dark_current_data = self.getPropertyValue("DarkCurrentFilename")
    property_manager_name = self.getProperty("ReductionProperties").value
    # Get instrument to use with FileFinder
    instrument = ''
    if property_manager.existsProperty("InstrumentName"):
        instrument = property_manager.getProperty("InstrumentName").value

    dark_current_property = "DefaultDarkCurrentAlgorithm"
    def _dark(ws, dark_current_property, dark_current_file=None):
        if property_manager.existsProperty(dark_current_property):
            p=property_manager.getProperty(dark_current_property)

            alg_props = {"InputWorkspace": ws,
                         "PersistentCorrection": False,
                         "ReductionProperties": property_manager_name\
                         }
            if dark_current_file is not None:
                alg_props["Filename"] = dark_current_file

            alg = _execute(p.valueAsStr, alg_props, is_name=False)
            msg = "Dark current subtracted"
            if alg.existsProperty("OutputMessage"):
                msg = alg.getProperty("OutputMessage").value
            ws = alg.getProperty("OutputWorkspace").value
            return ws, msg.replace('\n', '\n|')+'\n'
        return ws, ""

    if len(dark_current_data.strip())>0:
        return _dark(workspace, "DefaultDarkCurrentAlgorithm",
                     dark_current_file=dark_current_data)
    elif use_sample_dc is True:
        return _dark(workspace, "DarkCurrentAlgorithm")
    return workspace, ""

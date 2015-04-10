#pylint: disable=invalid-name
from mantid.simpleapi import *
from mantid.api import TextAxis
from mantid import config, logger

from IndirectImport import import_mantidplot

import os.path
import math
import datetime
import re
import numpy as np
import itertools


def StartTime(prog):
    logger.notice('----------')
    message = 'Program ' + prog + ' started @ ' + str(datetime.datetime.now())
    logger.notice(message)


def EndTime(prog):
    message = 'Program ' + prog + ' ended @ ' + str(datetime.datetime.now())
    logger.notice(message)
    logger.notice('----------')


def getInstrRun(ws_name):
    """
    Get the instrument name and run number from a workspace.

    @param ws_name - name of the workspace
    @return tuple of form (instrument, run number)
    """
    workspace = mtd[ws_name]
    run_number = str(workspace.getRunNumber())
    if run_number == '0':
        # Attempt to parse run number off of name
        match = re.match(r'([a-zA-Z]+)([0-9]+)', ws_name)
        if match:
            run_number = match.group(2)
        else:
            raise RuntimeError("Could not find run number associated with workspace.")

    instrument = workspace.getInstrument().getName()
    facility = config.getFacility()
    instrument = facility.instrument(instrument).filePrefix(int(run_number))
    instrument = instrument.lower()
    return instrument, run_number


def getWSprefix(wsname):
    """
    Returns a string of the form '<ins><run>_<analyser><refl>_' on which
    all of our other naming conventions are built. The workspace is used to get the
    instrument parameters.
    """
    if wsname == '':
        return ''

    workspace = mtd[wsname]
    facility = config['default.facility']

    ws_run = workspace.getRun()
    if 'facility' in ws_run:
        facility = ws_run.getLogData('facility').value

    (instrument, run_number) = getInstrRun(wsname)
    if facility == 'ILL':
        run_name = instrument + '_' + run_number
    else:
        run_name = instrument + run_number

    try:
        analyser = workspace.getInstrument().getStringParameter('analyser')[0]
        reflection = workspace.getInstrument().getStringParameter('reflection')[0]
    except IndexError:
        analyser = ''
        reflection = ''

    prefix = run_name + '_' + analyser + reflection

    if len(analyser + reflection) > 0:
        prefix += '_'

    return prefix


def getEfixed(workspace, detIndex=0):
    inst = mtd[workspace].getInstrument()
    return inst.getNumberParameter("efixed-val")[0]


def checkUnitIs(ws, unit_id, axis_index=0):
    """
    Check that the workspace has the correct units by comparing
    against the UnitID.
    """
    axis = mtd[ws].getAxis(axis_index)
    unit = axis.getUnit()
    return unit.unitID() == unit_id

def getDefaultWorkingDirectory():
    """
    Get the default save directory and check it's valid.
    """
    workdir = config['defaultsave.directory']

    if not os.path.isdir(workdir):
        raise IOError("Default save directory is not a valid path!")

    return workdir


def createQaxis(inputWS):
    result = []
    workspace = mtd[inputWS]
    num_hist = workspace.getNumberHistograms()
    if workspace.getAxis(1).isSpectra():
        inst = workspace.getInstrument()
        sample_pos = inst.getSample().getPos()
        beam_pos = sample_pos - inst.getSource().getPos()
        for i in range(0, num_hist):
            efixed = getEfixed(inputWS, i)
            detector = workspace.getDetector(i)
            theta = detector.getTwoTheta(sample_pos, beam_pos) / 2
            lamda = math.sqrt(81.787 / efixed)
            q = 4 * math.pi * math.sin(theta) / lamda
            result.append(q)
    else:
        axis = workspace.getAxis(1)
        msg = 'Creating Axis based on Detector Q value: '
        if not axis.isNumeric():
            msg += 'Input workspace must have either spectra or numeric axis.'
            raise ValueError(msg)
        if axis.getUnit().unitID() != 'MomentumTransfer':
            msg += 'Input must have axis values of Q'
            raise ValueError(msg)
        for i in range(0, num_hist):
            result.append(float(axis.label(i)))
    return result


def GetWSangles(inWS):
    num_hist = mtd[inWS].getNumberHistograms()    					# get no. of histograms/groups
    source_pos = mtd[inWS].getInstrument().getSource().getPos()
    sample_pos = mtd[inWS].getInstrument().getSample().getPos()
    beam_pos = sample_pos - source_pos
    angles = []    									# will be list of angles
    for index in range(0, num_hist):
        detector = mtd[inWS].getDetector(index)    				# get index
        two_theta = detector.getTwoTheta(sample_pos, beam_pos) * 180.0 / math.pi    	# calc angle
        angles.append(two_theta)    					# add angle
    return angles


def GetThetaQ(ws):
    """
    Returns the theta and elastic Q for each spectrum in a given workspace.

    @param ws Wotkspace to get theta and Q for
    @returns A tuple containing a list of theta values and a list of Q values
    """

    e_fixed = getEfixed(ws)
    wavelas = math.sqrt(81.787 / e_fixed)  # Elastic wavelength
    k0 = 4.0 * math.pi / wavelas

    axis = mtd[ws].getAxis(1)

    # If axis is in spec number need to retrieve angles and calculate Q
    if axis.isSpectra():
        theta = np.array(GetWSangles(ws))
        q = k0 * np.sin(0.5 * np.radians(theta))

    # If axis is in Q need to calculate back to angles and just return axis values
    elif axis.isNumeric() and axis.getUnit().unitID() == 'MomentumTransfer':
        q_bin_edge = axis.extractValues()
        q = list()
        for i in range(1, len(q_bin_edge)):
            q_centre = ((q_bin_edge[i] - q_bin_edge[i - 1]) / 2) + q_bin_edge[i - 1]
            q.append(q_centre)
        np_q = np.array(q)
        theta = 2.0 * np.degrees(np.arcsin(np_q / k0))

    # Out of options here
    else:
        raise RuntimeError('Cannot get theta and Q for workspace %s' % ws)

    return theta, q


def ExtractFloat(data_string):
    """
    Extract float values from an ASCII string
    """
    values = data_string.split()
    values = map(float, values)
    return values


def ExtractInt(data_string):
    """
    Extract int values from an ASCII string
    """
    values = data_string.split()
    values = map(int, values)
    return values


def PadArray(inarray, nfixed):
    """
    Pad a list to specified size.
    """
    npt = len(inarray)
    padding = nfixed - npt
    outarray = []
    outarray.extend(inarray)
    outarray += [0] * padding
    return outarray


def CheckAnalysers(in1WS, in2WS):
    """
    Check workspaces have identical analysers and reflections

    Args:
      @param in1WS - first 2D workspace
      @param in2WS - second 2D workspace

    Returns:
      @return None

    Raises:
      @exception Valuerror - workspaces have different analysers
      @exception Valuerror - workspaces have different reflections
    """
    ws1 = mtd[in1WS]
    analyser_1 = ws1.getInstrument().getStringParameter('analyser')[0]
    reflection_1 = ws1.getInstrument().getStringParameter('reflection')[0]
    ws2 = mtd[in2WS]
    analyser_2 = ws2.getInstrument().getStringParameter('analyser')[0]
    reflection_2 = ws2.getInstrument().getStringParameter('reflection')[0]
    if analyser_1 != analyser_2:
        raise ValueError('Workspace %s and %s have different analysers' % (ws1, ws2))
    elif reflection_1 != reflection_2:
        raise ValueError('Workspace %s and %s have different reflections' % (ws1, ws2))
    else:
        logger.information('Analyser is %s, reflection %s' % (analyser_1, reflection_1))


def CheckHistZero(inWS):
    """
    Retrieves basic info on a worskspace

    Checks the workspace is not empty, then returns the number of histogram and
    the number of X-points, which is the number of bin boundaries minus one

    Args:
      @param inWS  2D workspace

    Returns:
      @return num_hist - number of histograms in the workspace
      @return ntc - number of X-points in the first histogram, which is the number of bin
           boundaries minus one. It is assumed all histograms have the same
           number of X-points.

    Raises:
      @exception ValueError - Worskpace has no histograms
    """
    num_hist = mtd[inWS].getNumberHistograms()  # no. of hist/groups in WS
    if num_hist == 0:
        raise ValueError('Workspace ' + inWS + ' has NO histograms')
    x_in = mtd[inWS].readX(0)
    ntc = len(x_in) - 1  # no. points from length of x array
    if ntc == 0:
        raise ValueError('Workspace ' + inWS + ' has NO points')
    return num_hist, ntc


def CheckHistSame(in1WS, name1, in2WS, name2):
    """
    Check workspaces have same number of histograms and bin boundaries

    Args:
      @param in1WS - first 2D workspace
      @param name1 - single-word descriptor of first 2D workspace
      @param in2WS - second 2D workspace
      @param name2 - single-word descriptor of second 2D workspace

    Returns:
      @return None

    Raises:
      Valuerror: number of histograms is different
      Valuerror: number of bin boundaries in the histograms is different
    """
    num_hist_1 = mtd[in1WS].getNumberHistograms()  # no. of hist/groups in WS1
    x_1 = mtd[in1WS].readX(0)
    x_len_1 = len(x_1)
    num_hist_2 = mtd[in2WS].getNumberHistograms()  # no. of hist/groups in WS2
    x_2 = mtd[in2WS].readX(0)
    x_len_2 = len(x_2)
    if num_hist_1 != num_hist_2:  # Check that no. groups are the same
        error_1 = '%s (%s) histograms (%d)' % (name1, in1WS, num_hist_1)
        error_2 = '%s (%s) histograms (%d)' % (name2, in2WS, num_hist_2)
        error = error_1 + ' not = ' + error_2
        raise ValueError(error)
    elif x_len_1 != x_len_2:
        error_1 = '%s (%s) array length (%d)' % (name1, in1WS, x_len_1)
        error_2 = '%s (%s) array length (%d)' % (name2, in2WS, x_len_2)
        error = error_1 + ' not = ' + error_2
        raise ValueError(error)


def CheckXrange(x_range, type):
    if not ((len(x_range) == 2) or (len(x_range) == 4)):
        raise ValueError(type + ' - Range must contain either 2 or 4 numbers')

    for lower, upper in zip(x_range[::2], x_range[1::2]):
        if math.fabs(lower) < 1e-5:
            raise ValueError('%s - input minimum (%f) is zero' % (type, lower))
        if math.fabs(upper) < 1e-5:
            raise ValueError('%s - input maximum (%f) is zero' % (type, upper))
        if upper < lower:
            raise ValueError('%s - input maximum (%f) < minimum (%f)' % (type, upper, lower))


def CheckElimits(erange, Xin):
    len_x = len(Xin) - 1

    if math.fabs(erange[0]) < 1e-5:
        raise ValueError('Elimits - input emin (%f) is Zero' % (erange[0]))
    if erange[0] < Xin[0]:
        raise ValueError('Elimits - input emin (%f) < data emin (%f)' % (erange[0], Xin[0]))
    if math.fabs(erange[1]) < 1e-5:
        raise ValueError('Elimits - input emax (%f) is Zero' % (erange[1]))
    if erange[1] > Xin[len_x]:
        raise ValueError('Elimits - input emax (%f) > data emax (%f)' % (erange[1], Xin[len_x]))
    if erange[1] < erange[0]:
        raise ValueError('Elimits - input emax (%f) < emin (%f)' % (erange[1], erange[0]))


def getInstrumentParameter(ws, param_name):
    """Get an named instrument parameter from a workspace.

    Args:
      @param ws The workspace to get the instrument from.
      @param param_name The name of the parameter to look up.
    """
    inst = mtd[ws].getInstrument()

    # Create a map of type parameters to functions. This is so we avoid writing lots of
    # if statements becuase there's no way to dynamically get the type.
    func_map = {'double': inst.getNumberParameter, 'string': inst.getStringParameter,
                'int': inst.getIntParameter, 'bool': inst.getBoolParameter}

    if inst.hasParameter(param_name):
        param_type = inst.getParameterType(param_name)
        if param_type != '':
            param = func_map[param_type](param_name)[0]
        else:
            raise ValueError('Unable to retrieve %s from Instrument Parameter file.' % param_name)
    else:
        raise ValueError('Unable to retrieve %s from Instrument Parameter file.' % param_name)

    return param


def plotSpectra(ws, y_axis_title, indicies=[]):
    """
    Plot a selection of spectra given a list of indicies

    @param ws - the workspace to plot
    @param y_axis_title - label for the y axis
    @param indicies - list of spectrum indicies to plot
    """
    if len(indicies) == 0:
        num_spectra = mtd[ws].getNumberHistograms()
        indicies = range(num_spectra)

    try:
        mtd_plot = import_mantidplot()
        plot = mtd_plot.plotSpectrum(ws, indicies, True)
        layer = plot.activeLayer()
        layer.setAxisTitle(mtd_plot.Layer.Left, y_axis_title)
    except RuntimeError:
        # User clicked cancel on plot so don't do anything
        return


def plotParameters(ws, *param_names):
    """
    Plot a number of spectra given a list of parameter names
    This searchs for relevent spectra using the text axis label.

    @param ws - the workspace to plot from
    @param param_names - list of names to search for
    """
    axis = mtd[ws].getAxis(1)
    if axis.isText() and len(param_names) > 0:
        num_spectra = mtd[ws].getNumberHistograms()

        for name in param_names:
            indicies = [i for i in range(num_spectra) if name in axis.label(i)]
            if len(indicies) > 0:
                plotSpectra(ws, name, indicies)


def convertToElasticQ(input_ws, output_ws=None):
    """
    Helper function to convert the spectrum axis of a sample to ElasticQ.

    @param input_ws - the name of the workspace to convert from
    @param output_ws - the name to call the converted workspace
    """

    if output_ws is None:
        output_ws = input_ws

    axis = mtd[input_ws].getAxis(1)
    if axis.isSpectra():
        e_fixed = getEfixed(input_ws)
        ConvertSpectrumAxis(input_ws, Target='ElasticQ', EMode='Indirect', EFixed=e_fixed,
                            OutputWorkspace=output_ws)

    elif axis.isNumeric():
        # Check that units are Momentum Transfer
        if axis.getUnit().unitID() != 'MomentumTransfer':
            raise RuntimeError('Input must have axis values of Q')

        CloneWorkspace(input_ws, OutputWorkspace=output_ws)

    else:
        raise RuntimeError('Input workspace must have either spectra or numeric axis.')


def transposeFitParametersTable(params_table, output_table=None):
    """
    Transpose the parameter table created from a multi domain Fit.

    This function will make the output consistent with PlotPeakByLogValue.
    @param params_table - the parameter table output from Fit.
    @param output_table - name to call the transposed table. If omitted,
            the output_table will be the same as the params_table
    """
    params_table = mtd[params_table]

    table_ws = '__tmp_table_ws'
    table_ws = CreateEmptyTableWorkspace(OutputWorkspace=table_ws)

    param_names = params_table.column(0)[:-1]  # -1 to remove cost function
    param_values = params_table.column(1)[:-1]
    param_errors = params_table.column(2)[:-1]

    # Find the number of parameters per function
    func_index = param_names[0].split('.')[0]
    num_params = 0
    for i, name in enumerate(param_names):
        if name.split('.')[0] != func_index:
            num_params = i
            break

    # Create columns with parameter names for headers
    column_names = ['.'.join(name.split('.')[1:]) for name in param_names[:num_params]]
    column_error_names = [name + '_Err' for name in column_names]
    column_names = zip(column_names, column_error_names)
    table_ws.addColumn('double', 'axis-1')
    for name, error_name in column_names:
        table_ws.addColumn('double', name)
        table_ws.addColumn('double', error_name)

    # Output parameter values to table row
    for i in xrange(0, params_table.rowCount() - 1, num_params):
        row_values = param_values[i:i + num_params]
        row_errors = param_errors[i:i + num_params]
        row = [value for pair in zip(row_values, row_errors) for value in pair]
        row = [i / num_params] + row
        table_ws.addRow(row)

    if output_table is None:
        output_table = params_table.name()

    RenameWorkspace(table_ws.name(), OutputWorkspace=output_table)


def search_for_fit_params(suffix, table_ws):
    """
    Find all fit parameters in a table workspace with the given suffix.

    @param suffix - the name of the parameter to find.
    @param table_ws - the name of the table workspace to search.
    """
    return [name for name in mtd[table_ws].getColumnNames() if name.endswith(suffix)]


def convertParametersToWorkspace(params_table, x_column, param_names, output_name):
    """
    Convert a parameter table output by PlotPeakByLogValue to a MatrixWorkspace.

    This will make a spectrum for each parameter name using the x_column vairable as the
    x values for the spectrum.

    @param params_table - the table workspace to convert to a MatrixWorkspace.
    @param x_column - the column in the table to use for the x values.
    @param parameter_names - list of parameter names to add to the workspace
    @param output_name - name to call the output workspace.
    """
    # Search for any parameters in the table with the given parameter names,
    # ignoring their function index and output them to a workspace
    workspace_names = []
    for param_name in param_names:
        column_names = search_for_fit_params(param_name, params_table)
        column_error_names = search_for_fit_params(param_name + '_Err', params_table)
        param_workspaces = []
        for name, error_name in zip(column_names, column_error_names):
            ConvertTableToMatrixWorkspace(params_table, x_column, name, error_name,
                                          OutputWorkspace=name)
            param_workspaces.append(name)
        workspace_names.append(param_workspaces)

    # Transpose list of workspaces, ignoring unequal length of lists
    # this handles the case where a parameter occurs only once in the whole workspace
    workspace_names = map(list, itertools.izip_longest(*workspace_names))
    workspace_names = [filter(None, sublist) for sublist in workspace_names]

    # Join all the parameters for each peak into a single workspace per peak
    temp_workspaces = []
    for peak_params in workspace_names:
        temp_peak_ws = peak_params[0]
        for param_ws in peak_params[1:]:
            ConjoinWorkspaces(temp_peak_ws, param_ws, False)
        temp_workspaces.append(temp_peak_ws)

    # Join all peaks into a single workspace
    temp_workspace = temp_workspaces[0]
    for temp_ws in temp_workspaces[1:]:  # TODO: fairly certain something is wrong here
        ConjoinWorkspaces(temp_workspace, temp_peak_ws, False)

    RenameWorkspace(temp_workspace, OutputWorkspace=output_name)

    # Replace axis on workspaces with text axis
    axis = TextAxis.create(mtd[output_name].getNumberHistograms())
    workspace_names = [name for sublist in workspace_names for name in sublist]
    for i, name in enumerate(workspace_names):
        axis.setLabel(i, name)
    mtd[output_name].replaceAxis(1, axis)


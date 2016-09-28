#pylint: disable=invalid-name
import mantid.simpleapi as s_api
from mantid import config, logger

from IndirectImport import import_mantidplot

import os.path
import math
import datetime
import re
import numpy as np


def StartTime(prog):
    logger.notice('----------')
    message = 'Program ' + prog + ' started @ ' + str(datetime.datetime.now())
    logger.notice(message)


def EndTime(prog):
    message = 'Program ' + prog + ' ended @ ' + str(datetime.datetime.now())
    logger.notice(message)
    logger.notice('----------')


def getWSprefix(wsname):
    """
    Returns a string of the form '<ins><run>_<analyser><refl>_' on which
    all of our other naming conventions are built. The workspace is used to get the
    instrument parameters.
    """
    if wsname == '':
        return ''

    workspace = s_api.mtd[wsname]
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


def getEfixed(workspace):
    inst = s_api.mtd[workspace].getInstrument()

    if inst.hasParameter('Efixed'):
        return inst.getNumberParameter('EFixed')[0]

    if inst.hasParameter('analyser'):
        analyser_name = inst.getStringParameter('analyser')[0]
        analyser_comp = inst.getComponentByName(analyser_name)

        if analyser_comp is not None and analyser_comp.hasParameter('Efixed'):
            return analyser_comp.getNumberParameter('EFixed')[0]

    raise ValueError('No Efixed parameter found')


def GetWSangles(inWS):
    num_hist = s_api.mtd[inWS].getNumberHistograms()    					# get no. of histograms/groups
    source_pos = s_api.mtd[inWS].getInstrument().getSource().getPos()
    sample_pos = s_api.mtd[inWS].getInstrument().getSample().getPos()
    beam_pos = sample_pos - source_pos
    angles = []    									# will be list of angles
    for index in range(0, num_hist):
        detector = s_api.mtd[inWS].getDetector(index)    				# get index
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

    axis = s_api.mtd[ws].getAxis(1)

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
    inst = s_api.mtd[ws].getInstrument()

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


def plotSpectra(ws, y_axis_title, indicies=None):
    """
    Plot a selection of spectra given a list of indicies

    @param ws - the workspace to plot
    @param y_axis_title - label for the y axis
    @param indicies - list of spectrum indicies to plot
    """
    if indicies is None:
        indicies = []

    if len(indicies) == 0:
        num_spectra = s_api.mtd[ws].getNumberHistograms()
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
    axis = s_api.mtd[ws].getAxis(1)
    if axis.isText() and len(param_names) > 0:
        num_spectra = s_api.mtd[ws].getNumberHistograms()

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

    axis = s_api.mtd[input_ws].getAxis(1)
    if axis.isSpectra():
        e_fixed = getEfixed(input_ws)
        s_api.ConvertSpectrumAxis(input_ws, Target='ElasticQ', EMode='Indirect', EFixed=e_fixed,
                                  OutputWorkspace=output_ws)

    elif axis.isNumeric():
        # Check that units are Momentum Transfer
        if axis.getUnit().unitID() != 'MomentumTransfer':
            raise RuntimeError('Input must have axis values of Q')

        s_api.CloneWorkspace(input_ws, OutputWorkspace=output_ws)

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
    params_table = s_api.mtd[params_table]

    table_ws = '__tmp_table_ws'
    table_ws = s_api.CreateEmptyTableWorkspace(OutputWorkspace=table_ws)

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

    s_api.RenameWorkspace(table_ws.name(), OutputWorkspace=output_table)


def IndentifyDataBoundaries(sample_ws):
    """
    Indentifies and returns the first and last no zero data point in a workspace

    For multiple workspace spectra, the data points that are closest to the centre
    out of all the spectra in the workspace are returned
    """

    sample_ws = s_api.mtd[sample_ws]
    nhists = sample_ws.getNumberHistograms()
    start_data_idx, end_data_idx = 0,0
    # For all spectra in the workspace
    for spectra in range(0, nhists):
        # Obtain first and last non zero values
        y_data = sample_ws.readY(spectra)
        spectra_start_data = firstNonZero(y_data)
        spectra_end_data = firstNonZero(list(reversed(y_data)))
        # Replace workspace start and end if data is closer to the center
        if spectra_start_data > start_data_idx:
            start_data_idx = spectra_start_data
        if spectra_end_data > end_data_idx:
            end_data_idx = spectra_end_data
    # Convert Bin index to data value
    x_data = sample_ws.readX(0)
    first_data_point = x_data[start_data_idx]
    last_data_point = x_data[len(x_data) - end_data_idx - 2]
    return first_data_point, last_data_point

def firstNonZero(data):
    """
    Returns the index of the first non zero value in the list
    """
    for i in range(len(data)):
        if data[i] != 0:
            return i

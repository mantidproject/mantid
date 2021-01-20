def analyze_outputs(cross_correlation_ws_dict, getdetoffset_result_ws_dict):
    """
    evaluate (by matching the fitted cross-correlation peaks to those calculated from
    CrossCorrelation) the peak fitting result from GetDetectorOffsets in order to create
    list of spectra to be masked.
    :param cross_correlation_ws_dict:
    :param getdetoffset_result_ws_dict:
    :return:
    """
    cost_ws_dict = dict()
    for bank_name in ['west', 'east', 'high angle']:
        cc_diamond_ws_name = cross_correlation_ws_dict[bank_name]
        fit_result_table_name = getdetoffset_result_ws_dict[bank_name]

        # create the workspaces
        cost_list = evaluate_cc_quality(cc_diamond_ws_name, fit_result_table_name)
        cost_array = numpy.array(cost_list).transpose()
        cost_ws_name_i = '{0}_cost'.format(bank_name)
        CreateWorkspace(DataX=cost_array[0], DataY=cost_array[1], NSpec=1,
                        OutputWorkspace=cost_ws_name_i)

        cost_ws_dict[bank_name] = cost_ws_name_i
    # END-FOR

    return cost_ws_dict


def calculate_difc(ws, ws_index):
    """ Calculate DIFC of one spectrum
    :param ws:
    :param ws_index:
    :return:
    """
    # det_id = ws.getDetector(i).getID()
    det_pos = ws.getDetector(ws_index).getPos()
    source_pos = ws.getInstrument().getSource().getPos()
    sample_pos = ws.getInstrument().getSample().getPos()

    source_sample = sample_pos - source_pos
    det_sample = det_pos - sample_pos
    angle = det_sample.angle(source_sample)

    L1 = source_sample.norm()
    L2 = det_sample.norm()

    # theta = angle * 180/3.14
    # print theta

    difc = 252.816 * 2 * math.sin(angle * 0.5) * (L1 + L2)  # math.sqrt(L1+L2) #\sqrt{L1+L2}

    return difc


def check_correct_difcs(ws_name, cal_table_name, mask_ws_name):
    """
    check and correct DIFCs if necessary: it is for 3 banks

    Renamed from check_correct_difcs_3banks

    :param ws_name:
    :param cal_table_name: name of Calibration workspace (a TableWorkspace)
    :return:
    """
    # west bank
    diamond_event_ws = mtd[ws_name]   # ['vulcan_diamond']
    cal_table_ws = mtd[cal_table_name]
    difc_col_index = 1
    # west_spec_vec = numpy.arange(0, 3234)
    west_idf_vec = numpy.ndarray(shape=(3234,), dtype='float')
    west_cal_vec = numpy.ndarray(shape=(3234,), dtype='float')
    for irow in range(0, 3234):
        west_idf_vec[irow] = calculate_difc(diamond_event_ws, irow)
        west_cal_vec[irow] = cal_table_ws.cell(irow, difc_col_index)
    # CreateWorkspace(DataX=west_spec_vec, DataY=west_difc_vec, NSpec=1, OutputWorkspace='west_idf_difc')

    # east bank
    # east_spec_vec = numpy.arange(3234, 6468)
    east_idf_vec = numpy.ndarray(shape=(3234,), dtype='float')
    east_cal_vec = numpy.ndarray(shape=(3234,), dtype='float')
    for irow in range(3234, 6468):
        east_idf_vec[irow - 3234] = calculate_difc(diamond_event_ws, irow)
        east_cal_vec[irow - 3234] = cal_table_ws.cell(irow, difc_col_index)

    # high angle bank
    # highangle_spec_vec = numpy.arange(6468, 24900)
    highangle_idf_vec = numpy.ndarray(shape=(24900 - 6468,), dtype='float')
    highangle_cal_vec = numpy.ndarray(shape=(24900 - 6468,), dtype='float')
    for irow in range(6468, 24900):
        highangle_idf_vec[irow - 6468] = calculate_difc(diamond_event_ws, irow)
        highangle_cal_vec[irow - 6468] = cal_table_ws.cell(irow, difc_col_index)

    mask_ws = mtd[mask_ws_name]

    # correct the unphysical (bad) calibrated DIFC to default DIF: west, east and high angle
    correct_difc_to_default(west_idf_vec, west_cal_vec, cal_table_ws, 0, 20, 1, mask_ws)
    correct_difc_to_default(east_idf_vec, east_cal_vec, cal_table_ws, 3234, 20, 1, mask_ws)
    correct_difc_to_default(highangle_idf_vec, highangle_idf_vec, cal_table_ws, 6468, 20, 1, mask_ws)

    return


# TODO - NIGHT - Implement!
def analyze_mask(event_ws, mask_ws, wi_start, wi_stop, output_dir):
    """ analyze mask workspace
    """
    assert mask_ws.getNumberHistograms() == event_ws.getNumberHistograms(), 'blabla'

    num_masked = 0
    num_masked_is_masked = 0
    zero_masked = 0
    zero_masked_is_masked = 0
    event_spectrum_list = list()
    for ws_index in range(wi_start, wi_stop+1):
        if mask_ws.readY(ws_index)[0] < 0.1:
            continue

        num_masked += 1

        # analyze masking information
        num_events_i = event_ws.getSpectrum(ws_index).getNumberEvents()
        if event_ws.getSpectrum(ws_index).getNumberEvents() == 0:
            zero_masked += 1
        else:
            event_spectrum_list.append((num_events_i, ws_index))
    
    # method 2: shall be same result as method 1
    for ws_index in range(wi_start, wi_stop+1):
        if not mask_ws.getDetector(ws_index).isMasked():
            continue

        num_masked_is_masked += 1
        mask_ws.dataY(ws_index)[0] = 1.0

        # analyze masking information
        if event_ws.getSpectrum(ws_index).getNumberEvents() == 0:
            case_i = 1  # no event
            zero_masked_is_masked += 1
        elif event_ws.getSpectrum:
            pass

    # 2. For each bank, sort the masked workspace from highest ban

    print ('[REPORT] From {} to {}: Masked = {} including (1) zero counts = {}'
           ''.format(wi_start, wi_stop-1, num_masked, zero_masked))
    print ('[REPORT] From {} to {}: Masked = {} including (1) zero counts = {}'
           ''.format(wi_start, wi_stop-1, num_masked_is_masked, zero_masked_is_masked))

    event_spectrum_list.sort(reverse=True)
    for i in range(min(10, len(event_spectrum_list))):
        num_events_i, ws_index = event_spectrum_list[i]
        print ('[REPORT] ws-index = {}, num events = {}, masked!'.format(ws_index, num_events_i))

    return None, None, None


def align_focus_event_ws(event_ws_name, calib_ws_name, group_ws_name):
    """
    overwrite the input
    """

    AlignDetectors(InputWorkspace=event_ws_name, OutputWorkspace=event_ws_name,
                   CalibrationWorkspace=calib_ws_name)

    DiffractionFocussing(InputWorkspace=event_ws_name, OutputWorkspace=event_ws_name,
                         GroupingWorkspace=group_ws_name)

    Rebin(InputWorkspace=event_ws_name, OutputWorkspace=event_ws_name, Params='0.5,-0.0003,3')

    ConvertToMatrixWorkspace(InputWorkspace=event_ws_name, OutputWorkspace=event_ws_name)

    EditInstrumentGeometry(Workspace=event_ws_name, PrimaryFlightPath=42, SpectrumIDs='1-3', L2='2,2,2',
                           Polar='89.9284,90.0716,150.059', Azimuthal='0,0,0', DetectorIDs='1-3',
                           InstrumentName='vulcan_3bank')

    return event_ws_name


def reduced_powder_data(ipts_number, run_number, calib_file_name, event_ws_name='vulcan_diamond',
                        focus_ws_name='vulcan_diamond_3bank'):
    """
    reduced: aligned detector and diffraction focus, powder data
    :param ipts_number:
    :param run_number:
    :return:
    """
    raw_nxs_file_name = '/SNS/VULCAN/IPTS-{}/nexus/VULCAN_{}.nxs.h5'.format(ipts_number, run_number)
    Load(Filename=raw_nxs_file_name, OutputWorkspace=event_ws_name)

    # load data file
    LoadDiffCal(InputWorkspace=event_ws_name,
                Filename=calib_file_name, WorkspaceName='vulcan')

    AlignDetectors(InputWorkspace=event_ws_name, OutputWorkspace=event_ws_name,
                   CalibrationWorkspace='vulcan_cal')

    DiffractionFocussing(InputWorkspace=event_ws_name, OutputWorkspace=event_ws_name,
                         GroupingWorkspace='vulcan_group')

    Rebin(InputWorkspace=event_ws_name, OutputWorkspace=event_ws_name, Params='0.5,-0.0003,3')

    ConvertToMatrixWorkspace(InputWorkspace='vulcan_diamond', OutputWorkspace=focus_ws_name)

    EditInstrumentGeometry(Workspace='vulcan_diamond_3bank', PrimaryFlightPath=42, SpectrumIDs='1-3', L2='2,2,2',
                           Polar='89.9284,90.0716,150.059', Azimuthal='0,0,0', DetectorIDs='1-3',
                           InstrumentName='vulcan_3bank')

    return




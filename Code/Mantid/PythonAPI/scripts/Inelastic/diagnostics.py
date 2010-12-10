"""
Provide functions to perform various diagnostic tests.

The output of each function is a workspace containing a single bin where:
    0 - denotes a masked spectra and
    1 - denotes an unmasked spectra.

This workspace can be summed with other masked workspaces to accumulate
masking and also passed to MaskDetectors to match masking there.
"""
from mantidsimple import *
import CommonFunctions as common

def diagnose(sample_run, white_run, other_white = None, remove_zero=False, 
             tiny=1e-10, large=1e10, median_lo=0.1, median_hi=3.0, signif=3.3, 
             bkgd_threshold=5.0, bkgd_range=None, effic_var=1.1, inst_name=None):
    """
    Run diagnostics on the provided run and white beam files.

    There are 3 possible tests, depending on the input given:
      White beam diagnosis
      Background tests
      Second white beam

    
    Required inputs:
    
      sample_run - The run number or filepath of the sample run
      white_run  - The run number or filepath of the white beam run
    
    Optional inputs:
      other_white   - If provided an addional set of tests is performed on this file. (default = None)
      remove_zero - If true then zeroes in the data will count as failed (default = False)
      tiny          - Minimum threshold for acceptance (default = 1e-10)
      large         - Maximum threshold for acceptance (default = 1e10)
      median_lo     - Fraction of median to consider counting low (default = 0.1)
      median_hi     - Fraction of median to consider counting high (default = 3.0)
      signif           - Counts within this number of multiples of the 
                      standard dev will be kept (default = 3.3)
      bkgd_threshold - High threshold for background removal in multiples of median (default = 5.0)
      bkgd_range - The background range as a list of 2 numbers: [min,max]. 
                   If not present then they are taken from the parameter file. (default = None)
      effic_var  - The number of medians the ratio of the first/second white beam can deviate from
                   the average by (default=1.1)
      inst_name  - The name of the instrument to perform the diagnosis.
                   If it is not provided then the default instrument is used (default = None)
    """
    # Set the default instrument so that Mantid can search for the runs correctly, 
    # but switch it back at the end
    def_inst = mtd.settings["default.instrument"]
    if inst_name == None:
        inst_name = def_inst
    elif inst_name != def_inst:
        mtd.settings["default.instrument"] = inst_name
    else: pass

    # Which tests are we runnning?
    _diag_total_mask = None
    failures_per_test = [0, 0, 0]

    white_counts = None
    if white_run is not None and str(white_run) != '':
        # Load and integrate
        data_ws = common.load_run(white_run, 'white-beam')
        white_counts = Integration(data_ws, "__counts_white-beam").workspace()        
        # Run first white beam tests
        _diag_total_mask, failures_per_test[0] = \
                          _do_white_test(white_counts, tiny, large, median_lo, median_hi, signif)
    else:
        raise RuntimeError('Invalid input for white run "%s"' % str(white_run))

    second_white_counts = None
    if other_white is not None and str(other_white) != '':
        # Load and integrate
        data_ws = common.load_run(other_white, 'white-beam2')
        second_white_counts = Integration(data_ws, "__counts_white-beam2").workspace()        
        # Run tests
        _second_white_masks, failures_per_test[1] = \
                             _do_second_white_test(white_counts, second_white_counts,
                                                   tiny, large, median_lo, median_hi,
                                                   signif,effic_var, _diag_total_mask)

        # Accumulate masks
        _diag_total_mask += _second_white_masks

    if sample_run is not None and sample_run != '':
        # Run the tests
        _bkgd_masks, failures_per_test[2] = \
                     _do_background_test(sample_run, white_counts, second_white_counts,
                                         bkgd_range, bkgd_threshold, remove_zero, signif,
                                         _diag_total_mask)
        # Accumulate the masks
        _diag_total_mask += _bkgd_masks
    else:
        raise RuntimeError('Invalid input for sample run "%s"' % str(sample_run))

    # Remove temporary workspaces
    mtd.deleteWorkspace(str(white_counts))    

    # Revert our default instrument changes if necessary
    if inst_name != def_inst:
        mtd.settings["default.instrument"] = def_inst
    
    # This will be a MaskWorkspace which contains the accumulation of all of the masks
    if lhs_info('nreturns') == 1:
        return _diag_total_mask
    else:
        return _diag_total_mask, failures_per_test

#-------------------------------------------------------------------------------

def _do_white_test(white_counts, tiny, large, median_lo, median_hi, signif):
    """
    Run the diagnostic tests on the integrated white beam run

    Required inputs:
    
      white_counts  - A workspace containing the integrated counts from a
                      white beam vanadium run
      tiny          - Minimum threshold for acceptance
      large         - Maximum threshold for acceptance
      median_lo     - Fraction of median to consider counting low
      median_hi     - Fraction of median to consider counting high
      signif        - Counts within this number of multiples of the 
                      standard dev will be kept (default = 3.3)
    """
    print 'Running white beam test'

    # What shall we call the output
    lhs_names = lhs_info('names')
    if len(lhs_names) > 0:
        ws_name = lhs_names[0]
    else:
        ws_name = '__do_white_test'

    # The output workspace will have the failed detectors masked
    range_check = FindDetectorsOutsideLimits(white_counts, ws_name, HighThreshold=large, LowThreshold=tiny)
    median_test = MedianDetectorTest(white_counts, ws_name, SignificanceTest=signif,
                                     LowThreshold=median_lo, HighThreshold=median_hi)

    num_failed = range_check['NumberOfFailures'].value + median_test['NumberOfFailures'].value
    
    return median_test.workspace(), num_failed

#-------------------------------------------------------------------------------

def _do_second_white_test(white_counts, comp_white_counts, tiny, large, median_lo,
                          median_hi, signif, variation, prev_masks=None):
    """
    Run additional tests comparing given another white beam count workspace, comparing
    to the first

    Required inputs:
    
      white_counts  - A workspace containing the integrated counts from a
                      white beam vanadium run
      comp_white_counts  - A workspace containing the integrated counts from a
                      white beam vanadium run
      tiny          - Minimum threshold for acceptance
      large         - Maximum threshold for acceptance
      median_lo     - Fraction of median to consider counting low
      median_hi     - Fraction of median to consider counting high
      signif          - Counts within this number of multiples of the 
                      standard dev will be kept
      variation     - Defines a range within which the ratio of the two counts is
                      allowed to fall in terms of the number of medians
    """
    
    print 'Running second white beam test'

    # What shall we call the output
    lhs_names = lhs_info('names')
    if len(lhs_names) > 0:
        ws_name = lhs_names[0]
    else:
        ws_name = '__do_second_white_test'
    
    if prev_masks is not None:
        MaskDetectors(comp_white_counts, MaskedWorkspace=prev_masks)
    # Do the white beam test
    __white_tests = _do_white_test(comp_white_counts, tiny, large, median_lo, median_hi, signif)[0]
    # and now compare it with the first
    effic_var = DetectorEfficiencyVariation(white_counts, __white_tests, ws_name, Variation=variation)
    
    mtd.deleteWorkspace(str(__white_tests))
    num_failed = effic_var['NumberOfFailures'].value 
    return effic_var.workspace(), num_failed

#------------------------------------------------------------------------------

def _do_background_test(sample_run, white_counts, comp_white_counts, bkgd_range,
                        bkgd_threshold, remove_zero, signif, prev_masks = None):
    """
    Run the background tests on the integrated sample run normalised by an
    integrated white beam run

    Required inputs:
    
      sample_run          - The run number or filepath of the sample run
      white_counts        - A workspace containing the integrated counts from a
                            white beam vanadium run
      comp_white_counts - A second workspace containing the integrated counts from a
                            different white beam vanadium run
      bkgd_range          - The background range as a list of 2 numbers: [min,max]. 
                            If not present then they are taken from the parameter file.
      bkgd_threshold      - High threshold for background removal in multiples of median
      remove_zero         - If true then zeroes in the data will count as failed (default = False)
      signif              - Counts within this number of multiples of the 
                            standard dev will be kept (default = 3.3)
      prev_ma sks         - If present then this masking is applied to the sample run before the
                            test is applied. It is expected as a MaskWorkspace
    """
    print 'Running background test'
    # Load and integrate the sample using the defined background range. If none is given use the
    # instrument defaults
    data_ws = common.load_run(sample_run, 'mono-sample')
    instrument = data_ws.getInstrument()
    if bkgd_range is None:
        min_value = float(instrument.getNumberParameter('bkgd-range-min')[0])
        max_value = float(instrument.getNumberParameter('bkgd-range-max')[0])
    elif len(bkgd_range) != 2:
        raise ValueError("The provided background range is incorrect. A list of 2 numbers is required.")
    else:
        min_value = bkgd_range[0]
        if min_value is None:
            float(instrument.getNumberParameter('bkgd-range-min')[0])
        max_value = bkgd_range[1]
        if max_value is None:
            float(instrument.getNumberParameter('bkgd-range-max')[0])

    # Get the total counts
    sample_counts = Integration(data_ws, '__counts_mono-sample', RangeLower=min_value, \
                                RangeUpper=max_value).workspace()
    
    # If we have another white beam then compute the harmonic mean of the counts
    # The harmonic mean: 1/av = (1/Iwbv1 + 1/Iwbv2)/2
    # We'll resuse the comp_white_counts workspace as we don't need it anymore
    white_count_mean = white_counts
    if comp_white_counts is not None:
        white_count_mean = (comp_white_counts * white_counts)/(comp_white_counts + white_counts)
        
    # Normalise the sample run
    sample_counts = Divide(sample_counts, white_count_mean, sample_counts).workspace()
    if comp_white_counts is not None:
        mtd.deleteWorkspace(str(white_count_mean))

    # If we need to remove zeroes as well then set the the low threshold to a tiny positive number
    if remove_zero:
        low_threshold = 1e-40
    else:
        low_threshold = -1.0

    # What shall we call the output
    lhs_names = lhs_info('names')
    if len(lhs_names) > 0:
        ws_name = lhs_names[0]
    else:
        ws_name = '__do_background_test'

    # Apply previous masking first
    if prev_masks is not None:
        MaskDetectors(sample_counts, MaskedWorkspace=prev_masks)
    median_test = MedianDetectorTest(sample_counts, ws_name, SignificanceTest=signif,
                                     LowThreshold=low_threshold, HighThreshold=bkgd_threshold)

    # Remove temporary
    mtd.deleteWorkspace(str(sample_counts))

    num_failed = median_test['NumberOfFailures'].value
    
    return median_test.workspace(), num_failed

    
    



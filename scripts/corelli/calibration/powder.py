# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from typing import List, Optional
from mantid.api import mtd
from mantid.dataobjects import Workspace2D
from mantid.kernel import logger
from mantid.simpleapi import DeleteWorkspace, LoadEventNexus, Plus, Rebin


def load_and_rebin(runs: List[int],
                   output_workspace: str,
                   rebin_params: List[float],
                   banks: Optional[List[int]] = None) -> Workspace2D:
    r"""
    @brief Load a list of run numbers and rebin

    This function assumes the runs are large and events cannot be all loaded into memory. Hence, a run is loaded
    at a time, rebinned to TOF counts, events are dropped, and counts are added to the cumulative histogram
    resulting from loading the previous runs.

    @param runs : list of run numbers
    @param rebin_params : a triad of first, step, and last. A negative step indicates logarithmic binning
    @param output_workspace : the name of the output `MatrixWorkspace`
    @param banks : list of bank numbers, if one wants to load only certain banks.
    @return handle to the output workspace
    """
    instrument = 'CORELLI'
    kwargs = {} if banks is None else {'BankName': ','.join([f'bank{b}' for b in banks])}

    # Load the first run
    logger.information(f'Loading run {runs[0]}. {len(runs)} runs remaining to be loaded')
    LoadEventNexus(Filename=f'{instrument}_{runs[0]}', OutputWorkspace=output_workspace, LoadLogs=False, **kwargs)
    if rebin_params is not None:
        Rebin(InputWorkspace=output_workspace,
              OutputWorkspace=output_workspace,
              Params=rebin_params,
              PreserveEvents=False)
    # Iteratively load the remaining run, adding to the final workspace each time
    try:
        single_run = '__single_run_' + output_workspace
        for i, run in enumerate(runs[1:]):
            logger.information(f'Loading run {run}. {len(runs) - 1 - i} runs remaining to be loaded')
            LoadEventNexus(Filename=f'{instrument}_{run}', OutputWorkspace=single_run, LoadLogs=False, **kwargs)
            if rebin_params is not None:
                Rebin(InputWorkspace=single_run, OutputWorkspace=single_run, Params=rebin_params, PreserveEvents=False)
            Plus(LHSWorkspace=output_workspace, RHSWorkspace=single_run, OutputWorkspace=output_workspace)
            DeleteWorkspace(single_run)  # save memory as quick as possible
    except RuntimeError:
        DeleteWorkspace(single_run)  # a bit of clean-up
    return mtd[output_workspace]

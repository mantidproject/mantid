#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
This module provides a new set of visualization tools to analyze the calibration
results from SCDCalibratePanels2 (on a per bank base).
"""

import logging
import numpy as np
from typing import List, Dict, Union
from mantid.dataobjects import PeaksWorkspace
from mantid.simpleapi import mtd, FilterPeaks


def get_plot_data(
    pws: PeaksWorkspace,
    banknames: List[str],
) -> Dict[str, np.ndarray]:
    """
    Gather data for Panel diagnostics

    @param pws: target PeaksWorkspace, must have an instrument attached
    @param banknames: list of bank names to gather
    """
    pltdata = {}
    oriented_lattice = pws.sample().getOrientedLattice()
    # det info
    pltdata["banknames"] = np.array(
        [pws.row(i)["BankName"] for i in range(pws.getNumberPeaks()) if pws.row(i)["BankName"] in banknames])
    pltdata["cols"] = np.array(
        [pws.row(i)["Col"] for i in range(pws.getNumberPeaks()) if pws.row(i)["BankName"] in banknames])
    pltdata["rows"] = np.array(
        [pws.row(i)["Row"] for i in range(pws.getNumberPeaks()) if pws.row(i)["BankName"] in banknames])
    # compute chi2_qsample
    qsample = [pws.getPeak(i).getQSampleFrame() for i in range(pws.getNumberPeaks()) if pws.row(i)["BankName"] in banknames]
    qsample_ideal = [
        np.array(oriented_lattice.qFromHKL(pws.getPeak(i).getIntHKL())) for i in range(pws.getNumberPeaks())
        if pws.row(i)["BankName"] in banknames
    ]
    pltdata["chi2_qsample"] = np.array([((qs - qs0)**2 / np.linalg.norm(qs0)**2).sum() for qs, qs0 in zip(qsample, qsample_ideal)])
    # compute chi2_dspacing
    dspacing = [pws.getPeak(i).getDSpacing() for i in range(pws.getNumberPeaks()) if pws.row(i)["BankName"] in banknames]
    dspacing_ideal = [
        oriented_lattice.d(pws.getPeak(i).getIntHKL()) for i in range(pws.getNumberPeaks())
        if pws.row(i)["BankName"] in banknames
    ]
    pltdata["chi2_dspacing"] = np.array([(d / d0 - 1)**2 for d, d0 in zip(dspacing, dspacing_ideal)])
    return pltdata


def SCDCalibratePanels2PanelDiagnosticsPlot(
    peaksWorkspace_engineering: Union[PeaksWorkspace, str],
    peaksWorkspace_calibrated: Union[PeaksWorkspace, str],
    banknames: Union[str, List[str]] = None,
    generate_report: bool = True,
    use_logscale: bool = True,
    config: Dict[str, str] = {
        "prefix": "fig",
        "type": "png",
        "saveto": ".",
        "mode": "boxplot",
    },
    showPlots: bool = True,
) -> List[Dict[str, np.ndarray]]:
    """
    Visualization of the diagnostic for SCDCalibratePanels2 on a per
    panel (bank) basis.

    @param peaksWorkspace: peaks workspace with calibrated instrument applied
    @param banknames: bank(s) for diagnostics
    @param generate_report: toggle on report generation, default is True.
    @param use_logscale: use log scale to plot Chi2, default is True.
    @param config: plot configuration dictionary
            type: ["png", "pdf", "jpeg"]
            mode: ["boxplot", "overlay"]
    @param showPlots: open diagnostics plots after generating them.
    """
    # parse input
    pws_eng = mtd[peaksWorkspace_engineering] if isinstance(peaksWorkspace_engineering, str) else peaksWorkspace_engineering
    logging.info(f"Peaksworkspace at negineering position: {peaksWorkspace_engineering.name()}.")
    pws_cal = mtd[peaksWorkspace_calibrated] if isinstance(peaksWorkspace_calibrated, str) else peaksWorkspace_calibrated
    logging.info(f"Peaksworkspace at calibrated position: {peaksWorkspace_calibrated.name()}.")
    #
    if config["type"].lower() == "png" and generate_report:
        logging.warning(f"Report must be in PDF format, ignoring type from config dict")
    # remove non-index peaks
    logging.info("Excluding non-index peaks")
    tmp_eng = FilterPeaks(
        InputWorkspace=pws_eng,
        Operator='>',
        FilterVariable='h^2+k^2+l^2',
        FilterValue=0,
    )
    tmp_cal = FilterPeaks(
        InputWorkspace=pws_cal,
        Operator='>',
        FilterVariable='h^2+k^2+l^2',
        FilterValue=0,
    )

    # process all banks if banknames is None
    if banknames is None:
        # NOTE: SCDCalibratePanels will not change the assigned the bank name, therefore only need to
        #       query the bankname from tmp_cal
        banknames = set([tmp_cal.row(i)["BankName"] for i in range(tmp_cal.getNumberPeaks())])
    elif isinstance(banknames, str):
        banknames = [me.strip() for me in banknames.split(",")]
    else:
        pass

    # prepare plotting data
    # NOTE: take advantage of the table structure
    logging.info("Prepare plotting data")
    pltdata_eng = get_plot_data(tmp_eng, banknames)
    pltdata_cal = get_plot_data(tmp_cal, banknames)

    # generate plots
    logging.info("Generating plots")

    # close backend handle
    if not showPlots:
        plt.close("all")

    return [pltdata_eng, pltdata_cal]


if __name__ == "__main__":
    pass

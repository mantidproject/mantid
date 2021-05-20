#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import logging
import numpy as np
import matplotlib.pyplot as plt
from mantid.dataobjects import PeaksWorkspace
from typing import List, Dict, Union


def collectAllBankNames(peaksWorkspace: PeaksWorkspace) -> list[str]:
    """
    Collect all bank names from given peaks workspace

    @param peaksWorkspace: input peaks workspace

    @return: list of unique bank names
    """
    pass


def SCDCalibratePanels2DiagnosticsPlotBank(
    filteredPeaksWS: PeaksWorkspace,
    figname: str,
    savedir: str,
    showPlots: bool,
) -> None:
    """
    Generate correlation plot of observed-calculated col&row ID
    plot for given peaksworkspace filtered by bank

    @param filteredPeaksWS: a peaks workspace with only one bank
    @figname: output figure name
    @savedir: save directory
    @showPlots: display plots
    """
    pws = filteredPeaksWS
    npeaks = pws.getNumberPeaks()
    ub = pws.sample().getOrientedLattice().getUB()
    # plot data container
    rowsID_measured = np.zeros(npeaks)
    colsID_measured = np.zeros(npeaks)
    rowsID_calculated = np.zeros(npeaks)
    colsID_calculated = np.zeros(npeaks)

    # iterative through peaks to generate data
    for i in range(npeaks):
        entry = pws.row(i)
        rowsID_measured[i] = entry["Row"]
        colsID_measured[i] = entry["Col"]
        # compute the calculated entry now
        pk = pws.getPeak(i)
        gm = pk.getGoniometerMatrix()
        hkl = np.array(pk.getHKL())
        qsample = 2 * np.pi * (ub @ hkl)
        qlab = gm @ qsample
        # utilize the IDF embeded in current pws
        pk_new = pws.createPeak(qlab)
        pws.addPeak(pk_new)
        entry_new = pws.row(i + npeaks)
        rowsID_calculated[i] = entry_new["Row"]
        colsID_calculated[i] = entry_new["Col"]

    # plotting
    fig, axes = plt.subplots(1, 2)  # side by side
    # plot col cmp
    # plot row cmp
    # display
    if showPlots:
        fig.show(block=False)
    # save
    logging.info(f"Figure {figname} is saved to {savedir}.")


def SCDCalibratePanels2DiagnosticsPlot(
    peaksWorkspace: PeaksWorkspace,
    banknames: Union[str, List[str]] = None,
    config: Dict[str, str] = {
        "prefix": "fig",
        "type": "png",
        "saveto": ".",
    },
    showPlots: bool = True,
) -> None:
    """
    Generate diagnostic plots from SCDCalibratePanels2

    @param peaksWorkspace: peaks workspace with calibrated instrument applied
    @param banknames: bank(s) for diagnostics
    @param config: plot configuration dictionary
    @param showPlots: open diagnostics plots after generating them.

    @returs: None
    """
    logging.info(f"Start diagnostics with {peaksWorkspace.getName()}.")
    pass


if __name__ == "__main__":
    print("Running SCDCalibratePanels2Diagnostics as script")

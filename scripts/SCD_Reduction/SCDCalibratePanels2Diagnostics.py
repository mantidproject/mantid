#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os
import logging
import numpy as np
import matplotlib.pyplot as plt
from typing import List, Dict, Union
from mantid.dataobjects import PeaksWorkspace
from mantid.simpleapi import mtd, FilterPeaks


def plot_bank_modern(
    rowsID_observed: np.ndarray,
    colsID_observed: np.ndarray,
    rowsID_calculated: np.ndarray,
    colsID_calculated: np.ndarray,
    bankname: str,
    npeaks: int,
) -> plt.Figure:
    """
    Plot modern style plots for diagnostics

    @param rowsID_observed
    @param colsID_observed
    @param rowsID_calculated
    @param colsID_calculated
    @param bankname
    @param npeaks

    @return: figure handle
    """
    # fancy plotting
    fig = plt.figure(figsize=(14, 7))
    gs = fig.add_gridspec(7, 15)
    # ---- CONFIG LAYOUT -----
    # -- col combo plot
    hist_observed_col = fig.add_subplot(gs[1:, 6])  # hist_y1
    hist_observed_col.get_yaxis().set_ticks([])
    hist_observed_col.set_xlabel("npeaks")
    hist_calculated_col = fig.add_subplot(gs[0, 0:6])  # hist_x1
    hist_calculated_col.get_xaxis().set_ticks([])
    hist_calculated_col.set_ylabel("npeaks")
    hist_calculated_col.set_title(f"{bankname}: Column")
    scatter_col = fig.add_subplot(gs[1:, 0:6])  # scatter1
    scatter_col.grid(color='k', linestyle='--', alpha=0.25)
    scatter_col.set_xlabel("calculated column number")
    scatter_col.set_ylabel("observed column number")
    # -- row combo plot
    hist_observed_row = fig.add_subplot(gs[1:, 14])  # hist_y2
    hist_observed_row.get_yaxis().set_ticks([])
    hist_observed_row.set_xlabel("npeaks")
    hist_calculated_row = fig.add_subplot(gs[0, 8:14])  # hist_x2
    hist_calculated_row.get_xaxis().set_ticks([])
    hist_calculated_row.set_ylabel("npeaks")
    hist_calculated_row.set_title(f"{bankname}: Row")
    scatter_row = fig.add_subplot(gs[1:, 8:14])  # scatter2
    scatter_row.grid(color='k', linestyle='--', alpha=0.25)
    scatter_row.set_xlabel("calculated row number")
    scatter_row.set_ylabel("observed row number")
    # ----- PLOT DATA -----
    # -- plot col
    max_coln = max(colsID_observed.max(), colsID_calculated.max())
    scatter_col.plot([0, max_coln], [0, max_coln], "k:", alpha=0.5)
    scatter_col.plot(colsID_calculated, colsID_observed, 'r+')
    scatter_col.set_xlim([-5, colsID_calculated.max() + 5])
    scatter_col.set_ylim([-5, colsID_observed.max() + 5])
    scatter_col.text(0.5, 0.2, f"Number of Peaks = {npeaks}", transform=scatter_col.transAxes)
    hist_calculated_col.hist(colsID_calculated, bins=1 + int(colsID_calculated.max()), color='red')
    hist_calculated_col.set_xlim([-5, colsID_calculated.max() + 5])
    hist_observed_col.hist(colsID_observed, bins=1 + int(colsID_observed.max()), color='red', orientation="horizontal")
    hist_observed_col.set_ylim([-5, colsID_observed.max() + 5])
    # -- plot row
    max_rown = max(rowsID_observed.max(), rowsID_calculated.max())
    scatter_row.plot([0, max_rown], [0, max_rown], "k:", alpha=0.5)
    scatter_row.plot(rowsID_calculated, rowsID_observed, 'b+')
    scatter_row.set_xlim([-5, rowsID_calculated.max() + 5])
    scatter_row.set_ylim([-5, rowsID_observed.max() + 5])
    scatter_row.text(0.5, 0.2, f"Number of Peaks = {npeaks}", transform=scatter_row.transAxes)
    hist_calculated_row.hist(rowsID_calculated, bins=int(rowsID_calculated.max()), color='blue')
    hist_calculated_row.set_xlim([-5, rowsID_calculated.max() + 5])
    hist_observed_row.hist(rowsID_observed, bins=int(rowsID_observed.max()), color='blue', orientation="horizontal")
    hist_observed_row.set_ylim([-5, rowsID_observed.max() + 5])

    return fig


def plot_bank_legacy(
    rowsID_observed: np.ndarray,
    colsID_observed: np.ndarray,
    rowsID_calculated: np.ndarray,
    colsID_calculated: np.ndarray,
    bankname: str,
    npeaks: int,
) -> plt.Figure:
    """
    Plot legacy style plots for diagnostics

    @param rowsID_observed
    @param colsID_observed
    @param rowsID_calculated
    @param colsID_calculated
    @param bankname
    @param npeaks

    @return: figure handle
    """
    # plotting
    fig, axes = plt.subplots(1, 2, figsize=(10, 5))  # side by side
    # plot col cmp
    # -- plot the diagnal helper line
    max_rown = max(rowsID_observed.max(), rowsID_calculated.max())
    axes[0].plot([0, max_rown], [0, max_rown], "b:", alpha=0.5)
    # -- plot the data
    axes[0].plot(rowsID_calculated, rowsID_observed, 'r+')
    # -- config grid
    axes[0].grid(color='k', linestyle='--', alpha=0.25)
    # -- config axes
    axes[0].set_xlabel("calculated row number")
    axes[0].set_ylabel("observed row number")
    axes[0].set_title(f"Detector Row Number Comparison, {bankname}")
    axes[0].text(0.5, 0.2, f"Number of Peaks = {npeaks}", transform=axes[0].transAxes)
    axes[0].set_aspect(aspect='equal')
    # plot row cmp
    # -- plot the diagnal helper line
    max_coln = max(colsID_observed.max(), colsID_calculated.max())
    axes[1].plot([0, max_coln], [0, max_coln], "b:", alpha=0.5)
    # -- plot the data
    axes[1].plot(colsID_calculated, colsID_observed, 'r+')
    # -- config grid
    axes[1].grid(color='k', linestyle='--', alpha=0.25)
    # -- config axes
    axes[1].set_xlabel("calculated column number")
    axes[1].set_ylabel("observed column number")
    axes[1].set_title(f"Detector Column Number Comparison, {bankname}")
    axes[1].text(0.5, 0.2, f"Number of Peaks = {npeaks}", transform=axes[1].transAxes)
    axes[1].set_aspect(aspect='equal')

    return fig


def SCDCalibratePanels2DiagnosticsPlotBank(
    filteredPeaksWS: PeaksWorkspace,
    figname: str,
    savedir: str,
    showPlots: bool,
    mode: str = 'modern',
) -> None:
    """
    Generate correlation plot of observed-calculated col&row ID
    plot for given peaksworkspace filtered by bank

    @param filteredPeaksWS: a peaks workspace with only one bank
    @figname: output figure name
    @savedir: save directory
    @showPlots: display plots
    @mode: plotting mode, ["modern", "legacy"]

    @return: None
    """
    pws = filteredPeaksWS
    npeaks = pws.getNumberPeaks()
    ub = pws.sample().getOrientedLattice().getUB()
    # plot data container
    rowsID_observed = np.zeros(npeaks)
    colsID_observed = np.zeros(npeaks)
    rowsID_calculated = np.zeros(npeaks)
    colsID_calculated = np.zeros(npeaks)
    # use the first peak to infer the bank name
    bankname = pws.row(0)["BankName"]

    # iterative through peaks to generate data
    for i in range(npeaks):
        entry = pws.row(i)
        rowsID_observed[i] = entry["Row"]
        colsID_observed[i] = entry["Col"]
        # compute the calculated entry now
        pk = pws.getPeak(i)
        gm = pk.getGoniometerMatrix()
        hkl = np.array(pk.getIntHKL())
        qsample = 2 * np.pi * (ub @ hkl)
        qlab = gm @ qsample
        # utilize the IDF embeded in current pws
        pk_new = pws.createPeak(qlab)
        pws.addPeak(pk_new)
        entry_new = pws.row(i + npeaks)
        rowsID_calculated[i] = entry_new["Row"]
        colsID_calculated[i] = entry_new["Col"]

    if mode.lower() == "modern":
        fig = plot_bank_modern(
            rowsID_observed,
            colsID_observed,
            rowsID_calculated,
            colsID_calculated,
            bankname,
            npeaks,
        )
    elif mode.lower() == "legacy":
        fig = plot_bank_legacy(
            rowsID_observed,
            colsID_observed,
            rowsID_calculated,
            colsID_calculated,
            bankname,
            npeaks,
        )
    else:
        logging.warning(f"unknown mode={mode}, falling back to legacy mode.")
        fig = plot_bank_legacy(
            rowsID_observed,
            colsID_observed,
            rowsID_calculated,
            colsID_calculated,
            bankname,
            npeaks,
        )

    # save
    # NOTE: save first, then display to avoid weird corruption of written figure
    fig.savefig(os.path.join(savedir, figname))
    # display
    if showPlots:
        fig.show()
    # notify users
    logging.info(f"Figure {figname} is saved to {savedir}.")


def SCDCalibratePanels2DiagnosticsPlot(
    peaksWorkspace: Union[PeaksWorkspace, str],
    banknames: Union[str, List[str]] = None,
    config: Dict[str, str] = {
        "prefix": "fig",
        "type": "png",
        "saveto": ".",
        "mode": "modern",
    },
    showPlots: bool = True,
) -> None:
    """
    Generate diagnostic plots from SCDCalibratePanels2

    @param peaksWorkspace: peaks workspace with calibrated instrument applied
    @param banknames: bank(s) for diagnostics
    @param config: plot configuration dictionary
            type: ["png", "pdf", "jpeg"]
            mode: ["modern", "legacy"]
    @param showPlots: open diagnostics plots after generating them.

    @returs: None
    """
    # parse input
    pws = mtd[peaksWorkspace] if isinstance(peaksWorkspace, str) else peaksWorkspace
    logging.info(f"Start diagnostics with {peaksWorkspace.name()}.")

    # process all banks if banknames is None
    if banknames is None:
        banknames = set([pws.row(i)["BankName"] for i in range(pws.getNumberPeaks())])
    elif isinstance(banknames, str):
        banknames = [me.strip() for me in banknames.split(",")]
    else:
        pass

    # one bank at a time
    for bn in banknames:
        logging.info(f"--processing bank: {bn}")
        pws_filtered = FilterPeaks(
            InputWorkspace=pws,
            FilterVariable='h^2+k^2+l^2',
            FilterValue=0,
            Operator='>',
            BankName=bn,
        )
        # generate the plot
        figname = f"{config['prefix']}_{bn}.{config['type']}"
        SCDCalibratePanels2DiagnosticsPlotBank(
            filteredPeaksWS=pws_filtered,
            figname=figname,
            savedir=config["saveto"],
            showPlots=showPlots,
            mode=config["mode"],
        )

    # close backend handle
    if not showPlots:
        plt.close("all")


if __name__ == "__main__":
    logging.warning("Running SCDCalibratePanels2Diagnostics as script is not supported at the moment.")

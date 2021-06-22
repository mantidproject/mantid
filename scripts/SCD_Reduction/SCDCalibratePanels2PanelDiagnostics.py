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

import os
import logging
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from typing import List, Tuple, Dict, Union
from mantid.dataobjects import PeaksWorkspace
from mantid.simpleapi import mtd


def bank_boxplot(
    pltdata_eng: Dict[str, np.ndarray],
    pltdata_cal: Dict[str, np.ndarray],
    figname: str = None,
    saveto: str = ".",
    use_logscale: bool = True,
    show_plots: bool = True,
) -> None:
    """
    Generate scatter plots of chi2 computed based on the
    given calibrated instrument.
    Both Chi2(q) and Chi2(d) are used to evaluate the Chi2
    results on each bank.

    @param pltdata_eng: plot data collected at engineering position.
    @param pltdata_cal: plot data collected at calibrated position.
    @param figname: figure name.
    @param saveto: directory to save the output
    @param use_logscale: use log scale to plot Chi2, default is True.
    """
    # prep data
    chi2qs_eng, chi2ds_eng, _, bn_num_eng = get_boxplot_data(pltdata_eng)
    chi2qs_cal, chi2ds_cal, _, bn_num_cal = get_boxplot_data(pltdata_cal)

    #
    fig = plt.figure(figsize=(12, 4))
    # -------
    # qsample
    # -------
    ax_qs = fig.add_subplot(2, 1, 1)
    #
    box_eng = ax_qs.boxplot(chi2qs_eng, positions=bn_num_eng, notch=True, showfliers=False)
    for _, lines in box_eng.items():
        for line in lines:
            line.set_color('b')
            line.set_linestyle(":")
    #
    box_cal = ax_qs.boxplot(chi2qs_cal, positions=bn_num_cal, notch=True, showfliers=False)
    for _, lines in box_cal.items():
        for line in lines:
            line.set_color('r')
    #
    ax_qs.set_xlabel(r'[bank no.]')
    ax_qs.set_ylabel(r'$\chi^2(Q)$')
    ax_qs.set_title(r'Goodness of fit')
    if use_logscale:
        ax_qs.set_yscale("log")
    ax_qs.legend(
        [box_eng["boxes"][0], box_cal["boxes"][0]],
        ["engineering", "calibration"],
        loc='lower right',
    )
    # --------
    # dspacing
    # --------
    ax_ds = fig.add_subplot(2, 1, 2)
    box_eng = ax_ds.boxplot(chi2ds_eng, positions=bn_num_eng, notch=True, showfliers=False)
    for _, lines in box_eng.items():
        for line in lines:
            line.set_color('b')
            line.set_linestyle(":")
    box_cal = ax_ds.boxplot(chi2ds_cal, positions=bn_num_cal, notch=True, showfliers=False)
    for _, lines in box_cal.items():
        for line in lines:
            line.set_color('r')
    ax_ds.set_xlabel(r'[bank no.]')
    ax_ds.set_ylabel(r'$\chi^2(d)$')
    if use_logscale:
        ax_ds.set_yscale("log")
    ax_ds.legend(
        [box_eng["boxes"][0], box_cal["boxes"][0]],
        ["engineering", "calibration"],
        loc='lower right',
    )

    # save figure
    # NOTE: save first, then display to avoid weird corruption of written figure
    fig.savefig(os.path.join(saveto, figname))
    # display
    if show_plots:
        fig.show()
    # notify users
    logging.info(f"Figure {figname} is saved to {saveto}.")


def get_boxplot_data(pltdata: Dict[str, np.ndarray]) -> Tuple[list, list, np.ndarray, np.ndarray]:
    """
    Helper function to organize the plot data for box plot

    @param pltdata: input dict containing plot data

    @return chi2qs: list
            chi2ds: list
            bn_str: np.ndarray (1, N_bank)
            bn_num: np.ndarray (1, N_bank)
    """
    bn_str = np.unique(pltdata["banknames"])
    bn_num = np.array([int(bn.replace("bank", "")) for bn in bn_str])
    chi2qs = [pltdata["chi2_qsample"][np.where(pltdata["banknames"] == bn)] for bn in bn_str]
    chi2ds = [pltdata["chi2_dspacing"][np.where(pltdata["banknames"] == bn)] for bn in bn_str]
    return chi2qs, chi2ds, bn_str, bn_num


def bank_overlay(
    pltdata_eng: Dict[str, np.ndarray],
    pltdata_cal: Dict[str, np.ndarray],
    generate_report: bool = True,
    use_logscale: bool = True,
) -> None:
    """
    """
    pp = PdfPages('test.pdf')
    pp.close()
    pass


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
    pltdata["banknames"] = np.array([pws.row(i)["BankName"] for i in range(pws.getNumberPeaks()) if pws.row(i)["BankName"] in banknames])
    pltdata["cols"] = np.array([pws.row(i)["Col"] for i in range(pws.getNumberPeaks()) if pws.row(i)["BankName"] in banknames])
    pltdata["rows"] = np.array([pws.row(i)["Row"] for i in range(pws.getNumberPeaks()) if pws.row(i)["BankName"] in banknames])
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
        oriented_lattice.d(pws.getPeak(i).getIntHKL()) for i in range(pws.getNumberPeaks()) if pws.row(i)["BankName"] in banknames
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
        "type": "pdf",
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
    tmp_eng = mtd[peaksWorkspace_engineering] if isinstance(peaksWorkspace_engineering, str) else peaksWorkspace_engineering
    logging.info(f"Peaksworkspace at negineering position: {peaksWorkspace_engineering.name()}.")
    tmp_cal = mtd[peaksWorkspace_calibrated] if isinstance(peaksWorkspace_calibrated, str) else peaksWorkspace_calibrated
    logging.info(f"Peaksworkspace at calibrated position: {peaksWorkspace_calibrated.name()}.")

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
    #
    if config["mode"] == "boxplot":
        logging.info("Mode -> boxplot for chi2")
        # call the plotter
        bank_boxplot(
            pltdata_eng=pltdata_eng,
            pltdata_cal=pltdata_cal,
            figname=f"{config['prefix']}.{config['type']}",
            saveto=config["saveto"],
            use_logscale=use_logscale,
            show_plots=showPlots,
        )
    elif config["mode"] == "overlay":
        logging.info("Mode -> overlay chi2 on bank")
        pass
    else:
        raise ValueError(f"Unsupported mode detected, only support boxplot and overlay.")

    # close backend handle
    if not showPlots:
        plt.close("all")

    return [pltdata_eng, pltdata_cal]


if __name__ == "__main__":
    pass

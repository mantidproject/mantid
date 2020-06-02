# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.interfacemanager import InterfaceManager
from mantidqt.plotting.figuretype import figure_type, FigureType

BASE_URL = "qthelp://org.mantidproject/doc/plotting/"
INDEX_PAGE = "index.html"
PLOT1D_PAGE = "1DPlotsHelp.html"
TILED_PAGE = "TiledPlotsHelp.html"
WATERFALL_PAGE = "WaterfallPlotsHelp.html"
PLOT3D_PAGE = "3DPlotsHelp.html"
COLORFILL_PAGE = "ColorfillPlotsHelp.html"

# Create a plot page for each enumeration in FigureType
# The values can be edited if there is a more relevant documentation page
# For example Line and Error bar plots currently point to the same page.
HELP_PAGES = {FigureType.Other: BASE_URL + INDEX_PAGE,
              FigureType.Line: BASE_URL + PLOT1D_PAGE,
              FigureType.Errorbar: BASE_URL + PLOT1D_PAGE,
              FigureType.Waterfall: BASE_URL + WATERFALL_PAGE,
              FigureType.Wireframe: BASE_URL + PLOT3D_PAGE,
              FigureType.Surface: BASE_URL + PLOT3D_PAGE,
              FigureType.Image: BASE_URL + COLORFILL_PAGE,
              FigureType.Contour: BASE_URL + COLORFILL_PAGE}


class PlotHelpPages(object):
    @classmethod
    def show_help_page_for_figure(cls, figure):
        fig_type = figure_type(figure)
        doc_url = HELP_PAGES[fig_type]
        InterfaceManager().showHelpPage(doc_url)

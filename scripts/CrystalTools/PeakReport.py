# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""
This is a module for creating peak integration reports.
"""

import os
import mantid.api
from mantidplot import plotSlice
from reportlab.lib.pagesizes import letter
from reportlab.lib.styles import getSampleStyleSheet
from reportlab.platypus import Image, Paragraph, SimpleDocTemplate, Spacer, Table


class PeakReport(object):
    """
    Peak Report is a class used to creating one or more peak integration reports as PDFs.
    """

    # output directory for the report.
    __out_location = None
    __log_scale = False
    __show_background = False
    __background_color = None
    __foreground_color = None
    __image_files = []
    __zoom_factor = 1
    __xydim = None

    def __init__(self, out_location):
        """
        Arguments:
          out_location -- Output directory location for reports.
        """
        if not os.path.exists(out_location):
            raise ValueError(out_location + " Directory not exist!")
        self.__out_location = out_location

    def clean_files(self):
        # Clean up generated image files
        for ifile in self.__image_files:
            os.remove(ifile)
            self.__image_files.remove(ifile)

    def __del__(self):
        self.clean_files()

    def set_log_scale(self, log_scale):
        """
        Arguments:
         log_scale -- True for log scaling
        """
        self.__log_scale = log_scale

    def set_show_background(self, show_background):
        """
        Arguments:
         show_background -- True to show background
        """
        self.__show_background = show_background

    def set_foreground_color(self, foreground_color):
        """
        Arguments:
         foreground_color-- Foreground color (qcolor)
        """
        self.__foreground_color = foreground_color

    def set_background_color(self, background_color):
        """
        Arguments:
         background_color -- Background color (qcolor)
        """
        self.__background_color = background_color

    def set_zoom(self, zoom_factor):
        """
        Arguments:
         zoom_factor -- Factor to zoom in or out by. Zoom < 1 denotes a zoom out. > 1 is zooming in.
        """
        self.__zoom_factor = zoom_factor

    def set_xy_dim(self, dim_x, dim_y):
        """
        Arguments:
         dim_x : X dimension name
         dim_y : Y dimension name
        """
        self.__xydim = (dim_x, dim_y)

    def make_report(self, md_workspace, peaks_workspace, pdf_name=None):
        """
        Arguments:
          md_workspace -- md workspace to process.
          peaks_workspace -- peaks workspace to process.
          pdf_name -- optional name of the pdf to create. Auto generated otherwise.
        """

        if not isinstance(md_workspace, mantid.api.IMDWorkspace):
            raise ValueError("A MD Workspace has not been provided.")
        if not isinstance(peaks_workspace, mantid.api.IPeaksWorkspace):
            raise ValueError("A Peaks Worksapce has not been provided.")
        if not pdf_name:
            pdf_name = md_workspace.name() + "IntegrationReport.pdf"

        doc = SimpleDocTemplate(
            self.__out_location + pdf_name, pagesize=letter, leftMargin=2, rightMargin=2, topMargin=2.5, bottomMargin=2, showBoundary=1
        )
        parts = list()
        styles = getSampleStyleSheet()
        title = Paragraph(
            "Peak Integration Report for %s" % md_workspace.name(),
            styles["Heading1"],
        )
        parts.append(title)
        parts.append(Paragraph("MD Workspace: %s" % md_workspace.name(), styles["Normal"]))
        parts.append(Paragraph("Peaks Workspace: %s" % peaks_workspace.name(), styles["Normal"]))
        parts.append(Spacer(0, 10))

        svw = plotSlice(md_workspace, colorscalelog=self.__log_scale)

        # Choose a different projection if one is specified
        if self.__xydim:
            svw.setXYDim(self.__xydim[0], self.__xydim[1])

        sv = svw.getSlicer()
        composite_presenter = sv.setPeaksWorkspaces([peaks_workspace.name()])
        peaks_presenter = composite_presenter.getPeaksPresenter(peaks_workspace.name())
        if self.__show_background:
            peaks_presenter.showBackgroundRadius(self.__show_background)
        if self.__background_color:
            peaks_presenter.setBackgroundColor(self.__background_color)
        if self.__foreground_color:
            peaks_presenter.setForegroundColor(self.__foreground_color)

        do_zoom = self.__zoom_factor != 1

        for i in range(peaks_workspace.rowCount()):
            peaks_presenter.zoomToPeak(i)

            # zoom in or out refinements
            if do_zoom:
                svw.zoomBy(self.__zoom_factor)

            filename = os.path.join(self.__out_location, str(i) + ".png")
            # Add for clean-up later
            self.__image_files.append(filename)
            svw.saveImage(filename)

            # Create an image
            img = Image(filename, width=150, height=100)
            # Get the peak object
            peak = peaks_workspace.getPeak(i)

            infoData = [
                ["PeakNumber:", i],
                ["Run Number:", peak.getRunNumber()],
                ["Intensity:", peak.getIntensity()],
                ["TOF:", peak.getTOF()],
            ]
            coordData = [["Q Lab:", peak.getQLabFrame()], ["Q Sample:", peak.getQSampleFrame()], ["HKL:", peak.getHKL()]]
            if "DetID" in peaks_workspace.getColumnNames():
                detector_id = peaks_workspace.row(i)["DetID"]
                coordData = [["Detector Id:", detector_id]] + coordData
            data = [[img, Table(infoData), Table(coordData)]]

            colwidths = (150, 160, 160)

            table = Table(data, colwidths, hAlign="LEFT")
            parts.append(table)
            parts.append(Spacer(0, 10))

        doc.build(parts)

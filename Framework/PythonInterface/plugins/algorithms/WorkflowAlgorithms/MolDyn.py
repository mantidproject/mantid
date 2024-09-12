# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *

import os


class MolDyn(PythonAlgorithm):
    def category(self):
        return "Workflow\\Inelastic;Inelastic\\DataHandling;Simulation"

    def summary(self):
        return "Imports and processes simulated functions from nMOLDYN."

    def PyInit(self):
        self.declareProperty("Data", "", validator=StringMandatoryValidator(), doc="")

        self.declareProperty(StringArrayProperty("Functions"), doc="A list of function to load")

        self.declareProperty(WorkspaceProperty("Resolution", "", Direction.Input, PropertyMode.Optional), doc="Resolution workspace")

        self.declareProperty(
            name="MaxEnergy", defaultValue=Property.EMPTY_DBL, doc="Crop the result spectra at a given energy (leave blank for no crop)"
        )

        self.declareProperty(name="SymmetriseEnergy", defaultValue=False, doc="Symmetrise functions in energy about x=0")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Output workspace name")

    def validateInputs(self):
        issues = dict()

        try:
            self._get_version_and_data_path()
        except ValueError as vex:
            issues["Data"] = str(vex)

        res_ws = self.getPropertyValue("Resolution")
        max_energy = self.getPropertyValue("MaxEnergy")

        if res_ws != "" and max_energy == Property.EMPTY_DBL:
            issues["MaxEnergy"] = "MaxEnergy must be set when convolving with an instrument resolution"

        return issues

    # pylint: disable=too-many-branches
    def PyExec(self):
        output_ws_name = self.getPropertyValue("OutputWorkspace")
        version, data_name, _ = self._get_version_and_data_path()

        logger.information("Detected data from nMoldyn version {0}".format(version))

        # Run nMOLDYN import
        if version == 3:
            LoadNMoldyn3Ascii(Filename=data_name, OutputWorkspace=output_ws_name, Functions=self.getPropertyValue("Functions"))
        elif version == 4:
            LoadNMoldyn4Ascii(Directory=data_name, OutputWorkspace=output_ws_name, Functions=self.getPropertyValue("Functions"))
        else:
            raise RuntimeError("No loader for input data")

        symmetrise = self.getProperty("SymmetriseEnergy").value
        max_energy_param = self.getProperty("MaxEnergy").value

        # Do processing specific to workspaces in energy
        if isinstance(mtd[output_ws_name], WorkspaceGroup):
            for ws_name in mtd[output_ws_name].getNames():
                if mtd[ws_name].getAxis(0).getUnit().unitID() == "Energy":
                    # Get an XMax value, default to max energy if not cropping
                    max_energy = mtd[ws_name].dataX(0).max()
                    logger.debug("Max energy in workspace %s: %f" % (ws_name, max_energy))

                    if max_energy_param != Property.EMPTY_DBL:
                        if max_energy_param > max_energy:
                            raise ValueError("MaxEnergy crop is out of energy range for function %s" % ws_name)
                        max_energy = max_energy_param

                    # If we are going to Symmetrise then there is no need to crop
                    # as the Symmetrise algorithm will do this
                    if symmetrise:
                        # Symmetrise the sample workspace in x=0
                        Symmetrise(InputWorkspace=ws_name, XMin=0, XMax=max_energy, OutputWorkspace=ws_name)

                    elif max_energy_param != Property.EMPTY_DBL:
                        CropWorkspace(InputWorkspace=ws_name, OutputWorkspace=ws_name, XMin=-max_energy, XMax=max_energy)

        # Do convolution if given a resolution workspace
        if self.getPropertyValue("Resolution") != "":
            # Create a workspace with enough spectra for convolution
            num_sample_hist = mtd[output_ws_name].getItem(0).getNumberHistograms()
            resolution_ws = self._create_res_ws(num_sample_hist)

            # Convolve all workspaces in output group
            for ws_name in mtd[output_ws_name].getNames():
                if "Energy" in mtd[ws_name].getAxis(0).getUnit().unitID():
                    self._convolve_with_res(resolution_ws, ws_name)
                else:
                    logger.information("Ignoring workspace %s in convolution step" % ws_name)

            # Remove the generated resolution workspace
            DeleteWorkspace(resolution_ws)

        # Set the output workspace
        self.setProperty("OutputWorkspace", output_ws_name)

    def _get_version_and_data_path(self):
        """
        Inspects the Data parameter to determine th eversion of nMoldyn it is
        loading from.

        @return Tuple of (version, data file/directory, file extension)
        """
        data = self.getPropertyValue("Data")
        extension = None

        if os.path.isdir(data):
            version = 4
        else:
            file_path = FileFinder.getFullPath(data)
            if os.path.isfile(file_path):
                version = 3
                extension = os.path.splitext(file_path)[1][1:]
                if extension not in ["dat", "cdl"]:
                    raise ValueError("Incorrect file type, expected file with extension .dat or .cdl")
                data = file_path
            else:
                raise RuntimeError("Unknown input data")

        return version, data, extension

    def _create_res_ws(self, num_sample_hist):
        """
        Creates a resolution workspace.

        @param num_sample_hist Number of histgrams required in workspace
        @returns The generated resolution workspace
        """
        res_ws_name = self.getPropertyValue("Resolution")

        num_res_hist = mtd[res_ws_name].getNumberHistograms()

        logger.notice("Creating resolution workspace.")
        logger.information("Sample has %d spectra\nResolution has %d spectra" % (num_sample_hist, num_res_hist))

        # If the sample workspace has more spectra than the resolution then copy the first spectra
        # to make a workspace with equal spectra count to sample
        if num_sample_hist > num_res_hist:
            logger.information("Copying first resolution spectra for convolution")

            res_ws_list = []
            for _ in range(0, num_sample_hist):
                res_ws_list.append(res_ws_name)

            res_ws_str_list = ",".join(res_ws_list)
            resolution_ws = ConjoinSpectra(res_ws_str_list, 0)

        # If sample has less spectra then crop the resolution to the same number of spectra as
        # resolution
        elif num_sample_hist < num_res_hist:
            logger.information("Cropping resolution workspace to sample")

            resolution_ws = CropWorkspace(InputWorkspace=res_ws_name, StartWorkspaceIndex=0, EndWorkspaceIndex=num_sample_hist)

        # If the spectra counts match then just use the resolution as it is
        else:
            logger.information("Using resolution workspace as is")

            resolution_ws = CloneWorkspace(res_ws_name)

        return resolution_ws

    def _convolve_with_res(self, resolution_ws, function_ws_name):
        """
        Performs convolution with an instrument resolution workspace.

        @param resolution_ws The resolution workspace to convolve with
        @param function_ws_name The workspace name for the function to convolute
        """

        logger.notice("Convoluting sample %s with resolution %s" % (function_ws_name, resolution_ws))

        # Convolve the symmetrised sample with the resolution
        ConvolveWorkspaces(OutputWorkspace=function_ws_name, Workspace1=resolution_ws, Workspace2=function_ws_name)

    def _plot_spectra(self, ws_name):
        """
        Plots up to the first 10 spectra from a workspace.

        @param ws_name Name of workspace to plot
        """

        num_hist = mtd[ws_name].getNumberHistograms()

        # Limit number of plotted histograms to 10
        if num_hist > 10:
            num_hist = 10

        # Build plot list
        plot_list = []
        for i in range(0, num_hist):
            plot_list.append(i)

        plotSpectrum(ws_name, plot_list)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(MolDyn)

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import PythonAlgorithm, AlgorithmFactory, PropertyMode, WorkspaceProperty, Progress, IMDHistoWorkspaceProperty, mtd
from mantid.kernel import Direction, FloatArrayProperty, FloatArrayLengthValidator, StringListValidator, FloatBoundedValidator
from mantid.geometry import SpaceGroupFactory, PointGroupFactory, SymmetryOperationFactory
from mantid.simpleapi import PlusMD
from mantid import config
from mantid import logger
import numpy as np


class ConvertWANDSCDtoQ(PythonAlgorithm):
    def category(self):
        return "DataHandling\\Nexus"

    def seeAlso(self):
        return ["LoadWANDSCD", "ConvertHFIRSCDtoMDE"]

    def name(self):
        return "ConvertWANDSCDtoQ"

    def summary(self):
        return "Convert the output of LoadWANDSCD to Q or HKL"

    def PyInit(self):
        self.declareProperty(
            IMDHistoWorkspaceProperty("InputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input), "Input Workspace"
        )
        self.declareProperty(
            IMDHistoWorkspaceProperty("NormalisationWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            "Workspace to use for normalisation",
        )
        self.declareProperty(
            WorkspaceProperty("UBWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            "Workspace containing the UB matrix to use",
        )
        self.declareProperty(
            IMDHistoWorkspaceProperty("BackgroundWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            "An optional Background Workspace",
        )
        self.declareProperty("Wavelength", 1.488, validator=FloatBoundedValidator(0.0), doc="Wavelength to set the workspace")
        self.declareProperty("S1Offset", 0.0, doc="Offset to apply (in degrees) to the s1 of the input workspace")
        self.declareProperty(
            "NormaliseBy", "Monitor", StringListValidator(["None", "Time", "Monitor"]), "Normalise to monitor, time or None."
        )
        self.declareProperty("Frame", "Q_sample", StringListValidator(["Q_sample", "HKL"]), "Selects Q-dimensions of the output workspace")
        self.declareProperty(
            FloatArrayProperty("Uproj", [1, 0, 0], FloatArrayLengthValidator(3), direction=Direction.Input),
            "Defines the first projection vector of the target Q coordinate system in HKL mode",
        )
        self.declareProperty(
            FloatArrayProperty("Vproj", [0, 1, 0], FloatArrayLengthValidator(3), direction=Direction.Input),
            "Defines the second projection vector of the target Q coordinate system in HKL mode",
        )
        self.declareProperty(
            FloatArrayProperty("Wproj", [0, 0, 1], FloatArrayLengthValidator(3), direction=Direction.Input),
            "Defines the third projection vector of the target Q coordinate system in HKL mode",
        )
        self.declareProperty(
            FloatArrayProperty("BinningDim0", [-8.02, 8.02, 401], FloatArrayLengthValidator(3), direction=Direction.Input),
            "Binning parameters for the 0th dimension. Enter it as a"
            "comma-separated list of values with the"
            "format: 'minimum,maximum,number_of_bins'.",
        )
        self.declareProperty(
            FloatArrayProperty("BinningDim1", [-0.82, 0.82, 41], FloatArrayLengthValidator(3), direction=Direction.Input),
            "Binning parameters for the 1st dimension. Enter it as a"
            "comma-separated list of values with the"
            "format: 'minimum,maximum,number_of_bins'.",
        )
        self.declareProperty(
            FloatArrayProperty("BinningDim2", [-8.02, 8.02, 401], FloatArrayLengthValidator(3), direction=Direction.Input),
            "Binning parameters for the 2nd dimension. Enter it as a"
            "comma-separated list of values with the"
            "format: 'minimum,maximum,number_of_bins'.",
        )
        self.declareProperty(
            "KeepTemporaryWorkspaces",
            False,
            "If True the normalization and data workspaces in addition to the normalized data will be outputted",
        )
        self.declareProperty(
            "ObliquityParallaxCoefficient",
            1.0,
            validator=FloatBoundedValidator(0.0),
            doc="Geometrical correction for shift in vertical beam position due to wide beam.",
        )
        self.declareProperty(
            "SymmetryOperations",
            "",
            direction=Direction.Input,
            doc="Space Group name, Point Group name, or list individual Symmetries used to perform the symmetrization",
        )
        self.declareProperty(
            IMDHistoWorkspaceProperty("TemporaryDataWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            "An (optional) input MDHistoWorkspace used to accumulate data from multiple MDEventWorkspaces."
            "If unspecified a blank MDHistoWorkspace will be created",
        )
        self.declareProperty(
            IMDHistoWorkspaceProperty("TemporaryNormalizationWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            "An (optional) input MDHistoWorkspace used to accumulate normalization data from multiple MDEventWorkspaces."
            "If unspecified a blank MDHistoWorkspace will be created",
        )
        self.declareProperty(
            IMDHistoWorkspaceProperty("TemporaryBackgroundDataWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            "An (optional) input MDHistoWorkspace used to accumulate background data from multiple MDEventWorkspaces."
            "If unspecified but BackgroundWorkspace is specified, a blank MDHistoWorkspace will be created",
        )
        self.declareProperty(
            IMDHistoWorkspaceProperty(
                "TemporaryBackgroundNormalizationWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input
            ),
            "An (optional) input MDHistoWorkspace used to accumulate background normalization data from multiple MDEventWorkspaces."
            "If unspecified but BackgroundWorkspace is specified, a blank MDHistoWorkspace will be created",
        )
        self.declareProperty(
            "OutputDataWorkspace",
            "",
            direction=Direction.Input,
            doc="Name for the Output Data Workspace",
        )
        self.declareProperty(
            "OutputNormalizationWorkspace",
            "",
            direction=Direction.Input,
            doc="Name for the Output Normalization Workspace",
        )
        self.declareProperty(
            "OutputBackgroundDataWorkspace",
            "",
            direction=Direction.Input,
            doc="Name for the Output Background Workspace",
        )
        self.declareProperty(
            "OutputBackgroundNormalizationWorkspace",
            "",
            direction=Direction.Input,
            doc="Name for the Output Background Normalization Workspace",
        )
        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output), "Output Workspace"
        )

    def validateInputs(self):  # noqa C901
        issues = dict()

        inWS = self.getProperty("InputWorkspace").value
        instrument = inWS.getExperimentInfo(0).getInstrument().getName()

        if inWS.getNumDims() != 3:
            issues["InputWorkspace"] = "InputWorkspace has wrong number of dimensions, need 3"
            return issues

        d0 = inWS.getDimension(0)
        d1 = inWS.getDimension(1)
        d2 = inWS.getDimension(2)
        number_of_runs = d2.getNBins()

        if d0.name != "y" or d1.name != "x" or d2.name != "scanIndex":
            issues["InputWorkspace"] = "InputWorkspace has wrong dimensions"
            return issues

        if inWS.getNumExperimentInfo() == 0:
            issues["InputWorkspace"] = "InputWorkspace is missing ExperimentInfo"
            return issues

        # Check that all logs are there and are of correct length
        run = inWS.getExperimentInfo(0).run()

        # Check number of goniometers
        if run.getNumGoniometers() != number_of_runs:
            issues["InputWorkspace"] = "goniometers not set correctly, did you run SetGoniometer with Average=False"

        if instrument == "HB3A":
            for prop in ["monitor", "time"]:
                if run.hasProperty(prop):
                    p = run.getProperty(prop).value
                    if np.size(p) != number_of_runs:
                        issues["InputWorkspace"] = "log {} is of incorrect length".format(prop)
                else:
                    issues["InputWorkspace"] = "missing log {}".format(prop)
        else:
            for prop in ["duration", "monitor_count"]:
                if run.hasProperty(prop):
                    p = run.getProperty(prop).value
                    if np.size(p) != number_of_runs:
                        issues["InputWorkspace"] = "log {} is of incorrect length".format(prop)
                else:
                    issues["InputWorkspace"] = "missing log {}".format(prop)

        for prop in ["azimuthal", "twotheta"]:
            if run.hasProperty(prop):
                p = run.getProperty(prop).value
                if np.size(p) != d0.getNBins() * d1.getNBins():
                    issues["InputWorkspace"] = "log {} is of incorrect length".format(prop)

        normWS = self.getProperty("NormalisationWorkspace").value

        if normWS:
            nd0 = normWS.getDimension(0)
            nd1 = normWS.getDimension(1)
            nd2 = normWS.getDimension(2)
            if (
                nd0.name != d0.name
                or nd0.getNBins() != d0.getNBins()
                or nd1.name != d1.name
                or nd1.getNBins() != d1.getNBins()
                or nd2.name != d2.name
            ):
                issues["NormalisationWorkspace"] = "NormalisationWorkspace dimensions are not compatible with InputWorkspace"

        ubWS = self.getProperty("UBWorkspace").value
        if ubWS:
            try:
                sample = ubWS.sample()
            except AttributeError:
                sample = ubWS.getExperimentInfo(0).sample()
            if not sample.hasOrientedLattice():
                issues["UBWorkspace"] = "UBWorkspace does not has an OrientedLattice"
        else:
            if self.getProperty("Frame").value == "HKL":
                if not inWS.getExperimentInfo(0).sample().hasOrientedLattice():
                    issues["Frame"] = (
                        "HKL selected but neither an UBWorkspace workspace was provided or " "the InputWorkspace has an OrientedLattice"
                    )
        symmetry = self.getProperty("SymmetryOperations").value
        if symmetry:
            try:
                if SpaceGroupFactory.isSubscribedSymbol(symmetry):
                    SpaceGroupFactory.createSpaceGroup(symmetry).getSymmetryOperations()
                elif PointGroupFactory.isSubscribed(symmetry):
                    PointGroupFactory.createPointGroup(symmetry).getSymmetryOperations()
                else:
                    SymmetryOperationFactory.createSymOps(symmetry)
            except RuntimeError:
                issues["SymmetryOperations"] = "SymmetryOperations is not a valid Space Group, Point Group or Symmetry Operation"

        return issues

    def PyExec(self):  # noqa C901
        inWS = self.getProperty("InputWorkspace").value
        normWS = self.getProperty("NormalisationWorkspace").value
        bkgWS = self.getProperty("BackgroundWorkspace").value
        tempData = self.getProperty("TemporaryDataWorkspace").value
        tempNorm = self.getProperty("TemporaryNormalizationWorkspace").value
        tempBkgData = self.getProperty("TemporaryBackgroundDataWorkspace").value
        tempBkgNorm = self.getProperty("TemporaryBackgroundNormalizationWorkspace").value

        keep_temp = self.getProperty("KeepTemporaryWorkspaces").value
        if bool(tempData) or bool(tempNorm) or bool(tempBkgData) or bool(tempBkgNorm):
            keep_temp = True

        _norm = bool(normWS)
        _bkg = bool(bkgWS)

        symmetry = self.getProperty("SymmetryOperations").value
        if symmetry:
            if SpaceGroupFactory.isSubscribedSymbol(symmetry):
                sym_ops = SpaceGroupFactory.createSpaceGroup(symmetry).getSymmetryOperations()
            elif PointGroupFactory.isSubscribed(symmetry):
                sym_ops = PointGroupFactory.createPointGroup(symmetry).getSymmetryOperations()
            else:
                sym_ops = SymmetryOperationFactory.createSymOps(symmetry)
        else:
            sym_ops = SymmetryOperationFactory.createSymOps("x,y,z")

        instrument = inWS.getExperimentInfo(0).getInstrument().getName()

        dim0_min, dim0_max, dim0_bins = self.getProperty("BinningDim0").value
        dim1_min, dim1_max, dim1_bins = self.getProperty("BinningDim1").value
        dim2_min, dim2_max, dim2_bins = self.getProperty("BinningDim2").value
        dim0_bins = int(dim0_bins)
        dim1_bins = int(dim1_bins)
        dim2_bins = int(dim2_bins)

        data_array = inWS.getSignalArray()  # getSignalArray returns a F_CONTIGUOUS view of the signal array

        number_of_runs = data_array.shape[2]

        progress_end = len(sym_ops) * number_of_runs + 12
        progress = Progress(self, 0.0, 1.0, progress_end)

        normaliseBy = self.getProperty("NormaliseBy").value
        if normaliseBy == "Monitor":
            if instrument == "HB3A":
                scale = np.asarray(inWS.getExperimentInfo(0).run().getProperty("monitor").value)
            else:
                scale = np.asarray(inWS.getExperimentInfo(0).run().getProperty("monitor_count").value)
        elif normaliseBy == "Time":
            if instrument == "HB3A":
                scale = np.asarray(inWS.getExperimentInfo(0).run().getProperty("time").value)
            else:
                scale = np.asarray(inWS.getExperimentInfo(0).run().getProperty("duration").value)
        else:
            scale = np.ones(number_of_runs)

        if _norm:
            norm_scale = 1.0
            if normaliseBy == "Monitor":
                if instrument == "HB3A":
                    norm_scale = np.sum(normWS.getExperimentInfo(0).run().getProperty("monitor").value)
                else:
                    norm_scale = np.sum(normWS.getExperimentInfo(0).run().getProperty("monitor_count").value)
            elif normaliseBy == "Time":
                if instrument == "HB3A":
                    norm_scale = np.sum(normWS.getExperimentInfo(0).run().getProperty("time").value)
                else:
                    norm_scale = np.sum(normWS.getExperimentInfo(0).run().getProperty("duration").value)
            norm_array = normWS.getSignalArray().sum(axis=2)
            norm_array /= norm_scale
        else:
            norm_array = np.ones_like(data_array[:, :, 0])

        if _bkg:
            if normaliseBy == "Monitor":
                if instrument == "HB3A":
                    bkg_scale = np.asarray(bkgWS.getExperimentInfo(0).run().getProperty("monitor").value)
                else:
                    bkg_scale = np.asarray(bkgWS.getExperimentInfo(0).run().getProperty("monitor_count").value)
            elif normaliseBy == "Time":
                if instrument == "HB3A":
                    bkg_scale = np.asarray(bkgWS.getExperimentInfo(0).run().getProperty("time").value)
                else:
                    bkg_scale = np.asarray(bkgWS.getExperimentInfo(0).run().getProperty("duration").value)
            else:
                bkg_scale = np.ones(number_of_runs)
            bkg_data_array = bkgWS.getSignalArray()

        W = np.eye(3)
        W[:, 0] = self.getProperty("Uproj").value
        W[:, 1] = self.getProperty("Vproj").value
        W[:, 2] = self.getProperty("Wproj").value

        UBW = np.eye(3)
        _hkl = False
        if self.getProperty("Frame").value == "HKL":
            _hkl = True

            ubWS = self.getProperty("UBWorkspace").value
            if ubWS:
                try:
                    ol = ubWS.sample().getOrientedLattice()
                except AttributeError:
                    ol = ubWS.getExperimentInfo(0).sample().getOrientedLattice()
                logger.notice("Using UB matrix from {} with {}".format(ubWS.name(), ol))
            else:
                ol = inWS.getExperimentInfo(0).sample().getOrientedLattice()
                logger.notice("Using UB matrix from {} with {}".format(inWS.name(), ol))
            UB = ol.getUB()
            UBW = np.dot(UB, W)
            char_dict = {0: "0", 1: "{1}", -1: "-{1}"}
            chars = ["H", "K", "L"]
            names = [
                "[" + ",".join(char_dict.get(j, "{0}{1}").format(j, chars[np.argmax(np.abs(W[:, i]))]) for j in W[:, i]) + "]"
                for i in range(3)
            ]
            # Slicing because we want the column vector and not the row vector
            units = "in {:.3f} A^-1,in {:.3f} A^-1,in {:.3f} A^-1".format(
                ol.qFromHKL(W[:, 0]).norm(), ol.qFromHKL(W[:, 1]).norm(), ol.qFromHKL(W[:, 2]).norm()
            )
            frames = "HKL,HKL,HKL"
        else:
            names = "Q_sample_x,Q_sample_y,Q_sample_z"
            units = "Angstrom^-1,Angstrom^-1,Angstrom^-1"
            frames = "QSample,QSample,QSample"
        k = 2 * np.pi / self.getProperty("Wavelength").value

        progress.report("Calculating Qlab for each pixel")
        if inWS.getExperimentInfo(0).run().hasProperty("twotheta"):
            polar = np.array(inWS.getExperimentInfo(0).run().getProperty("twotheta").value)
        else:
            di = inWS.getExperimentInfo(0).detectorInfo()
            polar = np.array([di.twoTheta(i) for i in range(di.size()) if not di.isMonitor(i)])
            if inWS.getExperimentInfo(0).getInstrument().getName() == "HB3A":
                polar = polar.reshape(512 * 3, 512).T.flatten()

        if inWS.getExperimentInfo(0).run().hasProperty("azimuthal"):
            azim = np.array(inWS.getExperimentInfo(0).run().getProperty("azimuthal").value)
        else:
            di = inWS.getExperimentInfo(0).detectorInfo()
            azim = np.array([di.azimuthal(i) for i in range(di.size()) if not di.isMonitor(i)])
            if inWS.getExperimentInfo(0).getInstrument().getName() == "HB3A":
                azim = azim.reshape(512 * 3, 512).T.flatten()

        # check convention to determine the sign
        if config["Q.convention"] == "Crystallography":
            k *= -1.0

        cop = self.getProperty("ObliquityParallaxCoefficient").value

        qlab = np.vstack((np.sin(polar) * np.cos(azim), np.sin(polar) * np.sin(azim) * cop, np.cos(polar) - 1)) * -k  # Kf - Ki(0,0,1)

        data_hist = np.zeros(dim0_bins * dim1_bins * dim2_bins)
        norm_hist = np.zeros_like(data_hist)
        if _bkg:
            bkg_data_hist = np.zeros_like(data_hist)
            bkg_norm_hist = np.zeros_like(data_hist)

        bins = [
            np.linspace(dim0_min, dim0_max, dim0_bins + 1),
            np.linspace(dim1_min, dim1_max, dim1_bins + 1),
            np.linspace(dim2_min, dim2_max, dim2_bins + 1),
        ]

        dim_bins = [dim0_bins, dim1_bins, dim2_bins]

        valid_range = [(0, len(bins[i]) - 2) for i in range(3)]
        valid_range_min = np.array([valid_range[i][0] for i in range(3)])
        valid_range_max = np.array([valid_range[i][1] for i in range(3)])

        progress.report("Calculating Q volume")

        assert not data_array[:, :, 0].flags.owndata
        assert not data_array[:, :, 0].ravel("F").flags.owndata
        assert data_array[:, :, 0].flags.fnc

        s1offset = np.deg2rad(self.getProperty("S1Offset").value)
        s1offset = np.array([[np.cos(s1offset), 0, np.sin(s1offset)], [0, 1, 0], [-np.sin(s1offset), 0, np.cos(s1offset)]])

        R_invs = [np.dot(s1offset, inWS.getExperimentInfo(0).run().getGoniometer(n).getR()).T for n in range(number_of_runs)]

        # flat array
        data_array_flat = data_array.T.reshape(number_of_runs, -1)
        norm_array_flat = norm_array.ravel(order="F")
        if _bkg:
            bkg_data_array_flat = bkg_data_array.T.reshape(number_of_runs, -1)

        # loop over symmetry operations for memory efficiency
        for sym_op in sym_ops:

            S = np.zeros((3, 3))
            S[:, 0] = sym_op.transformHKL([1, 0, 0])
            S[:, 1] = sym_op.transformHKL([0, 1, 0])
            S[:, 2] = sym_op.transformHKL([0, 0, 1])

            # transform matrix
            if _hkl:
                T = np.linalg.multi_dot([2 * np.pi * UB, S, W])
            else:
                T = np.dot(S, W)
            T_inv = np.linalg.inv(T)

            for i_gon, R_inv in enumerate(R_invs):
                # transformation matrix
                combined_matrix = np.matmul(T_inv, R_inv)  # (3 x 3) x (3 x 3) = (3 x 3)

                # matrix-vector multiplication for all q
                qvals = np.matmul(combined_matrix, qlab)  # (3 x 3) x (3 x n_det) = (3 x n_det)

                # map q values to bin edges
                bin_indices = np.stack([np.digitize(qvals[i], bins[i]) - 1 for i in range(3)], axis=-1)

                # mask to exclude out-of-bound data
                mask = np.all((bin_indices >= valid_range_min) & (bin_indices <= valid_range_max), axis=1)

                bin_indices = np.ravel_multi_index(bin_indices[mask].T, dim_bins)
                unique_indices, inverse_indices = np.unique(bin_indices, return_inverse=True)

                # sum weights for each unique bin index
                data_hist[unique_indices] += np.bincount(inverse_indices, data_array_flat[i_gon][mask])
                norm_hist[unique_indices] += np.bincount(inverse_indices, norm_array_flat[mask]) * scale[i_gon]

                if _bkg:
                    # sum weights for each unique bin index
                    bkg_data_hist[unique_indices] += np.bincount(inverse_indices, bkg_data_array_flat[i_gon][mask])
                    bkg_norm_hist[unique_indices] += np.bincount(inverse_indices, norm_array_flat[mask]) * bkg_scale[i_gon]

                progress.report()

        data_hist = data_hist.reshape(dim_bins)
        norm_hist = norm_hist.reshape(dim_bins)
        if _bkg:
            bkg_data_hist = bkg_data_hist.reshape(dim_bins)
            bkg_norm_hist = bkg_norm_hist.reshape(dim_bins)

        if keep_temp:
            # Create data workspace
            progress.report("Creating data MDHistoWorkspace")
            createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
            createWS_alg.setProperty("SignalInput", data_hist.ravel("F"))
            createWS_alg.setProperty("ErrorInput", np.sqrt(data_hist).ravel("F"))
            createWS_alg.setProperty("Dimensionality", 3)
            createWS_alg.setProperty("Extents", "{},{},{},{},{},{}".format(dim0_min, dim0_max, dim1_min, dim1_max, dim2_min, dim2_max))
            createWS_alg.setProperty("NumberOfBins", "{},{},{}".format(dim0_bins, dim1_bins, dim2_bins))
            createWS_alg.setProperty("Names", names)
            createWS_alg.setProperty("Units", units)
            createWS_alg.setProperty("Frames", frames)
            createWS_alg.execute()
            outWS_data = createWS_alg.getProperty("OutputWorkspace").value
            if self.getProperty("OutputDataWorkspace").value:
                mtd.addOrReplace(self.getProperty("OutputDataWorkspace").value, outWS_data)
            else:
                mtd.addOrReplace(self.getPropertyValue("OutputWorkspace") + "_data", outWS_data)

            # Create normalisation workspace
            progress.report("Creating norm MDHistoWorkspace")
            createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
            createWS_alg.setProperty("SignalInput", norm_hist.ravel("F"))
            createWS_alg.setProperty("ErrorInput", np.sqrt(norm_hist).ravel("F"))
            createWS_alg.setProperty("Dimensionality", 3)
            createWS_alg.setProperty("Extents", "{},{},{},{},{},{}".format(dim0_min, dim0_max, dim1_min, dim1_max, dim2_min, dim2_max))
            createWS_alg.setProperty("NumberOfBins", "{},{},{}".format(dim0_bins, dim1_bins, dim2_bins))
            createWS_alg.setProperty("Names", names)
            createWS_alg.setProperty("Units", units)
            createWS_alg.setProperty("Frames", frames)
            createWS_alg.execute()
            outWS_norm = createWS_alg.getProperty("OutputWorkspace").value
            if self.getProperty("OutputNormalizationWorkspace").value:
                mtd.addOrReplace(self.getProperty("OutputNormalizationWorkspace").value, outWS_norm)
            else:
                mtd.addOrReplace(self.getPropertyValue("OutputWorkspace") + "_normalization", outWS_norm)

            if _bkg:
                # Create background data workspace
                progress.report("Creating background data MDHistoWorkspace")
                createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
                createWS_alg.setProperty("SignalInput", bkg_data_hist.ravel("F"))
                createWS_alg.setProperty("ErrorInput", np.sqrt(bkg_data_hist).ravel("F"))
                createWS_alg.setProperty("Dimensionality", 3)
                createWS_alg.setProperty("Extents", "{},{},{},{},{},{}".format(dim0_min, dim0_max, dim1_min, dim1_max, dim2_min, dim2_max))
                createWS_alg.setProperty("NumberOfBins", "{},{},{}".format(dim0_bins, dim1_bins, dim2_bins))
                createWS_alg.setProperty("Names", names)
                createWS_alg.setProperty("Units", units)
                createWS_alg.setProperty("Frames", frames)
                createWS_alg.execute()
                outWS_bkg = createWS_alg.getProperty("OutputWorkspace").value
                if self.getProperty("OutputBackgroundDataWorkspace").value:
                    mtd.addOrReplace(self.getProperty("OutputBackgroundDataWorkspace").value, outWS_bkg)
                else:
                    mtd.addOrReplace(self.getPropertyValue("OutputWorkspace") + "_background_data", outWS_bkg)

                # Create background normalisation workspace
                progress.report("Creating background normalization MDHistoWorkspace")
                createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
                createWS_alg.setProperty("SignalInput", bkg_norm_hist.ravel("F"))
                createWS_alg.setProperty("ErrorInput", np.sqrt(bkg_norm_hist).ravel("F"))
                createWS_alg.setProperty("Dimensionality", 3)
                createWS_alg.setProperty("Extents", "{},{},{},{},{},{}".format(dim0_min, dim0_max, dim1_min, dim1_max, dim2_min, dim2_max))
                createWS_alg.setProperty("NumberOfBins", "{},{},{}".format(dim0_bins, dim1_bins, dim2_bins))
                createWS_alg.setProperty("Names", names)
                createWS_alg.setProperty("Units", units)
                createWS_alg.setProperty("Frames", frames)
                createWS_alg.execute()
                outWS_bkgNorm = createWS_alg.getProperty("OutputWorkspace").value
                if self.getProperty("OutputBackgroundNormalizationWorkspace").value:
                    mtd.addOrReplace(self.getProperty("OutputBackgroundNormalizationWorkspace").value, outWS_bkgNorm)
                else:
                    mtd.addOrReplace(self.getPropertyValue("OutputWorkspace") + "_background_normalization", outWS_bkgNorm)

        old_settings = np.seterr(divide="ignore", invalid="ignore")  # Ignore RuntimeWarning: invalid value encountered in true_divide
        result = data_hist / norm_hist  # We often divide by zero here and we get NaN's, this is desired behaviour
        result_var = data_hist / norm_hist**2
        if _bkg:
            result -= bkg_data_hist / bkg_norm_hist
            result_var += bkg_data_hist / bkg_norm_hist**2
        np.seterr(**old_settings)

        progress.report("Creating MDHistoWorkspace")
        createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
        createWS_alg.setProperty("SignalInput", result.ravel("F"))
        createWS_alg.setProperty("ErrorInput", np.sqrt(result_var).ravel("F"))
        createWS_alg.setProperty("Dimensionality", 3)
        createWS_alg.setProperty("Extents", "{},{},{},{},{},{}".format(dim0_min, dim0_max, dim1_min, dim1_max, dim2_min, dim2_max))
        createWS_alg.setProperty("NumberOfBins", "{},{},{}".format(dim0_bins, dim1_bins, dim2_bins))
        createWS_alg.setProperty("Names", names)
        createWS_alg.setProperty("Units", units)
        createWS_alg.setProperty("Frames", frames)
        createWS_alg.execute()
        outWS = createWS_alg.getProperty("OutputWorkspace").value

        # Copy experiment infos
        if inWS.getNumExperimentInfo() > 0:
            outWS.copyExperimentInfos(inWS)

        outWS.getExperimentInfo(0).run().addProperty("RUBW_MATRIX", list(UBW.flatten()), True)
        outWS.getExperimentInfo(0).run().addProperty("W_MATRIX", list(W.flatten()), True)
        outWS.getExperimentInfo(0).run().addProperty("wavelength", self.getProperty("Wavelength").value, True)
        try:
            if outWS.getExperimentInfo(0).sample().hasOrientedLattice():
                outWS.getExperimentInfo(0).sample().getOrientedLattice().setUB(UB)
        except NameError:
            pass

        if keep_temp:
            outWS_data.copyExperimentInfos(outWS)

            if bool(tempData):
                progress.report("Accumulating Data Workspace")
                PlusMD(LHSWorkspace=outWS_data, RHSWorkspace=tempData, OutputWorkspace=tempData)
            if bool(tempNorm):
                progress.report("Accumulating Normalization Workspace")
                PlusMD(LHSWorkspace=outWS_norm, RHSWorkspace=tempNorm, OutputWorkspace=tempNorm)
            if bool(tempBkgData):
                progress.report("Accumulating Background Data Workspace")
                PlusMD(LHSWorkspace=outWS_bkg, RHSWorkspace=tempBkgData, OutputWorkspace=tempBkgData)
            if bool(tempBkgNorm):
                progress.report("Accumulating Background Normalization Workspace")
                PlusMD(LHSWorkspace=outWS_bkgNorm, RHSWorkspace=tempBkgNorm, OutputWorkspace=tempBkgNorm)

        progress.report(progress_end, "Done")
        self.setProperty("OutputWorkspace", outWS)


AlgorithmFactory.subscribe(ConvertWANDSCDtoQ)

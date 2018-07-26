from __future__ import (absolute_import, division, print_function)
from mantid.api import (PythonAlgorithm, AlgorithmFactory,
                        PropertyMode, WorkspaceProperty, Progress,
                        IMDHistoWorkspaceProperty, mtd)
from mantid.kernel import Direction, FloatArrayProperty, FloatArrayLengthValidator, StringListValidator, FloatBoundedValidator
from mantid import logger
import numpy as np


class ConvertWANDSCDtoQ(PythonAlgorithm):

    def category(self):
        return 'DataHandling\\Nexus'

    def seeAlso(self):
        return [ "LoadWANDSCD" ]

    def name(self):
        return 'ConvertWANDSCDtoQ'

    def summary(self):
        return 'Convert the output of LoadWANDSCD to Q or HKL'

    def PyInit(self):
        self.declareProperty(IMDHistoWorkspaceProperty("InputWorkspace", "",
                                                       optional=PropertyMode.Mandatory,
                                                       direction=Direction.Input),
                             "Input Workspace")
        self.declareProperty(IMDHistoWorkspaceProperty("NormalisationWorkspace", "",
                                                       optional=PropertyMode.Optional,
                                                       direction=Direction.Input),
                             "Workspace to use for normalisation")
        self.declareProperty(WorkspaceProperty("UBWorkspace", "",
                                               optional=PropertyMode.Optional,
                                               direction=Direction.Input),
                             "Workspace containing the UB matrix to use")
        self.declareProperty("Wavelength", 1.488, validator=FloatBoundedValidator(0.0), doc="Wavelength to set the workspace")
        self.declareProperty('NormaliseBy', 'Monitor', StringListValidator(['None', 'Time', 'Monitor']),
                             "Normalise to monitor, time or None.")
        self.declareProperty('Frame', 'Q_sample', StringListValidator(['Q_sample', 'HKL']),
                             "Selects Q-dimensions of the output workspace")
        self.declareProperty(FloatArrayProperty("Uproj", [1,0,0], FloatArrayLengthValidator(3), direction=Direction.Input),
                             "Defines the first projection vector of the target Q coordinate system in HKL mode")
        self.declareProperty(FloatArrayProperty("Vproj", [0,1,0], FloatArrayLengthValidator(3), direction=Direction.Input),
                             "Defines the second projection vector of the target Q coordinate system in HKL mode")
        self.declareProperty(FloatArrayProperty("Wproj", [0,0,1], FloatArrayLengthValidator(3), direction=Direction.Input),
                             "Defines the third projection vector of the target Q coordinate system in HKL mode")
        self.declareProperty(FloatArrayProperty("BinningDim0", [-8.02,8.02,401], FloatArrayLengthValidator(3), direction=Direction.Input),
                             "Binning parameters for the 0th dimension. Enter it as a"
                             "comma-separated list of values with the"
                             "format: 'minimum,maximum,number_of_bins'.")
        self.declareProperty(FloatArrayProperty("BinningDim1", [-0.82,0.82,41], FloatArrayLengthValidator(3), direction=Direction.Input),
                             "Binning parameters for the 1st dimension. Enter it as a"
                             "comma-separated list of values with the"
                             "format: 'minimum,maximum,number_of_bins'.")
        self.declareProperty(FloatArrayProperty("BinningDim2", [-8.02,8.02,401], FloatArrayLengthValidator(3), direction=Direction.Input),
                             "Binning parameters for the 2nd dimension. Enter it as a"
                             "comma-separated list of values with the"
                             "format: 'minimum,maximum,number_of_bins'.")
        self.declareProperty('KeepTemporaryWorkspaces', False,
                             "If True the normalization and data workspaces in addition to the normalized data will be outputted")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "",
                                               optional=PropertyMode.Mandatory,
                                               direction=Direction.Output),
                             "Output Workspace")

    def validateInputs(self):
        issues = dict()

        inWS = self.getProperty("InputWorkspace").value

        if inWS.getNumDims() != 3:
            issues["InputWorkspace"] = "InputWorkspace has wrong number of dimensions, need 3"
            return issues

        d0 = inWS.getDimension(0)
        d1 = inWS.getDimension(1)
        d2 = inWS.getDimension(2)
        number_of_runs = d2.getNBins()

        if (d0.name is not 'y' or d1.name is not 'x' or d2.name != 'scanIndex'):
            issues["InputWorkspace"] = "InputWorkspace has wrong dimensions"
            return issues

        if inWS.getNumExperimentInfo() == 0:
            issues["InputWorkspace"] = "InputWorkspace is missing ExperimentInfo"
            return issues

        # Check that all logs are there and are of correct length
        run = inWS.getExperimentInfo(0).run()
        for prop in ['duration', 'monitor_count', 's1']:
            if run.hasProperty(prop):
                p = run.getProperty(prop).value
                if np.size(p) != number_of_runs:
                    issues["InputWorkspace"] = "log {} is of incorrect length".format(prop)
            else:
                issues["InputWorkspace"] = "missing log {}".format(prop)

        for prop in ['azimuthal', 'twotheta']:
            if run.hasProperty(prop):
                p = run.getProperty(prop).value
                if np.size(p) != d0.getNBins()*d1.getNBins():
                    issues["InputWorkspace"] = "log {} is of incorrect length".format(prop)
            else:
                issues["InputWorkspace"] = "missing log {}".format(prop)

        normWS = self.getProperty("NormalisationWorkspace").value

        if normWS:
            nd0 = normWS.getDimension(0)
            nd1 = normWS.getDimension(1)
            nd2 = normWS.getDimension(2)
            if (nd0.name != d0.name or nd0.getNBins() != d0.getNBins()
               or nd1.name != d1.name or nd1.getNBins() != d1.getNBins()
               or nd2.name != d2.name):
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
            if self.getProperty("Frame").value == 'HKL':
                if not inWS.getExperimentInfo(0).sample().hasOrientedLattice():
                    issues["Frame"] = "HKL selected but neither an UBWorkspace workspace was provided or " \
                                      "the InputWorkspace has an OrientedLattice"

        return issues

    def PyExec(self):
        inWS = self.getProperty("InputWorkspace").value
        normWS = self.getProperty("NormalisationWorkspace").value
        _norm = bool(normWS)

        dim0_min, dim0_max, dim0_bins = self.getProperty('BinningDim0').value
        dim1_min, dim1_max, dim1_bins = self.getProperty('BinningDim1').value
        dim2_min, dim2_max, dim2_bins = self.getProperty('BinningDim2').value
        dim0_bins = int(dim0_bins)
        dim1_bins = int(dim1_bins)
        dim2_bins = int(dim2_bins)
        dim0_bin_size = (dim0_max-dim0_min)/dim0_bins
        dim1_bin_size = (dim1_max-dim1_min)/dim1_bins
        dim2_bin_size = (dim2_max-dim2_min)/dim2_bins

        data_array = inWS.getSignalArray() # getSignalArray returns a F_CONTIGUOUS view of the signal array

        number_of_runs = data_array.shape[2]

        progress = Progress(self, 0.0, 1.0, number_of_runs+4)

        # Get rotation array
        s1 = np.deg2rad(inWS.getExperimentInfo(0).run().getProperty('s1').value)

        normaliseBy = self.getProperty("NormaliseBy").value
        if normaliseBy == "Monitor":
            scale = np.asarray(inWS.getExperimentInfo(0).run().getProperty('monitor_count').value)
        elif normaliseBy == "Time":
            scale = np.asarray(inWS.getExperimentInfo(0).run().getProperty('duration').value)
        else:
            scale = np.ones(number_of_runs)

        if _norm:
            if normaliseBy == "Monitor":
                norm_scale = np.sum(normWS.getExperimentInfo(0).run().getProperty('monitor_count').value)
            elif normaliseBy == "Time":
                norm_scale = np.sum(normWS.getExperimentInfo(0).run().getProperty('duration').value)
            else:
                norm_scale = 1.
            norm_array = normWS.getSignalArray().sum(axis=2)

        W = np.eye(3)
        UBW = np.eye(3)
        if self.getProperty("Frame").value == 'HKL':
            W[:,0] = self.getProperty('Uproj').value
            W[:,1] = self.getProperty('Vproj').value
            W[:,2] = self.getProperty('Wproj').value
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
            char_dict = {0:'0', 1:'{1}', -1:'-{1}'}
            chars=['H','K','L']
            names = ['['+','.join(char_dict.get(j, '{0}{1}')
                                  .format(j,chars[np.argmax(np.abs(W[:,i]))]) for j in W[:,i])+']' for i in range(3)]
            units = 'in {:.3f} A^-1,in {:.3f} A^-1,in {:.3f} A^-1'.format(ol.qFromHKL(W[0]).norm(),
                                                                          ol.qFromHKL(W[1]).norm(),
                                                                          ol.qFromHKL(W[2]).norm())
            frames = 'HKL,HKL,HKL'
            k = 1/self.getProperty("Wavelength").value # Not 2pi/wavelength to save dividing by 2pi later
        else:
            names = 'Q_sample_x,Q_sample_y,Q_sample_z'
            units = 'Angstrom^-1,Angstrom^-1,Angstrom^-1'
            frames = 'QSample,QSample,QSample'
            k = 2*np.pi/self.getProperty("Wavelength").value

        progress.report('Calculating Qlab for each pixel')
        polar = np.array(inWS.getExperimentInfo(0).run().getProperty('twotheta').value)
        azim = np.array(inWS.getExperimentInfo(0).run().getProperty('azimuthal').value)
        qlab = np.vstack((np.sin(polar)*np.cos(azim),
                          np.sin(polar)*np.sin(azim),
                          np.cos(polar) - 1)).T * -k # Kf - Ki(0,0,1)

        progress.report('Calculating Q volume')

        output = np.zeros((dim0_bins+2, dim1_bins+2, dim2_bins+2))
        outputr = output.ravel()

        output_scale = np.zeros_like(output)
        output_scaler = output_scale.ravel()

        if _norm:
            output_norm = np.zeros_like(output)
            output_normr = output_norm.ravel()
            output_norm_scale = np.zeros_like(output)
            output_norm_scaler = output_norm_scale.ravel()

        bin_size = np.array([[dim0_bin_size],
                             [dim1_bin_size],
                             [dim2_bin_size]])

        offset = np.array([[dim0_min/dim0_bin_size],
                           [dim1_min/dim1_bin_size],
                           [dim2_min/dim2_bin_size]])-0.5

        assert not data_array[:,:,0].flags.owndata
        assert not data_array[:,:,0].ravel('F').flags.owndata
        assert data_array[:,:,0].flags.fnc

        for n in range(number_of_runs):
            R = np.array([[ np.cos(s1[n]), 0, np.sin(s1[n])],
                          [             0, 1,             0],
                          [-np.sin(s1[n]), 0, np.cos(s1[n])]])
            RUBW = np.dot(R,UBW)
            q = np.round(np.dot(np.linalg.inv(RUBW),qlab.T)/bin_size-offset).astype(np.int)
            q_index = np.ravel_multi_index(q, (dim0_bins+2, dim1_bins+2, dim2_bins+2), mode='clip')
            q_uniq, inverse = np.unique(q_index, return_inverse=True)
            outputr[q_uniq] += np.bincount(inverse, data_array[:,:,n].ravel('F'))
            output_scaler[q_uniq] += np.bincount(inverse)*scale[n]
            if _norm:
                output_normr[q_uniq] += np.bincount(inverse, norm_array.ravel('F'))
                output_norm_scaler[q_uniq] += np.bincount(inverse)

            progress.report()

        if _norm:
            output *= output_norm_scale*norm_scale
            output_norm *= output_scale
        else:
            output_norm = output_scale

        if self.getProperty('KeepTemporaryWorkspaces').value:
            # Create data workspace
            progress.report('Creating data MDHistoWorkspace')
            createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
            createWS_alg.setProperty("SignalInput", output[1:-1,1:-1,1:-1].ravel('F'))
            createWS_alg.setProperty("ErrorInput", np.sqrt(output[1:-1,1:-1,1:-1].ravel('F')))
            createWS_alg.setProperty("Dimensionality", 3)
            createWS_alg.setProperty("Extents", '{},{},{},{},{},{}'.format(dim0_min,dim0_max,dim1_min,dim1_max,dim2_min,dim2_max))
            createWS_alg.setProperty("NumberOfBins", '{},{},{}'.format(dim0_bins,dim1_bins,dim2_bins))
            createWS_alg.setProperty("Names", names)
            createWS_alg.setProperty("Units", units)
            createWS_alg.setProperty("Frames", frames)
            createWS_alg.execute()
            outWS_data = createWS_alg.getProperty("OutputWorkspace").value
            mtd.addOrReplace(self.getPropertyValue("OutputWorkspace")+'_data', outWS_data)

            # Create normalisation workspace
            progress.report('Creating norm MDHistoWorkspace')
            createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
            createWS_alg.setProperty("SignalInput", output_norm[1:-1,1:-1,1:-1].ravel('F'))
            createWS_alg.setProperty("ErrorInput", np.sqrt(output_norm[1:-1,1:-1,1:-1].ravel('F')))
            createWS_alg.setProperty("Dimensionality", 3)
            createWS_alg.setProperty("Extents", '{},{},{},{},{},{}'.format(dim0_min,dim0_max,dim1_min,dim1_max,dim2_min,dim2_max))
            createWS_alg.setProperty("NumberOfBins", '{},{},{}'.format(dim0_bins,dim1_bins,dim2_bins))
            createWS_alg.setProperty("Names", names)
            createWS_alg.setProperty("Units", units)
            createWS_alg.setProperty("Frames", frames)
            createWS_alg.execute()
            mtd.addOrReplace(self.getPropertyValue("OutputWorkspace")+'_normalization', createWS_alg.getProperty("OutputWorkspace").value)

        old_settings = np.seterr(divide='ignore', invalid='ignore') # Ignore RuntimeWarning: invalid value encountered in true_divide
        output /= output_norm # We often divide by zero here and we get NaN's, this is desired behaviour
        np.seterr(**old_settings)

        progress.report('Creating MDHistoWorkspace')
        createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
        createWS_alg.setProperty("SignalInput", output[1:-1,1:-1,1:-1].ravel('F'))
        createWS_alg.setProperty("ErrorInput", np.sqrt(output[1:-1,1:-1,1:-1].ravel('F')))
        createWS_alg.setProperty("Dimensionality", 3)
        createWS_alg.setProperty("Extents", '{},{},{},{},{},{}'.format(dim0_min,dim0_max,dim1_min,dim1_max,dim2_min,dim2_max))
        createWS_alg.setProperty("NumberOfBins", '{},{},{}'.format(dim0_bins,dim1_bins,dim2_bins))
        createWS_alg.setProperty("Names", names)
        createWS_alg.setProperty("Units", units)
        createWS_alg.setProperty("Frames", frames)
        createWS_alg.execute()
        outWS = createWS_alg.getProperty("OutputWorkspace").value

        # Copy experiment infos
        if inWS.getNumExperimentInfo() > 0:
            outWS.copyExperimentInfos(inWS)

        outWS.getExperimentInfo(0).run().addProperty('RUBW_MATRIX', list(UBW.flatten()), True)
        outWS.getExperimentInfo(0).run().addProperty('W_MATRIX', list(W.flatten()), True)
        try:
            if outWS.getExperimentInfo(0).sample().hasOrientedLattice():
                outWS.getExperimentInfo(0).sample().getOrientedLattice().setUB(UB)
        except NameError:
            pass

        if self.getProperty('KeepTemporaryWorkspaces').value:
            outWS_data.copyExperimentInfos(outWS)

        progress.report()
        self.setProperty("OutputWorkspace", outWS)


AlgorithmFactory.subscribe(ConvertWANDSCDtoQ)

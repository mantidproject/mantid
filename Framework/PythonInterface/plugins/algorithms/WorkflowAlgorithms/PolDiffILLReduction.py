# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import FileProperty, MatrixWorkspaceProperty, MultipleFileProperty, \
    PropertyMode, Progress, PythonAlgorithm, WorkspaceGroup, WorkspaceGroupProperty, \
    FileAction, AlgorithmFactory
from mantid.kernel import Direction, EnabledWhenProperty, IntBoundedValidator, LogicOperator, \
    PropertyCriterion, StringListValidator

from mantid.simpleapi import *

import numpy as np
import math


class PolDiffILLReduction(PythonAlgorithm):

    _mode = 'Monochromatic'
    _method = 'Uniaxial'
    _instrument = None
    _DEG_2_RAD =  np.pi / 180.0

    _sampleGeometry = None
    _gyromagnetiRatio = 1.832472e8 # [s^-1 T^-1], from NIST
    _r0 = 2.817940e-15 # [m], classical e radius, from NIST

    @staticmethod
    def _max_value_per_detector(ws):
        max_values = np.zeros(mtd[ws].getItem(0).getNumberOfHistograms())
        for entry in mtd[ws]:
            for spectrum_no in range(entry.getNumberOfHistograms()):
                dataY = entry.readY(spectrum_no)
                if dataY > max_values[spectrum_no]:
                    max_values = dataY
        return max_values

    def category(self):
        return 'ILL\\Diffraction'

    def summary(self):
        return 'Performs polarized diffraction data reduction at the ILL.'

    def seeAlso(self):
        return ['PolDIffILLAutoProcess']

    def name(self):
        return 'PolDiffILLReduction'

    def validateInputs(self):
        issues = dict()
        process = self.getPropertyValue('ProcessAs')
        if process == 'Transmission' and self.getProperty('BeamInputWorkspace').isDefault:
            issues['BeamInputWorkspace'] = 'Beam input workspace is mandatory for transmission calculation.'
            issues['CadmiumTransmissionInputWorkspace'] = 'Cadmium transmission input workspace is mandatory for transmission calculation.'

        if process == 'Quartz' and ( self.getProperty('TransmissionInputWorkspace').isDefault
                                     or self.getProperty('ContainerInputWorkspace').isDefault
                                     or self.getProperty('AbsorberInputWorkspace').isDefault ) :
            issues['TransmissionInputWorkspace'] = 'Quartz transmission is mandatory for polarization correction calculation.'
            issues['ContainerInputWorkspace'] = 'Container input workspace is mandatory for polarization correction calculation.'
            issues['AbsorberInputWorkspace'] ='Absorber input workspace is mandatory for polarization correction calculation.'

        if process == 'Vanadium' and ( self.getProperty('TransmissionInputWorkspace').isDefault
                                       or self.getProperty('ContainerInputWorkspace').isDefault
                                       or self.getProperty('QuartzInputWorkspace').isDefault
                                       or self.getProperty('AbsorberInputWorkspace').isDefault ) :
            issues['TransmissionInputWorkspace'] = 'Vanadium transmission is mandatory for vanadium data reduction.'
            issues['QuartzInputWorkspace'] = 'Polarisation correction workspace is mandatory for vanadium data reduction.'
            issues['ContainerInputWorkspace'] = 'Container input workspace is mandatory for vanadium data reduction.'
            issues['AbsorberInputWorkspace'] = 'Absorber input workspace is mandatory for vanadium data reduction.'

        if process == 'Sample' or process == 'Vanadium':
            if self.getProperty('SampleGeometry') != 'None' and self.getProperty('ChemicalFormula').isDefault:
                issues['ChemicalFormula'] = 'Chemical formula of the sample is required for self-attenuation correction.'
            if self.getProperty('SampleGeometryDictionary').isDefault:
                issues['SampleGeometryDictionary'] = 'Sample parameters need to be defined.'

        if process == 'Sample':
            if ( self.getProperty('TransmissionInputWorkspace').isDefault
                 or self.getProperty('QuartzInputWorkspace').isDefault
                 or self.getProperty('ContainerInputWorkspace').isDefault
                 or self.getProperty('AbsorberInputWorkspace').isDefault ) :
                issues['TransmissionInputWorkspace'] = 'Sample transmission is mandatory for sample data reduction.'
                issues['QuartzInputWorkspace'] = 'Vanadium input workspace is mandatory for sample data reduction.'
                issues['ContainerInputWorkspace'] = 'Container input workspace is mandatory for sample data reduction.'
                issues['AbsorberInputWorkspace'] = 'Absorber input workspace is mandatory for sample data reduction.'

            if (self.getProperty('DetectorEfficiencyCalibration') == 'Vanadium'
                    and self.getProperty('VanadiumInputWorkspace').isDefault):
                issues['VanadiumInputWorkspace'] = 'Vanadium input workspace is mandatory for sample data reduction when \
                    detector efficiency calibration is based "Vanadium".'

        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        options = ['Absorber', 'Beam', 'Transmission', 'Container', 'Quartz', 'Vanadium', 'Sample']

        self.declareProperty(name='ProcessAs',
                             defaultValue='Sample',
                             validator=StringListValidator(options),
                             doc='Choose the process type.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc='The output workspace based on the value of ProcessAs.')

        sample = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Sample')

        transmission = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Transmission')

        container = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Container')

        absorber = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Absorber')

        quartz = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Quartz')

        vanadium = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Vanadium')

        reduction = EnabledWhenProperty(quartz, EnabledWhenProperty(vanadium, sample, LogicOperator.Or), LogicOperator.Or)

        scan = EnabledWhenProperty(reduction, EnabledWhenProperty(absorber, container, LogicOperator.Or), LogicOperator.Or)

        self.declareProperty(WorkspaceGroupProperty('AbsorberInputWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the absorber workspace.')

        self.setPropertySettings('AbsorberInputWorkspace',
                                 EnabledWhenProperty(quartz,
                                                     EnabledWhenProperty(vanadium, sample, LogicOperator.Or),
                                                     LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('BeamInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the empty beam input workspace.')

        self.setPropertySettings('BeamInputWorkspace', transmission)

        self.declareProperty(MatrixWorkspaceProperty('CadmiumTransmissionInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the cadmium transmission input workspace.')

        self.setPropertySettings('CadmiumTransmissionInputWorkspace', transmission)

        self.declareProperty(MatrixWorkspaceProperty('TransmissionInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the transmission input workspace.')

        self.setPropertySettings('TransmissionInputWorkspace', reduction)

        self.declareProperty(WorkspaceGroupProperty('ContainerInputWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the container workspace.')

        self.setPropertySettings('ContainerInputWorkspace', reduction)

        self.declareProperty(WorkspaceGroupProperty('QuartzInputWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the polarisation efficiency correction workspace.')

        self.setPropertySettings('QuartzInputWorkspace',
                                 EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty(WorkspaceGroupProperty('VanadiumInputWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the vanadium workspace.')

        self.setPropertySettings('VanadiumInputWorkspace', EnabledWhenProperty('DetectorEfficiencyCalibration',
                                                                               PropertyCriterion.IsEqualTo, 'Vanadium'))

        self.declareProperty('SumScan', False,
                             doc='Whether or not to sum the multiple scan steps into a single distribution')

        self.setPropertySettings('SumScan', scan)

        self.declareProperty('SubtractBackground', True,
                             doc='Whether or not to subtract background from the current sample data.')

        self.setPropertySettings('SubtractBackground', reduction)

        self.declareProperty('AbsoluteUnitsNormalisation', True,
                             doc='Whether or not express the output in absolute units.')

        self.setPropertySettings('AbsoluteUnitsNormalisation', EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty('ClearCache', True,
                             doc='Whether or not to clear the cache of intermediate workspaces.')

        # self-attenuation group:
        self.declareProperty(name="SampleGeometry",
                             defaultValue="FlatPlate",
                             validator=StringListValidator(["FlatPlate", "Cylinder", "Annulus", "Custom", "None"]),
                             direction=Direction.Input,
                             doc="Sample geometry for self-attenuation correction to be applied.")

        self.setPropertySettings('SampleGeometry', EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty(name="SampleGeometryDictionary",
                             defaultValue="",
                             direction=Direction.Input,
                             doc="Dictionary for the geometry used for self-attenuation correction.")

        self.setPropertySettings('SampleGeometryDictionary',
                                 EnabledWhenProperty('SampleGeometry', PropertyCriterion.IsNotEqualTo, 'Custom'))

        self.declareProperty(FileProperty(name="SampleGeometryFile",
                             defaultValue="",
                             action=FileAction.OptionalLoad),
                             doc="The path to the custom geometry for self-attenuation correction to be applied.")

        self.setPropertySettings('SampleGeometryFile', EnabledWhenProperty('SampleGeometry', PropertyCriterion.IsEqualTo, 'Custom'))

        self.declareProperty(name="ChemicalFormula",
                             defaultValue="",
                             direction=Direction.Input,
                             doc="Chemical formula of the sample")

        self.setPropertySettings('ChemicalFormula', EnabledWhenProperty('SampleGeometry', PropertyCriterion.IsNotEqualTo, 'None'))

        self.declareProperty(name="DetectorEfficiencyCalibration",
                             defaultValue="Incoherent",
                             validator=StringListValidator(["Incoherent", "Vanadium",  "Paramagnetic"]),
                             direction=Direction.Input,
                             doc="Detector efficiency calibration type.")

        self.setPropertySettings('DetectorEfficiencyCalibration', sample)

        self.declareProperty(name="IncoherentCrossSection",
                             defaultValue=0,
                             validator=IntBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc="Cross-section for incoherent scattering, necessary for setting the output on the absolute scale.")

        incoherent = EnabledWhenProperty('DetectorEfficiencyCalibration', PropertyCriterion.IsEqualTo, 'Incoherent')

        absoluteNormalisation = EnabledWhenProperty('AbsoluteUnitsNormalisation', PropertyCriterion.IsDefault)

        self.setPropertySettings('IncoherentCrossSection', EnabledWhenProperty(incoherent, absoluteNormalisation,
                                                                               LogicOperator.Or))

        self.declareProperty(FileProperty('InstrumentParameterFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['.xml']),
                             doc='The path to the calibrated Instrument Parameter File.')

        self.setPropertySettings('InstrumentParameterFile', scan)

    def _figureMeasurementMethod(self, ws):
        nEntriesPerNumor = mtd[ws].getNumberOfEntries() / len(self.getPropertyValue('Run').split(','))
        if nEntriesPerNumor == 10:
            self._method = '10-p'
        elif nEntriesPerNumor == 6:
            self._method = 'XYZ'
        elif nEntriesPerNumor == 2:
            self._method = 'Uniaxial'
        else:
            print(self.getPropertyValue('ProcessAs'))
            if self.getPropertyValue("ProcessAs") not in ['Beam', 'Transmission']:
                raise RuntimeError("The analysis options are: Uniaxial, XYZ, and 10-point. \
                    The provided input does not fit in any of these measurement types.")

    def _merge_polarizations(self, ws):
        """ws_group: large group of many files with the same number of POL directions"""
        pol_directions = set()
        numors = set()
        for name in mtd[ws].getNames():
            numors.add(name[:-2])
            pol_directions.add(name[-1])
        if len(numors) > 1:
            names_list = []
            for direction in sorted(list(pol_directions)):
                list_pol = []
                for numor in numors:
                    list_pol.append('{0}_{1}'.format(numor, direction))
                SumOverlappingTubes(','.join(list_pol), OutputWorkspace='{0}_{1}'.format(ws[2:], direction),
                                    OutputType='1D', ScatteringAngleBinning=0.5, Normalise=True, HeightAxis='-0.1,0.1')
                names_list.append('{0}_{1}'.format(ws[2:], direction))
            GroupWorkspaces(InputWorkspaces=names_list, OutputWorkspace=ws)
        return ws

    def _normalise(self, ws):
        """Normalizes the provided WorkspaceGroup to the second monitor."""
        monID = 100000
        monitorIndices = "{},{}".format(mtd[ws].getItem(0).getNumberHistograms()-2,
                                        mtd[ws].getItem(0).getNumberHistograms()-1)
        for entry_no, entry in enumerate(mtd[ws]):
            mon = ws + '_mon'
            ExtractSpectra(InputWorkspace=entry, DetectorList=monID, OutputWorkspace=mon)
            if 0 in mtd[mon].readY(0):
                raise RuntimeError('Cannot normalise to monitor; monitor has 0 counts.')
            else:
                Divide(LHSWorkspace=entry, RHSWorkspace=mon, OutputWorkspace=entry)
                RemoveSpectra(entry, WorkspaceIndices=monitorIndices, OutputWorkspace=entry)
                DeleteWorkspace(mon)
        return ws

    def _calculate_transmission(self, ws, ws_beam, ws_cadmium):
        # extract Monitor2 values
        monID = 100001
        mon = ws + '_mon'
        ExtractSpectra(InputWorkspace=ws, DetectorList=monID, OutputWorkspace=mon)
        if 0 in mtd[mon].getItem(0).readY(0):
            raise RuntimeError('Cannot calculate transmission; monitor has 0 counts.')
        beam_mon = ws_beam + '_mon'
        ExtractSpectra(InputWorkspace=ws_beam, DetectorList=monID, OutputWorkspace=beam_mon)
        if 0 in mtd[beam_mon].readY(0):
            raise RuntimeError('Cannot calculate transmission; beam monitor has 0 counts.')
        cadmium_mon = ws_cadmium + '_mon'
        ExtractSpectra(InputWorkspace=ws_cadmium, DetectorList=monID, OutputWorkspace=cadmium_mon)
        if 0 in mtd[beam_mon].readY(0):
            raise RuntimeError('Cannot calculate transmission; beam monitor has 0 counts.')
        else:
            Divide(LHSWorkspace=mtd[mon]-mtd[cadmium_mon], RHSWorkspace=mtd[beam_mon]-mtd[cadmium_mon], OutputWorkspace=ws)
            DeleteWorkspace(mon)
            DeleteWorkspace(beam_mon)
            DeleteWorkspace(cadmium_mon)
        return ws

    def _background_subtract(self, ws, ws_absorber, ws_container, ws_transmission):
        """ Subtracs empty container and cadmium scaled by transmission."""
        for entry_no, entry in enumerate(mtd[ws]):
            ws_absorber_entry = mtd[ws_absorber].getItem(entry_no).name()
            ws_container_entry = mtd[ws_container].getItem(entry_no).name()
            Minus(LHSWorkspace=entry.name(),
                  RHSWorkspace=mtd[ws_transmission] * mtd[ws_container_entry]
                  + (1-mtd[ws_transmission]) * mtd[ws_absorber_entry],
                  OutputWorkspace=entry)
        return ws

    def _calculate_polarizing_efficiencies(self, ws):
        """Calculates the polarizing efficiencies using quartz data."""
        flipper_eff = 1.0 # this could be extracted from data if 4 measurements are done
        nMeasurementsPerPOL = 2
        tmp_names = []
        index = 0
        for entry_no in range(1, mtd[ws].getNumberOfEntries()+1, nMeasurementsPerPOL):
            # two polarizer-analyzer states, fixed flipper_eff
            ws_00 = mtd[ws].getItem(entry_no).name()
            ws_01 = mtd[ws].getItem(entry_no-1).name()
            tmp_name = '{0}_{1}_{2}'.format(ws[2:], mtd[ws_00].getRun().getLogData('POL.actual_state').value, index)
            Divide(LHSWorkspace=mtd[ws_00]-mtd[ws_01],
                   RHSWorkspace=(2*flipper_eff-1)*mtd[ws_00]+mtd[ws_01],
                   OutputWorkspace=tmp_name)
            tmp_names.append(tmp_name)
            print (entry_no, index, tmp_name, self._method, entry_no%6)
            if self._method == 'Uniaxial' and entry_no % 2 == 1:
                index += 1
            elif self._method == 'XYZ' and entry_no % 6 == 5:
                index += 1
            elif self._method == '10-p' and entry_no % 10 == 9:
                index += 1

        GroupWorkspaces(InputWorkspaces=tmp_names, OutputWorkspace='tmp')
        DeleteWorkspaces(ws)
        RenameWorkspace(InputWorkspace='tmp', OutputWorkspace=ws)
        return ws

    def _detector_analyser_energy_efficiency(self, ws):
        pass

    def _frame_overlap_correction(self, ws):
        pass

    def _apply_polarization_corrections(self, ws, pol_eff_ws):
        fp = 1 # flipper efficiency, placeholder
        for entry_no in range(mtd[ws].getNumberOfEntries()):
            if entry_no % 2 != 0:
                continue
            phi = mtd[pol_eff_ws].getItem(int(entry_no/2)).name()
            intensity_0 = mtd[ws].getItem(entry_no).name()
            intensity_1 = mtd[ws].getItem(entry_no+1).name()
            tmp_names = [intensity_0 + '_tmp']
            tmp_names.append(intensity_1 + '_tmp')

            Divide(LHSWorkspace=((1.0-mtd[phi])*(1.0-fp) + fp*(1+mtd[phi]))*mtd[intensity_0]
                   -(1.0-mtd[phi])*mtd[intensity_1],
                   RHSWorkspace=2.0 * fp * mtd[phi],
                   OutputWorkspace=tmp_names[0])
            Divide(LHSWorkspace=(1+mtd[phi])*mtd[intensity_1]
                   - ( (1+mtd[phi])*(1-fp) - fp*(1-mtd[phi]) )*mtd[intensity_0],
                   RHSWorkspace=2.0 * fp * mtd[phi],
                   OutputWorkspace=tmp_names[1])

            RenameWorkspace(tmp_names[0], intensity_0)
            RenameWorkspace(tmp_names[1], intensity_1)

        return ws

    def _read_sample_geometry(self):
        raw_list = (self.getPropertyValue('SampleGeometryDictionary')).split(';')
        self._sampleGeometry = { item.split('=')[0] : item.split('=')[1] for item in raw_list }
        geometry_type = self.getPropertyValue('SampleGeometry')
        print (self._sampleGeometry)
        required_keys = ['mass', 'density', 'n_atoms', 'beam_height', 'beam_width', 'container_density']

        if geometry_type == 'FlatPlate':
            required_keys += ['height', 'width', 'thickness', 'container_front_thickness', 'container_back_thickness']
        if geometry_type == 'Cylinder':
            required_keys += ['height', 'radius', 'container_radius']
        if geometry_type == 'Annulus':
            required_keys += ['height', 'inner_radius', 'outer_radius', 'container_inner_radius', 'container_outer_radius']

        for key in required_keys:
            if key not in self._sampleGeometry:
                raise RuntimeError('{} needs to be defined in the dictionary'.format(key))

    def _apply_self_attenuation_correction(self, sample_ws, container_ws):
        geometry_type = self.getPropertyValue('SampleGeometry')

        kwargs = {}
        kwargs['BeamHeight'] = self._sampleGeometry['beam_height']
        kwargs['BeamWidth'] = self._sampleGeometry['beam_width']
        kwargs['SampleDensity'] = self._sampleGeometry['density']
        kwargs['SampleHeight'] = self._sampleGeometry['height']
        kwargs['SampleChemicalFormula'] = self.getPropertyValue('ChemicalFormula')
        kwargs['ContainerChemicalFormula'] = self._sampleGeometry['container_formula']
        kwargs['ContainerDensity'] = self.getPropertyValue('container_density')

        if geometry_type == 'FlatPlate':
            kwargs['SampleWidth'] = self._sampleGeometry['width']
            kwargs['SampleThickness'] = self._sampleGeometry['thickness']
            kwargs['ContainerFrontThickness'] = self._sampleGeometry['container_front_thickness']
            kwargs['ContainerBackThickness'] = self._sampleGeometry['container_back_thickness']
        elif geometry_type == 'Cylinder':
            kwargs['SampleRadius'] = self._sampleGeometry['radius']
            kwargs['ContainerRadius'] = self._sampleGeometry['container_radius']
        elif geometry_type == 'Annulus':
            kwargs['SampleInnerRadius'] = self._sampleGeometry['inner_radius']
            kwargs['SampleOuterRadius']  = self._sampleGeometry['outer_radius']
            kwargs['ContainerInnerRadius'] = self._sampleGeometry['container_inner_radius']
            kwargs['ContainerOuterRadius'] = self._sampleGeometry['container_outer_radius']
        elif geometry_type == 'Custom':
            pass

        PaalmanPingsMonteCarloAbsorption(
                SampleWorkspace=sample_ws,
                Shape=geometry_type,
                **kwargs,
                CorrectionsWorkspace='{}_corr'.format(geometry_type)
        )
        ApplyPaalmanPingsCorrection(SampleWorkspace=sample_ws,
                                    CorrectionsWorkspace='{}_corr'.format(geometry_type),
                                    CanWorkspace=container_ws)

        return ws

    def _component_separation(self, ws):
        tmp_names = []
        nMeasurements = 0
        if self._method == '10-p':
            raise RuntimeError("10-p component separation not implemented")
        elif self._method == 'XYZ':
            nMeasurements = 6
        elif self._method == 'Uniaxial':
            nMeasurements = 2

        for entry_no in range(0, mtd[ws].getNumberOfEntries(), nMeasurements):
            dataX = np.zeros(shape=(mtd[ws].getItem(entry_no).getNumberHistograms(), 3))
            dataY = np.zeros(shape=(mtd[ws].getItem(entry_no).getNumberHistograms(), 3))
            for spectrum in range(mtd[ws].getItem(entry_no).getNumberHistograms()):
                sigma_z_sf = mtd[ws].getItem(entry_no).readY(spectrum)
                sigma_z_nsf = mtd[ws].getItem(entry_no+1).readY(spectrum)
                if nMeasurements == 2:
                    dataY[spectrum][0] = 0 # Magnetic
                    dataY[spectrum][1] = 2.0 * sigma_z_nsf - sigma_z_sf  # Nuclear coherent
                    dataY[spectrum][2] = 2.0 * sigma_z_sf - sigma_z_nsf # Incoherent
                elif nMeasurements == 6 or nMeasurements == 10:
                    sigma_y_sf = mtd[ws].getItem(entry_no+2).readY(spectrum)
                    sigma_y_nsf = mtd[ws].getItem(entry_no+3).readY(spectrum)
                    sigma_x_sf = mtd[ws].getItem(entry_no+4).readY(spectrum)
                    sigma_x_nsf = mtd[ws].getItem(entry_no+5).readY(spectrum)
                    if nMeasurements == 6:
                        # Magnetic component
                        dataY[spectrum][0] = 2.0 * (2.0 * sigma_z_nsf - sigma_x_nsf - sigma_y_nsf )
                        # Nuclear coherent
                        dataY[spectrum][1] = (2.0*(sigma_x_nsf + sigma_y_nsf + sigma_z_nsf) - sigma_x_sf - sigma_y_sf - sigma_z_sf ) / 6.0
                        # Incoherent
                        dataY[spectrum][2] = 0.5 * (sigma_x_sf + sigma_y_sf + sigma_z_sf) - dataY[spectrum][0]
                    else:
                        # sigma_xmy_sf = mtd[ws].getItem(entry_no+6).readY(spectrum)
                        # sigma_xmy_nsf = mtd[ws].getItem(entry_no+7).readY(spectrum)
                        # sigma_xpy_sf = mtd[ws].getItem(entry_no+8).readY(spectrum)
                        # sigma_xpy_nsf = mtd[ws].getItem(entry_no+9).readY(spectrum)

                        raise RuntimeError('10-point method has not been implemented yet')

                dataX[spectrum] = range(3)
                dataE = np.sqrt(dataY)
                tmp_name = str(mtd[ws].getItem(entry_no).name())[:-1] + 'comp_sep'
                tmp_names.append(tmp_name)
                CreateWorkspace(DataX=dataX, DataY=dataY, dataE=dataE,
                                Nspec=mtd[ws].getItem(entry_no).getNumberHistograms(),
                                OutputWorkspace=tmp_name)

        GroupWorkspaces(tmp_names, OutputWorkspace='component_separation')

    def _detector_efficiency(self, ws):
        calibrationType = self.getPropertyValue('DetectorEfficiencyCalibration')
        normaliseToAbsoluteUnits = self.getProperty('AbsoluteUnitsNormalisation')
        tmp_name = 'det_eff'
        tmp_names = []
        if calibrationType == 'Vanadium':
            ws_vanadium = self.getPropertyValue('VanadiumWorkspaceInput')
            if normaliseToAbsoluteUnits:
                normFactor = self._formulaUnits
            else:
                for entry_no, entry in enumerate(mtd[ws_vanadium]):
                    if not normaliseToAbsoluteUnits:
                        normFactor = math.max(entry.readY())
            ws_name = '{0}_{1}'.format(tmp_name, entry_no)
            tmp_names.append(ws_name)
            Divide(LHSWorkspace=ws_vanadium,
                   RHSWorkspace=CreateSingleValuedWorkspace(normFactor),
                   OutputWorkspace=ws_name)
        elif calibrationType in  ['Paramagnetic', 'Incoherent']:
            if calibrationType == 'Paramagnetic':
                if self._mode == 'TOF':
                    raise RuntimeError('Paramagnetic calibration is not valid in the TOF mode.')
                if self._method == 'Uniaxial':
                    raise RuntimeError('Paramagnetic calibration is not valid in the Uniaxial measurement mode.')
                for entry_no, entry in enumerate(mtd[ws]):
                    ws_name = '{0}_{1}'.format(tmp_name, entry_no)
                    tmp_names.append(ws_name)
                    const = (2.0/3.0) * math.pow(self._gyromagneticRatio*self._r0, 2)
                    paramagneticComponent = mtd[conjoined_components].getItem(2)
                    Divide(LHSWorkspace=const * entry * (entry+1),
                           RHSWorkspace=paramagneticComponent,
                           OutputWorkspace=ws_name)
            else: # Incoherent
                if self._mode == 'TOF':
                    raise RuntimeError('Incoherent calibration is not valid in the TOF mode.')
                dataY = np.zeros(mtd[conjoined_components].getItem(1).getNumberOfHistograms())
                for spectrum_no in range(mtd[conjoined_components].getItem(1).getNumberOfHistograms()):
                    if normaliseToAbsoluteUnits:
                        normFactor = float(self.getPropertyValue('IncoherentCrossSection'))
                        CreateSingleValuedWorkspace(DataValue=normFactor, OutputWorkspace='normalisation_ws')
                    else:
                        normalisationFactors = self._max_values_per_detector(vanadium_ws)
                        dataE = np.sqrt(normalisationFactors)
                        CreateWorkspace(dataX=mtd[vanadium_ws].getItem(0).readX(0), dataY=normalisationFactors,
                                        dataE=dataE,
                                        NSpec=mtd[vanadium_ws].getItem(0).getNumberOfHistograms(),
                                        OutputWorkspace='normalisation_ws')
                    ws_name = '{0}_{1}'.format(tmp_name, entry_no)
                    tmp_names.append(ws_name)

                Divide(LHSWorkspace='normalisation_ws',
                       RHSWorkspace=component_ws,
                       OutputWorkspace=ws_name)

        GroupWorkspaces(tmp_names, OutputWorkspace='det_efficiency')

        for entry_no, entry in enumerate(mtd[ws]):
            Multiply(LHSWorkspace=entry,
                     RHSWorkspace=mtd['det_efficiency'].getItem(entry_no),
                     OutputWorkspace=entry)

        return ws

    def _output_vanadium(self, ws, n_atoms):
        if self._mode != 'TOF':
            Divide(LHSWorkspace=CreateSingleValuedWorkspace(0.404*n_atoms), RHSWorkspace=ws, OutputWorkspace=ws)
        else:
            raise RuntimeError("TOF reduction not implemented")
        return ws

    def _output_sample(self, ws):
        pass

    def _finalize(self, ws, process):
        ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws, NaNValue=0,
                             NaNError=0, InfinityValue=0, InfinityError=0)
        mtd[ws].getItem(0).getRun().addProperty('ProcessedAs', process, True)
        if self.getProperty('SumScan').value and isinstance(mtd[ws], WorkspaceGroup) and mtd[ws].getNumberOfEntries() > 1:
            self._merge_polarizations(ws)
        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        self.setProperty('OutputWorkspace', mtd[ws[2:]])

    def PyExec(self):
        process = self.getPropertyValue('ProcessAs')
        processes = ['Absorber', 'Beam', 'Transmission', 'Container', 'Quartz', 'Vanadium', 'Sample']
        progress = Progress(self, start=0.0, end=1.0, nreports=processes.index(process) + 1)
        ws = '__' + self.getPropertyValue('OutputWorkspace')
        # we do not want the summing done by LoadAndMerge since it will be pair-wise and slow
        # instead we load and list, and merge once with merge runs
        calibration_setting = 'YIGFile'
        if self.getProperty('InstrumentParameterFile').isDefault:
            calibration_setting = 'None'

        Load(Filename=self.getPropertyValue('Run').replace('+',','), LoaderName='LoadILLPolarizedDiffraction',
             PositionCalibration=calibration_setting, YIGFileName=self.getPropertyValue('InstrumentParameterFile'),
             OutputWorkspace=ws)

        self._instrument = mtd[ws].getItem(0).getInstrument().getName()
        run = mtd[ws].getItem(0).getRun()
        if run['acquisition_mode'].value == 1:
            self._mode = 'TOF'
        self._figureMeasurementMethod(ws)
        progress.report()
        if process in ['Transmission']:
            beam_ws = self.getPropertyValue('BeamInputWorkspace')
            cadmium_ws = self.getPropertyValue('CadmiumTransmissionInputWorkspace')
            self._calculate_transmission(ws, beam_ws, cadmium_ws)
            progress.report()
        elif process not in ['Beam']:
            self._normalise(ws)
        if process in ['Quartz', 'Vanadium', 'Sample']:
            absorber_ws = self.getPropertyValue('AbsorberInputWorkspace')
            container_ws = self.getPropertyValue('ContainerInputWorkspace')
            transmission_ws = self.getPropertyValue('TransmissionInputWorkspace')
            if self.getProperty('SubtractBackground').value:
                self._background_subtract(ws, absorber_ws, container_ws, transmission_ws)
                progress.report()

            if process == 'Quartz':
                ws = self._calculate_polarizing_efficiencies(ws)
                progress.report()

            if process in ['Vanadium', 'Sample']:
                if self._mode == 'TOF':
                    if process == 'Vanadium':
                        self._detector_analyser_energy_efficiency(ws)
                    else:
                        self._frame_overlap_correction(ws)
                    progress.report()
                pol_eff_ws = self.getPropertyValue('QuartzInputWorkspace')
                if pol_eff_ws:
                    self._apply_polarization_corrections(ws, pol_eff_ws)
                    progress.report()
                self._read_sample_geometry()
                self._apply_self_attenuation_correction(ws, container_ws)
                progress.report()
                self._component_separation(ws)
                progress.report()
                if process == 'Vanadium':
                    self._output_vanadium(ws, self._sampleGeometry['n_atoms'])
                else:
                    self._detector_efficiency_correction(ws)
                    progress.report()
                    self._output_sample(ws)

        self._finalize(ws, process)


AlgorithmFactory.subscribe(PolDiffILLReduction)

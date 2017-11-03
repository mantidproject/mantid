# pylint: disable=no-init,too-few-public-methods
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import *
from mantid.simpleapi import *
from mantid.api import *
from mantid.geometry import SpaceGroupFactory, CrystalStructure, UnitCell

from six import iteritems
import re
import numpy as np


# pylint: disable=invalid-name
def removeErrorEstimateFromNumber(numberString):
    errorBegin = numberString.find('(')

    if errorBegin == -1:
        return numberString

    return numberString[:errorBegin]


def getFloatOrNone(strValue):
    try:
        return float(strValue)
    except ValueError:
        return None


def convertBtoU(bIso):
    if bIso is None:
        return None

    return bIso / (8.0 * np.pi * np.pi)


class SpaceGroupBuilder(object):
    """
    Helper class that extracts the space group from CIF data provided by PyCifRW.

    For testing purposes, dictionaries with the appropriate data can
    be passed in as well, so the source of the parsed data is replaceable.
    """

    def __init__(self, cifData=None):
        if cifData is not None:
            self.spaceGroup = self._getSpaceGroup(cifData)

    def _getSpaceGroup(self, cifData):
        try:
            return self._getSpaceGroupFromString(cifData)
        # pylint: disable=unused-variable
        except (RuntimeError, ValueError):
            try:
                return self._getSpaceGroupFromNumber(cifData)
            # pylint: disable=unused-variable,invalid-name
            except RuntimeError:
                raise RuntimeError(
                    'Can not create space group from supplied CIF-file. You could try to modify the HM-symbol '
                    'to contain spaces between the components.\n'
                    'Keys to look for: _space_group_name_H-M_alt, _symmetry_space_group_name_H-M')

    def _getSpaceGroupFromString(self, cifData):
        # Try two possibilities for space group symbol. If neither is present, throw a RuntimeError.
        rawSpaceGroupSymbol = [str(cifData[x]) for x in
                               ['_space_group_name_h-m_alt', '_symmetry_space_group_name_h-m'] if
                               x in cifData.keys()]

        if len(rawSpaceGroupSymbol) == 0:
            raise RuntimeError('No space group symbol in CIF.')

        cleanSpaceGroupSymbol = self._getCleanSpaceGroupSymbol(rawSpaceGroupSymbol[0])

        # If the symbol is not registered, throw as well.
        return SpaceGroupFactory.createSpaceGroup(cleanSpaceGroupSymbol).getHMSymbol()

    def _getCleanSpaceGroupSymbol(self, rawSpaceGroupSymbol):
        # Remove :1 and :H from the symbol. Those are not required at the moment because they are the default.
        # Also substitute 'R' and 'Z' endings used by ICSD to indicate alternative origin choice or settings
        mappings = {':[1Hh]':'', ' S$':'', ' H$':'', ' Z$':' :2', ' R$':' :r'}
        for k, v in mappings.items():
            rawSpaceGroupSymbol = re.sub(k, v, rawSpaceGroupSymbol)
        return rawSpaceGroupSymbol.strip()

    def _getSpaceGroupFromNumber(self, cifData):
        spaceGroupNumber = [int(cifData[x]) for x in
                            ['_space_group_it_number', '_symmetry_int_tables_number'] if
                            x in cifData.keys()]

        if len(spaceGroupNumber) == 0:
            raise RuntimeError('No space group symbol in CIF.')

        possibleSpaceGroupSymbols = SpaceGroupFactory.subscribedSpaceGroupSymbols(spaceGroupNumber[0])

        if len(possibleSpaceGroupSymbols) != 1:
            raise RuntimeError(
                'Can not use space group number to determine space group for no. {0}'.format(spaceGroupNumber))

        return SpaceGroupFactory.createSpaceGroup(possibleSpaceGroupSymbols[0]).getHMSymbol()


class UnitCellBuilder(object):
    """
    Helper class that builds a unit cell from CIF data provided by PyCifRW.

    For testing purposes, dictionaries with the appropriate data can
    be passed in as well, so the source of the parsed data is replaceable.
    """

    def __init__(self, cifData=None):
        if cifData is not None:
            self.unitCell = self._getUnitCell(cifData)

    def _getUnitCell(self, cifData):
        unitCellComponents = ['_cell_length_a', '_cell_length_b', '_cell_length_c',
                              '_cell_angle_alpha', '_cell_angle_beta', '_cell_angle_gamma']

        unitCellValueMap = dict([(str(x), removeErrorEstimateFromNumber(str(cifData[x]))) if x in cifData.keys()
                                 else (str(x), None) for x in
                                 unitCellComponents])

        if unitCellValueMap['_cell_length_a'] is None:
            raise RuntimeError('The a-parameter of the unit cell is not specified in the supplied CIF.\n'
                               'Key to look for: _cell_length_a')

        replacementMap = {
            '_cell_length_b': str(unitCellValueMap['_cell_length_a']),
            '_cell_length_c': str(unitCellValueMap['_cell_length_a']),
            '_cell_angle_alpha': '90.0', '_cell_angle_beta': '90.0', '_cell_angle_gamma': '90.0'}

        unitCellValues = [
            unitCellValueMap[str(key)] if unitCellValueMap[str(key)] is not None else replacementMap[str(key)] for key
            in unitCellComponents]

        return ' '.join(unitCellValues)


class AtomListBuilder(object):
    """
    Helper class that builds a list of atoms from CIF data provided by PyCifRW.

    For testing purposes, dictionaries with the appropriate data can
    be passed in as well, so the source of the parsed data is replaceable.
    """

    def __init__(self, cifData=None, unitCell=None):
        if cifData is not None:
            self.atomList = self._getAtoms(cifData, unitCell)

    def _getAtoms(self, cifData, unitCell=None):
        labels = self._getLabels(cifData)

        atomCoordinates = self._getAtomCoordinates(cifData, labels)
        occupancies = self._getOccupancies(cifData, labels)
        atomSymbols = self._getAtomSymbols(cifData, labels)

        isotropicUs = self._getIsotropicUs(cifData, labels, unitCell)

        atomLines = []
        for atomLabel in labels:
            stringAtomLine = [str(x) for x in (
                atomSymbols[atomLabel], atomCoordinates[atomLabel], occupancies[atomLabel], isotropicUs[atomLabel]) if
                              x is not None]

            cleanLine = [stringAtomLine[0]] + [removeErrorEstimateFromNumber(x) for x in
                                               list(stringAtomLine[1:])]
            atomLines.append(' '.join(cleanLine))

        return ';'.join(atomLines)

    def _getLabels(self, cifData):
        try:
            return cifData['_atom_site_label']
        except KeyError:
            # If there are no atomic coordinates specified, there is really no point in continuing with replacement labels.
            if '_atom_site_fract_x' not in cifData.keys():
                raise RuntimeError(
                    'Too much information missing from CIF-file. Does it contain a loop_ that defines atoms?')

            return [str(x) for x in range(len(cifData['_atom_site_fract_x']))]

    def _getAtomCoordinates(self, cifData, labels):
        coordinateFields = ['_atom_site_fract_x', '_atom_site_fract_y', '_atom_site_fract_z']

        for field in coordinateFields:
            if field not in cifData.keys():
                raise RuntimeError(
                    'Mandatory field {0} not found in CIF-file.'
                    'Please check the atomic position definitions.'.format(field))

        # Return a dict like { 'label1': 'x y z', 'label2': 'x y z' }
        return dict(
            [(label, ' '.join([removeErrorEstimateFromNumber(c) for c in (x, y, z)])) for label, x, y, z in
             zip(labels, *[cifData[field] for field in coordinateFields])])

    def _getOccupancies(self, cifData, labels):
        occupancyField = '_atom_site_occupancy'

        occupancies = []
        if occupancyField in cifData.keys():
            occupancies += cifData[occupancyField]
        else:
            occupancies += ['1.0'] * len(labels)

        return dict(list(zip(labels, occupancies)))

    def _getAtomSymbols(self, cifData, labels):
        rawAtomSymbols = [cifData[x] for x in ['_atom_site_type_symbol', '_atom_site_label'] if x in
                          cifData.keys()]

        if len(rawAtomSymbols) == 0:
            raise RuntimeError('Cannot determine atom types, both _atom_site_type_symbol and _atom_site_label are '
                               'missing.')

        # Return a dict like { 'label1': 'Element1', ... } extracted from either _atom_site_type_symbol or _atom_site_label
        return dict(
            [(label, self._getCleanAtomSymbol(x)) for label, x in zip(labels, rawAtomSymbols[0])])

    def _getCleanAtomSymbol(self, atomSymbol):
        nonCharacterRe = re.compile('[^a-z]', re.IGNORECASE)

        return re.sub(nonCharacterRe, '', atomSymbol)

    def _getIsotropicUs(self, cifData, labels, unitCell):

        keyUIso = '_atom_site_u_iso_or_equiv'
        keyBIso = '_atom_site_b_iso_or_equiv'

        # Try to get a list of isotropic U-values, replace invalid ones by None
        isotropicUs = []
        if keyUIso in cifData.keys():
            isotropicUNoErrors = [removeErrorEstimateFromNumber(u) for u in cifData[keyUIso]]
            isotropicUs += [getFloatOrNone(u) for u in isotropicUNoErrors]
        elif keyBIso in cifData.keys():
            isotropicBsNoErrors = [removeErrorEstimateFromNumber(b) for b in cifData[keyBIso]]
            isotropicUs += [convertBtoU(getFloatOrNone(b)) for b in isotropicBsNoErrors]
        else:
            isotropicUs += [None] * len(labels)

        isotropicUMap = dict(list(zip(labels, isotropicUs)))

        # If there are None-objects in the list, try to get the equivalent U-values
        if None in isotropicUs:
            try:
                anisoLabels = self._get_ansitropic_labels(cifData)
                equivalentUMap = self._getEquivalentUs(cifData, anisoLabels, unitCell)

                for key, uIso in iteritems(isotropicUMap):
                    if uIso is None and key in equivalentUMap:
                        isotropicUMap[key] = equivalentUMap[key]

            except RuntimeError:
                pass

        # Return dict like { 'label1': 'U_iso_or_equiv', ... }
        return isotropicUMap

    def _getEquivalentUs(self, cifData, labels, unitCell):
        anisotropicParameters = self._getAnisotropicParametersU(cifData, labels)
        sumWeights = self._getMetricDependentWeights(unitCell)

        # Return U_equiv calculated according to [Fischer & Tillmanns, Acta Cryst C44, p775, 10.1107/S0108270187012745]
        # in a dict like { 'label1': 'U_equiv1' ... }. Invalid matrices (containing None) are excluded.
        return dict([(label, np.around(np.sum(np.multiply(uMatrix, sumWeights)) / 3., decimals=5))
                     for label, uMatrix in iteritems(anisotropicParameters) if uMatrix.dtype.type != np.object_])

    def _getAnisotropicParametersU(self, cifData, labels):
        # Try to extract U or if that fails, B.
        try:
            return self._getTensors(cifData, labels,
                                    ['_atom_site_aniso_u_11', '_atom_site_aniso_u_12', '_atom_site_aniso_u_13',
                                     '_atom_site_aniso_u_22', '_atom_site_aniso_u_23', '_atom_site_aniso_u_33'])
        except RuntimeError:
            bTensors = self._getTensors(cifData, labels,
                                        ['_atom_site_aniso_b_11', '_atom_site_aniso_b_12', '_atom_site_aniso_b_13',
                                         '_atom_site_aniso_b_22', '_atom_site_aniso_b_23', '_atom_site_aniso_b_33'])
            return dict([(label, convertBtoU(bTensor)) for label, bTensor in iteritems(bTensors)])

    def _get_ansitropic_labels(self, cifData):
        anisoLabel = '_atom_site_aniso_label'
        if anisoLabel not in cifData.keys():
            raise RuntimeError('Mandatory field \'_atom_site_aniso_label\' is missing.')
        anisoLabels = cifData[anisoLabel]
        return anisoLabels

    def _getTensors(self, cifData, labels, keys):
        values = []

        for key in keys:
            if key not in cifData.keys():
                raise RuntimeError('Can not construct tensor with missing element \'{0}\'.'.format(key))
            else:
                values.append([getFloatOrNone(removeErrorEstimateFromNumber(x)) for x in cifData[key]])

        # Return a 3x3-matrix for each label based on the assumption that u_j,i == u_i,j
        return dict([(label, np.array([[u11, u12, u13], [u12, u22, u23], [u13, u23, u33]])) for
                     label, u11, u12, u13, u22, u23, u33 in zip(labels, *values)])

    def _getMetricDependentWeights(self, unitCell):
        metricTensor = unitCell.getG()
        reciprocalMatrix = self._getReciprocalLengthSquaredMatrix(unitCell)

        return np.multiply(metricTensor, reciprocalMatrix)

    def _getReciprocalLengthSquaredMatrix(self, unitCell):
        reciprocalLengthVector = np.array([[unitCell.astar(), unitCell.bstar(), unitCell.cstar()]])

        return np.dot(reciprocalLengthVector.transpose(), reciprocalLengthVector)


class CrystalStructureBuilder(object):
    """
    This helper class simplifies the creation of CrystalStructure-objects from CIF-files. It uses the helper classes
    defined above.
    """

    def __init__(self, cifFile=None):
        if cifFile is not None:
            cifData = cifFile[list(cifFile.keys())[0]]

            self.spaceGroup = SpaceGroupBuilder(cifData).spaceGroup
            self.unitCell = UnitCellBuilder(cifData).unitCell

            self.atoms = AtomListBuilder(cifData, UnitCell(*[float(removeErrorEstimateFromNumber(x)) for x in
                                                             self.unitCell.split()])).atomList

    def getCrystalStructure(self):
        return CrystalStructure(self.unitCell, self.spaceGroup, self.atoms)


class UBMatrixBuilder(object):
    def __init__(self, cifFile=None):
        if cifFile is not None:
            cifData = cifFile[list(cifFile.keys())[0]]

            self._ubMatrix = self._getUBMatrix(cifData)

    def getUBMatrix(self):
        return self._ubMatrix

    def _getUBMatrix(self, cifData):
        ubMatrixKeys = ['_diffrn_orient_matrix_ub_11', '_diffrn_orient_matrix_ub_12', '_diffrn_orient_matrix_ub_13',
                        '_diffrn_orient_matrix_ub_21', '_diffrn_orient_matrix_ub_22', '_diffrn_orient_matrix_ub_23',
                        '_diffrn_orient_matrix_ub_31', '_diffrn_orient_matrix_ub_32', '_diffrn_orient_matrix_ub_33']

        ubValues = [str(cifData[key]) if key in cifData.keys() else None for key in ubMatrixKeys]

        if None in ubValues:
            raise RuntimeError('Can not load UB matrix from CIF, values are missing.')

        return ','.join(ubValues)


class LoadCIF(PythonAlgorithm):
    def category(self):
        return "Diffraction\\DataHandling"

    def name(self):
        return "LoadCIF"

    def summary(self):
        return "This algorithm loads a CIF file using the PyCifRW package and assigns a CrystalStructure to the sample of the workspace."

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty(name='Workspace',
                              defaultValue='', direction=Direction.InOut),
            doc='Workspace into which the crystal structure is placed.')

        self.declareProperty(
            FileProperty(name='InputFile',
                         defaultValue='',
                         action=FileAction.Load,
                         extensions=['cif']),
            doc='A CIF file containing a crystal structure.')

        self.declareProperty('LoadUBMatrix', False,
                             direction=Direction.Input,
                             doc='Load UB-matrix from CIF file if available.')

    def PyExec(self):
        try:
            self._loadFromCif()
        except ImportError:
            raise RuntimeError('This algorithm requires an additional Python package: PyCifRW'
                               ' (https://pypi.python.org/pypi/PyCifRW/4.1)')

    def _loadFromCif(self):
        from CifFile import ReadCif

        cifFileUrl = self._getFileUrl()
        workspace = self.getProperty('Workspace').value

        # Try to parse cif file using PyCifRW
        parsedCifFile = ReadCif(cifFileUrl)

        self._setCrystalStructureFromCifFile(workspace, parsedCifFile)

        ubOption = self.getProperty('LoadUBMatrix').value
        if ubOption:
            self._setUBMatrixFromCifFile(workspace, parsedCifFile)

    def _getFileUrl(self):
        # ReadCif requires a URL, windows path specs seem to confuse urllib,
        # so the pathname is converted to a URL before passing it to ReadCif.
        # pylint: disable=no-name-in-module
        try:
            from urllib import pathname2url
        except ImportError:
            from urllib.request import pathname2url

        cifFileName = self.getProperty('InputFile').value
        return pathname2url(cifFileName)

    def _setCrystalStructureFromCifFile(self, workspace, cifFile):
        crystalStructure = self._getCrystalStructureFromCifFile(cifFile)
        workspace.sample().setCrystalStructure(crystalStructure)

    def _getCrystalStructureFromCifFile(self, cifFile):
        builder = CrystalStructureBuilder(cifFile)
        crystalStructure = builder.getCrystalStructure()

        self.log().information('''Loaded the following crystal structure:
  Unit cell:
    {0}
  Space group:
    {1}
  Atoms:
    {2}
'''.format(builder.unitCell, builder.spaceGroup, '\n    '.join(builder.atoms.split(';'))))

        return crystalStructure

    def _setUBMatrixFromCifFile(self, workspace, cifFile):
        ubMatrix = self._getUBMatrixFromCifFile(cifFile)

        setUBAlgorithm = self.createChildAlgorithm('SetUB')
        setUBAlgorithm.setProperty('Workspace', workspace)
        setUBAlgorithm.setProperty('UB', ubMatrix)
        setUBAlgorithm.execute()

    def _getUBMatrixFromCifFile(self, cifFile):
        builder = UBMatrixBuilder(cifFile)

        return builder.getUBMatrix()


AlgorithmFactory.subscribe(LoadCIF)

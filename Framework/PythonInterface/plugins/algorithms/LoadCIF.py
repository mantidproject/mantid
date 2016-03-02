# pylint: disable=no-init,too-few-public-methods
from mantid.kernel import *
from mantid.simpleapi import *
from mantid.api import *
from mantid.geometry import SpaceGroupFactory, CrystalStructure, UnitCell

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
    return bIso / (8.0 * np.pi * np.pi)


class SpaceGroupBuilder(object):
    def __init__(self, cifData=None):
        if cifData is not None:
            self.spaceGroup = self._getSpaceGroup(cifData)

    def _getSpaceGroup(self, cifData):
        try:
            return self._getSpaceGroupFromString(cifData)
        # pylint: disable=unused-variable
        except (RuntimeError, ValueError) as error:
            try:
                return self._getSpaceGroupFromNumber(cifData)
            # pylint: disable=unused-variable,invalid-name
            except RuntimeError as e:
                raise RuntimeError(
                    'Can not create space group from supplied CIF-file. You could try to modify the HM-symbol ' \
                    'to contain spaces between the components.\n' \
                    'Keys to look for: _space_group_name_H-M_alt, _symmetry_space_group_name_H-M')

    def _getSpaceGroupFromString(self, cifData):
        # Try two possibilities for space group symbol. If neither is present, throw a RuntimeError.
        rawSpaceGroupSymbol = [str(cifData[x]) for x in
                               [u'_space_group_name_h-m_alt', u'_symmetry_space_group_name_h-m'] if
                               x in cifData.keys()]

        if len(rawSpaceGroupSymbol) == 0:
            raise RuntimeError('No space group symbol in CIF.')

        cleanSpaceGroupSymbol = self._getCleanSpaceGroupSymbol(rawSpaceGroupSymbol[0])

        # If the symbol is not registered, throw as well.
        return SpaceGroupFactory.createSpaceGroup(cleanSpaceGroupSymbol).getHMSymbol()

    def _getCleanSpaceGroupSymbol(self, rawSpaceGroupSymbol):
        # Remove :1 and :H from the symbol. Those are not required at the moment because they are the default.
        removalRe = re.compile(':[1H]', re.IGNORECASE)
        return re.sub(removalRe, '', rawSpaceGroupSymbol).strip()

    def _getSpaceGroupFromNumber(self, cifData):
        spaceGroupNumber = [int(cifData[x]) for x in
                            [u'_space_group_it_number', u'_symmetry_int_tables_number'] if
                            x in cifData.keys()]

        if len(spaceGroupNumber) == 0:
            raise RuntimeError('No space group symbol in CIF.')

        possibleSpaceGroupSymbols = SpaceGroupFactory.subscribedSpaceGroupSymbols(spaceGroupNumber[0])

        if len(possibleSpaceGroupSymbols) != 1:
            raise RuntimeError(
                'Can not use space group number to determine space group for no. {0}'.format(spaceGroupNumber))

        return SpaceGroupFactory.createSpaceGroup(possibleSpaceGroupSymbols[0]).getHMSymbol()


class UnitCellBuilder(object):
    def __init__(self, cifData=None):
        if cifData is not None:
            self.unitCell = self._getUnitCell(cifData)

    def _getUnitCell(self, cifData):
        unitCellComponents = [u'_cell_length_a', u'_cell_length_b', u'_cell_length_c',
                              u'_cell_angle_alpha', u'_cell_angle_beta', u'_cell_angle_gamma']

        unitCellValueMap = dict([(str(x), removeErrorEstimateFromNumber(str(cifData[x]))) if x in cifData.keys()
                                 else (str(x), None) for x in
                                 unitCellComponents])

        if unitCellValueMap['_cell_length_a'] is None:
            raise RuntimeError('The a-parameter of the unit cell is not specified in the supplied CIF.\n' \
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
    def __init__(self, cifData=None, unitCell=None):
        if cifData is not None:
            self.atomList = self._getAtoms(cifData, unitCell)

    def _getAtoms(self, cifData, unitCell=None):
        labelKey = u'_atom_site_label'

        if labelKey not in cifData.keys():
            raise RuntimeError('The field \'_atom_site_label\' is not present in the supplied CIF-file. Aborting.')

        labels = cifData[labelKey]

        atomCoordinates = self._getAtomCoordinates(cifData, labels)
        occupancies = self._getOccupancies(cifData, labels)
        atomSymbols = self._getAtomSymbols(cifData, labels)
        isotropicUs = self._getIsotropicUs(cifData, labels)

        atomLines = []
        for atomLabel in labels:
            stringAtomLine = [str(x) for x in (
                atomSymbols[atomLabel], atomCoordinates[atomLabel], occupancies[atomLabel], isotropicUs[atomLabel])]

            cleanLine = [stringAtomLine[0]] + [removeErrorEstimateFromNumber(x) for x in
                                               list(stringAtomLine[1:])]
            atomLines.append(' '.join(cleanLine))

        return ';'.join(atomLines)

    def _getAtomCoordinates(self, cifData, labels):
        coordinateFields = [u'_atom_site_fract_x', u'_atom_site_fract_y', u'_atom_site_fract_z']

        for field in coordinateFields:
            if field not in cifData.keys():
                raise RuntimeError(
                    'Mandatory field {0} not found in CIF-file.' \
                    'Please check the atomic position definitions.'.format(field))

        return dict(
            [(label, ' '.join([removeErrorEstimateFromNumber(c) for c in (x, y, z)])) for label, x, y, z in
             zip(labels, *[cifData[field] for field in coordinateFields])])

    def _getAtomSymbols(self, cifData, labels):
        rawAtomSymbols = [cifData[x] for x in [u'_atom_site_type_symbol', u'_atom_site_label'] if x in
                          cifData.keys()]

        if len(rawAtomSymbols) == 0:
            raise RuntimeError('Cannot determine atom types, both _atom_site_type_symbol and _atom_site_label are '
                               'missing.')

        return dict(
            [(label, self._getCleanAtomSymbol(x)) for label, x in zip(labels, rawAtomSymbols[0])])

    def _getCleanAtomSymbol(self, atomSymbol):
        nonCharacterRe = re.compile('[^a-z]', re.IGNORECASE)

        return re.sub(nonCharacterRe, '', atomSymbol)

    def _getOccupancies(self, cifData, labels):
        occupancyField = u'_atom_site_occupancy'

        occupancies = []
        if occupancyField in cifData.keys():
            occupancies += cifData[occupancyField]
        else:
            occupancies += ['1.0'] * len(labels)

        return dict(zip(labels, occupancies))

    def _getIsotropicUs(self, cifData, labels):

        keyUIso = u'_atom_site_u_iso_or_equiv'
        keyBIso = u'_atom_site_b_iso_or_equiv'

        isotropicUs = []
        if keyUIso in cifData.keys():
            isotropicUs += [getFloatOrNone(u) for u in cifData[keyUIso]]
        elif keyBIso in cifData.keys():
            isotropicUs += [convertBtoU(getFloatOrNone(b)) if getFloatOrNone(b) else None for b in
                            cifData[keyBIso]]
        else:
            isotropicUs += [None] * len(labels)

        return dict(zip(labels, isotropicUs))

    def _getEquivalentUs(self, cifData, unitCell):
        uMatrices = self._getUMatrices(cifData)
        sumWeights = self._getSumWeights(unitCell)

        return [np.sum(np.multiply(uMatrix, sumWeights)) / 3. if uMatrix is not None else None for uMatrix in uMatrices]

    def _getUMatrices(self):
        return []

    def _getSumWeights(self, unitCell):
        metricTensor = unitCell.getG()
        reciprocalMatrix = self._getReciprocalLengthSquaredMatrix(unitCell)

        return np.multiply(metricTensor, reciprocalMatrix)

    def _getReciprocalLengthSquaredMatrix(self, unitCell):
        reciprocalLengthVector = np.array([[unitCell.astar(), unitCell.bstar(), unitCell.cstar()]])

        return np.dot(reciprocalLengthVector.transpose(), reciprocalLengthVector)


class CrystalStructureBuilder(object):
    '''
    Helper class that builds a CrystalStructure file from the result
    of the ReadCif-function provided by PyCifRW.

    For testing purposes, dictionaries with the appropriate data can
    be passed in as well, so the source of the parsed data is replaceable.
    '''

    def __init__(self, cifFile=None):
        if cifFile is not None:
            cifData = cifFile[cifFile.keys()[0]]

            self.spaceGroup = SpaceGroupBuilder(cifData).spaceGroup
            self.unitCell = UnitCellBuilder(cifData).unitCell

            self.atoms = self._getAtoms(cifData, UnitCell(*[' '.split(self.unitCell)]))

    def getCrystalStructure(self):
        return CrystalStructure(self.unitCell, self.spaceGroup, self.atoms)


class UBMatrixBuilder(object):
    def __init__(self, cifFile=None):
        if cifFile is not None:
            cifData = cifFile[cifFile.keys()[0]]

            self._ubMatrix = self._getUBMatrix(cifData)

    def getUBMatrix(self):
        return self._ubMatrix

    def _getUBMatrix(self, cifData):
        ubMatrixKeys = [u'_diffrn_orient_matrix_ub_11', u'_diffrn_orient_matrix_ub_12', u'_diffrn_orient_matrix_ub_13',
                        u'_diffrn_orient_matrix_ub_21', u'_diffrn_orient_matrix_ub_22', u'_diffrn_orient_matrix_ub_23',
                        u'_diffrn_orient_matrix_ub_31', u'_diffrn_orient_matrix_ub_32', u'_diffrn_orient_matrix_ub_33']

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
            raise RuntimeError('This algorithm requires an additional Python package: PyCifRW' \
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
        from urllib import pathname2url

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

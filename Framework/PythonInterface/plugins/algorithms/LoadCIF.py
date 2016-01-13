# pylint: disable=no-init,invalid-name,too-few-public-methods,unused-import
from mantid.kernel import *
from mantid.simpleapi import *
from mantid.api import *
from mantid.geometry import SpaceGroupFactory

import re


class CrystalStructureBuilder(object):
    def __init__(self, cifFile):
        self._cifData = cifFile[cifFile.keys()[0]]

        self._spaceGroup = self._getSpaceGroup()
        self._unitCell = self._getUnitCell()

    def _getSpaceGroup(self):
        try:
            return self._getSpaceGroupFromString()
        except RuntimeError as error:
            print error.message
            try:
                return self._getSpaceGroupFromNumber()
            except RuntimeError as e:
                print e.message
                raise RuntimeError(
                        'Can create space group from supplied CIF-file. You could try to modify the HM-symbol' \
                        'to contain spaces between the components.\n' \
                        'Keys to look for: _space_group_name_H-M_alt, _symmetry_space_group_name_H-M')

    def _getSpaceGroupFromString(self):
        # Try two possibilities for space group symbol. If neither is present, throw a RuntimeError.
        rawSpaceGroupSymbol = [self._cifData[x] for x in
                               ['_space_group_name_H-M_alt', '_symmetry_space_group_name_H-M'] if
                               x in self._cifData.keys()]

        if len(rawSpaceGroupSymbol) == 0:
            raise RuntimeError('No space group symbol in CIF.')

        cleanSpaceGroupSymbol = self._getCleanSpaceGroupSymbol(rawSpaceGroupSymbol)

        # If the symbol is not registered, throw as well.
        return SpaceGroupFactory.createSpaceGroup(cleanSpaceGroupSymbol).getHMSymbol()

    def _getCleanSpaceGroupSymbol(self, rawSpaceGroupSymbol):
        # Remove :1 and :H from the symbol. Those are not required at the moment because they are the default.
        removalRe = re.compile(':[1H]', re.IGNORECASE)
        return re.sub(removalRe, '', rawSpaceGroupSymbol)

    def _getSpaceGroupFromNumber(self):
        spaceGroupNumber = int([self._cifData[x] for x in
                                ['_space_group_IT_number', '_symmetry_Int_Tables_number'] if
                                x in self._cifData.keys()][0])

        possibleSpaceGroupSymbols = SpaceGroupFactory.subscribedSpaceGroupSymbols(spaceGroupNumber)

        if len(possibleSpaceGroupSymbols) != 1:
            raise RuntimeError(
                    'Can not use space group number to determine space group for no. {0}'.format(spaceGroupNumber))

        return SpaceGroupFactory.createSpaceGroup(possibleSpaceGroupSymbols[0]).getHMSymbol()

    def _getUnitCell(self):
        unitCellComponents = ['_cell_length_a', '_cell_length_b', '_cell_length_c',
                              '_cell_angla_alpha', '_cell_angle_beta', '_cell_angle_gamma']

        replacementMap = {'_cell_length_b': '_cell_length_a',
                          '_cell_length_c': '_cell_length_a',
                          '_cell_angle_alpha': '90.0', '_cell_angle_beta': '90.0', '_cell_angle_gamma': '90.0'}

        unitCellValueMap = dict(
                [(x, self._cifData[x]) if x in self._cifData.keys() else None for x in unitCellComponents])

        if unitCellValueMap['_cell_length_a'] is None:
            raise RuntimeError('The a-parameter of the unit cell is not specified in the supplied CIF.\n' \
                               'Key to look for: _cell_length_a')

        unitCellValues = [value if value is not None else replacementMap[key] for key, value in
                          unitCellValueMap.iteritems()]

        return unitCellValues.join(' ')

    def _getAtoms(self):
        atomFieldsRequirements = [('_atom_site_label', True),
                                  ('_atom_site_fract_x', True),
                                  ('_atom_site_fract_y', True),
                                  ('_atom_site_fract_z', True),
                                  ('_atom_site_occupancy', False),
                                  ('_atom_site_U_iso_or_equiv', False)]

        atomFields = []

        for field, required in atomFieldsRequirements:
            if field in self._cifData:
                atomFields.append(field)
            else:
                if required:
                    raise RuntimeError(
                            'Mandatory field {0} not found in CIF-file.' \
                            'Please check the atomic position definitions.'.format(field))

        atomLists = [self._cifData[x] for x in atomFields]

        atomLines = []
        for atomLine in zip(atomLists):
            cleanLine = [self._getCleanAtomSymbol(atomLine[0])] + list(atomLine[1:])
            atomLines.append(atomLine.join(' '))

    def _getCleanAtomSymbol(self, atomSymbol):
        nonCharacterRe = re.compile('[^a-z]', re.IGNORECASE)

        return re.sub(nonCharacterRe, '', atomSymbol)


class LoadCIF(PythonAlgorithm):
    def category(self):
        return "Diffraction\\DataHandling"

    def name(self):
        return "LoadCIF"

    def summary(self):
        return "This algorithm loads a CIF file using the PyCifRW package and assigns a CrystalStructure to the sample of the workspace."

    def PyInit(self):
        self.declareProperty(
                FileProperty(name='InputFile',
                             defaultValue='',
                             action=FileAction.Load,
                             extensions=['cif']),
                doc='A CIF file containing a crystal structure.')

        self.declareProperty('LoadUBMatrix', False,
                             direction=Direction.Input,
                             doc='Load UB-matrix from CIF file if available.')

        self.declareProperty(
                WorkspaceProperty(name='Workspace',
                                  defaultValue='', direction=Direction.InOut),
                doc='Workspace into which the crystal structure is placed.')

    def PyExec(self):
        cifFileName = self.getProperty('InputFile').value

        crystalStructure = self._getCrystalStructureFromFile(cifFileName)

    def _getCrystalStructureFromFile(self, filename):
        parsedCifFile = ReadCif(filename)
        self._getCrystalStructureFromCifFile(parsedCifFile)

    def _getCrystalStructureFromCifFile(self, cifFile):
        a = CrystalStructureBuilder(cifFile)

        print a._getCleanAtomSymbol('O1')
        print a._getCleanAtomSymbol('O1+')
        print a._getCleanAtomSymbol('Os4-')
        print a._getCleanAtomSymbol('Ni-1')
        print a._getCleanAtomSymbol('Ni(Bla)')


try:
    from CifFile import ReadCif

    AlgorithmFactory.subscribe(LoadCIF)
except ImportError:
    logger.debug('Failed to subscribe algorithm LoadCIF; Python package PyCifRW' \
                 'may be missing (https://pypi.python.org/pypi/PyCifRW/4.1)')

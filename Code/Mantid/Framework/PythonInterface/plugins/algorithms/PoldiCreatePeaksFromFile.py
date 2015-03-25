# pylint: disable=no-init,invalid-name,too-few-public-methods
from mantid.kernel import *
from mantid.simpleapi import *
from mantid.api import *
from mantid.geometry import *

from pyparsing import *

import os


class PoldiCompound(object):
    """Small helper class to handle the results from PoldiCrystalFileParser."""
    _name = ""
    _spacegroup = ""
    _atomString = ""
    _cellDict = ""

    def __init__(self, name, elements):
        self._name = name

        self.assign(elements)

    def assign(self, elements):
        for c in elements:
            if c[0] == "atoms":
                self._atomString = ';'.join(c[1:])
            elif c[0] == "lattice":
                cellNames = ['a', 'b', 'c', 'alpha', 'beta', 'gamma']
                self._cellDict = dict(zip(cellNames, c[1:]))
            elif c[0] == "spacegroup":
                self._spacegroup = c[1]

    def getAtomString(self):
        return self._atomString

    def getCellParameters(self):
        return self._cellDict

    def getSpaceGroup(self):
        return self._spacegroup

    def getName(self):
        return self._name


def raiseParseErrorException(message):
    raise ParseException(message)


class PoldiCrystalFileParser(object):
    """Small parser for crystal structure files used at POLDI

    This class encapsulates a small parser for crystal structure files that are used at
    POLDI. The files contains information about the lattice, the space group and the basis (atoms
    in the asymmetric unit).

    The file format is defined as follows:

        Compound_1 {
            Lattice: [1 - 6 floats] => a, b, c, alpha, beta, gamma
            Spacegroup: [valid space group symbol]
            Atoms; {
                Element x y z [occupancy [U_eq]]
                Element x y z [occupancy [U_eq]]
            }
        }

        Compound_2 {
            ...
        }

    The parser returns a list of PoldiCompound objects with the compounds that were found
    in the file. These are then processed by PoldiCreatePeaksFromFile to generate arguments
    for calling PoldiCreatePeaksFromCell.
    """
    elementSymbol = Word(alphas, min=1, max=2).setFailAction(
        lambda o, s, loc, token: raiseParseErrorException("Element symbol must be one or two characters."))
    integerNumber = Word(nums)
    decimalSeparator = Literal('.')
    floatNumber = Combine(
        integerNumber +
        Optional(decimalSeparator + Optional(integerNumber))
    )

    whiteSpace = Suppress(White())

    atomLine = Combine(
        elementSymbol + whiteSpace +
        delimitedList(floatNumber, delim=White()),
        joinString=' '
    )

    keyValueSeparator = Suppress(Literal(":"))

    groupOpener = Suppress(Literal('{'))
    groupCloser = Suppress(Literal('}'))

    atomsGroup = Group(CaselessLiteral("atoms") + keyValueSeparator +
                       groupOpener + delimitedList(atomLine, delim=lineEnd) + groupCloser)

    unitCell = Group(CaselessLiteral("lattice") + keyValueSeparator + delimitedList(
        floatNumber, delim=White()))

    spaceGroup = Group(CaselessLiteral("spacegroup") + keyValueSeparator + Word(
        alphanums + "-" + ' '))

    compoundContent = Each([atomsGroup, unitCell, spaceGroup]).setFailAction(
        lambda o, s, loc, token: raiseParseErrorException(
            "One of 'Lattice', 'SpaceGroup', 'Atoms' is missing or contains errors."))

    compoundName = Word(alphanums + '_')

    compound = Group(compoundName + Optional(whiteSpace) + \
                     groupOpener + compoundContent + groupCloser)

    comment = Suppress(Literal('#') + restOfLine)

    compounds = OneOrMore(compound).ignore(comment) + stringEnd

    def __call__(self, contentString):
        parsedContent = None

        if os.path.isfile(contentString):
            parsedContent = self._parseFile(contentString)
        else:
            parsedContent = self._parseString(contentString)

        return [PoldiCompound(x[0], x[1:]) for x in parsedContent]

    def _parseFile(self, filename):
        return self.compounds.parseFile(filename)

    def _parseString(self, stringContent):
        return self.compounds.parseString(stringContent)


class PoldiCreatePeaksFromFile(PythonAlgorithm):
    _parser = PoldiCrystalFileParser()

    def category(self):
        return "SINQ\\POLDI"

    def name(self):
        return "PoldiLoadCrystalData"

    def summary(self):
        return ("The algorithm reads a POLDI crystal structure file and creates a WorkspaceGroup that contains tables"
                "with the expected reflections.")

    def PyInit(self):
        self.declareProperty(
            FileProperty(name="InputFile",
                         defaultValue="",
                         action=FileAction.Load,
                         extensions=["dat"]),
            doc="A file with POLDI crystal data.")

        self.declareProperty("LatticeSpacingMin", 0.5,
                             direction=Direction.Input,
                             doc="Lowest allowed lattice spacing.")

        self.declareProperty("LatticeSpacingMax", 0.0,
                             direction=Direction.Input,
                             doc="Lowest allowed lattice spacing.")

        self.declareProperty(
            WorkspaceProperty(name="OutputWorkspace",
                              defaultValue="", direction=Direction.Output),
            doc="WorkspaceGroup with reflection tables.")


    def PyExec(self):
        crystalFileName = self.getProperty("InputFile").value
        try:
            # Try parsing the supplied file using PoldiCrystalFileParser
            compounds = self._parser(crystalFileName)

            dMin = self.getProperty("LatticeSpacingMin").value
            dMax = self.getProperty("LatticeSpacingMax").value

            workspaces = []

            # Go through found compounds and run "_createPeaksFromCell" for each of them
            # If two compounds have the same name, a warning is written to the log.
            for compound in compounds:
                if compound.getName() in workspaces:
                    self.log().warning("A compound with the name '" + compound.getName() + \
                                       "' has already been created. Please check the file '" + crystalFileName + "'")
                else:
                    workspaces.append(self._createPeaksFromCell(compound, dMin, dMax))

            self.setProperty("OutputWorkspace", GroupWorkspaces(workspaces))

        # All parse errors are caught here and logged as errors
        except ParseException as error:
            errorString = "Could not parse input file '" + crystalFileName + "'.\n"
            errorString += "The parser reported the following error:\n\t" + str(error)

            self.log().error(errorString)


    def _createPeaksFromCell(self, compound, dMin, dMax):
        if not SpaceGroupFactory.isSubscribedSymbol(compound.getSpaceGroup()):
            raise RuntimeError("SpaceGroup '" + compound.getSpaceGroup() + "' is not registered.")

        PoldiCreatePeaksFromCell(SpaceGroup=compound.getSpaceGroup(),
                                 Atoms=compound.getAtomString(),
                                 LatticeSpacingMin=dMin,
                                 LatticeSpacingMax=dMax,
                                 OutputWorkspace=compound.getName(),
                                 **compound.getCellParameters())

        return compound.getName()


AlgorithmFactory.subscribe(PoldiCreatePeaksFromFile)

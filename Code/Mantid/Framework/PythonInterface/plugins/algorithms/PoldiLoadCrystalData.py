# pylint: disable=no-init,invalid-name,bare-except
from mantid.kernel import *
from mantid.simpleapi import *
from mantid.api import *

from pyparsing import *

import os


class PoldiCompound(object):
    def __init__(self, name, content, tolerance, elements):
        self._name = name
        self._content = content
        self._tolerance = tolerance
        self._spacegroup = ""
        self._atomString = ""
        self._latticeDict = ""

        self.assign(elements)

    def assign(self, elements):
        for c in elements:
            if c[0] == "atoms":
                self._atomString = c[1][0]
            elif c[0] == "lattice":
                pNames = ['a', 'b', 'c', 'alpha', 'beta', 'gamma']
                self._latticeDict = dict(zip(pNames, c[1:]))
            elif c[0] == "spacegroup":
                self._spacegroup = c[1]

    def getAtoms(self):
        return self._atomString

    def getLatticeDict(self):
        return self._latticeDict

    def getSpaceGroup(self):
        return self._spacegroup

    def getContent(self):
        return self._content

    def getTolerance(self):
        return self._tolerance

    def getName(self):
        return self._name


class PoldiCrystalFileParser(object):
    elementSymbol = Word(alphas, exact=2)
    integerNumber = Word(nums)
    decimalSeparator = Literal('.')
    floatNumber = Combine(
        integerNumber +
        Optional(decimalSeparator + Optional(integerNumber))
    )

    atomLine = Combine(
        elementSymbol + Suppress(White()) +
        delimitedList(floatNumber, delim=White()),
        joinString=' '
    )

    keyValueSeparator = Suppress(Literal(":"))

    atomsGroup = Group(CaselessLiteral("atoms") + keyValueSeparator + nestedExpr(
        opener="{", closer="}",
        content=Combine(
            delimitedList(atomLine, delim='\n'),
            joinString=";")))

    unitCell = Group(CaselessLiteral("lattice") + keyValueSeparator + delimitedList(
        floatNumber, delim=White()))

    spaceGroup = Group(CaselessLiteral("spacegroup") + keyValueSeparator + Word(
        alphanums + "-" + ' '))

    compoundContent = Each([atomsGroup, unitCell, spaceGroup])

    compoundName = Word(alphanums)

    compound = Group(Suppress(CaselessLiteral("compound")) + Suppress(White()) + \
                     compoundName + Suppress(White()) + floatNumber + \
                     Suppress(White()) + floatNumber + \
                     nestedExpr(opener='{', closer='}', content=compoundContent))

    comment = Suppress(Literal('#') + restOfLine)

    compounds = OneOrMore(compound).ignore(comment)

    def __call__(self, contentString):
        parsedContent = None

        if os.path.isfile(contentString):
            parsedContent = self._parseFile(contentString)
        else:
            parsedContent = self._parseString(contentString)

        return [PoldiCompound(*x[:3]) for x in parsedContent]

    def _parseFile(self, filename):
        return self.compounds.parseFile(filename)

    def _parseString(self, stringContent):
        return self.compounds.parseString(stringContent)


class PoldiLoadCrystalData(PythonAlgorithm):
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

        self.declareProperty("LatticeSpacingMin", 0.0,
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
        pass

AlgorithmFactory.subscribe(PoldiLoadCrystalData)
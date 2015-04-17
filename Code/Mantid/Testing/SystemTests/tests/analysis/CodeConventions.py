#pylint: disable=no-init
import stresstesting
import mantid
from mantid.simpleapi import *
import re

MAX_ALG_LEN = 40 # TODO convention says 20 is the maximum

SPECIAL = ["InputWorkspace", "OutputWorkspace", "Workspace",
           "ReductionProperties"]
SPECIAL_UPPER = [specialname.upper for specialname in SPECIAL]

# TODO this list should be empty
ALG_BAD_PARAMS = {
    "CalculateUMatrix(v1)":("a", "b", "c", "alpha", "beta", "gamma"),
    "ConvertToMD(v1)":("dEAnalysisMode"),
    "ConvertToMDMinMaxLocal(v1)":("dEAnalysisMode"),
    "ConvertToMDMinMaxGlobal(v1)":("dEAnalysisMode"),
    "FindUBUsingLatticeParameters(v1)":("a", "b", "c", "alpha", "beta", "gamma"),
    "IndexSXPeaks(v1)":("a", "b", "c", "alpha", "beta", "gamma", "dTolerance"),
    "ModeratorTzero(v1)":("tolTOF"),
    "MuscatFunc(v1)":("dQ", "dW"),
    "OptimizeCrystalPlacement(v1)":("nPeaks", "nParams", "nIndexed"),
    "PDFFourierTransform(v1)":("rho0"),
    "PoldiAutoCorrelation(v5)":("wlenmin", "wlenmax"),
    "PoldiLoadChopperSlits(v1)":("nbLoadedSlits"),
    "PoldiLoadSpectra(v1)":("nbSpectraLoaded"),
    "PoldiProjectRun(v1)":("wlenmin", "wlenmax"),
    "PoldiRemoveDeadWires(v1)":("nbExcludedWires", "nbAuteDeadWires"),
    "SaveIsawQvector(v1)":("Qx_vector", "Qy_vector", "Qz_vector"),
    "SCDCalibratePanels(v1)":("a", "b", "c", "alpha", "beta", "gamma",
                              "useL0", "usetimeOffset", "usePanelWidth",
                              "usePanelHeight", "usePanelPosition",
                              "usePanelOrientation", "tolerance",
                              "MaxPositionChange_meters"),
    "SetSampleMaterial(v1)":("bAverage", "bSquaredAverage"),
    "SetUB(v1)":("a", "b", "c", "alpha", "beta", "gamma", "u", "v"),
    "ViewBOA(v1)":("CD-Distance"),
    "PoldiCreatePeaksFromCell(v1)":("a", "b", "c", "alpha", "beta", "gamma"),
    "CreateMD(v1)" : ("u", "v")
    }

# TODO this list should be empty
FUNC_BAD_NAME = ("Muon_ExpDecayOscTest")

# TODO this list should be empty
FUNC_BAD_PARAMS = {
    "Bk2BkExpConvPV":("TOF_h"),
    "CubicSpline":("y0", "y1", "y2"),
    "DiffRotDiscreteCircle":("f0.Height", "f0.Radius"),
    "DiffSphere":("f0.Height", "f0.Radius"),
    "LatticeErrors":("p0", "p1", "p2", "p3", "p4", "p5"),
    "Muon_ExpDecayOscTest":("lambda", "frequency", "phi"),
    "SCDPanelErrors":("f0_detWidthScale", "f0_detHeightScale",
                      "f0_Xoffset", "f0_Yoffset", "f0_Zoffset",
                      "f0_Xrot", "f0_Yrot", "f0_Zrot",
                      "l0", "t0"),
    "StretchedExpFT":("height", "tau", "beta"),
    "PawleyParameterFunction":("a","b","c"),
    "PawleyFunction":("f0.a","f0.b","f0.c", "f0.Alpha", "f0.Beta", "f0.Gamma", "f0.ZeroShift"),
    }

class Algorithms(stresstesting.MantidStressTest):

    def __init__(self):
        super(Algorithms, self).__init__()
        self.__ranOk = 0
        self.algRegExp = re.compile(r'^[A-Z][a-zA-Z0-9]+$')
        self.paramRegExp = re.compile(r'^[A-Z][a-zA-Z0-9]*$')
        self.categoryRegExp = re.compile(r'^([A-Z][a-zA-Z]+\\?)+$')

    def verifyAlgName(self, name):
        if not self.algRegExp.match(name):
            print "Algorithm " + name + " has a name that violates conventions"
            return False

        if bool(len(name) > MAX_ALG_LEN):
            print "%s has a name that is longer than " % name, \
                "%d characters (%d > %d)" % (MAX_ALG_LEN, len(name), MAX_ALG_LEN)
            return False

        # passed all of the checks
        return True

    def verifyCategories(self, name, categories):
        if len(categories) <= 0:
            print name + " has no categories"

        for category in categories:
            if not self.categoryRegExp.match(category):
                print name + " has a bad category " + category
                return False

        return True

    def checkAllowed(self, alg_descr, name):
        if alg_descr not in ALG_BAD_PARAMS.keys():
            return False

        return name in  ALG_BAD_PARAMS[alg_descr]

    def verifyProperty(self, alg_descr, name):
        upper = name.upper()
        if (upper in SPECIAL_UPPER) and (not name in SPECIAL):
            index = SPECIAL_UPPER.index(upper)
            print alg_descr + " property (" + name + ") has special name "\
                + "with wrong case: " + name + " should be " + SPECIAL[index]
            return False

        if not self.paramRegExp.match(name):
            if not self.checkAllowed(alg_descr, name):
                print alg_descr + " property (" + name +") violates conventions"
                return False

        # passed all of the checks
        return True

    def runTest(self):
        algs = AlgorithmFactory.getRegisteredAlgorithms(True)

        for (name, versions) in algs.iteritems():
            if not self.verifyAlgName(name):
                self.__ranOk += 1
                continue
            for version in versions:
                # get an instance
                alg = mantid.AlgorithmManager.create(name, version)
                alg_descr = "%s(v%d)" % (name, version)

                # verify the categories
                if not self.verifyCategories(alg_descr, alg.categories()):
                    self.__ranOk += 1

                # verify the properties
                props = alg.getProperties()
                for prop in props:
                    if not self.verifyProperty(alg_descr, prop.name):
                        self.__ranOk += 1


    def validate(self):
        if self.__ranOk > 0:
            print "Found %d errors. Coding conventions found at" % self.__ranOk,\
                "http://www.mantidproject.org/Mantid_Standards"
            return False

        return True

class FitFunctions(stresstesting.MantidStressTest):
    def __init__(self):
        super(FitFunctions, self).__init__()
        self.__ranOk = 0
        self.funcRegExp = re.compile(r'^[A-Z][a-zA-Z0-9]+$')
        self.paramRegExp = re.compile(r'^[A-Z][a-zA-Z0-9]*$')
        self.categoryRegExp = re.compile(r'^([A-Z][a-zA-Z]+\\?)+$')

    def verifyFuncName(self, name):
        if name in FUNC_BAD_NAME:
            return True

        if not self.funcRegExp.match(name):
            print "Function " + name + " has a name that violates conventions"
            return False

        if bool(len(name) > MAX_ALG_LEN):
            print "%s has a name that is longer than " % name, \
                "%d characters (%d > %d)" % (MAX_ALG_LEN, len(name), MAX_ALG_LEN)
            return False

        # passed all of the checks
        return True

    def verifyCategories(self, name, categories):
        if len(categories) <= 0:
            print name + " has no categories"

        for category in categories:
            # TODO remove the special case
            if category == "C++ User Defined":
                return True

            if not self.categoryRegExp.match(category):
                print name + " has a bad category " + category
                return False

        return True

    def checkAllowed(self, func, name):
        if func not in FUNC_BAD_PARAMS.keys():
            return False

        return name in  FUNC_BAD_PARAMS[func]

    def verifyParameter(self, alg_descr, name):

        if not self.paramRegExp.match(name):
            if not self.checkAllowed(alg_descr, name):
                print alg_descr + " property (" + name +") violates conventions"
                return False

        # passed all of the checks
        return True

    def runTest(self):
        functions = mantid.api.FunctionFactory.getFunctionNames()
        for name in functions:
            if not self.verifyFuncName(name):
                self.__ranOk += 1
                continue

            function = mantid.api.FunctionFactory.createFunction(name)

            if not self.verifyCategories(name, function.categories()):
                self.__ranOk += 1

            for i in xrange(function.numParams()):
                if not self.verifyParameter(name, function.getParamName(i)):
                    self.__ranOk += 1

    def validate(self):
        if self.__ranOk > 0:
            print "Found %d errors. Coding conventions found at" % self.__ranOk,\
                "http://www.mantidproject.org/Mantid_Standards"
            return False

        return True

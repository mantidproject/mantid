# import mantid algorithms, numpy and matplotlib
import numpy as np
from mantid.api import PythonAlgorithm, AlgorithmFactory
import math as mt
from mantid.kernel import FloatArrayProperty, FloatArrayLengthValidator, Direction, FloatBoundedValidator
from mantid.api import IPeaksWorkspaceProperty
from mantid.geometry import UnitCell


# Scattering + 1 peak algorithm
class FindUBFromScatteringPlane(PythonAlgorithm):
    @staticmethod
    def category():
        return "Crystal\\UBMatrix"

    def PyInit(self):
        ArrayLength3 = FloatArrayLengthValidator(3)
        LowerLatticeParameterLimit = FloatBoundedValidator(lower=0.0)
        AngleLimit = FloatBoundedValidator(lower=0.0, upper=180)
        # declaring intake variables to define the scattering plane,
        self.declareProperty(FloatArrayProperty(name="Vector1", direction=Direction.Input, validator=ArrayLength3))
        self.declareProperty(FloatArrayProperty(name="Vector2", direction=Direction.Input, validator=ArrayLength3))
        # might need it to raise an error here

        # declaring intake variables abc, alpha, beta, gamma, and the peak
        self.declareProperty(
            IPeaksWorkspaceProperty(name="PeakWorkSpace", defaultvalue="", direction=Direction.InOut)
        )  # mantid does not like

        self.declareProperty(name="a", direction=Direction.Input, validator=LowerLatticeParameterLimit)
        self.declareProperty(name="b", direction=Direction.Input, validator=LowerLatticeParameterLimit)
        self.declareProperty(name="c", direction=Direction.Input, validator=LowerLatticeParameterLimit)

        self.declareProperty(name="alpha", direction=Direction.Input, validator=AngleLimit)
        self.declareProperty(name="beta", direction=Direction.Input, validator=AngleLimit)
        self.declareProperty(name="gamma", direction=Direction.Input, validator=AngleLimit)

        # self.declareProperty(name="UB", direction=Direction.Output) not sure this is needed as we are tying to workspace

    def exec_child_alg(self, alg_name: str, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props

    # Can change these to be 2 arrays of abc, and alpha,beta,gamma if that is preferred
    def PyExec(self):
        # getting lattice parameters
        a = self.getProperty("a").value
        b = self.getProperty("b").value
        c = self.getProperty("c").value

        alpha = self.getProperty("alpha").value
        beta = self.getProperty("beta").value
        gamma = self.getProperty("gamma").value

        # getting vectors to define the plane
        v1 = np.array(self.getProperty("Vector1").value)
        v1 = v1.reshape(-1, 1)  # making column vector
        v2 = np.array(self.getProperty("Vector2").value)
        v2 = v2.reshape(-1, 1)  # making column vector

        pi = mt.pi

        # defining sample cell inorder to find B to orthogonalise the vectors provided
        sample_cell = UnitCell(a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma)
        B = sample_cell.getB()

        B_v1 = np.matmul(B, v1)
        B_v2 = np.matmul(B, v2)

        SingleCrystalPeakTable = self.getProperty("PeakWorkSpace").value
        peak_ip = SingleCrystalPeakTable.getPeak(0)
        hklpeak = np.array(peak_ip.getHKL())
        hklpeak = hklpeak.reshape(-1, 1)
        Ql = np.array(peak_ip.getQLabFrame())
        Ql = Ql.reshape(-1, 1)

        Qs = np.matmul(B, hklpeak)

        # defining the normal to the scattering plane
        Sp_N = np.cross(B_v1.flatten(), B_v2.flatten())

        # Finding scalar product may need to add some tolerance as B is not exact
        SCP = round(np.dot(Sp_N, Qs.flatten()), 1)
        if SCP != 0:
            raise ValueError("given peak does not lie in the plane")

        Sp_NN = Sp_N / (np.linalg.norm(Sp_N))
        Ql_mag = float(np.linalg.norm(Ql))
        Qs_mag = float(np.linalg.norm(Qs))

        Theta = mt.acos(np.dot(Ql, Qs) / (Qs_mag * Ql_mag))

        ## we don't know the direction of this angle
        ## writing a matrix with both anticlockwise and clockwise rotation, applying it and checking whichever gives us the better result
        ## cheeky rotation about arbitrary axis matrix using Rodrigues' rotation formula, where Axis is a unit vector for axis of rotation
        # Axis=Sp_N/np.linalg.norm(Sp_N)
        Axis = Sp_NN
        Identity3 = np.identity(3)
        sintheta = mt.sin(Theta)
        negativesintheta = -1 * sintheta
        one_costheta = 1 - mt.cos(Theta)
        K = np.array([[0, -Axis[2], Axis[1]], [Axis[2], 0, -Axis[0]], [-Axis[1], Axis[0], 0]])

        RUclockwise = 2 * pi * (Identity3 + sintheta * K + one_costheta * (np.matmul(K, K)))
        RUanticlockwise = 2 * pi * (Identity3 + negativesintheta * K + one_costheta * np.matmul(K, K))

        ##time to apply the matrix to check
        Qlcalc = np.matmul(RUclockwise, Qs)
        Qlcalcanti = np.matmul(RUanticlockwise, Qs)

        # making some scalar products, whichever is larger will be the more correct RU (won't be exact due to rounding)
        SP_clockwise = np.dot(Ql, Qlcalc)
        SP_anticlockwise = np.dot(Ql, Qlcalcanti)

        if SP_clockwise > SP_anticlockwise:
            RUB = np.matmul(RUclockwise, B)
        elif SP_anticlockwise > SP_clockwise:
            RUB = np.matmul(RUanticlockwise, B)
        else:
            RUB = np.matmul(RUclockwise, B)
        self.exec_child_alg("SetUB", workspace=SingleCrystalPeakTable, UB=RUB)
        ## set ub on the workspace


AlgorithmFactory.subscribe(FindUBFromScatteringPlane)

# import mantid algorithms, numpy and matplotlib
import numpy as np
import math as mt
from scipy.spatial.transform import Rotation as R
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import FloatArrayProperty, FloatArrayLengthValidator, Direction, FloatBoundedValidator
from mantid.api import IPeaksWorkspaceProperty
from mantid.geometry import UnitCell


# Scattering + 1 peak algorithm
class FindUBFromScatteringPlane(PythonAlgorithm):
    def category(self):
        return "Crystal\\UBMatrix"

    def PyInit(self):
        array_length_3 = FloatArrayLengthValidator(3)
        lower_lattice_parameter_limit = FloatBoundedValidator(lower=0.0)
        angle_limit = FloatBoundedValidator(lower=0.0, upper=180)

        # declaring intake variables to define the scattering plane,
        self.declareProperty(FloatArrayProperty(name="vector_1", direction=Direction.Input, validator=array_length_3))
        self.declareProperty(FloatArrayProperty(name="vector_2", direction=Direction.Input, validator=array_length_3))
        # might need it to raise an error here

        # declaring intake variables abc, alpha, beta, gamma, and the peak
        self.declareProperty(IPeaksWorkspaceProperty(name="PeaksWorkspace", defaultValue="", direction=Direction.Input))

        self.declareProperty(name="a", direction=Direction.Input, validator=lower_lattice_parameter_limit, defaultValue=1.0)
        self.declareProperty(name="b", direction=Direction.Input, validator=lower_lattice_parameter_limit, defaultValue=1.0)
        self.declareProperty(name="c", direction=Direction.Input, validator=lower_lattice_parameter_limit, defaultValue=1.0)

        self.declareProperty(name="alpha", direction=Direction.Input, validator=angle_limit, defaultValue=90.0)
        self.declareProperty(name="beta", direction=Direction.Input, validator=angle_limit, defaultValue=90.0)
        self.declareProperty(name="gamma", direction=Direction.Input, validator=angle_limit, defaultValue=90.0)

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
        vector_1 = np.array(self.getProperty("vector_1").value)
        vector_1 = vector_1.reshape(-1, 1)  # making column vector
        vector_2 = np.array(self.getProperty("vector_2").value)
        vector_2 = vector_2.reshape(-1, 1)  # making column vector

        # defining sample cell inorder to find B to orthogonalise the vectors provided
        sample_cell = UnitCell(a, b, c, alpha, beta, gamma)
        B = sample_cell.getB()

        # applying B to the vectors to orthogonalise them
        B_vector_1 = B @ vector_1
        B_vector_2 = B @ vector_2

        SingleCrystalPeakTable = self.getProperty("PeaksWorkspace").value
        peak_ip = SingleCrystalPeakTable.getPeak(0)
        hkl_peak = np.array(peak_ip.getHKL())
        hkl_peak = hkl_peak.reshape(-1, 1)
        Ql = np.array(peak_ip.getQLabFrame())
        Ql = Ql.reshape(-1, 1)

        Qs = 2 * np.pi * (B @ hkl_peak)

        # defining the normal to the scattering plane
        scatt_plane_norm = np.array(np.cross(B_vector_1.flatten(), B_vector_2.flatten()))
        theta = mt.acos(np.dot(Ql.flatten(), Qs.flatten()) / (np.linalg.norm(Ql) * np.linalg.norm(Qs)))
        # if np.isclose(np.dot(Qs.flatten(), scatt_plane_norm), 0, atol=0.01) != True:
        #    raise ValueError("given peak does not lie in the plane")

        #        (
        #            U,
        #            rssd,
        #        ) = R.align_vectors(Ql.flatten(), Qs.flatten())
        #        U = U.as_matrix()

        rotation_object = R.from_rotvec(theta * scatt_plane_norm.flatten() / np.linalg.norm(scatt_plane_norm))
        U = rotation_object.as_matrix()

        UB_rel = U @ B
        self.exec_child_alg("SetUB", workspace=SingleCrystalPeakTable, UB=UB_rel)

        ## set ub on the workspace


AlgorithmFactory.subscribe(FindUBFromScatteringPlane)

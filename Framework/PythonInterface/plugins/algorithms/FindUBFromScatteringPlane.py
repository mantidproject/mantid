# import mantid algorithms, numpy and matplotlib
import numpy as np
from scipy.spatial.transform import Rotation as R
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import FloatArrayProperty, FloatArrayLengthValidator, Direction, FloatBoundedValidator
from mantid.api import IPeaksWorkspaceProperty
from mantid.kernel import V3D, logger
from scipy.optimize import minimize


# Scattering + 1 peak algorithm
class FindUBFromScatteringPlane(PythonAlgorithm):
    def category(self):
        return "Crystal\\UBMatrix"

    ## put in doc args
    def PyInit(self):
        array_length_3 = FloatArrayLengthValidator(3)
        lower_lattice_parameter_limit = FloatBoundedValidator(lower=0.0)
        angle_limit = FloatBoundedValidator(lower=0.0, upper=180)

        # declaring intake variables to define the scattering plane,
        self.declareProperty(
            FloatArrayProperty(name="vector_1", direction=Direction.Input, validator=array_length_3),
            doc="1st Vector defining scattering plane, provide as a list: [h,k,l]",
        )
        self.declareProperty(
            FloatArrayProperty(name="vector_2", direction=Direction.Input, validator=array_length_3),
            doc="2nd Vector defining scattering plane, provide as a list: [h,k,l] ",
        )
        # might need it to raise an error here

        # declaring intake variables abc, alpha, beta, gamma, and the peak
        self.declareProperty(
            IPeaksWorkspaceProperty(name="PeaksWorkspace", defaultValue="", direction=Direction.Input),
            doc="PeaksWorkspace with 1 peak in the scattering plane, if more than 1 peak is provided"
            "the first peak in the Workspace is chosen",
        )

        self.declareProperty(
            name="a", direction=Direction.Input, validator=lower_lattice_parameter_limit, defaultValue=1.0, doc="Lattice Parameter a"
        )
        self.declareProperty(
            name="b", direction=Direction.Input, validator=lower_lattice_parameter_limit, defaultValue=1.0, doc="Lattice Parameter b"
        )
        self.declareProperty(
            name="c", direction=Direction.Input, validator=lower_lattice_parameter_limit, defaultValue=1.0, doc="Lattice Parameter c"
        )

        self.declareProperty(
            name="alpha", direction=Direction.Input, validator=angle_limit, defaultValue=90.0, doc="Lattice Parameter alpha"
        )
        self.declareProperty(name="beta", direction=Direction.Input, validator=angle_limit, defaultValue=90.0, doc="Lattice Parameter beta")
        self.declareProperty(
            name="gamma", direction=Direction.Input, validator=angle_limit, defaultValue=90.0, doc="Lattice Parameter gamma"
        )

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
        vector_1 = V3D(vector_1[0], vector_1[1], vector_1[2])
        vector_2 = np.array(self.getProperty("vector_2").value)
        vector_2 = V3D(vector_2[0], vector_2[1], vector_2[2])

        # defining sample cell inorder to find B to orthogonalise the vectors provided

        SingleCrystalPeakTable = self.getProperty("PeaksWorkspace").value
        self.exec_child_alg(
            "SetUB", Workspace=SingleCrystalPeakTable, u=vector_1, v=vector_2, a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma
        )
        OrientedLattice = SingleCrystalPeakTable.sample().getOrientedLattice()
        UB_arb = OrientedLattice.getUB()

        vertical_dir = SingleCrystalPeakTable.getInstrument().getReferenceFrame().vecPointingUp()
        vertical_dir = V3D(vertical_dir[0], vertical_dir[1], vertical_dir[2])

        peak_ip = SingleCrystalPeakTable.getPeak(0)
        hkl_peak = np.array(peak_ip.getHKL())
        hkl_peak = hkl_peak.reshape(-1, 1)
        Qscalc = np.array(np.pi * 2 * (UB_arb @ hkl_peak))
        Qscalc = V3D(float(Qscalc[0]), float(Qscalc[1]), float(Qscalc[2]))
        Qsreal = np.array(peak_ip.getQSampleFrame())
        Qsreal = V3D(float(Qsreal[0]), float(Qsreal[1]), float(Qsreal[2]))

        if not np.isclose(
            np.dot(np.array(Qscalc) / np.linalg.norm(Qscalc), np.array(vertical_dir) / np.linalg.norm(vertical_dir)), 0, atol=0.03
        ):
            logger.warning("given peak does not lie in the plane")

        def rotation(theta, vertical_dir):
            axis = np.array(vertical_dir) / np.linalg.norm(vertical_dir)
            rotation_object = R.from_rotvec(theta * axis)
            return rotation_object

        def objective(angle, vertical_dir, target_vector):
            rotation_object = rotation(angle, vertical_dir)
            initial_vector = np.array(Qscalc)
            rotated_vector = rotation_object.apply(initial_vector)

            cost = (np.linalg.norm(np.array(target_vector) - rotated_vector)) ** 2
            return cost

        target_vector = np.array(Qsreal)
        result = minimize(objective, 0, args=(np.array(vertical_dir), target_vector), bounds=[(-np.pi, np.pi)], method="Nelder-Mead")
        theta_new = result.x[0]
        print(np.degrees(theta_new))

        rotation_object = R.from_rotvec(theta_new * np.array(vertical_dir) / np.linalg.norm(np.array(vertical_dir)))
        rotation_matrix = rotation_object.as_matrix()
        UB_final = rotation_matrix @ UB_arb

        self.exec_child_alg("SetUB", Workspace=SingleCrystalPeakTable, UB=UB_final)
        OrientedLattice = SingleCrystalPeakTable.sample().getOrientedLattice()
        print(UB_final @ hkl_peak * 2 * np.pi)


AlgorithmFactory.subscribe(FindUBFromScatteringPlane)

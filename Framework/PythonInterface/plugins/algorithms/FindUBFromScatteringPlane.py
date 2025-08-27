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

    def PyInit(self):
        array_length_3 = FloatArrayLengthValidator(3)
        lower_lattice_parameter_limit = FloatBoundedValidator(lower=0.0)
        angle_limit = FloatBoundedValidator(lower=0.0, upper=180)

        # declaring intake variables to define the scattering plane,
        self.declareProperty(
            FloatArrayProperty(name="Vector1", direction=Direction.Input, validator=array_length_3),
            doc="First vector in the scattering plane in reciprocal lattice units - i.e. H,K,L",
        )
        self.declareProperty(
            FloatArrayProperty(name="Vector2", direction=Direction.Input, validator=array_length_3),
            doc="Second vector in the scattering plane in reciprocal lattice units - i.e. H,K,L",
        )

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

    def validateInputs(self):
        issues = dict()
        vector_1 = V3D(*self.getProperty("Vector1").value)
        vector_2 = V3D(*self.getProperty("Vector2").value)
        if np.isclose(vector_1.norm(), 0, atol=np.finfo(np.float64).eps) or np.isclose(vector_2.norm(), 0, rtol=1e-05):
            issues["Unsuitable_vector"] = "Vector cannot be 0"
        if np.isclose(np.sin(vector_1.angle(vector_2)), 0):
            issues["Collinear"] = "Vectors cannot be collinear"
        return issues

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
        vector_1 = V3D(*self.getProperty("Vector1").value)
        vector_2 = V3D(*self.getProperty("Vector2").value)

        # Find a UB with the correct scattering plane (even if `u` and `v` passed to `SetUB` are not orthogonal)

        peak_table = self.getProperty("PeaksWorkspace").value
        self.exec_child_alg("SetUB", Workspace=peak_table, u=vector_1, v=vector_2, a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma)
        lattice = peak_table.sample().getOrientedLattice()
        ub_arb = lattice.getUB()

        vertical_dir = peak_table.getInstrument().getReferenceFrame().vecPointingUp()
        vertical_dir = vertical_dir * (1 / vertical_dir.norm())

        peak_ip = peak_table.getPeak(0)
        hkl_peak = np.array(peak_ip.getHKL())
        qsample_calc = np.array(np.pi * 2 * (ub_arb @ hkl_peak))
        qsample_calc = V3D(*qsample_calc)
        qsample_obs = np.array(peak_ip.getQSampleFrame())
        qsample_obs = V3D(*qsample_obs)

        if not 85 < (np.degrees(qsample_calc.angle(vertical_dir))) < 95:
            logger.debug(f"{np.degrees(qsample_calc.angle(vertical_dir))}")
            logger.warning("given peak hkl does not lie in the plane")

        def objective(theta):
            axis = np.array(vertical_dir)
            rotation_object = R.from_rotvec(theta * axis)
            rotated_vector = rotation_object.apply(np.array(qsample_calc))
            res = qsample_obs - rotated_vector
            cost = (np.linalg.norm(res)) ** 2
            return cost

        result = minimize(objective, 0, bounds=[(-np.pi, np.pi)], method="Nelder-Mead")
        theta_new = result.x[0]
        logger.debug(f"Angle of rotation applied to ub_arb {np.degrees(theta_new)}")

        rotation_object = R.from_rotvec(theta_new * vertical_dir)
        rotation_matrix = rotation_object.as_matrix()
        ub_final = rotation_matrix @ ub_arb

        self.exec_child_alg("SetUB", Workspace=peak_table, UB=ub_final)


AlgorithmFactory.subscribe(FindUBFromScatteringPlane)

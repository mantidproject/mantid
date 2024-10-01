# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from isis_reduction_steps import StripEndNans
from isis_instrument import LARMOR
from mantid.simpleapi import *
from mantid.kernel import Logger
import SANSUtility


class FindDirectionEnum(object):  # pylint: disable=R0903
    ALL = 0
    UP_DOWN = 1
    LEFT_RIGHT = 2

    def __init__(self):
        super(FindDirectionEnum, self).__init__()


def is_workspace_which_requires_angle(reducer):
    """
    Check if the sample worksapce requires the first
    coordinate to be an angle
    @param reducer: the reducer object
    @returns true if the workspace requires an angle otherwise false
    """
    instrument_name = reducer.instrument.name()
    if instrument_name != LARMOR._NAME:
        return False
    workspace_name = reducer.get_sample().wksp_name
    if workspace_name:
        ws_ref = mtd[workspace_name]
        return LARMOR.is_run_new_style_run(ws_ref)
    return False


def get_bench_rotation(reducer):
    """
    Extract the bench rotation from the instrument
    of the reducer
    @param reducer: the reducer object
    @returns the bench rotation in degrees
    """
    bench_rotation = 0.0
    # pylint: disable=bare-except
    try:
        ws = mtd[str(reducer.get_sample().get_wksp_name())]
        bench_rotation = reducer.instrument.getDetValues(ws)[0]
    except:
        bench_rotation = 0.0
    return bench_rotation


class CentreFinder(object):
    """
    Aids estimating the effective centre of the particle beam by calculating Q in four
    quadrants and using the asymmetry to calculate the direction to the beam centre. A
    better estimate for the beam centre position can hence be calculated iteratively
    """

    QUADS = ["Left", "Right", "Up", "Down"]

    def __init__(self, guess_centre, sign_policy, find_direction=FindDirectionEnum.ALL):
        """
        Takes a loaded reducer (sample information etc.) and the initial guess of the centre
        position that are required for all later iterations
        @param guess_centre: the starting position that the trial x and y are relative to
        @param sign_policy: sets the sign for the move operation.
        @param find_direction: Find beam centre for directions, ie if all or only up/down
                               or only left right
        """
        self.logger = Logger("CentreFinder")
        self._last_pos = guess_centre
        self.detector = None
        self.coord1_scale_factor = 1.0
        self.coord2_scale_factor = 1.0
        self.find_direction = find_direction
        self.sign_coord1 = -1.0
        self.sign_coord2 = -1.0
        if sign_policy is not None and len(sign_policy) == 2:
            self.sign_coord1 = sign_policy[0]
            self.sign_coord2 = sign_policy[1]

    def SeekCentre(self, setup, trial):
        """
        Does four calculations of Q to estimate a better centre location than the one passed
        to it
        @param setup: the reduction chain object that contains information about the reduction
        @param trial: the coordinates of the location to test as a list in the form [x, y]
        @return: the asymmetry in the calculated Q in the x and y directions
        """
        self.detector = setup.instrument.cur_detector().name()

        # populate the x and y scale factor values at this point for the text box
        self.coord1_scale_factor = setup.instrument.beam_centre_scale_factor1
        self.coord2_scale_factor = setup.instrument.beam_centre_scale_factor2

        # We are looking only at the difference between the old position and the trial.
        self.move(setup, trial[0] - self._last_pos[0], trial[1] - self._last_pos[1])

        # phi masking will remove areas of the detector that we need
        setup.mask.mask_phi = False

        setup.pre_process()
        setup.output_wksp = "centre"
        steps = setup._conv_Q
        steps = steps[0 : len(steps) - 1]
        setup._reduce(init=False, post=False, steps=steps)

        self._group_into_quadrants(setup, "centre", suffix="_tmp")

        if setup.get_can():
            # reduce the can here
            setup.reduce_can("centre_can", run_Q=False)

            self._group_into_quadrants(setup, "centre_can", suffix="_can")
            Minus(LHSWorkspace="Left_tmp", RHSWorkspace="Left_can", OutputWorkspace="Left_tmp")
            Minus(LHSWorkspace="Right_tmp", RHSWorkspace="Right_can", OutputWorkspace="Right_tmp")
            Minus(LHSWorkspace="Up_tmp", RHSWorkspace="Up_can", OutputWorkspace="Up_tmp")
            Minus(LHSWorkspace="Down_tmp", RHSWorkspace="Down_can", OutputWorkspace="Down_tmp")
            DeleteWorkspace(Workspace="Left_can")
            DeleteWorkspace(Workspace="Right_can")
            DeleteWorkspace(Workspace="Up_can")
            DeleteWorkspace(Workspace="Down_can")
            DeleteWorkspace(Workspace="centre_can")

        DeleteWorkspace(Workspace="centre")
        self._last_pos = trial

        # prepare the workspaces for "publication", after they have their
        # standard names calculations will be done on them and they will be plotted
        for out_wksp in self.QUADS:
            in_wksp = out_wksp + "_tmp"
            ReplaceSpecialValues(InputWorkspace=in_wksp, OutputWorkspace=in_wksp, NaNValue=0, InfinityValue=0)
            rem_nans = StripEndNans()
            rem_nans.execute(setup, in_wksp)

            RenameWorkspace(InputWorkspace=in_wksp, OutputWorkspace=out_wksp)

        return self._calculate_residue()

    def move(self, setup, x, y):
        """
        Move the selected detector in both the can and sample workspaces, remembering the
        that ISIS SANS team see the detector from the other side
        @param setup: the reduction chain object that contains information about the reduction
        @param x: the distance to move in the x (-x) direction in metres
        @param y: the distance to move in the y (-y) direction in metres
        """
        # Displacing the beam by +5 is equivalent to displacing the isntrument by -5. Hence we change
        # the sign here. LARMOR does this correction in the instrument itself, while for the others
        # we don't
        x = self.sign_coord1 * x
        y = self.sign_coord2 * y

        setup.instrument.elementary_displacement_of_single_component(
            workspace=setup.get_sample().wksp_name,
            component_name=self.detector,
            coord1=x,
            coord2=y,
            coord1_scale_factor=1.0,
            coord2_scale_factor=1.0,
            relative_displacement=True,
        )
        if setup.get_can():
            setup.instrument.elementary_displacement_of_single_component(
                workspace=setup.get_can().wksp_name,
                component_name=self.detector,
                coord1=x,
                coord2=y,
                coord1_scale_factor=1.0,
                coord2_scale_factor=1.0,
                relative_displacement=True,
            )

    # Create a workspace with a quadrant value in it
    def _create_quadrant(self, setup, reduced_ws, quadrant, r_min, r_max, suffix):
        out_ws = quadrant + suffix
        # Need to create a copy because we're going to mask 3/4 out and that's a one-way trip
        CloneWorkspace(InputWorkspace=reduced_ws, OutputWorkspace=out_ws)

        objxml = SANSUtility.QuadrantXML([0, 0, 0.0], r_min, r_max, quadrant)
        # Mask out everything outside the quadrant of interest
        MaskDetectorsInShape(Workspace=out_ws, ShapeXML=objxml)

        setup.to_Q.execute(setup, out_ws)
        # Q1D(output,rawcount_ws,output,q_bins,AccountForGravity=GRAVITY)

    # Create 4 quadrants for the centre finding algorithm and return their names
    def _group_into_quadrants(self, setup, input_value, suffix=""):
        r_min = setup.CENT_FIND_RMIN
        r_max = setup.CENT_FIND_RMAX

        for q in self.QUADS:
            self._create_quadrant(setup, input_value, q, r_min, r_max, suffix)

    def _calculate_residue(self):
        """
        Calculate the sum squared difference between pairs of workspaces named Left, Right, Up
        and Down. This assumes that a workspace with one spectrum for each of the quadrants
        @return: difference left to right, difference up down
        """
        residueX = 0
        if self.find_direction == FindDirectionEnum.ALL or self.find_direction == FindDirectionEnum.LEFT_RIGHT:
            yvalsAX = mtd["Left"].readY(0)
            yvalsBX = mtd["Right"].readY(0)
            qvalsAX = mtd["Left"].readX(0)
            qvalsBX = mtd["Right"].readX(0)
            qrangeX = [len(yvalsAX), len(yvalsBX)]
            nvalsX = min(qrangeX)
            id1X = "LR1"
            id2X = "LR2"
            residueX = self._residual_calculation_for_single_direction(
                yvalsA=yvalsAX, yvalsB=yvalsBX, qvalsA=qvalsAX, qvalsB=qvalsBX, qrange=qrangeX, nvals=nvalsX, id1=id1X, id2=id2X
            )

        residueY = 0
        if self.find_direction == FindDirectionEnum.ALL or self.find_direction == FindDirectionEnum.UP_DOWN:
            yvalsAY = mtd["Up"].readY(0)
            yvalsBY = mtd["Down"].readY(0)
            qvalsAY = mtd["Up"].readX(0)
            qvalsBY = mtd["Down"].readX(0)
            qrangeY = [len(yvalsAY), len(yvalsBY)]
            nvalsY = min(qrangeY)
            id1Y = "UD1"
            id2Y = "UD2"
            residueY = self._residual_calculation_for_single_direction(
                yvalsA=yvalsAY, yvalsB=yvalsBY, qvalsA=qvalsAY, qvalsB=qvalsBY, qrange=qrangeY, nvals=nvalsY, id1=id1Y, id2=id2Y
            )
        return residueX, residueY

    def _residual_calculation_for_single_direction(self, yvalsA, yvalsB, qvalsA, qvalsB, qrange, nvals, id1, id2):
        residue = 0
        indexB = 0
        for indexA in range(0, nvals):
            if qvalsA[indexA] < qvalsB[indexB]:
                self.logger.notice(id1 + " " + str(indexA) + " " + str(indexB))
                continue
            elif qvalsA[indexA] > qvalsB[indexB]:
                while qvalsA[indexA] > qvalsB[indexB]:
                    self.logger(id2 + " " + str(indexA) + " " + str(indexB))
                    indexB += 1
            if indexA > nvals - 1 or indexB > nvals - 1:
                break
            residue += pow(yvalsA[indexA] - yvalsB[indexB], 2)
            indexB += 1
        return residue

    def _get_cylinder_direction(self, workspace):
        """
        Get the direction that the masking clyinder needs to point at. This should be the normal
        of the tilted detector bench. The original normal is along the beam axis as defined in
        the instrument definition file.
        @param workspace: the workspace with the tilted detector bench
        @returns the required direction of the cylinder axis
        """
        ws = mtd[workspace]
        instrument = ws.getInstrument()
        quat = instrument.getComponentByName(self.detector).getRotation()
        cylinder_direction = instrument.getReferenceFrame().vecPointingAlongBeam()
        quat.rotate(cylinder_direction)
        return cylinder_direction.X(), cylinder_direction.Y(), cylinder_direction.Z()


class CentrePositioner(object):
    """
    Handles the positions and increments for beam finding.
    """

    def __init__(self, reducer, position_type, coord1_start, coord2_start, coord1_step, coord2_step, tolerance):
        """
        Set the CentrePositioner. It requires:
        @param reducer:: The reducer
        @param position_type: If do a full search or only UP/DOWN or LEFT/RIGHT
        @param coord1_start: The initial value for the first coordinate
        @param coord2_start: The initial value for the second coordinate
        @param coord1_step: The initial step size for the first coordinate
        @param coord2_step: The initial step size for the second coordinate
        @param tolerance: The tolerance
        @param scale_factor_coord1: the scale factor for the first coordinate
        @param scale_factor_coord2: the scale factor for the second coordinate
        """
        super(CentrePositioner, self).__init__()
        # Create the appropriate position updater
        pos_factory = BeamCentrePositionUpdaterFactory()
        self.position_updater = pos_factory.create_beam_centre_position_updater(position_type)

        # Create the appropriate increment converter
        position_provider_factory = PositionProviderFactory(coord1_step, coord2_step, tolerance, position_type)
        self.position_provider = position_provider_factory.create_position_provider(reducer)

        # set the correct units starting coordinate. such that we are dealing with units of
        # either [m,m] or [degree, m]
        self.coord1_start = self.position_provider.get_coord1_for_input_with_correct_scaling(coord1_start)
        self.coord2_start = coord2_start

        self.current_coord1 = self.coord1_start
        self.current_coord2 = self.coord2_start

    def increment_position(self, coord1_old, coord2_old):
        """
        Increment the coordinates
        @param coord1_old: the first old coordinate
        @param coord2_old: the seocond old coordinate
        @returns the incremented coordinates
        """
        coord1_increment = self.position_provider.get_increment_coord1()
        coord2_increment = self.position_provider.get_increment_coord2()
        return self.position_updater.increment_position(coord1_old, coord2_old, coord1_increment, coord2_increment)

    def set_new_increment_coord1(self):
        """
        Set the new increment for the first coordinate.
        """
        self.position_provider.half_and_reverse_increment_coord1()

    def set_new_increment_coord2(self):
        """
        Set the new increment for the second coordinate.
        """
        self.position_provider.half_and_reverse_increment_coord2()

    def produce_final_position(self, coord1_new, coord2_new):
        """
        Produce the final coordinates
        @param coord1_new: the newest version of coordinate 1
        @param coord2_new: the newest version of coordinate 2
        @returns the final coordinates
        """
        # We need to make sure that the first coordinate is returned with the correct scaling for the BaseBeamFinder,
        # ie either [m, m] or [degree/1000, m] also if it might have a bench rotation applied to it
        coord1 = self.position_provider.get_coord1_for_output_with_correct_scaling_and_offset(coord1_new)
        return coord1, coord2_new

    def produce_initial_position(self):
        """
        Produce the initial position. This is important especially for the LARMOR
        case where we can have an additional BENCH Rotation.
        @returns the initital position for coord1 and coord2
        """
        return self.position_provider.produce_initial_position(self.coord1_start, self.coord2_start)

    def is_increment_coord1_smaller_than_tolerance(self):
        """
        Check if the increment for the first coordinate is smaller than the tolerance
        @returns True if the increment of the first coordinate is smaller than tolerance else False
        """
        return self.position_provider.is_coord1_increment_smaller_than_tolerance()

    def is_increment_coord2_smaller_than_tolerance(self):
        """
        Check if the increment for the second coordinate is smaller than the tolerance
        @returns True if the increment of the second coordinate is smaller than tolerance else False
        """
        return self.position_provider.is_coord2_increment_smaller_than_tolerance()

    def produce_sign_policy(self):
        """
        Gets a tuple of sign policies for the translation of the instrument.
        """
        return self.position_provider.provide_sign_policy()


# These classes make sure that only the relevant directions are updated
# They are not instrument dependent, they should only dependt on the user's choice.


class BeamCentrePositionUpdaterFactory(object):  # pylint: disable=R0903
    """
    Creates the required beam centre position updater.
    """

    def __init__(self):
        super(BeamCentrePositionUpdaterFactory, self).__init__()

    def create_beam_centre_position_updater(self, beam_centre_position_udpater_type):
        """
        Factory method to create the appropriate Beam Centre Position Updater
        @param beam_centre_position_udpater_type: the type of updater
        """
        if beam_centre_position_udpater_type == FindDirectionEnum.LEFT_RIGHT:
            return BeamCentrePositionUpdaterLeftRight()
        elif beam_centre_position_udpater_type == FindDirectionEnum.UP_DOWN:
            return BeamCentrePositionUpdaterUpDown()
        elif beam_centre_position_udpater_type == FindDirectionEnum.ALL:
            return BeamCentrePositionUpdaterAll()
        else:
            RuntimeError(
                "Error in BeamCentrePositionUpdaterFactory: You need to provide a position update" "policy, ie up/down, left/right or all"
            )


class BeamCentrePositionUpdater(object):  # pylint: disable=R0903
    """
    Handles the position updates, ie if we are only intereseted in left/right or up/down or all
    """

    def __init__(self):
        super(BeamCentrePositionUpdater, self).__init__()

    def increment_position(self, coord1_old, coord2_old, coord1_increment, coord2_increment):
        raise RuntimeError("BeamCentrePositionUpdater is not implemented")


class BeamCentrePositionUpdaterAll(BeamCentrePositionUpdater):
    """
    Handles the position updates when all directions are being selected
    """

    def __init__(self):
        super(BeamCentrePositionUpdaterAll, self).__init__()

    def increment_position(self, coord1_old, coord2_old, coord1_increment, coord2_increment):
        """
        Increment the coordinates
        @param coord1_old: the old first coordinate
        @param coord2_old: the old second coordinate
        @param coord1_increment: the increment for the first coordinate
        @param coord2_increment: the increment for the second coordinate
        """
        return coord1_old + coord1_increment, coord2_old + coord2_increment


class BeamCentrePositionUpdaterUpDown(BeamCentrePositionUpdater):
    """
    Handles the position updates when only up/down is selected
    """

    def __init__(self):
        super(BeamCentrePositionUpdaterUpDown, self).__init__()

    def increment_position(self, coord1_old, coord2_old, coord1_increment, coord2_increment):
        """
        Increment the coordinates.
        @param coord1_old: the old first coordinate
        @param coord2_old: the old second coordinate
        @param coord1_increment: the increment for the first coordinate
        @param coord2_increment: the increment for the second coordinate
        @returns the incremented position
        """
        return coord1_old, coord2_old + coord2_increment


class BeamCentrePositionUpdaterLeftRight(BeamCentrePositionUpdater):
    """
    Handles the position updates when only right/left is selected
    """

    def __init__(self):
        super(BeamCentrePositionUpdaterLeftRight, self).__init__()

    def increment_position(self, coord1_old, coord2_old, coord1_increment, coord2_increment):
        """
        Increment the coordinates.
        @param coord1_old: the old first coordinate
        @param coord2_old: the old second coordinate
        @param coord1_increment: the increment for the first coordinate
        @param coord2_increment: the increment for the second coordinate
        @returns the incremented position
        """
        return coord1_old + coord1_increment, coord2_old

    def produce_final_position(self, x_new, x_initial, y_new, y_initial):
        """
        Produce the final position.
        @param coord1_new: the new first coordinate
        @param coord1_initial: the first initial coordinate
        @param coord2_new: the new second coordinate
        @param coord2_initial: the second initial coordinate
        @returns the final position
        """
        return x_new, y_initial


# Provides the positions and increments with the correct scaling
# These set of classes are instrument dependent.
class PositionProviderFactory(object):
    """
    Creates the required increment provider. The increments for the two coordinates
    depend on the instrument, eg Larmor's first coordinate is an angle for certain
    run numbers.
    """

    def __init__(self, increment_coord1, increment_coord2, tolerance, position_type):
        """
        Initialize the PositionProviderFactory
        @param increment_coord1: The increment for the first coordinate
        @param increment_coord2: The increment for the second coordinate
        @param tolerance: The tolerance setting
        @param position_type: The type of the psoitio, ie left/right, up/down or all
        """
        super(PositionProviderFactory, self).__init__()
        self.increment_coord1 = increment_coord1
        self.increment_coord2 = increment_coord2
        self.tolerance = tolerance
        self.position_type = position_type

    def create_position_provider(self, reducer):
        """
        Factory method for the PositionProvider
        @param reducer: The reducer object
        @returns The correct increment provider
        """
        if is_workspace_which_requires_angle(reducer):
            # The angle increment is currently not specified in the instrument parameters file,
            # hence we set a value here. This is also true for the tolerance
            # increment_coord1_angle = self.get_increment_coord1_angle(reducer, self.tolerance)
            # tolerance_angle = self.get_tolerance_for_angle(self.tolerance, self.increment_coord1, increment_coord1_angle)
            increment_coord1_angle = self.increment_coord1 / 1000.0  # The tolerance needs to be specified in angles
            tolerance_angle = self.tolerance

            # Find the bench rotation. Only supply the bench rotation if it is really needed. If we supply an offset
            # through a bench rotation we need to take into account that the directionality of the angles is not
            # the same as in Mantid. We need to reverse the sign of the bench rotation
            coord1_offset = -1 * get_bench_rotation(reducer)

            # Get the scale factor for the first coordinate
            coord1_scale_factor = reducer.get_beam_center_scale_factor1()

            return PositionProviderAngleY(
                increment_coord1=increment_coord1_angle,
                increment_coord2=self.increment_coord2,
                tolerance=self.tolerance,
                tolerance_angle=tolerance_angle,
                coord1_offset=coord1_offset,
                coord1_scale_factor=coord1_scale_factor,
            )
        else:
            return PositionProviderXY(
                increment_coord1=self.increment_coord1, increment_coord2=self.increment_coord2, tolerance=self.tolerance
            )

    def get_increment_coord1_angle(self, reducer, tolerance):
        """
        Estimate an increment for the angle based on the specified coord1 increment for linear
        displacements and the distance from the sample to the detector.
        For a distance D and an increment dx, we estimate dAlpha to be tan(dAlpha)=dx/D.
        Since D >> dx, we can use Taylor expansion to set dAlpha = dx/D
        @param reducer: the reducer object
        @param tolerance: the tolerance
        """
        workspace_name = reducer.get_sample().wksp_name
        workspace = mtd[workspace_name]

        instrument = workspace.getInstrument()
        detector_name = reducer.instrument.cur_detector().name()

        sample = instrument.getSample()
        component = instrument.getComponentByName(detector_name)

        # We use here the first detector entry. Do we need to have this smarter in the future?
        detector_bench = component[0]
        distance = detector_bench.getDistance(sample)

        return tolerance / distance

    def get_tolerance_for_angle(self, tolerance_linear, increment_linear, increment_angle):
        """
        The tolerance associated with a linear disaplacement is translated into
        a tolerance for an angle. Tol_Angle = Increment_Angle *(Tol_Linear/Increment_Linear)
        @param tolerance_linear: the tolerance for the linear displacement
        @param increment_lienar: the increment of the linear displacement
        @param increment_angle: the increment of the rotation
        """
        return (increment_angle / increment_linear) * tolerance_linear


class PositionProvider(object):
    def __init__(self, increment_coord1, increment_coord2, tolerance):
        super(PositionProvider, self).__init__()

    def get_coord1_for_input_with_correct_scaling(self, coord1):
        RuntimeError("The PositionProvider interface is not implemented")

    def get_coord1_for_output_with_correct_scaling_and_offset(self, coord1):
        RuntimeError("The PositionProvider interface is not implemented")

    def produce_initial_position(self, coord1, coord2):
        RuntimeError("The PositionProvider interface is not implemented")

    def half_and_reverse_increment_coord1(self):
        RuntimeError("The PositionProvider interface is not implemented")

    def half_and_reverse_increment_coord2(self):
        RuntimeError("The PositionProvider interface is not implemented")

    def is_coord1_increment_smaller_than_tolerance(self):
        RuntimeError("The PositionProvider interface is not implemented")

    def is_coord2_increment_smaller_than_tolerance(self):
        RuntimeError("The PositionProvider interface is not implemented")

    def get_increment_coord1(self):
        RuntimeError("The PositionProvider interface is not implemented")

    def get_increment_coord2(self):
        RuntimeError("The PositionProvider interface is not implemented")

    def check_is_smaller_than_tolerance(self, to_check, tolerance):
        if abs(to_check) < tolerance:
            return True
        else:
            return False

    def provide_sign_policy(self):
        RuntimeError("The PositionProvider interface is not implemented")


class PositionProviderXY(PositionProvider):
    """
    Handles the increments for the case when both coordinates are cartesian
    """

    def __init__(self, increment_coord1, increment_coord2, tolerance):
        super(PositionProviderXY, self).__init__(increment_coord1, increment_coord2, tolerance)
        self.increment_x = increment_coord1
        self.increment_y = increment_coord2
        self.tolerance = tolerance
        # The sign policy
        self.sign_policy_x = -1.0
        self.sign_policy_y = -1.0

    def get_coord1_for_input_with_correct_scaling(self, coord1):
        """
        Get the coordinate with the correct scaling;
        in this case it is already correct as we have [m,m]
        @param coord1: first coordinate in m
        @returns the first coordinate in m
        """
        return coord1

    def get_coord1_for_output_with_correct_scaling_and_offset(self, coord1):
        """
        Get the coordinate with the correct scaling
        @param coord1: first coordinate in m
        @returns the first coordinate in m
        """
        return coord1

    def produce_initial_position(self, coord1, coord2):
        return coord1, coord2

    def half_and_reverse_increment_coord1(self):
        """
        Halves the step size and reverses the step direction
        """
        self.increment_x = -self.increment_x / 2.0

    def half_and_reverse_increment_coord2(self):
        """
        Halves the step size and reverses the step direction
        """
        self.increment_y = -self.increment_y / 2.0

    def is_coord1_increment_smaller_than_tolerance(self):
        """
        Check if the increment for the first coordinate is smaller than the tolerance
        @returns True if the increment is smaller than the tolerance, otherwise false
        """
        return self.check_is_smaller_than_tolerance(self.increment_x, self.tolerance)

    def is_coord2_increment_smaller_than_tolerance(self):
        """
        Check if the increment for the second coordinate is smaller than the tolerance
        @returns True if the increment is smaller than the tolerance, otherwise false
        """
        return self.check_is_smaller_than_tolerance(self.increment_y, self.tolerance)

    def get_increment_coord1(self):
        return self.increment_x

    def get_increment_coord2(self):
        return self.increment_y

    def provide_sign_policy(self):
        """
        Get the sign policy for the x, y translations. Displacing the beam by 5mm
        is equivalent to displacing the instrument by -5mm, hence we need the
        minus sign here.
        """
        return self.sign_policy_x, self.sign_policy_y


class PositionProviderAngleY(PositionProvider):
    """
    Handles the increments for the case when the first coordinate is an angle
    and the second is a cartesian coordinate
    """

    def __init__(self, increment_coord1, increment_coord2, tolerance, tolerance_angle, coord1_offset, coord1_scale_factor):
        super(PositionProviderAngleY, self).__init__(increment_coord1, increment_coord2, tolerance)
        self.increment_angle = increment_coord1
        self.increment_y = increment_coord2
        self.tolerance = tolerance
        self.tolerance_angle = tolerance_angle
        self.coord1_offset = coord1_offset
        self.coord1_scale_factor = coord1_scale_factor
        # The sign policy
        self.sign_policy_angle = 1.0
        self.sign_policy_y = -1.0

    def get_coord1_for_input_with_correct_scaling(self, coord1):
        """
        Get the coordinate with the correct scaling
        @param coord1: first coordinate in degree/COORD1SF
        @returns the first coordinate in degree
        """
        return coord1 * self.coord1_scale_factor

    def get_coord1_for_output_with_correct_scaling_and_offset(self, coord1):
        """
        Get the coordinate with the correct scaling
        @param coord1: first coordinate in degree
        @returns the first coordinate in degree/COORD1SF
        """
        # At this point we want to take out the offset, hence we need to substract it.
        return (coord1 - self.coord1_offset) / self.coord1_scale_factor

    def produce_initial_position(self, coord1, coord2):
        """
        The initial position might require a correction for the bench rotation.
        """
        return coord1 + self.coord1_offset, coord2

    def half_and_reverse_increment_coord1(self):
        """
        Halves the step size and reverses the step direction
        """
        self.increment_angle = -self.increment_angle / 2.0

    def half_and_reverse_increment_coord2(self):
        """
        Halves the step size and reverses the step direction
        """
        self.increment_y = -self.increment_y / 2.0

    def is_coord1_increment_smaller_than_tolerance(self):
        """
        Check if the increment for the first coordinate is smaller than the tolerance
        @returns True if the increment is smaller than the tolerance, otherwise false
        """
        return self.check_is_smaller_than_tolerance(self.increment_angle, self.tolerance_angle)

    def is_coord2_increment_smaller_than_tolerance(self):
        """
        Check if the increment for the second coordinate is smaller than the tolerance
        @returns True if the increment is smaller than the tolerance, otherwise false
        """
        return self.check_is_smaller_than_tolerance(self.increment_y, self.tolerance)

    def get_increment_coord1(self):
        return self.increment_angle * self.coord1_scale_factor

    def get_increment_coord2(self):
        return self.increment_y

    def provide_sign_policy(self):
        """
        Get the sign policy for the angle, y translations. Displacing the beam by 5mm
        is equivalent to displacing the instrument by -5mm. The angle displacement in
        LARMOR does the sign switch already. Hence we have a positive sign policy for
        the angle direction
        """
        return self.sign_policy_angle, self.sign_policy_y


# The classes below provide a logging facility for the Beam Centre Finder mechanism
class BeamCenterLogger(object):
    """
    Logger during the beam centre operation. The logging will
    depend partially on the type of the first coordinate, ie [m, m] or [degree, m].
    It will also perform a correction for potential offsets like bench rotations.
    """

    def __init__(self, reducer, coord1_scale_factor, coord2_scale_factor):
        super(BeamCenterLogger, self).__init__()
        self.logger = Logger("CentreFinder")
        self.using_angle = False
        if is_workspace_which_requires_angle(reducer):
            self.coord1_scale_factor = 1.0
            self.using_angle = True
            # Find the bench rotation. Only supply the bench rotation if it is really needed. If we supply an offset
            # through a bench rotation we need to take into account that the directionality of the angles is not
            # the same as in Mantid. We need to reverse the sign of the bench rotation to get the correct rotation.
            self.offset_coord1 = -1 * get_bench_rotation(reducer)
        else:
            self.coord1_scale_factor = coord1_scale_factor
            self.offset_coord1 = 0.0

        self.coord2_scale_factor = coord2_scale_factor
        self.offset_coord2 = 0.0

    def report_init(self, coord1, coord2):
        """
        Report the initial setup
        @param coord1: the first coordinate
        @param coord2: the second coordinate
        """
        if self.using_angle:
            initial_msg = "beta_start"
        else:
            initial_msg = "x_start"
        # We need to substract the offset from the coordinate, since we do not want to display the offset
        # which is on the data
        val1 = (coord1 - self.offset_coord1) * self.coord1_scale_factor
        val2 = (coord2 - self.offset_coord2) * self.coord2_scale_factor

        msg = initial_msg + ",ystart= %s    %s" % (str(val1), str(val2))
        self.logger.notice(msg)
        self.logger.notice("Starting centre finding routine ...")

    def report_status(self, iteration, coord1, coord2, resid1, resid2):  # pylint: disable=too-many-arguments
        """
        Report the status of a beam finder iteration
        @param iteration: the number of the iteration
        @param coord1: the first coordinate
        @param coord2: the second coordinate
        @param resid1: the residual of the first coordinate
        @param resid2: the residual of the second coordinate
        """
        msg = self.get_status_message(iteration, coord1, coord2, resid1, resid2)
        self.logger.notice(msg)

    def get_status_message(self, iteration, coord1, coord2, resid1, resid2):  # pylint: disable=too-many-arguments
        """
        Report the status of a beam finder iteration
        @param iteration: the number of the iteration
        @param coord1: the first coordinate
        @param coord2: the second coordinate
        @param resid1: the residual of the first coordinate
        @param resid2: the residual of the second coordinate
        """
        # We need to substract the offset from the coordinate, since we do not want to display the offset
        # which is on the data
        val1 = (coord1 - self.offset_coord1) * self.coord1_scale_factor
        val2 = (coord2 - self.offset_coord2) * self.coord2_scale_factor
        coord1str = str(val1).ljust(10)[0:9]
        coord2str = str(val2).ljust(10)[0:9]
        res1str = str(resid1).ljust(7)[0:6]
        res2str = str(resid2).ljust(7)[0:6]
        msg = "Itr %i: (%s,   %s)    SX=%s   SY=%s" % (iteration, coord1str, coord2str, res1str, res2str)
        return msg

    def report(self, msg):
        """
        Report a general message
        @param msg: the message to report
        """
        self.logger.notice(msg)

    def report_final(self, coord1, coord2):
        """
        Report the final coordinates which are set in the reducer
        @param coord1: the first coordinate
        @param coord2: the second coordinate
        """
        # We shouldn't need an offset correction at this point as a possible.
        # Also we need to multiply both entries with the same (1000) scaling,
        # Because the first coordinate should have been corrected before
        # being passed into this method. For reporting purposes we revert this
        # correction. Also we don't need to remove the offset, since it the input
        # already has it removed.
        general_scale = self.coord2_scale_factor
        val1 = (coord1) * general_scale
        val2 = (coord2) * general_scale
        msg = "Centre coordinates updated: [ %f,  %f ]" % (val1, val2)
        self.logger.notice(msg)

#pylint: disable=invalid-name

import math
from mantid.simpleapi import *  # New API

def l2q(ws,whichDet,theta, sample_component_name):
    '''
    call signature::call signature::

    q=l2q(ws, whichDet, theta, sample_component_name)

    Convert from lambda to qz.

    Arguments:
      ws : run to be converted into Q.
      whichDet : Name of the component detector
      theta : final theta angle to use.
      sample_component_name: Name of the component that corresponds to the sample

    TODO:
        1) This is currently a point detector only function. This will either need
        to be expanded or a new one written for the multidetector.


    '''

    # pick up the sample to detector distance
    from mantid.api import WorkspaceGroup
    if isinstance(ws, WorkspaceGroup):
        inst = ws[0].getInstrument()
    else:
        inst = ws.getInstrument()

    sampleLocation=inst.getComponentByName(sample_component_name).getPos()
    detLocation=inst.getComponentByName(whichDet).getPos()
    sample2detector=detLocation-sampleLocation    # meters

    theta=theta*math.pi/180.0     # convert to radians

    # Fetch the reference frame to determine the instrument orientation.
    reference_frame = inst.getReferenceFrame()

    sample_to_detector_along_beam = sample2detector.scalar_prod( reference_frame.vecPointingAlongBeam() )

    # calculate new detector position based on angle theta in degrees:
    across_offset = 0.0                            # Across the beam    (side to side)
    up_offset = sample_to_detector_along_beam * math.sin(2.0 * theta)        # Normal to the beam     (up)
    beam_offset = detLocation.scalar_prod( reference_frame.vecPointingAlongBeam() )

    coord_args = dict()
    coord_args[ reference_frame.pointingAlongBeamAxis() ] = beam_offset
    coord_args[ reference_frame.pointingUpAxis() ] = up_offset
    coord_args[ reference_frame.pointingHorizontalAxis() ] = across_offset

    logger.information('Correcting detector location')
    MoveInstrumentComponent(ws, ComponentName=whichDet, RelativePosition=False, **coord_args )

    # Now convert to momentum transfer
    IvsQ = ConvertUnits(InputWorkspace=ws,OutputWorkspace="IvsQ",Target="MomentumTransfer")
    return IvsQ


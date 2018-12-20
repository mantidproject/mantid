from __future__ import (absolute_import, division, print_function)

import numpy as np
import mantid.simpleapi as api


def create_fake_dns_workspace(wsname, angle=-7.53, flipper='ON', dataY=None, loadinstrument=False):
    """
    creates DNS workspace with fake data
        @param angle   Angle of detector bank rotation
        @param flipper Flipper state (ON/OFF)
        @param dataY   Data array to set as DataY of the created workspace, will be set to np.ones if None
        @param loadinstrument  If True api.LoadInstrument will be executed, needed for DNSMergeRuns
    """
    ndet = 24
    dataX = np.zeros(2*ndet)
    dataX.fill(4.2 + 0.00001)
    dataX[::2] -= 0.000002
    if dataY is None:
        dataY = np.ones(ndet)
    dataE = np.sqrt(dataY)
    # create workspace
    api.CreateWorkspace(OutputWorkspace=wsname, DataX=dataX, DataY=dataY,
                        DataE=dataE, NSpec=ndet, UnitX="Wavelength")
    outws = api.mtd[wsname]
    p_names = 'deterota,wavelength,slit_i_left_blade_position,slit_i_right_blade_position,normalized,\
            slit_i_lower_blade_position,slit_i_upper_blade_position,polarisation,polarisation_comment,flipper'
    p_values = str(angle) + ',4.2,10,10,duration,5,20,x,7a,' + flipper
    api.AddSampleLogMultiple(Workspace=outws, LogNames=p_names, LogValues=p_values, ParseType=True)
    # rotate instrument component
    if loadinstrument:
        api.LoadInstrument(outws, InstrumentName='DNS', RewriteSpectraMap=True)
        api.RotateInstrumentComponent(outws, "bank0", X=0, Y=1, Z=0, Angle=angle)

    return outws

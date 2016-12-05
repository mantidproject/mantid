#pylint: disable=invalid-name
import numpy as np
import mantid


def calculateCenter(ws):
    intensities=ws.extractY()[:,0]
    detector_table=mantid.simpleapi.PreprocessDetectorsToMD(ws,GetMaskState="1")
    l2=np.array(detector_table.column(1))       #in meters
    tt=np.array(detector_table.column(2))       #in radians
    phi=np.array(detector_table.column(3))     #in radians
    masked=1-np.array(detector_table.column(7))
    intensities=intensities*masked
    x=l2*np.sin(tt)*np.cos(phi)
    y=l2*np.sin(tt)*np.sin(phi)
    z=l2*np.cos(tt)
    avex=(x*intensities).sum()/intensities.sum()
    avey=(y*intensities).sum()/intensities.sum()
    avez=(z*intensities).sum()/intensities.sum()
    rotation010=np.degrees(mantid.kernel.V3D(avex,0,avez).angle(mantid.kernel.V3D(0,0,1)))
    return (-avey,-rotation010)

central=mantid.simpleapi.LoadEventNexus('REF_M_22715',NXentryName='entry-Off_Off')
original=mantid.simpleapi.CloneWorkspace(central)
translation,rotation=calculateCenter(original)
mantid.simpleapi.MoveInstrumentComponent(Workspace=central,ComponentName="DetectorArm",X=0,Y=translation,Z=1,RelativePosition=1)
mantid.simpleapi.RotateInstrumentComponent(Workspace=central,ComponentName="DetectorArm",X=0,Y=1,Z=0,Angle=rotation,RelativeRotation=1)

atangle=mantid.simpleapi.LoadEventNexus('REF_M_22710',NXentryName='entry-Off_Off')
mantid.simpleapi.MoveInstrumentComponent(Workspace=atangle,ComponentName="DetectorArm",X=0,Y=translation,Z=1,RelativePosition=1)
mantid.simpleapi.RotateInstrumentComponent(Workspace=atangle,ComponentName="DetectorArm",X=0,Y=1,Z=0,Angle=rotation,RelativeRotation=1)

central=ConvertUnits(central,Target="Wavelength",EMode="Elastic")
central=Rebin(central,"3.5,0.1,7")

atangle=ConvertUnits(atangle,Target="Wavelength",EMode="Elastic")
atangle=Rebin(atangle,"3.5,0.1,7")

csum=SumSpectra(central)
asum=SumSpectra(atangle)
normalized=asum/csum

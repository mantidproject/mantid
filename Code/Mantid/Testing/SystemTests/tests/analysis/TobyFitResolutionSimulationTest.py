#pylint: disable=no-init,invalid-name
"""Testing of the VATES quantification using
the TobyFitResolutionModel
"""
from stresstesting import MantidStressTest
from mantid.simpleapi import *

def create_cuboid_xml(xlength,ylength,zlength):
    xml = """<cuboid id="sample0">
<left-front-bottom-point x="%(xpt)f" y="-%(ypt)f" z="-%(zpt)f"  />
<left-front-top-point  x="%(xpt)f" y="-%(ypt)f" z="%(zpt)f"  />
<left-back-bottom-point  x="-%(xpt)f" y="-%(ypt)f" z="-%(zpt)f"  />
<right-front-bottom-point  x="%(xpt)f" y="%(ypt)f" z="-%(zpt)f"  />
</cuboid>
<algebra val="sample0" />
"""
    return xml % {"xpt": xlength/2.0,"ypt":ylength/2.0,"zpt":zlength/2.0}

class TobyFitResolutionSimulationTest(MantidStressTest):

    _success = False

    def skipTests(self):
        return False

    def requiredMemoryMB(self):
        return 16000

    def runTest(self):
        ei = 300.
        bins = [-30,3,279]
        temperature = 6.
        chopper_speed = 600.

        # Oriented lattice & goniometer.
        alatt = 5.57
        blatt = 5.51
        clatt = 12.298
        uvec = [9.700000e-03,9.800000e-03,9.996000e-01]
        vvec = [9.992000e-01,-3.460000e-02,-4.580000e-02]

        # sample dimensions
        sx = 0.05 # Perp
        sy = 0.025 # Up direction
        sz = 0.04 # Beam direction

        # Crystal mosaic
        eta_sig = 4.0

        fake_data = CreateSimulationWorkspace(Instrument='MERLIN',
                                              BinParams=bins,UnitX='DeltaE',
                                              DetectorTableFilename='MER06398.raw')

        ##
        ## Required log entries, can be taken from real ones by placing an instrument parameter of the same
        ## name pointing to the log name
        ##
        AddSampleLog(Workspace=fake_data, LogName='Ei',LogText=str(ei), LogType="Number")
        AddSampleLog(Workspace=fake_data, LogName='temperature_log',LogText=str(temperature), LogType="Number")
        AddSampleLog(Workspace=fake_data, LogName='chopper_speed_log',LogText=str(chopper_speed), LogType="Number")
        AddSampleLog(Workspace=fake_data, LogName='eta_sigma',LogText=str(eta_sig), LogType="Number")

        ##
        ## Sample shape
        ##
        CreateSampleShape(InputWorkspace=fake_data, ShapeXML=create_cuboid_xml(sx,sy,sz))

        ##
        ## Chopper & Moderator models.
        ##
        CreateModeratorModel(Workspace=fake_data,ModelType='IkedaCarpenterModerator',
                             Parameters="TiltAngle=32,TauF=2.7,TauS=0,R=0")
        CreateChopperModel(Workspace=fake_data,ModelType='FermiChopperModel',
                           Parameters="AngularVelocity=chopper_speed_log,ChopperRadius=0.049,\
                           SlitThickness=0.0023,SlitRadius=1.3,Ei=Ei,JitterSigma=0.0")

        ##
        ## UB matrix
        ##
        SetUB(Workspace=fake_data,a=alatt,b=blatt,c=clatt,u=uvec,v=vvec)

        ##
        ## Sample rotation. Simulate 1 run at zero degrees psi
        ##

        psi = 0.0
        AddSampleLog(Workspace=fake_data,LogName='psi',LogText=str(psi),LogType='Number')
        SetGoniometer(Workspace=fake_data,Axis0="psi,0,1,0,1")

        # Create the MD workspace
        qscale = 'Q in A^-1'
        fake_md = ConvertToMD(InputWorkspace=fake_data, QDimensions="Q3D", QConversionScales=qscale,
                              SplitInto=[3], SplitThreshold=100,MinValues="-15,-15,-15,-30",
                              MaxValues="25,25,25,279",OverwriteExisting=True)

        # Run the simulation.
        resol_model = "TobyFitResolutionModel"
        xsec_model = "Strontium122"
        # Use sobol & restart each pixel to ensure reproducible result
        parameters = "Seff=0.7,J1a=38.7,J1b=-5.0,J2=27.3,SJc=10.0,GammaSlope=0.08,MultEps=0,TwinType=0,MCLoopMin=10,MCLoopMax=10,MCType=1"
        simulated = SimulateResolutionConvolvedModel(InputWorkspace=fake_md,
                                                     ResolutionFunction=resol_model,
                                                     ForegroundModel=xsec_model,
                                                     Parameters=parameters)
        # Take a slice
        slice_ws = BinMD(   InputWorkspace=simulated,
                            AlignedDim0='[H,0,0], -12.000000, 9.000000, 100',
                            AlignedDim1='[0,K,0], -6.000000, 7.000000, 100',
                            AlignedDim2='[0,0,L], 0.000000, 6.000000, 1',
                            AlignedDim3='DeltaE, 100.000000, 150.000000, 1')

        # Check
        ref_file = LoadMD(Filename='TobyFitResolutionSimulationTest.nxs')
        result = CheckWorkspacesMatch(Workspace1=slice_ws,
                                      Workspace2=ref_file,
                                      Tolerance=1e-08)
        self._success = ('success' in result.lower())

        if not self._success:
            SaveMD(InputWorkspace=slice_ws,
                   Filename='TobyFitResolutionSimulationTest-mismatch.nxs')

    def validate(self):
        return self._success


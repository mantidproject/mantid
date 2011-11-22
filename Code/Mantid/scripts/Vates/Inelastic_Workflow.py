#Common names
filename = 'fe_demo_30.sqw'
ws_in ='fe_demo_30' 

#Load an SQW file and internally convert to a Multidimensional event workspace (MDEW)
if not mtd.workspaceExists(ws_in):
 LoadSQW(filename, OutputWorkspace=ws_in)

#Bin the workspace in an axis aligned manner. Creates a Histogrammed MD workspace.
BinMD(InputWorkspace=ws_in,OutputWorkspace='binned_axis_aligned',AxisAligned=True,
AlignedDimX='Q_\\zeta,-1.5,5,100',
AlignedDimY='Q_\\xi,-6,6,100',
AlignedDimZ='Q_\\eta,-6,6,100',
AlignedDimT='E,0,150,30')

#Bin the workpace using a coordinate transformation to rotate the output.. Creates a Histogrammed MD workspace.
BinMD(InputWorkspace=ws_in,OutputWorkspace='binned_rotated',AxisAligned=False,
BasisVectorX='Qx,Ang,1,0.5,0,0,1,100',
BasisVectorY='Qy,Ang,-0.5,1,0,0,1,100',
BasisVectorZ='Qz,Ang,0,0,1.25,0,1,100',
Origin='0,0,0,0')

#Save the MDEW workspace in the MDEW nexus format.
SaveMD(ws_in, Filename='MDEW_fe_demo_30.nxs')

#Could reload the MDEW at this point.

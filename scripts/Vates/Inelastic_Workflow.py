# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
# Common names
import mantid.simpleapi as mantid

filename = 'fe_demo_30.sqw'
ws_in = 'fe_demo_30'

# Load an SQW file and internally convert to a Multidimensional event workspace (MDEW)
if not mantid.mtd.doesExist(ws_in):
    mantid.LoadSQW(filename, OutputWorkspace=ws_in)

# Bin the workspace in an axis aligned manner. Creates a Histogrammed MD workspace.
mantid.BinMD(InputWorkspace=ws_in, OutputWorkspace='binned_axis_aligned', AxisAligned=True,
             AlignedDim0='Q_\\zeta,-1.5,5,100',
             AlignedDim1='Q_\\xi,-6,6,100',
             AlignedDim2='Q_\\eta,-6,6,100',
             AlignedDim3='E,0,150,30')

# Bin the workpace using a coordinate transformation to rotate the output.. Creates a Histogrammed MD workspace.
mantid.BinMD(InputWorkspace=ws_in, OutputWorkspace='binned_rotated', AxisAligned=False,
             BasisVector0='Qx,Ang,1,0.5,0,0,1,100',
             BasisVector1='Qy,Ang,-0.5,1,0,0,1,100',
             BasisVector2='Qz,Ang,0,0,1.25,0,1,100',
             Origin='0,0,0,0')

# Save the MDEW workspace in the MDEW nexus format.
mantid.SaveMD(ws_in, Filename='MDEW_fe_demo_30.nxs')

# Could reload the MDEW at this point.

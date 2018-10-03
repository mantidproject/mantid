# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
LoadMD(Filename='TOPAZ_3680_5_sec_MDEW.nxs', OutputWorkspace='TOPAZ_3680')
BinMD(InputWorkspace='TOPAZ_3680', AlignedDim0='Q_lab_x,-5,5,20', AlignedDim1='Q_lab_y,-5,5,20', AlignedDim2='Q_lab_z,-5,5,20', OutputWorkspace='TOPAZ_3680_3D')

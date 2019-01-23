# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
##-------------------------------------------------------------------------
## Author: Owen Arnold @ ISIS/Tessella
## Date: 24/03/2011
## Purpose: Resets the time animation range to reflect the timestep values on the active source.
##
##-------------------------------------------------------------------------

src = GetActiveSource()
src.Modified()
src.UpdatePipelineInformation()
src.UpdatePipeline()
#Get the vtkSMProperty timestep values and use those.
timesteps = src.GetProperty("TimestepValues")
scene = GetAnimationScene()
lastTime = timesteps.GetElement(len(timesteps)-1)
scene.EndTime = lastTime

Render()

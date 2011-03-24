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

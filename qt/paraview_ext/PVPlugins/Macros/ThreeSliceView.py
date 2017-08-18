try: paraview.simple
except: from paraview.simple import *
paraview.simple._DisableFirstRenderCameraReset()

MantidRebinningCutter1 = GetActiveSource()
Slice1 = Slice( SliceType="Plane" )

Slice1.SliceOffsetValues = [0.0]
Slice1.SliceType.Origin = [2.1109004616737366, 0.050000429153442383, 4.76837158203125e-07]
Slice1.SliceType = "Plane"

RenderView3 = GetRenderView()
SetActiveSource(MantidRebinningCutter1)
Slice2 = Slice( SliceType="Plane" )

RenderView3.CameraClippingRange = [23.049235935630485, 58.853809426182622]

Slice2.SliceOffsetValues = [0.0]
Slice2.SliceType.Origin = [2.1109004616737366, 0.050000429153442383, 4.76837158203125e-07]
Slice2.SliceType = "Plane"

SetActiveSource(MantidRebinningCutter1)
Slice3 = Slice( SliceType="Plane" )

RenderView3.CameraClippingRange = [23.049235935630485, 58.853809426182622]

Slice3.SliceOffsetValues = [0.0]
Slice3.SliceType.Origin = [2.1109004616737366, 0.050000429153442383, 4.76837158203125e-07]
Slice3.SliceType = "Plane"

SQWReader1 = FindSource("SQWReader1")
my_representation0 = GetDisplayProperties(SQWReader1)
DataRepresentation2 = GetDisplayProperties(MantidRebinningCutter1)
a1_signal_PVLookupTable = GetLookupTableForArray( "signal", 1 )

SetActiveSource(Slice1)
DataRepresentation3 = Show()
DataRepresentation3.EdgeColor = [0.0, 0.0, 0.50000762951094835]
DataRepresentation3.ColorAttributeType = 'CELL_DATA'
DataRepresentation3.ColorArrayName = 'signal'
DataRepresentation3.LookupTable = a1_signal_PVLookupTable
DataRepresentation3.CubeAxesVisibility = 1

RenderView3.CameraClippingRange = [19.947473030485547, 62.758283859535496]

SetActiveSource(Slice2)
DataRepresentation4 = Show()
DataRepresentation4.EdgeColor = [0.0, 0.0, 0.50000762951094835]
DataRepresentation4.ColorAttributeType = 'CELL_DATA'
DataRepresentation4.ColorArrayName = 'signal'
DataRepresentation4.LookupTable = a1_signal_PVLookupTable
DataRepresentation4.CubeAxesVisibility = 1

RenderView3.CameraClippingRange = [19.669938932708867, 63.107642091045619]

SetActiveSource(Slice3)
DataRepresentation5 = Show()
DataRepresentation5.EdgeColor = [0.0, 0.0, 0.50000762951094835]
DataRepresentation5.ColorAttributeType = 'CELL_DATA'
DataRepresentation5.ColorArrayName = 'signal'
DataRepresentation5.LookupTable = a1_signal_PVLookupTable
DataRepresentation5.CubeAxesVisibility = 1

RenderView3.CameraClippingRange = [19.87618303888042, 62.848023362167204]

RenderView3.CameraClippingRange = [19.947473030485547, 62.758283859535496]

Slice2.SliceType.Normal = [0.0, 1.0, 0.0]

Slice3.SliceType.Normal = [0.0, 0.0, 1.0]

DataRepresentation2.Visibility = 0

AnimationScene3 = GetAnimationScene()
RenderView4 = CreateRenderView()
RenderView4.CameraParallelScale = 1.7320508075688772
RenderView4.CompressorConfig = 'vtkSquirtCompressor 0 3'
RenderView4.UseLight = 1
RenderView4.CameraPosition = [0.0, 0.0, 6.6921304299024635]
RenderView4.LightSwitch = 0
RenderView4.RemoteRenderThreshold = 3.0
RenderView4.CameraClippingRange = [0.025760333547106849, 25.76033354710685]
RenderView4.ViewTime = 0.0
RenderView4.LODResolution = 50.0
RenderView4.Background = [0.31999694819562063, 0.34000152590218968, 0.42999923704890519]
RenderView4.LODThreshold = 5.0
RenderView4.CenterOfRotation = [2.1109006404876709, 0.050000429153442383, 4.76837158203125e-07]

RenderView5 = CreateRenderView()
RenderView5.CameraParallelScale = 1.7320508075688772
RenderView5.CompressorConfig = 'vtkSquirtCompressor 0 3'
RenderView5.UseLight = 1
RenderView5.CameraPosition = [0.0, 0.0, 6.6921304299024635]
RenderView5.LightSwitch = 0
RenderView5.RemoteRenderThreshold = 3.0
RenderView5.CameraClippingRange = [4.6352091256034385, 9.2975123863510003]
RenderView5.ViewTime = 0.0
RenderView5.LODResolution = 50.0
RenderView5.Background = [0.31999694819562063, 0.34000152590218968, 0.42999923704890519]
RenderView5.LODThreshold = 5.0

RenderView6 = CreateRenderView()
RenderView6.CameraParallelScale = 13.400119358296035
RenderView6.CompressorConfig = 'vtkSquirtCompressor 0 3'
RenderView6.UseLight = 1
RenderView6.CameraPosition = [-49.66318505206786, 0.050000429153442383, 4.76837158203125e-07]
RenderView6.LightSwitch = 0
RenderView6.RemoteRenderThreshold = 3.0
RenderView6.CameraClippingRange = [51.256344599595579, 52.55069673594901]
RenderView6.ViewTime = 0.0
RenderView6.LODResolution = 50.0
RenderView6.Background = [0.31999694819562063, 0.34000152590218968, 0.42999923704890519]
RenderView6.CameraFocalPoint = [2.1109004020690918, 0.050000429153442383, 4.76837158203125e-07]
RenderView6.CameraViewUp = [0.0, 0.0, 1.0]
RenderView6.LODThreshold = 5.0
RenderView6.CenterOfRotation = [2.1109004020690918, 0.050000429153442383, 4.76837158203125e-07]

SetActiveSource(Slice1)
DataRepresentation6 = Show()
DataRepresentation6.ColorArrayName = 'signal'
DataRepresentation6.ColorAttributeType = 'CELL_DATA'
DataRepresentation6.LookupTable = a1_signal_PVLookupTable
DataRepresentation6.EdgeColor = [0.0, 0.0, 0.50000762951094835]

SetActiveSource(Slice2)
SetActiveView(RenderView4)
DataRepresentation7 = Show()
DataRepresentation7.ColorArrayName = 'signal'
DataRepresentation7.ColorAttributeType = 'CELL_DATA'
DataRepresentation7.LookupTable = a1_signal_PVLookupTable
DataRepresentation7.EdgeColor = [0.0, 0.0, 0.50000762951094835]

RenderView3.CameraClippingRange = [23.049238554037068, 58.853800209391451]

DataRepresentation2.Visibility = 1

AnimationScene3.ViewModules = [ RenderView3, RenderView4, RenderView5, RenderView6 ]

SetActiveSource(Slice3)
SetActiveView(RenderView5)
DataRepresentation8 = Show()
DataRepresentation8.ColorArrayName = 'signal'
DataRepresentation8.ColorAttributeType = 'CELL_DATA'
DataRepresentation8.LookupTable = a1_signal_PVLookupTable
DataRepresentation8.EdgeColor = [0.0, 0.0, 0.50000762951094835]

RenderView4.CameraViewUp = [0.0, 0.0, 1.0]
RenderView4.CameraPosition = [2.1109004020690918, -46.581135779311495, 4.76837158203125e-07]
RenderView4.CameraClippingRange = [46.164824846380284, 47.330603251591917]
RenderView4.CameraFocalPoint = [2.1109004020690918, 0.050000429153442383, 4.76837158203125e-07]
RenderView4.CameraParallelScale = 12.069026145520475
RenderView4.CenterOfRotation = [2.1109004020690918, 0.050000429153442383, 4.76837158203125e-07]

RenderView5.CameraPosition = [2.1109004020690918, 0.050000429153442383, -46.63066875713379]
RenderView5.CameraClippingRange = [30.386606389478224, 67.19107357456258]
RenderView5.CameraFocalPoint = [2.1109004020690918, 0.050000429153442383, 4.76837158203125e-07]
RenderView5.CameraParallelScale = 12.068905283627853
RenderView5.CenterOfRotation = [2.1109004020690918, 0.050000429153442383, 4.76837158203125e-07]

Render()


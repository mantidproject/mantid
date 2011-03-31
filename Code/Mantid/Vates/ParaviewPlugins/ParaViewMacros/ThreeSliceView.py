try: paraview.simple
except: from paraview.simple import *
paraview.simple._DisableFirstRenderCameraReset()

RenderView1 = GetRenderView()
AnimationScene1 = GetAnimationScene()
Cone1 = GetActiveSource()
RenderView2 = CreateRenderView()
RenderView2.CameraParallelScale = 1.7320508075688772
RenderView2.CompressorConfig = 'vtkSquirtCompressor 0 3'
RenderView2.UseLight = 1
RenderView2.CameraPosition = [0.0, 0.0, 6.6921304299024635]
RenderView2.LightSwitch = 0
RenderView2.RemoteRenderThreshold = 3.0
RenderView2.CameraClippingRange = [4.6352091256034385, 9.297512386351]
RenderView2.ViewTime = 0.0
RenderView2.LODResolution = 50.0
RenderView2.Background = [0.31999694819562063, 0.3400015259021897, 0.4299992370489052]
RenderView2.LODThreshold = 5.0

RenderView3 = CreateRenderView()
RenderView3.CameraParallelScale = 1.7320508075688772
RenderView3.CompressorConfig = 'vtkSquirtCompressor 0 3'
RenderView3.UseLight = 1
RenderView3.CameraPosition = [0.0, 0.0, 6.6921304299024635]
RenderView3.LightSwitch = 0
RenderView3.RemoteRenderThreshold = 3.0
RenderView3.CameraClippingRange = [4.6352091256034385, 9.297512386351]
RenderView3.ViewTime = 0.0
RenderView3.LODResolution = 50.0
RenderView3.Background = [0.31999694819562063, 0.3400015259021897, 0.4299992370489052]
RenderView3.LODThreshold = 5.0

RenderView4 = CreateRenderView()
RenderView4.CameraParallelScale = 1.7320508075688772
RenderView4.CompressorConfig = 'vtkSquirtCompressor 0 3'
RenderView4.UseLight = 1
RenderView4.CameraPosition = [0.0, 0.0, 6.6921304299024635]
RenderView4.LightSwitch = 0
RenderView4.RemoteRenderThreshold = 3.0
RenderView4.CameraClippingRange = [4.6352091256034385, 9.297512386351]
RenderView4.ViewTime = 0.0
RenderView4.LODResolution = 50.0
RenderView4.Background = [0.31999694819562063, 0.3400015259021897, 0.4299992370489052]
RenderView4.LODThreshold = 5.0

Slice1 = Slice( SliceType="Plane" )

AnimationScene1.ViewModules = [ RenderView1, RenderView2, RenderView3, RenderView4 ]

Slice1.SliceOffsetValues = [0.0]
Slice1.SliceType = "Plane"

DataRepresentation1 = GetDisplayProperties(Cone1)
SetActiveView(RenderView1)
DataRepresentation2 = Show()
DataRepresentation2.EdgeColor = [0.0, 0.0, 0.5000076295109483]

RenderView1.CameraClippingRange = [2.12790563200402, 4.565435444007267]

DataRepresentation1.Visibility = 0

SetActiveSource(Cone1)
Slice2 = Slice( SliceType="Plane" )

RenderView1.CameraClippingRange = [2.740729759529111, 3.7940161277005577]

Slice2.SliceOffsetValues = [0.0]
Slice2.SliceType = "Plane"

DataRepresentation3 = Show()
DataRepresentation3.Visibility = 1
DataRepresentation3.EdgeColor = [0.0, 0.0, 0.5000076295109483]

RenderView1.CameraClippingRange = [2.12790563200402, 4.565435444007267]

Slice2.SliceType.Normal = [0.0, 1.0, 0.0]

SetActiveSource(Cone1)
Slice3 = Slice( SliceType="Plane" )

RenderView1.CameraClippingRange = [2.309882128879306, 4.336364527086367]

Slice3.SliceOffsetValues = [0.0]
Slice3.SliceType = "Plane"

DataRepresentation4 = Show()
DataRepresentation4.Visibility = 1
DataRepresentation4.EdgeColor = [0.0, 0.0, 0.5000076295109483]

RenderView1.CameraClippingRange = [1.945012432679412, 4.7956603004435205]

Slice3.SliceType.Normal = [0.0, 0.0, 1.0]

SetActiveSource(Slice1)
SetActiveView(RenderView2)
DataRepresentation5 = Show()
DataRepresentation5.EdgeColor = [0.0, 0.0, 0.5000076295109483]

SetActiveSource(Slice2)
SetActiveView(RenderView4)
DataRepresentation6 = Show()
DataRepresentation6.EdgeColor = [0.0, 0.0, 0.5000076295109483]

SetActiveSource(Slice3)
SetActiveView(RenderView3)
DataRepresentation7 = Show()
DataRepresentation7.EdgeColor = [0.0, 0.0, 0.5000076295109483]

RenderView1.CameraViewUp = [0.10536742395994034, 0.9941991918590245, -0.021579454926129397]
RenderView1.CameraPosition = [-0.5383178932315406, 0.12554480883624924, 3.1555652373449012]
RenderView1.CameraClippingRange = [1.7139871590385782, 5.086473521785271]

RenderView2.CameraViewUp = [0.0, 0.0, 1.0]
RenderView2.CameraPosition = [-1.2777997508187628, 0.0, 0.0]
RenderView2.CameraClippingRange = [1.2650217533105752, 1.2969667470810444]
RenderView2.CameraFocalPoint = [0.0, 0.0, 0.0]
RenderView2.CameraParallelScale = 0.33071891133915116
RenderView2.CenterOfRotation = [0.0, 0.0, 0.0]

RenderView3.CameraPosition = [0.0, 0.0, -3.919439245554676]
RenderView3.CameraClippingRange = [3.880244853099129, 3.978230834237996]
RenderView3.CameraFocalPoint = [0.0, 0.0, 0.0]
RenderView3.CameraParallelScale = 1.0144255228718055
RenderView3.CenterOfRotation = [0.0, 0.0, 0.0]

RenderView4.CameraViewUp = [0.0, 0.0, 1.0]
RenderView4.CameraPosition = [0.0, -3.919439245554676, 0.0]
RenderView4.CameraClippingRange = [3.880244853099129, 3.978230834237996]
RenderView4.CameraFocalPoint = [0.0, 0.0, 0.0]
RenderView4.CameraParallelScale = 1.0144255228718055
RenderView4.CenterOfRotation = [0.0, 0.0, 0.0]

DataRepresentation1.Visibility = 1

DataRepresentation2.Visibility = 1

DataRepresentation3.Visibility = 1

DataRepresentation4.Visibility = 1

Render()

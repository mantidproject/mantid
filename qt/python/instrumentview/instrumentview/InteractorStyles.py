from vtkmodules.vtkInteractionStyle import vtkInteractorStyleRubberBand3D, vtkInteractorStyleRubberBand2D, vtkInteractorStyleRubberBandZoom
from vtkmodules.vtkCommonCore import vtkCommand


class CustomInteractorStyleRubberBand3D(vtkInteractorStyleRubberBand3D):
    """Wrapper for RubberBand3D style"""

    def __init__(self):
        super().__init__()
        self.AddObserver(vtkCommand.RightButtonPressEvent, lambda *_: self.OnLeftButtonDown())
        self.AddObserver(vtkCommand.RightButtonReleaseEvent, lambda *_: self.OnLeftButtonUp())
        self.AddObserver(vtkCommand.LeftButtonPressEvent, lambda *_: self.OnRightButtonDown())
        self.AddObserver(vtkCommand.LeftButtonReleaseEvent, lambda *_: self.OnRightButtonUp())
        self.AddObserver(vtkCommand.SelectionChangedEvent, on_selection_changed)


class CustomInteractorStyleZoomAndSelect(vtkInteractorStyleRubberBandZoom):
    def __init__(self):
        super().__init__()
        self.rubber_band = _ReversedInteractorStyleRubberBand2D()

        self.AddObserver(vtkCommand.RightButtonPressEvent, lambda *_: None)
        self.AddObserver(vtkCommand.RightButtonReleaseEvent, lambda *_: None)

    def set_interactor(self, interactor):
        self.rubber_band.SetInteractor(interactor)
        self.SetInteractor(interactor)

    def remove_interactor(self):
        self.SetInteractor(None)
        self.rubber_band.SetInteractor(None)


class _ReversedInteractorStyleRubberBand2D(vtkInteractorStyleRubberBand2D):
    def __init__(self):
        super().__init__()
        self.AddObserver(vtkCommand.LeftButtonPressEvent, lambda *_: None)
        self.AddObserver(vtkCommand.LeftButtonReleaseEvent, lambda *_: None)
        self.AddObserver(vtkCommand.RightButtonPressEvent, lambda *_: self.OnLeftButtonDown())
        self.AddObserver(vtkCommand.RightButtonReleaseEvent, lambda *_: self.OnLeftButtonUp())
        self.AddObserver(vtkCommand.SelectionChangedEvent, on_selection_changed)


def on_selection_changed(caller, *_):
    start_position = caller.GetStartPosition()
    end_position = caller.GetEndPosition()
    render_window_interactor = caller.GetInteractor()
    windows_size = render_window_interactor.GetRenderWindow().GetSize()

    min_x = min(start_position[0], end_position[0])
    min_y = min(start_position[1], end_position[1])
    max_x = max(start_position[0], end_position[0])
    max_y = max(start_position[1], end_position[1])

    min_x = max(0, min_x)
    min_y = max(0, min_y)
    max_x = max(0, max_x)
    max_y = max(0, max_y)

    def _clamp_to_window(coord, window_size):
        return min(coord, window_size - 2)

    # Clamp to window boundaries (from VTK C++ source)
    min_x = _clamp_to_window(min_x, windows_size[0])
    min_y = _clamp_to_window(min_y, windows_size[1])
    max_x = _clamp_to_window(max_x, windows_size[0])
    max_y = _clamp_to_window(max_y, windows_size[1])

    # Trigger picker
    render_window_interactor.StartPickCallback()
    render_window_interactor.GetPicker().AreaPick(min_x, min_y, max_x, max_y, caller.GetCurrentRenderer())
    render_window_interactor.EndPickCallback()
    render_window_interactor.Render()

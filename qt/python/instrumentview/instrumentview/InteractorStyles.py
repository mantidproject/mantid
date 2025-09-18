from pyvista.plotting.render_window_interactor import InteractorStyleCaptureMixin
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleRubberBand3D, vtkInteractorStyleRubberBand2D
from vtk import vtkCommand


class InteractorStyleRubberBand3D(InteractorStyleCaptureMixin, vtkInteractorStyleRubberBand3D):
    """Wrapper for RubberBand3D style"""

    def __init__(self, render_window_interactor):
        super().__init__(render_window_interactor)
        shared_init(self)


class InteractorStyleRubberBand2D(InteractorStyleCaptureMixin, vtkInteractorStyleRubberBand2D):
    """Wrapper for RubberBand3D style"""

    def __init__(self, render_window_interactor):
        super().__init__(render_window_interactor)
        shared_init(self)


def shared_init(self):
    self.RemoveObserver(vtkCommand.LeftButtonPressEvent)
    self.RemoveObserver(vtkCommand.LeftButtonReleaseEvent)
    self.RemoveObserver(vtkCommand.RightButtonPressEvent)
    self.RemoveObserver(vtkCommand.RightButtonReleaseEvent)

    self.AddObserver(vtkCommand.LeftButtonPressEvent, lambda *_: self.OnRightButtonDown())
    self.AddObserver(vtkCommand.LeftButtonReleaseEvent, lambda *_: self.OnRightButtonUp())
    self.AddObserver(vtkCommand.RightButtonPressEvent, lambda *_: self.OnLeftButtonDown())
    self.AddObserver(vtkCommand.RightButtonReleaseEvent, lambda *_: self.OnLeftButtonUp())

    self.AddObserver(vtkCommand.SelectionChangedEvent, on_selection_changed)


def on_selection_changed(caller, *_):
    print("Selection called!")
    start_position = caller.GetStartPosition()
    end_position = caller.GetEndPosition()
    render_window_interactor = caller.GetInteractor()
    windows_size = render_window_interactor.GetRenderWindow().GetSize()

    min_x = min(start_position[0], end_position[0])
    min_y = min(start_position[1], end_position[1])
    max_x = max(start_position[0], end_position[0])
    max_y = max(start_position[1], end_position[1])

    min_x = 0 if min_x < 0 else min_x
    min_y = 0 if min_y < 0 else min_y
    max_x = 0 if max_x < 0 else max_x
    max_y = 0 if max_y < 0 else max_y

    # TODO: Figure out what it does and if it's required
    min_x = windows_size[0] - 2 if min_x >= windows_size[0] else min_x
    min_y = windows_size[1] - 2 if min_y >= windows_size[1] else min_y
    max_x = windows_size[0] - 2 if max_x >= windows_size[0] else max_x
    max_y = windows_size[1] - 2 if max_y >= windows_size[0] else max_y

    # Trigger picker
    render_window_interactor.StartPickCallback()
    render_window_interactor.GetPicker().AreaPick(min_x, min_y, max_x, max_y, caller.GetCurrentRenderer())
    render_window_interactor.EndPickCallback()
    render_window_interactor.Render()

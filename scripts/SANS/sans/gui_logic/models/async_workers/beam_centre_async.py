# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import TYPE_CHECKING, Optional, Dict

from mantid.py36compat import dataclass
from mantidqt.utils.async_qt_adaptor import qt_async_task, IQtAsync
from mantid.kernel import Logger
from mantidqt.utils.asynchronous import AsyncTaskFailure, AsyncTaskSuccess
from sans.common.enums import DetectorType, FindDirectionEnum
from sans.sans_batch import SANSCentreFinder
from sans.state.AllStates import AllStates

if TYPE_CHECKING:
    # Avoid circular dependencies at runtime
    from sans.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter


@dataclass
class BeamCentreFields:
    component: DetectorType
    centre_of_mass: bool
    find_direction: FindDirectionEnum

    lab_pos_1: float
    lab_pos_2: float
    hab_pos_1: Optional[float]
    hab_pos_2: Optional[float]

    max_iterations: int
    r_min: float
    r_max: float
    tolerance: float
    verbose: bool


class BeamCentreAsync(IQtAsync):
    def __init__(self, parent_presenter: "BeamCentrePresenter"):
        super().__init__()
        self._parent_presenter = parent_presenter
        self._logger = Logger("CentreFinder")

    def success_cb_slot(self, result: AsyncTaskSuccess) -> None:
        self._parent_presenter.on_update_centre_values(result.output)

    def finished_cb_slot(self) -> None:
        self._parent_presenter.on_processing_finished_centre_finder()

    def error_cb_slot(self, result: AsyncTaskFailure) -> None:
        self._parent_presenter.on_processing_error_centre_finder(str(result))

    @qt_async_task
    def find_beam_centre(self, state: AllStates, settings: BeamCentreFields) -> Optional[Dict]:
        """
        This is called from the GUI and runs the find beam centre algorithm given a state model and a beam_centre_model object.

        :param state: A SANS state object
        :param settings: A class containing relevant fields for the beam settings
        :returns: The centre position found.
        """
        centre_finder = SANSCentreFinder()
        if not settings.find_direction:
            self._logger.error("No direction has been selected - please select either one or both of the Direction checkboxes.")
            raise ValueError("Unable to run beam centre finder as no direction settings have been provided.")

        pos_1 = settings.lab_pos_1 if settings.component is DetectorType.LAB else settings.hab_pos_1
        pos_2 = settings.lab_pos_2 if settings.component is DetectorType.LAB else settings.hab_pos_2

        if settings.centre_of_mass:
            centre = centre_finder(
                state,
                r_min=settings.r_min,
                r_max=settings.r_max,
                max_iter=settings.max_iterations,
                x_start=pos_1,
                y_start=pos_2,
                tolerance=settings.tolerance,
                find_direction=settings.find_direction,
                reduction_method=False,
                component=settings.component,
            )

            centre = centre_finder(
                state,
                r_min=settings.r_min,
                r_max=settings.r_max,
                max_iter=settings.max_iterations,
                x_start=centre["pos1"],
                y_start=centre["pos2"],
                tolerance=settings.tolerance,
                find_direction=settings.find_direction,
                reduction_method=True,
                verbose=settings.verbose,
                component=settings.component,
            )
        else:
            centre = centre_finder(
                state,
                r_min=settings.r_min,
                r_max=settings.r_max,
                max_iter=settings.max_iterations,
                x_start=pos_1,
                y_start=pos_2,
                tolerance=settings.tolerance,
                find_direction=settings.find_direction,
                reduction_method=True,
                verbose=settings.verbose,
                component=settings.component,
            )

        return centre

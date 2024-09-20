# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The settings diagnostic tab which visualizes the SANS state object."""

from sans.common.enums import FitType
from mantidqtinterfaces.sans_isis.gui_logic.presenter.presenter_common import PresenterCommon


class SettingsAdjustmentPresenter(PresenterCommon):
    def __init__(self, view, model):
        """unittest takes exactly 1 argument
        Creates a presenter for managing the Adjustment tab under settings
        :param view: The view handle for the tab
        :param model: The SettingsAdjustmentModel to use
        """
        super(SettingsAdjustmentPresenter, self).__init__(view=view, model=model)

    def default_gui_setup(self):
        # Set the fit options
        fit_types = [FitType.LINEAR.value, FitType.LOGARITHMIC.value, FitType.POLYNOMIAL.value]

        self._view.transmission_sample_fit_type = fit_types
        self._view.transmission_can_fit_type = fit_types

        # Use the instrument provided by the model
        self.update_instrument(self._model.instrument)

    def update_instrument(self, instrument):
        self._model.instrument = instrument
        monitor_5_movement = self._model.does_instrument_support_monitor_5()
        self._view.set_monitor_5_enabled(monitor_5_movement)

    def update_view_from_model(self):
        self._set_on_view("normalization_incident_monitor")
        self._set_on_view("normalization_interpolate")

        self._set_on_view("transmission_incident_monitor")
        self._set_on_view("transmission_interpolate")
        self._set_on_view("transmission_roi_files")
        self._set_on_view("transmission_mask_files")
        self._set_on_view("transmission_radius")
        self._set_on_view("transmission_monitor")
        self._set_on_view("transmission_mn_4_shift", 1)
        self._set_on_view("transmission_mn_5_shift", 1)

        self._set_on_view_transmission_fit()

        self._set_on_view("pixel_adjustment_det_1")
        self._set_on_view("pixel_adjustment_det_2")
        self._set_on_view("wavelength_adjustment_det_1")
        self._set_on_view("wavelength_adjustment_det_2")

    def update_model_from_view(self):
        state_model = self._model

        if state_model is None:
            return state_model

        self._set_on_custom_model("normalization_incident_monitor", self._model)
        self._set_on_custom_model("normalization_interpolate", state_model)

        self._set_on_custom_model("transmission_incident_monitor", state_model)
        self._set_on_custom_model("transmission_interpolate", state_model)
        self._set_on_custom_model("transmission_roi_files", state_model)
        self._set_on_custom_model("transmission_mask_files", state_model)
        self._set_on_custom_model("transmission_radius", state_model)
        self._set_on_custom_model("transmission_monitor", state_model)
        self._set_on_custom_model("transmission_mn_4_shift", state_model)
        self._set_on_custom_model("transmission_mn_5_shift", state_model)

        self._set_on_state_model_transmission_fit(state_model)

        self._set_on_custom_model("pixel_adjustment_det_1", state_model)
        self._set_on_custom_model("pixel_adjustment_det_2", state_model)
        self._set_on_custom_model("wavelength_adjustment_det_1", state_model)
        self._set_on_custom_model("wavelength_adjustment_det_2", state_model)

    def _set_on_state_model_transmission_fit(self, state_model):
        # Behaviour depends on the selection of the fit
        if self._view.use_same_transmission_fit_setting_for_sample_and_can():
            use_fit = self._view.transmission_sample_use_fit
            fit_type = self._view.transmission_sample_fit_type
            polynomial_order = self._view.transmission_sample_polynomial_order
            state_model.transmission_sample_fit_type = fit_type if use_fit else FitType.NO_FIT
            state_model.transmission_can_fit_type = fit_type if use_fit else FitType.NO_FIT
            state_model.transmission_sample_polynomial_order = polynomial_order
            state_model.transmission_can_polynomial_order = polynomial_order

            # Wavelength settings
            if self._view.transmission_sample_use_wavelength:
                wavelength_min = self._view.transmission_sample_wavelength_min
                wavelength_max = self._view.transmission_sample_wavelength_max
                state_model.transmission_sample_wavelength_min = wavelength_min
                state_model.transmission_sample_wavelength_max = wavelength_max
                state_model.transmission_can_wavelength_min = wavelength_min
                state_model.transmission_can_wavelength_max = wavelength_max
        else:
            # Sample
            use_fit_sample = self._view.transmission_sample_use_fit
            fit_type_sample = self._view.transmission_sample_fit_type
            polynomial_order_sample = self._view.transmission_sample_polynomial_order
            state_model.transmission_sample_fit_type = fit_type_sample if use_fit_sample else FitType.NO_FIT
            state_model.transmission_sample_polynomial_order = polynomial_order_sample

            # Wavelength settings
            if self._view.transmission_sample_use_wavelength:
                wavelength_min = self._view.transmission_sample_wavelength_min
                wavelength_max = self._view.transmission_sample_wavelength_max
                state_model.transmission_sample_wavelength_min = wavelength_min
                state_model.transmission_sample_wavelength_max = wavelength_max

            # Can
            use_fit_can = self._view.transmission_can_use_fit
            fit_type_can = self._view.transmission_can_fit_type
            polynomial_order_can = self._view.transmission_can_polynomial_order
            state_model.transmission_can_fit_type = fit_type_can if use_fit_can else FitType.NO_FIT
            state_model.transmission_can_polynomial_order = polynomial_order_can

            # Wavelength settings
            if self._view.transmission_can_use_wavelength:
                wavelength_min = self._view.transmission_can_wavelength_min
                wavelength_max = self._view.transmission_can_wavelength_max
                state_model.transmission_can_wavelength_min = wavelength_min
                state_model.transmission_can_wavelength_max = wavelength_max

    def _set_on_view_transmission_fit_sample_settings(self):
        # Set transmission_sample_use_fit
        fit_type = self._model.transmission_sample_fit_type
        use_fit = fit_type is not FitType.NO_FIT
        self._view.transmission_sample_use_fit = use_fit

        # Set the polynomial order for sample
        polynomial_order = self._model.transmission_sample_polynomial_order if fit_type is FitType.POLYNOMIAL else 2
        self._view.transmission_sample_polynomial_order = polynomial_order

        # Set the fit type for the sample
        fit_type = fit_type if fit_type is not FitType.NO_FIT else FitType.LINEAR
        self._view.transmission_sample_fit_type = fit_type

        # Set the wavelength
        wavelength_min = self._model.transmission_sample_wavelength_min
        wavelength_max = self._model.transmission_sample_wavelength_max
        if wavelength_min and wavelength_max:
            self._view.transmission_sample_use_wavelength = True
            self._view.transmission_sample_wavelength_min = wavelength_min
            self._view.transmission_sample_wavelength_max = wavelength_max

    def _set_on_view_transmission_fit(self):
        # Steps for adding the transmission fit to the view
        # 1. Check if individual settings exist. If so then set the view to separate, else set them to both
        # 2. Apply the settings
        separate_settings = self._model.has_transmission_fit_got_separate_settings_for_sample_and_can()
        self._view.set_fit_selection(use_separate=separate_settings)

        if separate_settings:
            self._set_on_view_transmission_fit_sample_settings()

            # Set transmission_sample_can_fit
            fit_type_can = self._model.transmission_can_fit_type()
            use_can_fit = fit_type_can is FitType.NO_FIT
            self._view.transmission_can_use_fit = use_can_fit

            # Set the polynomial order for can
            polynomial_order_can = self._model.transmission_can_polynomial_order if fit_type_can is FitType.POLYNOMIAL else 2
            self._view.transmission_can_polynomial_order = polynomial_order_can

            # Set the fit type for the can
            fit_type_can = fit_type_can if fit_type_can is not FitType.NO_FIT else FitType.LINEAR
            self.transmission_can_fit_type = fit_type_can

            # Set the wavelength
            wavelength_min = self._model.transmission_can_wavelength_min
            wavelength_max = self._model.transmission_can_wavelength_max
            if wavelength_min and wavelength_max:
                self._view.transmission_can_use_wavelength = True
                self._view.transmission_can_wavelength_min = wavelength_min
                self._view.transmission_can_wavelength_max = wavelength_max
        else:
            self._set_on_view_transmission_fit_sample_settings()

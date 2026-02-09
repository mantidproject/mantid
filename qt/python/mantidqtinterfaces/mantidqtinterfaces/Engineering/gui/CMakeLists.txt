# Tests for ISIS Engineering Diffraction

set(TEST_PY_FILES
    # Common
    engineering_diffraction/tabs/common/test/test_vanadium_corrections.py
    engineering_diffraction/tabs/common/data_handling/test/test_data_model.py
    engineering_diffraction/tabs/common/data_handling/test/test_data_presenter.py
    engineering_diffraction/tabs/common/show_sample/test/test_show_sample_presenter.py
    engineering_diffraction/tabs/common/show_sample/test/test_show_sample_model.py
    # Cropping
    engineering_diffraction/tabs/common/cropping/test/test_cropping_model.py
    engineering_diffraction/tabs/common/cropping/test/test_cropping_presenter.py
    # Settings
    engineering_diffraction/settings/test/test_settings_helper.py
    engineering_diffraction/settings/test/test_settings_model.py
    engineering_diffraction/settings/test/test_settings_presenter.py
    # Calibration
    engineering_diffraction/tabs/calibration/test/test_calib_model.py
    engineering_diffraction/tabs/calibration/test/test_calib_presenter.py
    # Correction
    engineering_diffraction/tabs/correction/test/test_correction_model.py
    engineering_diffraction/tabs/correction/test/test_correction_presenter.py
    # Fitting
    engineering_diffraction/tabs/fitting/test/test_presenter.py
    engineering_diffraction/tabs/fitting/plotting/test/test_plot_presenter.py
    engineering_diffraction/tabs/fitting/plotting/test/test_plot_model.py
    # Focus
    engineering_diffraction/tabs/focus/test/test_focus_model.py
    engineering_diffraction/tabs/focus/test/test_focus_presenter.py
    # Pole Figure
    engineering_diffraction/tabs/texture/test/test_texture_presenter.py
    engineering_diffraction/tabs/texture/test/test_texture_model.py
    # GSAS2
    engineering_diffraction/tabs/gsas2/test/test_gsas2_model.py
    engineering_diffraction/tabs/gsas2/test/test_gsas2_presenter.py
    # IO
    engineering_diffraction/test/engineering_diffraction_io_test.py
    # general
    engineering_diffraction/test/presenter_test.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

if(ENABLE_WORKBENCH)
  set(PYUNITTEST_QT_API pyqt5)
  pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.EngineeringDiffraction ${TEST_PY_FILES})
  unset(PYUNITTEST_QT_API)
endif()

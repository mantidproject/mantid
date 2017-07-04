#pylint: disable=invalid-name
"""
    Script used to start the Test Interface from MantidPlot
"""
from ui.sans_isis import sans_data_processor_gui
from sans.gui_logic.presenter.main_presenter import MainPresenter
from sans.common.enums import SANSFacility

# -----------------------------------------------
# Create presenter
# -----------------------------------------------
main_presenter = MainPresenter(SANSFacility.ISIS)

# -------------------------------------------------
# Create view
# Note that the view owns the presenter, but only
# uses it for the setup.
# ------------------------------------------------
ui = sans_data_processor_gui.SANSDataProcessorGui(main_presenter)

# -----------------------------------------------
# Set view on the presenter.
# The presenter delegates the view.
# -----------------------------------------------
main_presenter.set_view(ui)

# Show
if ui.setup_layout():
    ui.show()

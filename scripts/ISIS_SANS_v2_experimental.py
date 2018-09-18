#pylint: disable=invalid-name
"""
    Script used to start the Test Interface from MantidPlot
"""
from ui.sans_isis import sans_data_processor_gui
from sans.gui_logic.presenter.run_tab_presenter import RunTabPresenter
from sans.common.enums import SANSFacility

# -----------------------------------------------
# Create presenter
# -----------------------------------------------
run_tab_presenter = RunTabPresenter(SANSFacility.ISIS)
# -------------------------------------------------
# Create view
# ------------------------------------------------
ui = sans_data_processor_gui.SANSDataProcessorGui()

# -----------------------------------------------
# Set view on the presenter.
# The presenter delegates the view.
# -----------------------------------------------
run_tab_presenter.set_view(ui)

# Show
if ui.setup_layout():
    ui.show()

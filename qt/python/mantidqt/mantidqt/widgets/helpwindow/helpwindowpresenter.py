from helpwindowmodel import HelpModel
from helpwindowview import HelpView
from qtpy.QtWidgets import QApplication


class HelpPresenter:
    _instance = None  # Singleton instance for integration

    @classmethod
    def instance(cls):
        """Ensure a single instance of HelpPresenter."""
        if cls._instance is None:
            app = QApplication.instance() or QApplication([])  # noqa: F841
            model = HelpModel()
            view = HelpView(None)
            cls._instance = cls(model, view)
            cls._instance.view = view
        return cls._instance

    def __init__(self, model, view):
        self.model = model
        self.view = view

    def handle_url_changed(self, url):
        """Handle URL changes in the view."""
        self.model.add_url(url.toString())
        self.update_navigation_buttons()

    def handle_go_back(self):
        """Handle the back button click."""
        url = self.model.go_back()
        if url:
            self.view.set_url(url)
        self.update_navigation_buttons()

    def handle_go_forward(self):
        """Handle the forward button click."""
        url = self.model.go_forward()
        if url:
            self.view.set_url(url)
        self.update_navigation_buttons()

    def handle_go_home(self):
        """Handle the home button click."""
        url = self.model.history[0] if self.model.history else "about:blank"
        self.view.set_url(url)

    def show_page(self, url):
        """Load a page in the view."""
        self.model.add_url(url)
        self.view.set_url(url)
        self.update_navigation_buttons()

    def show_algorithm(self, name, version):
        """Show an algorithm-specific help page."""
        base_url = "file:///path/to/docs/algorithms/"
        algorithm_url = f"{base_url}{name}-v{version}.html"
        self.show_page(algorithm_url)

    def show_concept(self, name):
        """Show a concept-specific help page."""
        base_url = "file:///path/to/docs/concepts/"
        concept_url = f"{base_url}{name}.html"
        self.show_page(concept_url)

    def update_navigation_buttons(self):
        """Update the state of navigation buttons in the view."""
        can_go_back = self.model.current_index > 0
        can_go_forward = self.model.current_index < len(self.model.history) - 1
        self.view.update_navigation_buttons(can_go_back, can_go_forward)

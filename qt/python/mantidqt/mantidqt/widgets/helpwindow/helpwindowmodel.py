class HelpModel:
    def __init__(self):
        self.history = []  # Track visited URLs
        self.current_index = -1  # Current position in history

    def add_url(self, url):
        """Add a URL to the history, truncating forward history if necessary."""
        if self.current_index < len(self.history) - 1:
            self.history = self.history[: self.current_index + 1]
        self.history.append(url)
        self.current_index += 1

    def go_back(self):
        """Navigate back in the history."""
        if self.current_index > 0:
            self.current_index -= 1
        return self.history[self.current_index]

    def go_forward(self):
        """Navigate forward in the history."""
        if self.current_index < len(self.history) - 1:
            self.current_index += 1
        return self.history[self.current_index]

    def get_current_url(self):
        """Get the current URL."""
        if self.current_index >= 0:
            return self.history[self.current_index]
        return None

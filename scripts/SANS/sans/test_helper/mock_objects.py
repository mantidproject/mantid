class MockRunTabView(object):
    def __init__(self):
        super(MockRunTabView, self).__init__()
        self._listener = None
        self._user_file_path = None
        self._batch_file_path = None

        self._event_slices = None
        self._reduction_dimensionality = None

        self._rows = []

    def add_listener(self, listener):
        self._listener = listener

    # Instrument related ----------------------
    def set_instrument_settings(self, value):
        pass

    # User file related -----------------------
    def on_user_file_load(self):
        self._listener.on_user_file_load()

    def get_user_file_path(self):
        return self._user_file_path

    def set_user_file_path(self, value):
        self._user_file_path = value

    # Batch file related -----------------------
    def on_batch_file_load(self):
        self._listener.on_batch_file_load()

    def get_batch_file_path(self):
        return self._batch_file_path

    def set_batch_file_path(self, value):
        self._batch_file_path = value

    def clear_table(self):
        pass

    def add_row(self, value):
        self._rows.append(value)

    def get_rows(self):
        return self._rows

    # State related -----------------------------
    def get_cell(self, row, column, convert_to):
        _ = convert_to  # noqa
        if row == 0:
            # For the first row we return the
            # all of hte sample data
            if column == 0:
                return "SANS2D00022024"
            elif column == 1:
                return "SANS2D00022048"
            elif column == 2:
                return "SANS2D00022048"
            else:
                return ""
        else:
            # For the other rows, we only return sample scatter
            if column == 0:
                return "SANS2D00022024"
            else:
                return ""

    # Other -------------------------------------
    def get_number_of_rows(self):
        return 2

    @property
    def event_slices(self):
        return self._event_slices

    @event_slices.setter
    def event_slices(self, value):
        self._event_slices = value

    @property
    def reduction_dimensionality(self):
        return self._reduction_dimensionality

    @reduction_dimensionality.setter
    def reduction_dimensionality(self, value):
        self._reduction_dimensionality = value

    @property
    def zero_error_free(self):
        return True

    @zero_error_free.setter
    def zero_error_free(self, value):
        pass

class MockStateModel(object):
    def __init__(self):
        super(MockStateModel, self).__init__()
        self.settings = {}

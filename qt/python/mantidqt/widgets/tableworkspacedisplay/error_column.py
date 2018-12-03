class ErrorColumn:
    def __init__(self, column, error_for_column, label_index):
        self.column = column
        self.error_for_column = error_for_column
        if self.column == self.error_for_column:
            raise ValueError("Cannot set Y column to be its own YErr")

        self.label_index = label_index

    def __eq__(self, other):
        if isinstance(other, ErrorColumn):
            return self.error_for_column == other.error_for_column or self.column == other.column
        elif isinstance(other, int):
            return self.column == other
        else:
            raise RuntimeError("Unhandled comparison logic with type {}".format(type(other)))

    def __cmp__(self, other):
        if isinstance(other, ErrorColumn):
            return self.column == other.column or self.error_for_column == other.error_for_column
        elif isinstance(other, int):
            return self.column == other
        else:
            raise RuntimeError("Unhandled comparison logic with type {}".format(type(other)))

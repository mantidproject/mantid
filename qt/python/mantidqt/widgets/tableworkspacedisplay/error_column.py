class ErrorColumn:
    def __init__(self, source_column, error_for_column, label_index):
        self.source_column = source_column
        self.error_for_column = error_for_column
        self.label_index = label_index

    def __eq__(self, other):
        if isinstance(other, ErrorColumn):
            return self.error_for_column == other.error_for_column or self.source_column == other.source_column
        elif isinstance(other, int):
            return self.source_column == other
        else:
            raise RuntimeError("Unhandled comparison logic with type {}".format(type(other)))

    def __cmp__(self, other):
        if isinstance(other, ErrorColumn):
            return self.source_column == other.source_column or self.error_for_column == other.error_for_column
        elif isinstance(other, int):
            return self.source_column == other
        else:
            raise RuntimeError("Unhandled comparison logic with type {}".format(type(other)))

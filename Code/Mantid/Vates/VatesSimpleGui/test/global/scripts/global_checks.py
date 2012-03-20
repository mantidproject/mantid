def fix_bool(value):
    if value:
        return True
    else:
        return False

def check_filter_selection(filter, expected_state, message=None):
    selection_model = waitForObject(":_QItemSelectionModel")
    current_state = selection_model.isSelected(filter)
    test.compare(fix_bool(current_state), expected_state, message)
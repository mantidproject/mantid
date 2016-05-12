type_sans_state_key = "type"
type_sans_state_convert_to_histogram = "convert_to_histogram_state"


def is_dictionary(state_container):
    return isinstance(state_container, dict)


def is_correct_type(state_container, type_of_dictionary):
    if type_sans_state_key not in state_container:
        return False
    return state_container[type_sans_state_key] == type_of_dictionary


class SANSStateBuilder(object):
    def __init__(self):
        super(SANSStateBuilder, self).__init__()

    def set_convert_event_to_histogram(self, state):
        if not self._is_valid_input(state, type_sans_state_convert_to_histogram):
            raise ValueError("The input for SANSStateConvertEventToHistogram seems to be invalid")
        #TODO store logic

    def build(self):
        pass

    def set_data(self, state):

    def _is_valid_input(self, state_container, type_sans_state):
        return is_dictionary(state_container) and is_correct_type(state_container, type_sans_state)
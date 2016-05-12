import abc


class SANSStateSchemerFactory(object):
    def __init__(self):
        super(SANSStateSchemerFactory, self).__init__()

    @staticmethod
    def create_schemer(instrument_type):
        if instrument_type == "ISIS":
            # TODO base this on sans_state once it is ready
            return SANSStateSchemerISIS()
        else:
            pass


class SANSStateSchemer(object):
    """
    The SANSState schemer allows us to enforce which variables and types are expected. Due to the large number of
    configuration options and the necessity of having to use dictionaries/PropertyManagers as inputs, we need to have
    a way to enforce state-correctness.
    """
    @abc.abstractmethod
    def check_sans_state_complete(self, state_complete):
        # TODO Add check for
        pass

    @abc.abstractmethod
    def check_sans_state_convert_event_to_histogram(self, event_to_histogram_state):
        pass


class SANSStateSchemerISIS(SANSStateSchemer):
    def check_sans_state_complete(self, state_complete):
        return ""

    def check_sans_state_convert_event_to_histogram(self, event_to_histogram_state):
        return ""







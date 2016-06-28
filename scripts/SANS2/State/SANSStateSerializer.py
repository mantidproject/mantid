from mantid.kernel import (PropertyManager)
import inspect
import importlib
from SANS2.State.SANSStateBase import (SANSStateBase, TypedParameter, ClassTypeParameter)


STATE_NAME = "state_name"
STATE_MODULE = "state_module"
SEPARATOR_SERIAL = "#"
class_type_parameter_id = "ClassTypeParameterID#"
MODULE = "__module__"


# ------------------------------------------------
# Free functions
# ------------------------------------------------
def is_state(property_manager):
    return property_manager.existsProperty(STATE_NAME) and property_manager.existsProperty(STATE_MODULE)


def get_module_and_class_name(instance):
    if inspect.isclass(instance):
        module_name, class_name = str(instance.__dict__[MODULE]), str(instance.__name__)
    else:
        module_name, class_name = str(type(instance).__dict__[MODULE]), str(type(instance).__name__)
    return module_name, class_name


def provide_class_from_module_and_class_name(module_name, class_name):
    module = importlib.import_module(module_name)
    return getattr(module, class_name)


def provide_class(instance):
    module_name = instance.getProperty(STATE_MODULE).value
    class_name = instance.getProperty(STATE_NAME).value
    return provide_class_from_module_and_class_name(module_name, class_name)


def is_class_type_parameter(value):
    return isinstance(value, basestring) and class_type_parameter_id in value


def get_module_and_class_name_from_encoded_string(encoder, value):
    without_encoder = value.replace(encoder, "")
    return without_encoder.split(SEPARATOR_SERIAL)


def create_module_and_class_name_from_encoded_string(class_type_id, module_name, class_name):
    return class_type_id + module_name + SEPARATOR_SERIAL + class_name


def create_sub_state(value):
    # We are dealing with a sub state. We first have to create it and then populate it
    sub_state_class = provide_class(value)

    # Create the sub state, populate it and set it on the super state
    sub_state = sub_state_class()
    sub_state.property_manager = value
    return sub_state


def get_descriptor_values(instance):
    # Get all descriptor names which are TypedParameter of instance's type
    descriptor_names = []
    descriptor_types = {}
    for descriptor_name, descriptor_object in inspect.getmembers(type(instance)):
        if inspect.isdatadescriptor(descriptor_object) and isinstance(descriptor_object, TypedParameter):
            descriptor_names.append(descriptor_name)
            descriptor_types.update({descriptor_name: descriptor_object})

    # Get the descriptor values from the instance
    descriptor_values = {}
    for key in descriptor_names:
        if hasattr(instance, key):
            value = getattr(instance, key)
            if value is not None:
                descriptor_values.update({key: value})
    return descriptor_values, descriptor_types


def get_class_descriptor_types(instance):
    # Get all descriptor names which are TypedParameter of instance's type
    descriptors = {}
    for descriptor_name, descriptor_object in inspect.getmembers(type(instance)):
        if inspect.isdatadescriptor(descriptor_object) and isinstance(descriptor_object, TypedParameter):
            descriptors.update({descriptor_name: type(descriptor_object)})
    return descriptors


def convert_state_to_dict(instance):
    descriptor_values, descriptor_types = get_descriptor_values(instance)
    # Add the descriptors to a dict
    state_dict = dict()
    for key, value in descriptor_values.iteritems():
        # If the value is a SANSBaseState then create a dict from it
        # If the value is a dict, then we need to check what the sub types are
        # IF the value is a ClassTypeParameter, then we need to encode it
        if isinstance(value, SANSStateBase):
            sub_state_dict = value.property_manager
            value = sub_state_dict
        elif isinstance(value, dict):
            # If we have a dict, then we need to watch out since a value in the dict might be a SANSState
            sub_dictionary = {}
            for k, v in value.iteritems():
                if isinstance(v, SANSStateBase):
                    sub_dictionary_value = v.property_manager
                else:
                    sub_dictionary_value = v
                sub_dictionary.update({k: sub_dictionary_value})
            value = sub_dictionary
        elif isinstance(descriptor_types[key], ClassTypeParameter):
            # The module will only know about the outer class name, therefore we need
            # 1. The module name
            # 2. The name of the outer class
            # 3. The name of the actual class
            module_name, class_name = get_module_and_class_name(value)
            outer_class_name = value.outer_class_name
            class_name = outer_class_name + SEPARATOR_SERIAL + class_name
            value = create_module_and_class_name_from_encoded_string(class_type_parameter_id, module_name, class_name)

        state_dict.update({key: value})
    # Add information about the current state object, such as in which module it lives and what its name is
    module_name, class_name = get_module_and_class_name(instance)
    state_dict.update({STATE_MODULE: module_name})
    state_dict.update({STATE_NAME: class_name})
    return state_dict


def set_state_from_property_manager(instance, property_manager):
    def _set_element(inst, k, v):
        if k != STATE_NAME and k != STATE_MODULE:
            setattr(inst, k, v)

    keys = property_manager.keys()
    for key in keys:
        value = property_manager.getProperty(key).value
        # There are four scenarios that need to be considered
        # 1. ParameterManager 1: This indicates (most often) that we are dealing with a new state -> create it and
        #                      apply recursion
        # 2. ParameterManager 2: In some cases the ParameterManager object is actually a map rather than a state ->
        #                         populate the state
        # 3. String with special meaning: Admittedly this is a hack, but we limited by the input property types
        #                                 of Mantid algorithms, which can be string, int, float and containers of these
        #                                 types (and ProprtyManagerProperties). We need a wider range of types, such
        #                                 as ClassTypeParameters. These are encoded (as good as possible) in a string
        # 4. Normal values: all is fine, just populate them
        if type(value) == PropertyManager and is_state(value):
            sub_state = create_sub_state(value)
            setattr(instance, key, sub_state)
        elif type(value) == PropertyManager:
            # We must be dealing with an actual dict descriptor
            sub_dict_keys = value.keys()
            dict_element = {}
            # We need to watch out if a value of the dictionary is a sub state
            for sub_dict_key in sub_dict_keys:
                sub_dict_value = value.getProperty(sub_dict_key).value
                if type(sub_dict_value) == PropertyManager and is_state(sub_dict_value):
                    sub_state = create_sub_state(sub_dict_value)
                    sub_dict_value_to_insert = sub_state
                else:
                    sub_dict_value_to_insert = sub_dict_value
                dict_element.update({sub_dict_key: sub_dict_value_to_insert})
            setattr(instance, key, dict_element)
        elif is_class_type_parameter(value):
            # We need to first get the outer class from the module
            module_name, outer_class_name, class_name = get_module_and_class_name_from_encoded_string(class_type_parameter_id, value)
            outer_class_type_parameter = provide_class_from_module_and_class_name(module_name, outer_class_name)
            # From the outer class we can then retrieve the inner class which normally defines the users selection
            class_type_parameter = getattr(outer_class_type_parameter, class_name)
            _set_element(instance, key, class_type_parameter)
        else:
            _set_element(instance, key, value)


def create_deserialized_sans_state_from_property_manager(property_manager):
    return create_sub_state(property_manager)

from mantid.kernel import (PropertyManager)
import inspect
import importlib
from State.SANSStateBase import (SANSStateBase, TypedParameter, ClassTypeParameter)


STATE_NAME = "state_name"
STATE_MODULE = "state_module"
SEPARATOR_SERIAL = "#"
CLASS_TYPE_PARAMETER_ID = "ClassTypeParameterID#"
MODULE = "__module__"


# ------------------------------------------------
# Free functions
# ------------------------------------------------
def is_state(property_manager):
    return property_manager.existsProperty(STATE_NAME) and property_manager.existsProperty(STATE_MODULE)


def get_module_and_class_name(instance):
    return str(type(instance).__dict__[MODULE]), str(type(instance).__name__)


def provide_class_from_module_and_class_name(module_name, class_name):
    module = importlib.import_module(module_name)
    return getattr(module, class_name)


def provide_class(instance):
    module_name = instance.getProperty(STATE_MODULE).value
    class_name = instance.getProperty(STATE_NAME).value
    return provide_class_from_module_and_class_name(module_name, class_name)


def is_class_type_parameter(value):
    return isinstance(value, basestring) and CLASS_TYPE_PARAMETER_ID in value


def get_module_and_class_name_from_encoded_string(encoder, value):
    without_encoder = value.replace(encoder, "")
    return without_encoder.split(SEPARATOR_SERIAL)


def get_descriptor_values(instance):
    # Get all descriptor names which are TypedParameter of instance's type
    descriptor_names = []
    for descriptor_name, descriptor_object in inspect.getmembers(type(instance)):
        if inspect.isdatadescriptor(descriptor_object) and isinstance(descriptor_object, TypedParameter):
            descriptor_names.append(descriptor_name)

    # Get the descriptor values from the instance
    descriptor_values = {}
    for key in descriptor_names:
        if hasattr(instance, key):
            value = getattr(instance, key)
            if value is not None:
                descriptor_values.update({key: value})
    return descriptor_values


def get_class_descriptor_types(instance):
    # Get all descriptor names which are TypedParameter of instance's type
    descriptors = {}
    for descriptor_name, descriptor_object in inspect.getmembers(type(instance)):
        if inspect.isdatadescriptor(descriptor_object) and isinstance(descriptor_object, TypedParameter):
            descriptors.update({descriptor_name: type(descriptor_object)})
    return descriptors


def convert_state_to_dict(instance):
    descriptor_values = get_descriptor_values(instance)
    # Add the descriptors to a dict
    state_dict = dict()
    for key, value in descriptor_values.iteritems():
        # If the value is a SANSBaseState then create a dict from it
        if isinstance(value, SANSStateBase):
            sub_state_dict = convert_state_to_dict(value)
            value = sub_state_dict
        elif isinstance(value, ClassTypeParameter):
            module_name, class_name = get_module_and_class_name(instance)
            value = module_name + SEPARATOR_SERIAL + class_name
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
        #                                 types (and ProeprtyManagerProperties). We need a wider range of types, such
        #                                 as ClassTypeParameters. These are encoded (as good as possible) in a string
        # 4. Normal values: all is fine, just populate them
        if type(value) == PropertyManager and is_state(value):
            # We are dealing with a sub state. We first have to create it and then populate it
            sub_state_class = provide_class(value)

            # Create the sub state, populate it and set it on the super state
            sub_state = sub_state_class()
            sub_state.property_manager = value
            setattr(instance, key, sub_state)

        elif type(value) == PropertyManager:
            # We must be dealing with an actual dict descriptor
            sub_dict_keys = value.keys()
            dict_element = {}
            for sub_dict_key in sub_dict_keys:
                #dict_element.update({sub_dict_key: value.getProperty(sub_dict_key).value})
                raise NotImplementedError("IMPLEMENT")
        elif is_class_type_parameter(value):
            module_name, class_name = get_module_and_class_name_from_encoded_string(CLASS_TYPE_PARAMETER_ID, value)
            class_type_parameter = provide_class_from_module_and_class_name(module_name, class_name)
            _set_element(key, class_type_parameter)
        else:
            _set_element(instance, key, value)

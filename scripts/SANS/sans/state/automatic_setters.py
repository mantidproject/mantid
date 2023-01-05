# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from functools import partial, wraps
import inspect

# -------------------------------------------------------------------------------------------------------------
# Automatic Setter functionality
# This creates setters on a builder/director instance for parameters of a state object.
# The setter name generation is fairly simple. There are two scenarios
# 1. Standard parameter: state -> parameter  results in setter name: "set_" + name of parameter
# 2. Parameter which is buried in dictionary: state->dict->parameter
#    This results in "set_" + dictionary key + name of parameter, e.g. set_HAB_x_translation_correction, where
#    HAB is a key of a dictionary and x_translation_correction is the parameter name
#
# The resulting decorator automatic_setters takes the type of class (essentially which state) to operate on and
# an exclusion list. Elements in the exclusion list will not have an automatic setter generated for them. This is
# desirable for parameters which are set internally during the initialization phase of the builder.
# -------------------------------------------------------------------------------------------------------------


def forwarding_setter(value, builder_instance, attribute_name_list):
    # The first element of the attribute list is the state object itself
    instance = getattr(builder_instance, attribute_name_list[0])

    # We need to exclude the first element, since we have already used. We also need to exclude the last
    # element since we don't want to get it but rather set it. We need to treat the instance differently if it is
    # a dictionary.
    for index in range(1, len(attribute_name_list) - 1):
        if isinstance(instance, dict):
            instance = instance[attribute_name_list[index]]
        else:
            instance = getattr(instance, attribute_name_list[index])
    # Finally, the last attribute name is used to set the value
    setattr(instance, attribute_name_list[-1], value)


def update_the_method(builder_instance, new_methods, setter_name, attribute_name, attribute_name_list):
    setter_name_copy = list(setter_name)
    setter_name_copy.append(attribute_name)
    try:
        method_name = "_".join(setter_name_copy)
    except TypeError:
        # An enum is being used for a key - the dev needs to create a manual setter
        return

    attribute_name_list_copy = list(attribute_name_list)
    attribute_name_list_copy.append(attribute_name)

    new_method = partial(forwarding_setter, builder_instance=builder_instance, attribute_name_list=attribute_name_list_copy)
    new_methods.update({method_name: new_method})


def get_all_typed_parameter_descriptors(instance):
    descriptor_types = {}
    for descriptor_name, descriptor_object in inspect.getmembers(instance):
        if not descriptor_name.startswith("__"):
            descriptor_types.update({descriptor_name: descriptor_object})
    return descriptor_types


def create_automatic_setters_for_state(attribute_value, builder_instance, attribute_name_list, setter_name, exclusions, new_methods):
    # Find all typed parameter descriptors which are on the instance, i.e. on attribute_value.
    all_descriptors = get_all_typed_parameter_descriptors(attribute_value)

    # Go through each descriptor and create a setter for it.
    for name, value in list(all_descriptors.items()):
        # If the name is in the exception list, then we don't want to create a setter for this attribute
        if name in exclusions:
            continue

        # There are two scenarios. The attribute can be:
        # 1. A dictionary which is empty or None-> install a setter
        # 2. A dictionary containing elements -> for each element apply a recursion
        # 3. A regular attribute -> install the setter
        if isinstance(value, dict):
            dict_parameter_value = getattr(attribute_value, name)
            if dict_parameter_value is None or len(dict_parameter_value) == 0:
                update_the_method(builder_instance, new_methods, setter_name, name, attribute_name_list)
            else:
                for dict_key, dict_value in list(dict_parameter_value.items()):
                    setter_name_copy = list(setter_name)
                    setter_name_copy.append(dict_key)

                    # We need to add the dict name to the attribute list and the key we are looking at now
                    attribute_name_list_copy = list(attribute_name_list)
                    attribute_name_list_copy.append(name)
                    attribute_name_list_copy.append(dict_key)
                    create_automatic_setters_for_state(
                        dict_value, builder_instance, attribute_name_list_copy, setter_name_copy, exclusions, new_methods
                    )
        else:
            update_the_method(builder_instance, new_methods, setter_name, name, attribute_name_list)


def create_automatic_setters(builder_instance, state_class, exclusions):
    # Get the name of the state object
    new_methods = {}
    for attribute_name, attribute_value in list(builder_instance.__dict__.items()):
        if isinstance(attribute_value, state_class):
            attribute_name_list = [attribute_name]
            setter_name = ["set"]
            create_automatic_setters_for_state(attribute_value, builder_instance, attribute_name_list, setter_name, exclusions, new_methods)

    # Apply the methods
    for method_name, method in list(new_methods.items()):
        builder_instance.__dict__.update({method_name: method})


def automatic_setters(state_class, exclusions=None):
    if exclusions is None:
        exclusions = []

    def automatic_setters_decorator(func):
        @wraps(func)
        def func_wrapper(self, *args, **kwargs):
            func(self, *args, **kwargs)
            create_automatic_setters(self, state_class, exclusions)

        return func_wrapper

    return automatic_setters_decorator


# ----------------------------------------------------------------------------------------------------------------------
# Automatic setter for director object
# Note that this is not a decorator, but just a free function for monkey patching.
# ----------------------------------------------------------------------------------------------------------------------
def forwarding_setter_for_director(value, builder, method_name):
    method = getattr(builder, method_name)
    method(value)


def set_up_setter_forwarding_from_director_to_builder(director, builder_name):
    """
    This function creates setter forwarding from a director object to builder objects.

    The method will look for any set_XXX method in the builder and add an equivalent method set_builder_XXX which is
    forwarded to set_XXX.
    :param director: a director object
    :param builder_name: the name of the builder on the director
    """
    set_tag = "set"
    builder_instance = getattr(director, builder_name)
    new_methods = {}
    for method in dir(builder_instance):
        if method.startswith(set_tag):
            method_name_director = set_tag + builder_name + "_" + method[4:]
            new_method = partial(forwarding_setter_for_director, builder=builder_instance, method_name=method)
            new_methods.update({method_name_director: new_method})
    director.__dict__.update(new_methods)

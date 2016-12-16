# pylint: disable=invalid-name, too-many-arguments
from functools import (partial, wraps)


# -------------------------------------------------------------------------------------------------------------
# Automatic Setter functionality
# This creates setters on a builder instance for parameters of a state object.
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


def update_the_method(builder_instance,  new_methods, setter_name, attribute_name, attribute_name_list):
    setter_name_copy = list(setter_name)
    setter_name_copy.append(attribute_name)
    method_name = "_".join(setter_name_copy)

    attribute_name_list_copy = list(attribute_name_list)
    attribute_name_list_copy.append(attribute_name)

    new_method = partial(forwarding_setter, builder_instance=builder_instance,
                         attribute_name_list=attribute_name_list_copy)
    new_methods.update({method_name: new_method})


def create_automatic_setters_for_state(attribute_value, builder_instance, attribute_name_list,
                                       setter_name, exclusions, new_methods):
    # Find all sub attributes which have a # in their name. This is setup by us in the TypedParameter
    for sub_attribute_name, sub_attribute_value in attribute_value.__dict__.iteritems():
        if "#" in sub_attribute_name:
            name = sub_attribute_name.split("#")[1]
            # If the name is in the exception list, then we don't want to create a setter for this attribute
            if name in exclusions:
                continue

            # There are two scenarios. The attribute can be:
            # 1. A dictionary which is empty -> install a setter
            # 2. A dictionary containing elements -> for each element apply a recursion
            # 3. A regular attribute -> install the setter
            if isinstance(sub_attribute_value, dict) and len(sub_attribute_value) == 0:
                update_the_method(builder_instance, new_methods, setter_name, name, attribute_name_list)
            elif isinstance(sub_attribute_value, dict) and len(sub_attribute_value) > 0:
                for key, value in sub_attribute_value.iteritems():
                    setter_name_copy = list(setter_name)
                    setter_name_copy.append(key)

                    # We need to add the dict name to the attribute list and the key we are looking at now
                    attribute_name_list_copy = list(attribute_name_list)
                    attribute_name_list_copy.append(name)
                    attribute_name_list_copy.append(key)
                    create_automatic_setters_for_state(value, builder_instance, attribute_name_list_copy,
                                                       setter_name_copy, exclusions, new_methods)
            else:
                update_the_method(builder_instance, new_methods, setter_name, name, attribute_name_list)


def create_automatic_setters(builder_instance, state_class, exclusions):
    # Get the name of the state object
    new_methods = {}
    for attribute_name, attribute_value in builder_instance.__dict__.iteritems():
        if isinstance(attribute_value, state_class):
            attribute_name_list = [attribute_name]
            setter_name = ["set"]
            create_automatic_setters_for_state(attribute_value, builder_instance, attribute_name_list,
                                               setter_name, exclusions, new_methods)
    # Apply the methods
    for method_name, method in new_methods.iteritems():
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

"""
    A set of interfaces to the validators.
    
    Some validators are C++ type based and here
    we provide a consistent Python interface to allow
    the types to be deduced without the user having
    to worry about this
"""

def BoundedValidator(lower = None, upper = None):
    """
        Factory function for creating a BoundedValidator
        of the correct type base on the passed types
    """
    if lower is None and upper is None:
        raise TypeError("Cannot create a BoundedValidator with both lower and upper limit unset.")
    
    def get_validator_class(value):
        from mantid.kernel import BoundedValidator_double, BoundedValidator_long
        if type(value) == float:
            return BoundedValidator_double
        elif type(value) == int:
            return BoundedValidator_long
        else:
            raise TypeError("Unknown type passed for BoundedValidator: %s" % str(type(value)))
    #
    if lower is not None and upper is not None:
        if type(lower) == type(upper):
            cls = get_validator_class(lower)
            validator = cls(lower,upper)
        else:
            raise TypeError("Type mismatch, both lower and upper must match: %s!=%s" % (str(type(lower)),str(type(upper))))
    elif lower is not None:
        cls = get_validator_class(lower)
        validator = cls()
        validator.setLower(lower)
    else:
        cls = get_validator_class(upper)
        validator = cls()
        validator.setUpper(lower)
    return validator
    


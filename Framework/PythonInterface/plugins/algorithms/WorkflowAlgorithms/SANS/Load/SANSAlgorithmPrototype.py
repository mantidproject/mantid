# pylint: disable=no-init,invalid-name,too-many-arguments,too-few-public-methods

from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm
from mantid.kernel import Direction, Property, PropertyManagerProperty
from SANS2.State.SANSStatePrototype import SANSStatePrototype


class SANSAlgorithmPrototype(DataProcessorAlgorithm):
    def category(self):
        return 'SANS'

    def summary(self):
        return 'Prototype Algorithm'

    def PyInit(self):
        self.declareProperty(PropertyManagerProperty("SANSStatePrototype"))
        self.declareProperty('Factor', defaultValue=Property.EMPTY_DBL, direction=Direction.Input,
                             doc='Factor')

    def PyExec(self):
        # We accept the State as a dictionary
        property_manager = self.getProperty("SANSStatePrototype").value
        # A state object is created and the dict/property manager is passed into the state
        state = SANSStatePrototype()
        state.property_manager = property_manager
        print state.parameter1
        # Algorithm logic comes here

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        property_manager = self.getProperty("SANSStatePrototype").value
        try:
             state = SANSStatePrototype()
             state.property_manager = property_manager
             state.validate()
        except ValueError, e:
             errors.update({"SANSStatePrototype": str(e)})
        return errors

AlgorithmFactory.subscribe(SANSAlgorithmPrototype)

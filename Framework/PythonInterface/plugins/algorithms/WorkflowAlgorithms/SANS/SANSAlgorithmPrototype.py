# pylint: disable=no-init,invalid-name,too-many-arguments,too-few-public-methods

from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode, AnalysisDataService
from mantid.kernel import Direction, Property, StringListValidator, UnitFactory, PropertyManagerProperty
from SANSStatePrototype import SANSStatePrototype


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
        a = 1
        b = 2
        print a
        # property_manager = self.getProperty("SANSStatePrototype").value
        # state = SANSStatePrototype()
        # state.property_manager = property_manager
        #
        # print "++++++++++++++++++++++"
        # print state.parameter1
        # print state.parameter2
        print "++=====================++++++"
        # Algorithm logic comes here

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        # property_manager = self.getProperty("SANSStatePrototype").value
        # try:
        #     state = SANSStatePrototype()
        #     state.property_manager = property_manager
        #     state.validate()
        # except ValueError, e:
        #     # Would have to be more understandable
        #     errors.update({"SANSStatePrototype": str(e)})
        return errors

AlgorithmFactory.subscribe(SANSAlgorithmPrototype)

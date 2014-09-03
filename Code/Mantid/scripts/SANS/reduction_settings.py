"""
The aim of this file is to change the way in which we pass settings around
between reduction steps.  The benefits to this new method are as follows:

1) Settings become easily transferrable between the C++/Python layers.
   Previously, the only way for the GUI to get/set variables in the reducer
   was to do something like:

   QString var = runPythonScript("print i.ReductionSingleton().step.var");
   double num = var.toDouble()
   runPythonScript("i.ReductionSingleton().step.var = " + value_from_user);

   This becomes:

   double num = settings.getProperty("step_var");
   settings.setProperty("step_var", value_from_user);

   which is good since:

   a) it reduces the amount of boilerplate code;
   b) no Python scripts have to be run; and
   c) we get the type-casting and checking of the PropertyManager for free.

2) It promotes decoupling of the ReductionStep objects.  Previously, the
   only way to pass information between reduction steps was to either:

   a) store the information as variables inside the reducer; or
   b) store the information as variables inside the reduction steps.

   The problem with a) is that things get very messy very quickly when we
   have lots of variables, and the downside of b) is that we have to write
   code like:

      needed_value = reducer.some_other_reduction_step_object.variable

   leaving us with a web of interconnected objects, which defeats the point
   of having separate ReductionStep objects in the first place.

3) Decoupled objects are far easier to unit test.  Previously, the only way
   we could start unit testing would be to set up a "mock" object for every
   other step our to-be-tested ReductionStep required information from:

   class MockReductionStep1:
       var = 100

   class MockReductionStep2:
       var = "value"

   class MockReducer:
       step1 = MockReductionStep1
       step2 = MockReductionStep2

   def test_step():
       step_to_be_tested = ReductionStep()
       step_to_be_tested.execute(MockReducer())

   This would now be done by simply setting the pre-requisite information
   of the ReductionStep in the PropertyManager.

4) It gets us slightly nearer to the "modern" way of way of reducing data
   in Mantid, namely "workflow" algorithms which call child algorithms,
   passing settings to each other via PropertyManager objects.
"""

from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *

REDUCTION_SETTINGS_OBJ_NAME = "ISISSANSReductionSettings"

def get_settings_object(settings_prop_man_name=REDUCTION_SETTINGS_OBJ_NAME):
    """
    Returns the PropertyManager object with the given name.  This could be used
    to store settings used by the reduction, or to seperately store some
    temporary or backup settings.  If a PropertyManager object does not exist,
    then one is created.

    It is not a good idea to make repeated calls to this function all over over
    the ISIS SANS reduction code.  Please make a single call to this function
    and then pass around the returned object to wherever it is needed.
    """
    class PropertyManagerPicklableWrapper:
        """
        Pickling has not been enabled for PropertyManager, and this is needed
        in the Reducer so that the "deep copy" stuff does not complain.  This
        is a workaround where we wrap the functionality of PropertyManager that
        we're interested in and internally just retrieve an instance of the
        manager every time the wrapper is used.

        Also take this opportunity to change typed property-getting to the
        simpler "[]" operator rather than getProperty().value.
        """
        def __init__(self, name):
            self.name = name

        def _get_prop_man(self, name):
            if not PropertyManagerDataService.doesExist(name):
                logger.debug("Creating reduction settings PropertyManager "\
                             "object with name \"%s\"." % name)
                PropertyManagerDataService.add(name, PropertyManager())

            return PropertyManagerDataService.retrieve(name)

        def _prop_man(self):
            return self._get_prop_man(self.name)

        def __contains__(self, key):
            return key in self._prop_man()

        def __getitem__(self, key):
            return self._prop_man().getProperty(key).value

        def __setitem__(self, key, value):
            self._prop_man()[key] = value

        def __len__(self):
            return len(self._prop_man())

        def keys(self):
            return [prop.name for prop in self._prop_man().getProperties()]

        def values(self):
            return [prop.value for prop in self._prop_man().getProperties()]

        def items(self):
            return [(prop.name, prop.value) for prop in self._prop_man().getProperties()]

        def clear(self):
            PropertyManagerDataService.remove(self.name)

        def clone(self, new_name):
            if new_name == self.name:
                raise RuntimeError("Cannot clone the settings object with name \"%s\" "\
                                   "into a new object with the same name." % new_name)

            if PropertyManagerDataService.doesExist(new_name):
                PropertyManagerDataService.remove(new_name)

            new_prop_man = PropertyManagerPicklableWrapper(new_name)
            new_prop_man.clear()
            for key, value in self.items():
                new_prop_man[key] = value
            return new_prop_man

    return PropertyManagerPicklableWrapper(settings_prop_man_name)

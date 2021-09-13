from mantid.api import AlgorithmFactory, PythonAlgorithm
from mantid.kernel import logger


class MyNamePython(PythonAlgorithm):

    def category(self):
        return 'Testing'

    def alias(self):
        return 'MyAliasPython'

    def aliasExpiration(self):
        return '2020-01-01'

    def PyInit(self):
        self.declareProperty('Meaning', 42, 'Assign a meaning to the Universe')

    def PyExec(self):
        logger.error(f'The meaning of the Universe is {self.getPropertyValue("Meaning")}')


AlgorithmFactory.subscribe(MyNamePython)

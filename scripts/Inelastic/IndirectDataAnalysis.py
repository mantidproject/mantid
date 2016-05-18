#pylint: disable=invalid-name,too-many-locals,too-many-arguments

from IndirectImport import import_mantidplot
MTD_PLOT = import_mantidplot()
from IndirectCommon import *

import math, re, os.path, numpy as np
from mantid.simpleapi import *
from mantid.api import TextAxis
from mantid import *


def createFuryMultiDomainFunction(function, input_ws):
    multi= 'composite=MultiDomainFunction,NumDeriv=true;'
    comp = '(composite=CompositeFunction,NumDeriv=true,$domains=i;' + function + ');'

    ties = []
    kwargs = {}
    num_spectra = mtd[input_ws].getNumberHistograms()
    for i in range(0, num_spectra):
        multi += comp
        kwargs['WorkspaceIndex_' + str(i)] = i

        if i > 0:
            kwargs['InputWorkspace_' + str(i)] = input_ws

            #tie beta for every spectrum
            tie = 'f%d.f1.Beta=f0.f1.Beta' % i
            ties.append(tie)

    ties = ','.join(ties)
    multi += 'ties=(' + ties + ')'

    return multi, kwargs


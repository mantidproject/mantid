# pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)

from collections import OrderedDict

import sys
import glob
import numpy as np
from scipy import interpolate
import scipy.optimize as optimize
from pprint import pprint
from operator import itemgetter
import os.path
from itertools import cycle
from pprint import pformat

from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty,
                        WorkspaceGroup, WorkspaceGroupProperty, ITableWorkspaceProperty,
                        Progress, PropertyMode)
from mantid.kernel import Direction
from mantid.simpleapi import *

'''


'''


class SANSSuperImpose(PythonAlgorithm):


    def category(self):
        return 'SANS'

    def name(self):
        return "SANSSuperImpose"

    def summary(self):
        return "From a Workspace Group of I(Q) calculates I(scaled) = f * I(original) – b"

    def PyInit(self):
        # In
        self.declareProperty(
            WorkspaceGroupProperty(
                "InputWorkspaces", "", direction=Direction.Input),
            doc="I(q, wavelength) non-scaled workspaces.")

        self.declareProperty(
            WorkspaceProperty(
                'InputWorkspaceReference', '', direction=Direction.Input,
                optional=PropertyMode.Optional,
            ),
            doc='Reference Workspace from the InputWorkspaceGroup. \
            If empty uses the first position from the InputWorkspaceGroup')

        self.declareProperty(
            name='Qmin', defaultValue=Property.EMPTY_DBL,
            doc='Start of the fitting range')

        self.declareProperty(
            name='Qmax', defaultValue=Property.EMPTY_DBL,
            doc='End of the fitting range')

        self.declareProperty(
            name='DiscardBegin', defaultValue=0,
            doc='Discard n points from the beginning of every dataset')

        self.declareProperty(
            name='DiscardEnd', defaultValue=0,
            doc='Discard n points from the end of every dataset.')

        self.declareProperty(
            name='K', defaultValue=Property.EMPTY_DBL,
            doc=r'Default K value for I_{scaled}(Q) = K*I(Q)+b.')

        self.declareProperty(
            FloatArrayProperty("KList", [], direction=Direction.Input),
            "List of K values. Must be the same length has data -1" +
            r"Default b value for I_{scaled}(Q) = K*I(Q)+b.")

        self.declareProperty(
            name='B', defaultValue=Property.EMPTY_DBL,
            doc=r'Default b value for I_{scaled}(Q) = K*I(Q)+b.')

        self.declareProperty(
            FloatArrayProperty("BList", [], direction=Direction.Input),
            " List of B values. Must be the same length has data -1" +
            r"Default b value for I_{scaled}(Q) = K*I(Q)+b.")

        # Out
        self.declareProperty(
            WorkspaceGroupProperty(
                "OutputWorkspaces", "",
                direction=Direction.Output,
                optional=PropertyMode.Optional),
            doc="I(q, wavelength) scaled workspaces")
        self.declareProperty(
            ITableWorkspaceProperty(
                'OutputWorkspaceTable', '',
                optional=PropertyMode.Optional,
                direction=Direction.Output),
            doc='Table workspace of fit parameters')

    def _setup(self):
        '''
        Sets the properties as method properties
        '''

        self.input_wss = self.getProperty('InputWorkspaces').value
        self.input_ws_reference = self.getProperty(
            'InputWorkspaceReference').value

        self.q_min = None if self.getProperty('Qmin').value == Property.EMPTY_DBL else self.getProperty('Qmin').value
        self.q_max = None if self.getProperty('Qmax').value == Property.EMPTY_DBL else self.getProperty('Qmax').value

        self.discard_begin = self.getProperty('DiscardBegin').value
        self.discard_end = self.getProperty('DiscardEnd').value

        self._k = None if self.getProperty('K').value == Property.EMPTY_DBL else self.getProperty('K').value
        self.k_list = None if len(self.getProperty('KList').value) ==0 else cycle(self.getProperty('KList').value)

        self._b = None if self.getProperty('B').value == Property.EMPTY_DBL else self.getProperty('B').value
        self.b_list = None if len(self.getProperty('BList').value) ==0 else cycle(self.getProperty('BList').value)

        self.out_wss_name = self.getPropertyValue("OutputWorkspaces")
        self.out_ws_table_name = self.getPropertyValue("OutputWorkspaceTable")

    def _ws_to_dict(self):
        d = OrderedDict()
        for ws in self.input_wss:
            name = ws.name()
            x_bins = ws.readX(0)
            x = [x_bins[i] + (abs(x_bins[i]-x_bins[i+1])/2.0) for i in range(len(x_bins)-1)]
            d[name] = {
                'x': np.array(x),
                'y': ws.readY(0),
                'e': ws.readE(0),
                'f': interpolate.interp1d(x, y),
            }
        return d

    def _find_q_range(self):
        '''
        Loop over all WS, find mins and maxs
        and find a :
            common minimal (max of all minimums)
            common maximal (min of all maximums)
        @return: Q range [min,max]
        '''

        mins =[]
        maxs = []
        for _, v in self.data.items():
            # X minimum when Y > 0
            x = v['x']
            y = v['y']
            mins.append(x[y>0].min())
            maxs.append(x[y>0].max())
        x_min = np.max(mins)
        x_max = np.min(maxs)
        if self.q_min and self.q_min > x_min:
            x_min = self.q_min
        if self.q_max and self.q_max < x_max:
            x_max = self.q_max
        return x_min, x_max

    @staticmethod
    def _residuals(p, x, f_target, f_to_optimise, k=None, b=None):
        """
        k and p are mutually exclusive (they cannot be both None)
        """
        if k is not None:
            b = p[0]
        elif b is not None:
            k = p[0]
        else:
            k, b = p
        err = f_target(x) - (k * f_to_optimise(x) - b)
        # return np.sum(err**2)
        return err

    @staticmethod
    def _peval(x, f, p, k=None, b=None):
        if k is not None:
            b = p[0]
        elif b is not None:
            k = p[0]
        else:
            k, b = p
        return k * f(x) - b

    @property
    def b(self):
        '''
        Get _b value if it exists
        if the b_list exists get the next element from b_list
        otherwise returns None
        '''
        if self._b:
            return self._b
        elif self.b_list:
            return next(self.b_list)
        else:
            return None
    
    @property
    def k(self):
        if self._k:
            return self._k
        elif self.k_list:
            return next(self.k_list)
        else:
            return None



    def PyExec(self):

        self._setup()
        self.data = self._ws_to_dict()
        q_min, q_max = self._find_q_range()
        logger.debug("q_min = %s, q_max = %s." %(q_min, q_max ))

        if self.input_ws_reference:
            input_ws_reference_name = self.input_ws_reference.name()
        else:
            input_ws_reference_name = self.input_wss[0].name()
        logger.debug("Reference WS: %s" % input_ws_reference_name)
        
        progress = Progress(self, 0.0, 0.05, 3)
        for k, v in self.data.items():
            q_space = np.linspace(q_min, q_max, 100)
            if k != input_ws_reference_name:

                logger.information('Running Fitting for workspace: %s' % ws)
                progress.report('Running Fitting for workspace: %s' % ws)

                logger.debug("K = %s" % self.k)
                logger.debug("B = %s" % self.b)
            

                plsq, cov, infodict,mesg, ier = optimize.leastsq(
                    self._residuals,
                    ([1,1] if self.k is None and self.b is None else [1]), # guess
                    args=(
                        q_space,
                        self.data[input_ws_reference_name]['f'], # reference interpolation function
                        v['f'],
                        self.k,self.b),
                    full_output=True)
                logger.debug(mesg)











        # Workspace Table
        outws = CreateEmptyTableWorkspace(
            OutputWorkspace=self.out_ws_table_name)
        columns = ["IQCurve", "K", "KError", "B", "BError"]

        outws.addColumn(type="str", name=columns[0])
        for col in columns[1:]:
            outws.addColumn(type="double", name=col)

        row = {
            "IQCurve": 'xpto',
            "K": 1,
            "KError": 0.1,
            "B": 2,
            "BError": 0.2
        }
        outws.addRow(row)
        self.setProperty("OutputWorkspaceTable", outws)
        return


##########################################################################


AlgorithmFactory.subscribe(SANSSuperImpose)

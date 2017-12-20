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

        self.declareProperty(
            name="OutputWorkspacePrefix",
            defaultValue="out_ws",
            direction=Direction.Input,
            doc="Optional Prefix for the output I(q, wavelength) scaled workspaces")
        
        # Out
        self.declareProperty(
            ITableWorkspaceProperty(
                'OutputWorkspaceTable', 'out_table',
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

        self.out_ws_table_name = self.getPropertyValue("OutputWorkspaceTable")

    def validateInputs(self):
        '''
        Called before everything else to make sure the inputs are valid
        '''
        issues = dict()

        if len(self.getProperty('BList').value) > 0 and len(self.getProperty('BList').value) != len(self.getProperty('InputWorkspaces').value)-1:
            message = "The length of B List Parameters must be equal to the length of the input workspaces - 1"
            issues['BList'] = message
        
        if len(self.getProperty('KList').value) > 0 and len(self.getProperty('KList').value) != len(self.getProperty('InputWorkspaces').value)-1:
            message = "The length of K List Parameters must be equal to the length of the input workspaces - 1"
            issues['KList'] = message

        return issues


    def _ws_to_dict(self):
        '''
        Convert the input grouped workspaces into a OrderedDict with the values
        of interest
        '''
        d = OrderedDict()
        for ws in self.input_wss:
            name = ws.name()
            logger.debug("Putthing WS %s in a dictionary..."%(name))
            x_bins = ws.readX(0)
            x = [x_bins[i] + (abs(x_bins[i]-x_bins[i+1])/2.0) for i in range(len(x_bins)-1)]
            # Values to np arrays
            x = np.array(x)
            y = ws.readY(0)
            e = ws.readE(0)

            d[name] = {
                'x': x,
                'y': y,
                'e': e,
                'f': interpolate.interp1d(x, y),
            }
        return d


    def _find_q_space(self):
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


    def _trim_data(self, q_min, q_max):
        '''
        trimmed is cutoff by q
        '''
        
        for _, v in self.data.items():
            x = v['x']
            y = v['y']
            e = v['e']
            # Getting rid of values where y <= 0
            x_positive = x[y>0]
            e_positive = e[y>0]
            y_positive = y[y>0]

            # Trimming of the q range
            e_trimmed = e[(x>=q_min)&(x<=q_max)]
            y_trimmed = y[(x>=q_min)&(x<=q_max)]
            x_trimmed = x[(x>=q_min)&(x<=q_max)]

            # discard aditional points
            if self.discard_end > 0:
                x_trimmed = x_trimmed[self.discard_begin:-self.discard_end]
                y_trimmed = y_trimmed[self.discard_begin:-self.discard_end]
                e_trimmed = e_trimmed[self.discard_begin:-self.discard_end]
            else:
                x_trimmed = x_trimmed[self.discard_begin:]
                y_trimmed = y_trimmed[self.discard_begin:]
                e_trimmed = e_trimmed[self.discard_begin:]

            v.update({
                'x_trimmed': x_trimmed,
                'y_trimmed': y_trimmed,
                'e_trimmed': e_trimmed,
                'x_positive': x_positive,
                'y_positive': y_positive,
                'e_positive': e_positive,
            })


    @staticmethod
    def _residuals(p, x, sigma, f_target, f_to_optimise, k=None, b=None):
        """
        k and p are mutually exclusive (they cannot be both None)
        """
        if k is not None:
            b = p[0]
        elif b is not None:
            k = p[0]
        else:
            k, b = p
        # residual = f_target(x) - (k * f_to_optimise(x) - b)

        residual = 1.0 / sigma * ( f_target(x) - (k * f_to_optimise(x) - b) )

        # return np.sum(err**2)
        # Leastsq expects that the residual function returns just the residual
        return residual

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

    def _fitting(self, input_ws_reference_name):
        
        progress = Progress(self, start=0.0, end=1.0, nreports=len(self.data.items()))
        for k, v in self.data.items():
            logger.notice("Fiting WS %s against %s." %(k, input_ws_reference_name))
            progress.report('Running Fitting for workspace: %s' % k)
            if k != input_ws_reference_name:

                logger.debug('Running Fitting for workspace %s with reference %s (K=%s, B-%s)' %
                    (k, input_ws_reference_name, self.k, self.b))

                plsq, cov, infodict, mesg, ier = optimize.leastsq(
                    self._residuals,
                    ([1,1] if self.k is None and self.b is None else [1]), # guess
                    args=(
                        # _residuals(p, x, sigma, f_target, f_to_optimise, k=None, b=None):
                        v['x_trimmed'],
                        v['e_trimmed'],
                        self.data[input_ws_reference_name]['f'], # reference interpolation function
                        v['f'],
                        self.k, self.b),
                    full_output=True)
                
                # Goodness of Fit Estimator
                ss_err = (infodict['fvec']**2).sum() # infodict['fvec'] is the array of residuals:
                ss_tot = ((y-y.mean())**2).sum()
                r_squared = 1 - (ss_err/ss_tot)

                y_trimmed_fit = self._peval(v['x_trimmed'], v['f'], plsq, k=self.k, b=self.b)
                y_positive_fit = self._peval(v['x_positive'], v['f'], plsq, k=self.k, b=self.b)
                y_fit = self._peval(v['x'], v['f'], plsq, k=self.k, b=self.b)
                v.update({
                    'y_trimmed_fit': y_trimmed_fit,
                    'y_positive_fit': y_positive_fit,
                    'y_fit': y_fit,
                    'plsq': plsq,
                    'cov' : cov,
                    'r_squared': r_squared,
                })
            else:
                v.update({
                    'y_trimmed_fit': v['f'](v['x_trimmed']),
                    'y_positive_fit': v['f'](v['x_positive']),
                    'y_fit': v['f'](v['x']),
                    'plsq': None,
                    'cov' : None,
                    'r_squared': 0,
                })
    
    def _create_grouped_wss(self):
        '''
        Create the two grouped workspaces for the fitted data for the whole x
        and for the minimal overlap q range
        '''
        out_ws_list = []
        for k, v in self.data.items():
            ws_name = "_" + k + "_trimmed_fit"
            CreateWorkspace(
                OutputWorkspace=ws_name,
                DataX=list(v['x_trimmed']), DataY=list(v['y_trimmed_fit']),
                UnitX="Q")
            out_ws_list.append(ws_name)
        prefix = self.getProperty("OutputWorkspacePrefix").value
        GroupWorkspaces(InputWorkspaces=out_ws_list, OutputWorkspace=prefix + '_trimmed_fit')

        out_ws_list = []
        for k, v in self.data.items():
            ws_name = "_" + k + "_positive_fit"
            CreateWorkspace(
                OutputWorkspace=ws_name,
                DataX=list(v['x_positive']), DataY=list(v['y_positive_fit']),
                UnitX="Q")
            out_ws_list.append(ws_name)
        prefix = self.getProperty("OutputWorkspacePrefix").value
        GroupWorkspaces(InputWorkspaces=out_ws_list, OutputWorkspace=prefix + '_positive_fit')

        out_ws_list = []
        for k, v in self.data.items():
            ws_name = "_" + k + "_fit"
            CreateWorkspace(
                OutputWorkspace=ws_name,
                DataX=list(v['x']), DataY=list(v['y_fit']),
                UnitX="Q")
            out_ws_list.append(ws_name)
        prefix = self.getProperty("OutputWorkspacePrefix").value
        GroupWorkspaces(InputWorkspaces=out_ws_list, OutputWorkspace=prefix + '_fit')
        

    def _create_table_ws(self):
        '''
        '''
        # Workspace Table
        outws = CreateEmptyTableWorkspace(
            OutputWorkspace=self.out_ws_table_name)
        columns = ["IQCurve", "K", "KError", "B", "BError"]

        outws.addColumn(type="str", name=columns[0])
        for col in columns[1:]:
            outws.addColumn(type="double", name=col)

        for name, v in self.data.items():
            if v['plsq'] is None and v['cov'] is None:
                # Reference dataset case
                row = {
                    "IQCurve": name,
                    "K": 1,
                    "KError": 0,
                    "B": 0,
                    "BError": 0,
                }
            else:
                if v['cov'].any():
                    errors = np.sqrt(np.diag(v['cov']))
                else:
                    errors = [-1, -1]
                k = self.k
                b = self.b
                if k is None and b is None:
                    # both k and b fitted
                    row = {
                        "IQCurve": name,
                        "K":  v['plsq'][0],
                        "KError": errors[0],
                        "B":  v['plsq'][1],
                        "BError": errors[1],
                    }
                elif b is None:
                    # b was fitted
                    row = {
                        "IQCurve": name,
                        "K": k,
                        "KError": 0,
                        "B": v['plsq'][0],
                        "BError": errors[0],
                    }
                elif k is None:
                    # k was fitted
                    row = {
                        "IQCurve": name,
                        "K": v['plsq'][0],
                        "KError": errors[0],
                        "B": b,
                        "BError": 0,
                    }
            logger.debug("%s"%row)
            outws.addRow(row)
        self.setProperty("OutputWorkspaceTable", outws)

    def PyExec(self):

        logger.debug("Reading input properties")
        self._setup()

        logger.debug("Transforming input WSs in OrderedDict")
        self.data = self._ws_to_dict()

        logger.debug("Finding common Q space for all datasets")
        q_min, q_max = self._find_q_space()
        logger.debug("q_min = %s, q_max = %s." %(q_min, q_max ))

        self._trim_data(q_min, q_max)
        
        if self.input_ws_reference:
            input_ws_reference_name = self.input_ws_reference.name()
        else:
            logger.information("Reference WS empty. Using the first WS in the group as reference.")
            input_ws_reference_name = self.input_wss[0].name()
        logger.debug("Reference WS: %s" % input_ws_reference_name)

        self._fitting(input_ws_reference_name)

        self._create_grouped_wss()

        self._create_table_ws()

        return


##########################################################################


AlgorithmFactory.subscribe(SANSSuperImpose)

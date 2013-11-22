'''*WIKI* 

This fitting function models the dynamics structure factor of a particle undergoing
discrete jumps on N-sites evenly distributed in a circle. The particle can only
jump to neighboring sites. This is the most common type of discrete rotational diffusion
in a circle.

The fitting parameters are the inverse of the transition rate, <math>\tau</math>
and the circle radius <math>r</math> 

<math> S(Q,E) = A_0(Q,r) \delta (\omega) + \frac{1}{\pi} \sum_{l=1}^{N-1} A_l (Q,r) \frac{\tau_l}{1+(\omega \tau_l)^2} </math}

<math> A_l(Q,r) = \frac{1}{N} \sum_{k=1}^{N} j_0( 2 Q r sin(\frac{{\pi k}{N}) ) cos(\frac{2\pi lk}{N}) </math>

<math> \tau_l^{-1} = 4 \tau^{-1} sin^2(\frac{\pi l}{N}) </math>

If the energy units of energy are micro-eV, then tau is expressed in pico-seconds. If E-units are micro-eV then
tau is expressed in nano-seconds.
*WIKI*
    
@author Jose Borreguero, NScD
@date November 22, 2013

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
'''

from mantid.api import IFunction1D, FunctionFactory #, BoundaryConstraint
from mantid import logger
import numpy as np
import copy

from pdb import set_trace as tr

class DiffRotDiscreteCircleElastic(IFunction1D):
    '''Elastic part of the structure factor
    '''
    def __init__(self):
        '''declare some constants'''
        super(DiffRotDiscreteCircleElastic, self).__init__()
        self._meV2ps = 4.136
        self._parmset = set(['height','tau','beta'])     #valid syntaxfor python >= 2.6
        self._parm2index = {'height':0,'tau':1,'beta':2} #order in which they were defined

    def category(self):
        return 'QENS'
    
    

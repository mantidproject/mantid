from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import numpy as np
import os.path
import re


class CutMD(DataProcessorAlgorithm):
    
    def category(self):
        return 'MDAlgorithms'

    def summary(self):
        return 'Slices multidimensional workspaces using input projection information and binning limits.'

    def PyInit(self):
        self.declareProperty(IMDEventWorkspaceProperty('InputWorkspace', '', direction=Direction.Input),
                             doc='MDWorkspace to slice')
        
        self.declareProperty(ITableWorkspaceProperty('Projection', '', direction=Direction.Input, optional = PropertyMode.Optional), doc='Projection')
        
        self.declareProperty(FloatArrayProperty(name='P1Bin', values=[]),
                             doc='Projection 1 binning')
        
        self.declareProperty(FloatArrayProperty(name='P2Bin', values=[]),
                             doc='Projection 2 binning')
        
        self.declareProperty(FloatArrayProperty(name='P3Bin', values=[]),
                             doc='Projection 3 binning')
        
        self.declareProperty(FloatArrayProperty(name='P4Bin', values=[]),
                             doc='Projection 4 binning')

        self.declareProperty(IMDWorkspaceProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='Output cut workspace')
        
        self.declareProperty(name="NoPix", defaultValue=False, doc="If False creates a full MDEventWorkspaces as output. True to create an MDHistoWorkspace as output. This is DND only in Horace terminology.")
        
        self.declareProperty(name="CheckAxes", defaultValue=True, doc="Check that the axis look to be correct, and abort if not.")
        
        
    def __to_mantid_slicing_binning(self, horace_binning, to_cut, dimension_index):
        
        dim = to_cut.getDimension(dimension_index)
        dim_min = dim.getMinimum()
        dim_max = dim.getMaximum()
        n_arguments = len(horace_binning)
        if n_arguments == 0:
            raise ValueError("binning parameter cannot be empty")
        elif n_arguments == 1:
            step_size = horace_binning[0]
            n_bins = int( (dim_max - dim_min) / step_size)
            # Calculate the maximum based on step size and number of bins
            dim_max = dim_min + ( n_bins * step_size )
        elif n_arguments == 2:
            dim_min = horace_binning[0]
            dim_max = horace_binning[1]
            n_bins = 1
        elif n_arguments == 3:
            dim_min = horace_binning[0]
            dim_max = horace_binning[2]
            step_size = horace_binning[1]
            n_bins = int( (dim_max - dim_min) / step_size)
            dim_max = dim_min + ( n_bins * step_size )
            #if dim_max != horace_binning[2]:
            #    pass # TODO, we should generate a warning at this point.
        else:
            raise ValueError("Too many arguments given to the binning parameter")
        if dim_min >= dim_max:
            raise ValueError("Dimension Min >= Max value. Min %.2f Max %.2f", min, max)
        if n_bins < 1:
            raise ValueError("Number of bins calculated to be < 1")
        return (dim_min, dim_max, n_bins)
    
    
    def __innermost_boundary(self, a, b):
         if np.absolute(a) < np.absolute(b):
             return b
         return b
     
    def __calculate_inner_extents(self, boundaries_a, boundaries_b, ws):
        
        extents = (self.__innermost_boundary(boundaries_a[0], boundaries_b[0]), 
                   self.__innermost_boundary(boundaries_a[1], boundaries_b[1]), 
                   self.__innermost_boundary(boundaries_a[2], boundaries_b[2]),
                   self.__innermost_boundary(boundaries_a[3], boundaries_b[3]),
                   self.__innermost_boundary(boundaries_a[4], boundaries_b[4]),
                   self.__innermost_boundary(boundaries_a[5], boundaries_b[5])
                   )
        
            # Copy extents for non crystallographic dimensions    
        non_crystallographic_dimensions = ws.getNumDims() - 3
        if non_crystallographic_dimensions > 0:
            for i in range(0, non_crystallographic_dimensions):
                extents.append(ws.getDimension(i + 3).getMinimum())
                extents.append(ws.getDimension(i + 3).getMaximum())
        
        return extents
    
    
    def __calculate_extents(self, v, u, w, limits):
        M=np.array([u,v,w])
        Minv=np.linalg.inv(M)
    
        # unpack limits
        Hrange, Krange, Lrange = limits
    
        # Create a numpy 2D array. Makes finding minimums and maximums for each transformed coordinates over every corner easier.
        new_coords = np.empty([8, 3])
        counter = 0
        for h in Hrange:
           for k in Krange:
               for l in Lrange:
                   original_corner=np.array([h,k,l])
                   new_coords[counter]=original_corner.dot(Minv)
                   counter += 1
        
        # Get the min max extents
        extents = list()
        for i in range(0,3):
            # Vertical slice down each corner for each dimension, then determine the max, min and use as extents
            extents.append(np.amin(new_coords[:,i]))
            extents.append(np.amax(new_coords[:,i]))
        
        return extents
    
    def __uvw_from_projection_table(self, projection_table):
        if not isinstance(projection_table, ITableWorkspace):
            I = np.identity(3)
            return (I[0, :], I[1, :], I[2, :])
        column_names = projection_table.getColumnNames()
        u = np.array(projection_table.column('u'))
        v = np.array(projection_table.column('v'))
        if not 'w' in column_names:
            w = np.cross(v,u)
        else:
            w = np.array(projection_table.column('w'))
        return (u, v, w)
    
    def __make_labels(self, projection):

        class Mapping:
            
            def __init__(self, replace):
                self.__replace = replace
                
            def replace(self, entry):
                if np.absolute(entry) == 1:
                    if entry > 0:
                        return self.__replace 
                    else:
                        return "-" + self.__replace
                elif entry == 0:
                    return 0
                else:
                    return "%.2f%s" % ( entry, self.__replace )
                return entry
                
        crystallographic_names = ['zeta', 'eta', 'xi' ]  
        labels = list()
        for i in range(len(projection)):
            cmapping = Mapping(crystallographic_names[i])
            labels.append( [cmapping.replace(x) for x in projection[i]  ] )
        
        return labels
    
    
    def __verify_input_workspace(self, to_cut):
        coord_system = to_cut.getSpecialCoordinateSystem()
        if not coord_system == SpecialCoordinateSystem.HKL:
            raise ValueError("Input Workspace must be in reciprocal lattice dimensions (HKL)")
        
        ndims = to_cut.getNumDims()
        if ndims < 3 or ndims > 4:
            raise ValueError("Input Workspace should be 3 or 4 dimensional")
        
        # Try to sanity check the order of the dimensions. This is important.
        axes_check = self.getProperty("CheckAxes").value
        if axes_check:
            predicates = ["^(H.*)|(\\[H,0,0\\].*)$","^(K.*)|(\\[0,K,0\\].*)$","^(L.*)|(\\[0,0,L\\].*)$"]  
            for i in range(ndims):
                dimension = to_cut.getDimension(i)
                p = re.compile(predicates[i])
                if not p.match( dimension.getName() ):
                    raise ValueError("Dimensions must be in order H, K, L")

    def __verify_projection_input(self, projection_table):
        if isinstance(projection_table, ITableWorkspace):
            column_names = set(projection_table.getColumnNames())
            logger.warning(str(column_names)) 
            if not column_names == set(['u', 'v', 'type']):
                    if not column_names == set(['u', 'v', 'offsets', 'type']):
                        if not column_names == set(['u', 'v', 'w', 'offsets', 'type']):
                            raise ValueError("Projection table schema is wrong! Column names received: " + str(column_names) )
            if projection_table.rowCount() != 3:
                raise ValueError("Projection table expects 3 rows")
        
    def PyExec(self):
        to_cut = self.getProperty("InputWorkspace").value
        self.__verify_input_workspace(to_cut)
        ndims = to_cut.getNumDims()
        
        nopix = self.getProperty("NoPix").value
        
        projection_table = self.getProperty("Projection").value
        self.__verify_projection_input(projection_table)
            
        p1_bins = self.getProperty("P1Bin").value
        p2_bins = self.getProperty("P2Bin").value
        p3_bins = self.getProperty("P3Bin").value
        p4_bins = self.getProperty("P4Bin").value 
        
        # TODO. THESE ARE WRONG. Need to consider the actual transformed extents as part of this.
        xbins = self.__to_mantid_slicing_binning(p1_bins, to_cut, 0);
        ybins = self.__to_mantid_slicing_binning(p2_bins, to_cut, 1);
        zbins = self.__to_mantid_slicing_binning(p3_bins, to_cut, 2); 
        bins = [ int(xbins[2]), int(ybins[2]), int(zbins[2]) ]
        if p4_bins:
            if (ndims == 4):
                ebins = self.__to_mantid_slicing_binning(p1_bins, to_cut, 3); 
                bins.append(int(ebins[2]))
            else:
                raise ValueError("Cannot specify P4Bins unless the workspace is of sufficient dimensions")
        
        projection = self.__uvw_from_projection_table(projection_table)
        u,v,w = projection
         
        h_limits_ws = (to_cut.getDimension(0).getMinimum(), to_cut.getDimension(0).getMaximum())
        k_limits_ws = (to_cut.getDimension(1).getMinimum(), to_cut.getDimension(1).getMaximum())
        l_limits_ws = (to_cut.getDimension(2).getMinimum(), to_cut.getDimension(2).getMaximum())
        
        extents_by_ws = self.__calculate_extents(v, u, w, (h_limits_ws, k_limits_ws, l_limits_ws))
        
        extents_by_bin_limits = self.__calculate_extents(v, u, w, ( (xbins[0], xbins[1]), (ybins[0], ybins[1]), (zbins[0], zbins[1])))
        
        extents = self.__calculate_inner_extents(extents_by_ws, extents_by_bin_limits, to_cut)
        
        projection_labels = self.__make_labels(projection)
        
        '''
        Actually perform the binning operation
        '''
        cut_alg_name = "BinMD" if nopix else "SliceMD"
        cut_alg = AlgorithmManager.Instance().create(cut_alg_name)
        cut_alg.setChild(True)
        cut_alg.initialize()
        cut_alg.setProperty("InputWorkspace", to_cut)
        cut_alg.setPropertyValue("OutputWorkspace", "sliced")
        cut_alg.setProperty("NormalizeBasisVectors", False)
        cut_alg.setProperty("AxisAligned", False)
        # Now for the basis vectors.
        for i in range(0, to_cut.getNumDims()):
            if i <= 2:
                label = projection_labels[i]
                unit = "TODO" # Haven't figured out how to do this yet.
                vec = projection[i]
                value = "%s, %s, %s" % ( label, unit, ",".join(map(str, vec))) 
                cut_alg.setPropertyValue("BasisVector{0}".format(i) , value)
            if i > 2:
                raise RuntimeError("Not implmented yet for non-crystallographic basis vector generation.")
        cut_alg.setProperty("OutputExtents", extents)
        cut_alg.setProperty("OutputBins", bins)
         
        cut_alg.execute()
        slice = cut_alg.getProperty("OutputWorkspace").value
        self.setProperty("OutputWorkspace", slice)
        
AlgorithmFactory.subscribe(CutMD)

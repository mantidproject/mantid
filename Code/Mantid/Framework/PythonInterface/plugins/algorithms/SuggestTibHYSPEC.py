#pylint: disable=no-init
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import FloatBoundedValidator,Direction,logger
import numpy


class SuggestTibHYSPEC(PythonAlgorithm):
    """ Check if certain sample logs exists on a workspace
    """
    def category(self):
        """ Return category
        """
        return "PythonAlgorithms;Utility;Inelastic"

    def name(self):
        """ Return name
        """
        return "SuggestTibHYSPEC"

    def summary(self):
        """ Return summary
        """
        return "Suggest possible time independent background range for HYSPEC"

    def PyInit(self):
        """ Declare properties
        """
        val=FloatBoundedValidator()
        val.setBounds(3,100) #reasonable incident nergy range for HYSPEC
        self.declareProperty("IncidentEnergy",0.,val,"Incident energy (3 to 100 meV)")
        self.declareProperty("TibMin",0.,Direction.Output)
        self.declareProperty("TibMax",0.,Direction.Output)
        return

    def e2v(self,energy):
        return numpy.sqrt(energy/5.227e-6)

    def PyExec(self):
        """ Main execution body
        """
        #get parameter
        energy = self.getProperty("IncidentEnergy").value

        msd=1800.0
        tail_length_us = 3000.0
        dist_mm = 39000.0 + msd + 4500.0
        T0_moderator = 0.0
        t_focEle_us = 39000.0 / self.e2v(energy) * 1000.0 + T0_moderator
        t_samp_us = (dist_mm - 4500.0) / self.e2v(energy) * 1000.0 + T0_moderator
        t_det_us = dist_mm /self.e2v(energy) * 1000 + T0_moderator
        frame_start_us = t_det_us - 16667/2
        frame_end_us = t_det_us + 16667/2
        index_under_frame = numpy.divide(int(t_det_us),16667)
        pre_lead_us = 16667 * index_under_frame
        pre_tail_us = pre_lead_us + tail_length_us
        post_lead_us = 16667 * (1+ index_under_frame)
        #post_tail_us = post_lead_us + tail_length_us
        #E_final_meV = -1
        #E_transfer_meV = -1
        # finding an ok TIB range
        MinTIB_us = 2000.0
        slop_frac = 0.2
        #print t_focEle_us,pre_lead_us,frame_start_us,MinTIB_us,slop_frac
        if (t_focEle_us < pre_lead_us) and (t_focEle_us-frame_start_us > MinTIB_us * (slop_frac + 1.0)):
            logger.debug('choosing TIB just before focus element-1')
            TIB_high_us = t_focEle_us - MinTIB_us * slop_frac / 2.0
            TIB_low_us = TIB_high_us - MinTIB_us
        elif (frame_start_us>pre_tail_us) and (t_focEle_us-frame_start_us > MinTIB_us * (slop_frac + 1.0)):
            logger.debug('choosing TIB just before focus element-2')
            TIB_high_us = t_focEle_us - MinTIB_us * slop_frac / 2.0
            TIB_low_us = TIB_high_us - MinTIB_us
        elif t_focEle_us-pre_tail_us > MinTIB_us * (slop_frac + 1.0) and (t_focEle_us-frame_start_us > MinTIB_us * (slop_frac + 1.0)):
            logger.debug('choosing TIB just before focus element-3')
            TIB_high_us = t_focEle_us - MinTIB_us * slop_frac / 2.0
            TIB_low_us = TIB_high_us - MinTIB_us
        elif t_samp_us-pre_tail_us > MinTIB_us * (slop_frac + 1.0) and (t_samp_us-frame_start_us > MinTIB_us * (slop_frac + 1.0)):
            logger.debug('choosing TIB just before sample-1')
            TIB_high_us = t_samp_us - MinTIB_us * slop_frac / 2.0
            TIB_low_us = TIB_high_us - MinTIB_us
        elif t_samp_us-pre_tail_us > MinTIB_us / 1.5 * (slop_frac + 1.0) and (t_samp_us-frame_start_us > MinTIB_us * (slop_frac + 1.0)):
            logger.debug('choosing TIB just before sample-2')
            TIB_high_us = t_samp_us - MinTIB_us / 1.5 * slop_frac / 2.0
            TIB_low_us = TIB_high_us - MinTIB_us / 1.5
        elif t_samp_us-pre_tail_us > MinTIB_us / 2.0 * (slop_frac + 1.0) and (t_samp_us-frame_start_us > MinTIB_us * (slop_frac + 1.0)):
            logger.debug('choosing TIB just before sample-3')
            TIB_high_us = t_samp_us - MinTIB_us / 2.0 * slop_frac / 2.0
            TIB_low_us = TIB_high_us - MinTIB_us / 2.0
        elif (pre_lead_us - frame_start_us > MinTIB_us * (slop_frac + 1.0)) and (t_focEle_us > pre_lead_us):
            logger.debug('choosing TIB just before leading edge before elastic-1')
            TIB_high_us = pre_lead_us - MinTIB_us * slop_frac / 2.0
            TIB_low_us = TIB_high_us - MinTIB_us
        elif (pre_lead_us - frame_start_us > MinTIB_us / 1.5 * (slop_frac + 1.0)) and (t_focEle_us > pre_lead_us):
            logger.debug('choosing TIB just before leading edge before elastic-2')
            TIB_high_us = pre_lead_us - MinTIB_us / 1.5 * slop_frac / 2.0
            TIB_low_us = TIB_high_us - MinTIB_us / 1.5
        elif (pre_lead_us - frame_start_us > MinTIB_us / 2.0 * (slop_frac + 1.0)) and (t_focEle_us > pre_lead_us):
            logger.debug('choosing TIB just before leading edge before elastic-3')
            TIB_high_us = pre_lead_us - MinTIB_us / 2.0 * slop_frac / 2.0
            TIB_low_us = TIB_high_us - MinTIB_us / 2.0
        # elif (pre_tail_us > frame_start_us) and (t_focEle_us - pre_tail_us > MinTIB_us * (slop_frac + 1.0)):
        #   logger.debug('choosing TIB just before focus element')
        # TIB_low_us = pre_tail_us + MinTIB_us * slop_frac / 2.0
        # TIB_high_us = TIB_low_us + MinTIB_us
        elif post_lead_us > frame_end_us:
            logger.debug('choosing TIB at end of frame')
            TIB_high_us = frame_end_us - MinTIB_us * slop_frac / 2.0
            TIB_low_us = TIB_high_us - MinTIB_us
        elif post_lead_us - t_det_us > MinTIB_us * (slop_frac + 1.0):
            logger.debug('choosing TIB between elastic peak and later prompt pulse leading edge')
            TIB_high_us = post_lead_us - MinTIB_us * slop_frac / 2.0
            TIB_low_us = TIB_high_us - MinTIB_us
        else:
            logger.debug('I cannot find a good TIB range')
            TIB_low_us = 0.0
            TIB_high_us = 0.0


        #return the result
        self.setProperty("TibMin",TIB_low_us)
        self.setProperty("TibMax",TIB_high_us)
        return


AlgorithmFactory.subscribe(SuggestTibHYSPEC)

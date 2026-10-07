- The ``ReflectometryISISPreprocess`` algorithm no longer applies detector calibration. The
  :ref:`ISIS Reflectometry Interface <interface-isis-refl>` instead applies calibration through
  :ref:`ReflectometryISISLoadAndProcess <algm-ReflectometryISISLoadAndProcess>` after workspace summation. The detector
  image and TOF plot in the reduction preview now display the raw workspace instead of the calibrated workspace, but no
  visual impact is expected because calibration changes detector geometry rather than the plotted signal.

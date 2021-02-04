import numpy as np
from matplotlib import pyplot as plt
from matplotlib.pyplot import figure
figure(num=None, figsize=(10, 6), dpi=80, facecolor='w', edgecolor='k')

# PDCalib
# peak_pos   obs_peak_pos     diff_peak_pos            max_y 			fwhm
pdcali_dataset = np.array([
                [0.63073,  0.6302079802588544,   -0.0005220197411456295 ,    506140.7526937912  ,  0.0025186480018594077],
                [0.68665,  0.6861197932366541,   -0.0005302067633459195 ,    1200084.9966796965 ,  0.002909060187412868 ],
                [0.7283 ,  0.7278011592753663,   -0.0004988407246336868 ,    2233345.5132392216 ,  0.003158982426636293 ],
                [0.81854,  0.8179916496150637,   -0.0005483503849363913 ,    2118235.987741105  ,  0.003681742883835462 ],
                [0.89198,  0.8913700005885286,   -0.0006099994114714402 ,    1491433.3408635673 ,  0.0041170219084159364],
                [1.07577,  1.0750548607012353,   -0.0007151392987645444 ,    4594774.237938608  ,  0.0051959316185726   ],
                [1.26146,  1.2607045243642978,   -0.0007554756357022185 ,    5363207.14846125   ,  0.006312789046539739 ]])

# Raw
raw_dataset = np.array([
                [0.63073,  0.6306584407539415,    -7.155924605850572e-05,     502390.52649700100,  0.0025428080077637060],
                [0.68665,  0.6866202654591979,   -2.9734540802128784e-05,     1188528.3066476902,  0.0029342154483606555],
                [0.7283 ,  0.7283330305616232,    3.3030561623292876e-05,     2216586.3145334306,  0.0031876003461174585], 
                [0.81854,  0.8185840388198395,    4.403881983949809e-05 ,    2097915.011775188  ,  0.0037146904611487013],
                [0.89198,  0.8920119378927177,    3.1937892717714966e-05,     1481189.2463500726,  0.004153917343450177 ],
                [1.07577,  1.0758565423476987,    8.654234769878144e-05 ,    4552111.530816161  ,  0.005246067189981423 ],
                [1.26146,  1.2616343350137238,    0.0001743350137237698 ,    5307894.289965995  ,  0.006370769402903865 ]])
                                                                                                     
# CC Mask
cc_mask_dataset = np.array([
                [0.63073,  0.6307746485040662,   4.464850406615817e-05  ,   506169.5504647702   ,  0.0025213856425225   ],
                [0.68665,  0.6867369869111138,   8.698691111386214e-05  ,   1199658.4954200473  ,  0.002912490086842116 ],
                [0.7283 ,  0.7284548785931801,   0.0001548785931801966  ,   2232952.2678632154  ,  0.0031636707269495044],
                [0.81854,  0.818731009401112 ,   0.0001910094011119412  ,   2117882.74204468    ,  0.0036867550347407384],
                [0.89198,  0.8921793639816501,   0.0001993639816501025  ,   1491777.2394576217  ,  0.0041236873698829585],
                [1.07577,  1.0760427739311111,   0.0002727739311112476  ,   4595832.02595791    ,  0.005204716285195556 ],
                [1.26146,  1.2618540471611457,   0.00039404716114566085 ,    5366036.167864191  ,  0.00631712742857134  ]])
                                                                                                     
# CC Fallback
cc_fallback_dataset = np.array([
                [0.63073,  0.6307740180237983,   4.4018023798275685e-05 ,    506223.2813105536  ,  0.0025209063695284238],
                [0.68665,  0.6867358009576815,   8.580095768151441e-05  ,   1199721.2232922434  ,  0.0029118148987038947],
                [0.7283 ,  0.7284535428558775,   0.00015354285587754202 ,    2233063.792387646  ,  0.003163051981278955 ],
                [0.81854,  0.8187287902067172,   0.00018879020671713764 ,    2117887.36818715   ,  0.003685969444714052 ],
                [0.89198,  0.8921750963928836,   0.00019509639288362024 ,    1491769.2856169152 ,  0.004122703020756825 ],
                [1.07577,  1.0760368222100394,   0.0002668222100394857  ,   4595701.795747642   ,  0.005203301278843772 ],
                [1.26146,  1.261850412372176 ,   0.0003904123721760744  ,   5366152.554847      ,  0.006315725677542478 ]])

# plot peak position to expected
title = 'Absolute difference from observed peak position to expected peak position: West bank (1)'
label_y = 'Delta peak position (A)'
out_png = 'delta_peak_pos.png'

# (2)
title = 'Ratio from observed peak position to expected peak position: West bank (1)'
label_y = 'observed peak position / expected peak position'
out_png = 'delta_d_over_d.png'
# 
# (3)
title = 'Ratio from observed peak height to uncalibrated peak height: West bank (1)'
label_y = 'observed peak height / uncalibrated peak height'
out_png = 'peak_height.png'
# 
# (4)
title = 'Ratio from observed peak width to uncalibrated peak width: West bank (1)'
label_y = 'observed peak width / uncalibrated peak width'
out_png = 'peak_width.png'

vec_x = raw_dataset[:, 0]
for dataset, name, color in [(raw_dataset, 'Raw', 'black'),
                             (pdcali_dataset, 'PDCalibration', 'red'),
                             (cc_mask_dataset, 'Cross-correlation (mask incorrect DIFC)', 'blue'),
                             (cc_fallback_dataset, 'Cross-correlation (fallback incorrect DIFC)', 'green')]:
    # (1) vec_y = dataset[:, 2]

    # (2) vec_y = dataset[:, 1] / vec_x

    # (3) vec_y = dataset[:, 3] / raw_dataset[:, 3]

    # (4)
    vec_y = dataset[:, 4] / raw_dataset[:, 4]
    vec_x = dataset[:, 0]
    vec_x = vec_x[vec_y > 0]
    vec_y = vec_y[vec_y > 0]

    if name == 'PDCalibration':
        plt.plot(vec_x, vec_y, color=color, label=name, linestyle='None', marker='D')
    else:
        plt.plot(vec_x, vec_y, color=color, label=name, linestyle='None', marker='o')


plt.title(title)
plt.xlabel('dSpacing (A)')
plt.ylabel(label_y)
plt.legend()
plt.savefig(out_png)
plt.show()

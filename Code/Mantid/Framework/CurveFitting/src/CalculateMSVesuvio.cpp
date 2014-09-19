//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidCurveFitting/CalculateMSVesuvio.h"
// Use helpers for storing detector/resolution parameters
#include "MantidCurveFitting/ConvertToYSpace.h"
#include "MantidCurveFitting/VesuvioResolution.h"

#include "MantidAPI/SampleShapeValidator.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/Track.h"

#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/VectorHelper.h"

#include <boost/make_shared.hpp>

namespace Mantid
{
  namespace CurveFitting
  {
    using namespace API;
    using namespace Kernel;
    using Geometry::Link;
    using Geometry::ParameterMap;
    using Geometry::Track;

    namespace
    {
      const size_t NSIMULATIONS = 10;
      const size_t NEVENTS = 500000;
      const size_t NSCATTERS = 3;
      const size_t MAX_SCATTER_PT_TRIES = 25;
      /// Conversion constant
      const double MASS_TO_MEV = 0.5*PhysicalConstants::NeutronMass/PhysicalConstants::meV;

      /**
       * Generate the final energy of a neutron for gold foil analyser at 293K
       * in double-difference mode:
       *   - THIN FOIL NUMBER DENSITY = 1.456E20 ATOMS/SQ CM.
       *   - THICK FOIL NUMBER DENSITY = 3.0* 1.456E20 ATOMS/SQ CM.
       * @param randv A random number between 0.0 & 1.0, sample from a flat distribution
       * @return A value to use for the final energy
       */
      double finalEnergyAuDD(const double randv)
      {
        // Tabulated values of absoprtion energies from S.F. Mughabghab, Neutron Cross Sections, Academic
        // Press, Orlando, Florida, 1984.
        const double ENERGIES[300] = {\
          2000.0, 2020.7, 2041.5, 2062.2, 2082.9, 2103.7, 2124.4, 2145.2, 2165.9, 2186.6, 2207.4, 2228.1,
          2248.8, 2269.6, 2290.3, 2311.0, 2331.8, 2352.5, 2373.2, 2394.0, 2414.7, 2435.5, 2456.2, 2476.9,
          2497.7, 2518.4, 2539.1, 2559.9, 2580.6, 2601.3, 2622.1, 2642.8, 2663.5, 2684.3, 2705.0, 2725.8,
          2746.5, 2767.2, 2788.0, 2808.7, 2829.4, 2850.2, 2870.9, 2891.6, 2912.4, 2933.1, 2953.8, 2974.6,
          2995.3, 3016.1, 3036.8, 3057.5, 3078.3, 3099.0, 3119.7, 3140.5, 3161.2, 3181.9, 3202.7, 3223.4,
          3244.1, 3264.9, 3285.6, 3306.4, 3327.1, 3347.8, 3368.6, 3389.3, 3410.0, 3430.8, 3451.5, 3472.2,
          3493.0, 3513.7, 3534.4, 3555.2, 3575.9, 3596.7, 3617.4, 3638.1, 3658.9, 3679.6, 3700.3, 3721.1,
          3741.8, 3762.5, 3783.3, 3804.0, 3824.7, 3845.5, 3866.2, 3887.0, 3907.7, 3928.4, 3949.2, 3969.9,
          3990.6, 4011.4, 4032.1, 4052.8, 4073.6, 4094.3, 4115.1, 4135.8, 4156.5, 4177.3, 4198.0, 4218.7,
          4239.5, 4260.2, 4280.9, 4301.7, 4322.4, 4343.1, 4363.9, 4384.6, 4405.4, 4426.1, 4446.8, 4467.6,
          4488.3, 4509.0, 4529.8, 4550.5, 4571.2, 4592.0, 4612.7, 4633.4, 4654.2, 4674.9, 4695.7, 4716.4,
          4737.1, 4757.9, 4778.6, 4799.3, 4820.1, 4840.8, 4861.5, 4882.3, 4903.0, 4923.7, 4944.5, 4965.2,
          4986.0, 5006.7, 5027.4, 5048.2, 5068.9, 5089.6, 5110.4, 5131.1, 5151.8, 5172.6, 5193.3, 5214.0,
          5234.8, 5255.5, 5276.3, 5297.0, 5317.7, 5338.5, 5359.2, 5379.9, 5400.7, 5421.4, 5442.1, 5462.9,
          5483.6, 5504.3, 5525.1, 5545.8, 5566.6, 5587.3, 5608.0, 5628.8, 5649.5, 5670.2, 5691.0, 5711.7,
          5732.4, 5753.2, 5773.9, 5794.6, 5815.4, 5836.1, 5856.9, 5877.6, 5898.3, 5919.1, 5939.8, 5960.5,
          5981.3, 6002.0, 6022.7, 6043.5, 6064.2, 6085.0, 6105.7, 6126.4, 6147.2, 6167.9, 6188.6, 6209.4,
          6230.1, 6250.8, 6271.6, 6292.3, 6313.0, 6333.8, 6354.5, 6375.3, 6396.0, 6416.7, 6437.5, 6458.2,
          6478.9, 6499.7, 6520.4, 6541.1, 6561.9, 6582.6, 6603.3, 6624.1, 6644.8, 6665.6, 6686.3, 6707.0,
          6727.8, 6748.5, 6769.2, 6790.0, 6810.7, 6831.4, 6852.2, 6872.9, 6893.6, 6914.4, 6935.1, 6955.9,
          6976.6, 6997.3, 7018.1, 7038.8, 7059.5, 7080.3, 7101.0, 7121.7, 7142.5, 7163.2, 7183.9, 7204.7,
          7225.4, 7246.2, 7266.9, 7287.6, 7308.4, 7329.1, 7349.8, 7370.6, 7391.3, 7412.0, 7432.8, 7453.5,
          7474.2, 7495.0, 7515.7, 7536.5, 7557.2, 7577.9, 7598.7, 7619.4, 7640.1, 7660.9, 7681.6, 7702.3,
          7723.1, 7743.8, 7764.5, 7785.3, 7806.0, 7826.8, 7847.5, 7868.2, 7889.0, 7909.7, 7930.4, 7951.2,
          7971.9, 7992.6, 8013.4, 8034.1, 8054.8, 8075.6, 8096.3, 8117.1, 8137.8, 8158.5, 8179.3, 8200.0
        };

        const double XVALUES[300] = {\
          0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000,
          0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000,
          0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00010, 0.00010,
          0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010,
          0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010,
          0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00010, 0.00020, 0.00020, 0.00020, 0.00020, 0.00020, 0.00020,
          0.00020, 0.00020, 0.00020, 0.00020, 0.00020, 0.00020, 0.00030, 0.00030, 0.00030, 0.00030, 0.00030, 0.00030,
          0.00030, 0.00030, 0.00040, 0.00040, 0.00040, 0.00040, 0.00040, 0.00050, 0.00050, 0.00050, 0.00050, 0.00060,
          0.00060, 0.00070, 0.00070, 0.00070, 0.00080, 0.00090, 0.00090, 0.00100, 0.00110, 0.00110, 0.00120, 0.00130,
          0.00150, 0.00160, 0.00170, 0.00190, 0.00210, 0.00230, 0.00260, 0.00290, 0.00320, 0.00360, 0.00410, 0.00470,
          0.00540, 0.00620, 0.00720, 0.00840, 0.00990, 0.01180, 0.01420, 0.01740, 0.02140, 0.02680, 0.03410, 0.04400,
          0.05770, 0.07680, 0.10360, 0.14050, 0.18960, 0.25110, 0.32310, 0.40240, 0.48540, 0.56870, 0.64930, 0.72370,
          0.78850, 0.84150, 0.88240, 0.91260, 0.93440, 0.95000, 0.96130, 0.96960, 0.97570, 0.98030, 0.98380, 0.98650,
          0.98870, 0.99040, 0.99180, 0.99290, 0.99380, 0.99460, 0.99520, 0.99580, 0.99620, 0.99660, 0.99700, 0.99730,
          0.99750, 0.99770, 0.99790, 0.99810, 0.99830, 0.99840, 0.99850, 0.99860, 0.99870, 0.99880, 0.99890, 0.99900,
          0.99900, 0.99910, 0.99910, 0.99920, 0.99920, 0.99930, 0.99930, 0.99940, 0.99940, 0.99940, 0.99940, 0.99950,
          0.99950, 0.99950, 0.99950, 0.99960, 0.99960, 0.99960, 0.99960, 0.99960, 0.99960, 0.99970, 0.99970, 0.99970,
          0.99970, 0.99970, 0.99970, 0.99970, 0.99970, 0.99970, 0.99980, 0.99980, 0.99980, 0.99980, 0.99980, 0.99980,
          0.99980, 0.99980, 0.99980, 0.99980, 0.99980, 0.99980, 0.99980, 0.99980, 0.99980, 0.99980, 0.99990, 0.99990,
          0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990,
          0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990,
          0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990, 0.99990,
          0.99990, 0.99990, 0.99990, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000,
          1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000,
          1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000
        };

        for(size_t i = 0; i < 299; ++i)
        {
          if(XVALUES[i] < randv && XVALUES[i+1] > randv)
          {
           double ef = ENERGIES[i] + (randv - XVALUES[i])*(ENERGIES[i+1] - ENERGIES[i])/(XVALUES[i+1] - XVALUES[i]);
           if(ef < 100.0) ef = 0.0;
           return ef;
          }
        }
        return 0.0;


      }

      /**
       * Generate the final energy of a neutron for gold foil analyser at 293K
       * with number density of 7.35E19 atoms/cm^2 in yap difference mode.
       * @param randv A random number between 0.0 & 1.0, sample from a flat distribution
       * @return A value to use for the final energy
       */
      double finalEnergyAuYap(const double randv)
      {
        // Tabulated values of absoprtion energies from S.F. Mughabghab, Neutron Cross Sections, Academic
        // Press, Orlando, Florida, 1984.
        const double ENERGIES[600] = {\
          4000.0, 4003.3, 4006.7, 4010.0, 4013.4, 4016.7, 4020.0, 4023.4, 4026.7, 4030.1, 4033.4, 4036.7,
          4040.1, 4043.4, 4046.7, 4050.1, 4053.4, 4056.8, 4060.1, 4063.4, 4066.8, 4070.1, 4073.5, 4076.8,
          4080.1, 4083.5, 4086.8, 4090.2, 4093.5, 4096.8, 4100.2, 4103.5, 4106.8, 4110.2, 4113.5, 4116.9,
          4120.2, 4123.5, 4126.9, 4130.2, 4133.6, 4136.9, 4140.2, 4143.6, 4146.9, 4150.3, 4153.6, 4156.9,
          4160.3, 4163.6, 4166.9, 4170.3, 4173.6, 4177.0, 4180.3, 4183.6, 4187.0, 4190.3, 4193.7, 4197.0,
          4200.3, 4203.7, 4207.0, 4210.4, 4213.7, 4217.0, 4220.4, 4223.7, 4227.0, 4230.4, 4233.7, 4237.1,
          4240.4, 4243.7, 4247.1, 4250.4, 4253.8, 4257.1, 4260.4, 4263.8, 4267.1, 4270.5, 4273.8, 4277.1,
          4280.5, 4283.8, 4287.1, 4290.5, 4293.8, 4297.2, 4300.5, 4303.8, 4307.2, 4310.5, 4313.9, 4317.2,
          4320.5, 4323.9, 4327.2, 4330.6, 4333.9, 4337.2, 4340.6, 4343.9, 4347.2, 4350.6, 4353.9, 4357.3,
          4360.6, 4363.9, 4367.3, 4370.6, 4374.0, 4377.3, 4380.6, 4384.0, 4387.3, 4390.7, 4394.0, 4397.3,
          4400.7, 4404.0, 4407.3, 4410.7, 4414.0, 4417.4, 4420.7, 4424.0, 4427.4, 4430.7, 4434.1, 4437.4,
          4440.7, 4444.1, 4447.4, 4450.8, 4454.1, 4457.4, 4460.8, 4464.1, 4467.4, 4470.8, 4474.1, 4477.5,
          4480.8, 4484.1, 4487.5, 4490.8, 4494.2, 4497.5, 4500.8, 4504.2, 4507.5, 4510.9, 4514.2, 4517.5,
          4520.9, 4524.2, 4527.5, 4530.9, 4534.2, 4537.6, 4540.9, 4544.2, 4547.6, 4550.9, 4554.3, 4557.6,
          4560.9, 4564.3, 4567.6, 4571.0, 4574.3, 4577.6, 4581.0, 4584.3, 4587.6, 4591.0, 4594.3, 4597.7,
          4601.0, 4604.3, 4607.7, 4611.0, 4614.4, 4617.7, 4621.0, 4624.4, 4627.7, 4631.1, 4634.4, 4637.7,
          4641.1, 4644.4, 4647.7, 4651.1, 4654.4, 4657.8, 4661.1, 4664.4, 4667.8, 4671.1, 4674.5, 4677.8,
          4681.1, 4684.5, 4687.8, 4691.2, 4694.5, 4697.8, 4701.2, 4704.5, 4707.8, 4711.2, 4714.5, 4717.9,
          4721.2, 4724.5, 4727.9, 4731.2, 4734.6, 4737.9, 4741.2, 4744.6, 4747.9, 4751.3, 4754.6, 4757.9,
          4761.3, 4764.6, 4767.9, 4771.3, 4774.6, 4778.0, 4781.3, 4784.6, 4788.0, 4791.3, 4794.7, 4798.0,
          4801.3, 4804.7, 4808.0, 4811.4, 4814.7, 4818.0, 4821.4, 4824.7, 4828.0, 4831.4, 4834.7, 4838.1,
          4841.4, 4844.7, 4848.1, 4851.4, 4854.8, 4858.1, 4861.4, 4864.8, 4868.1, 4871.5, 4874.8, 4878.1,
          4881.5, 4884.8, 4888.1, 4891.5, 4894.8, 4898.2, 4901.5, 4904.8, 4908.2, 4911.5, 4914.9, 4918.2,
          4921.5, 4924.9, 4928.2, 4931.6, 4934.9, 4938.2, 4941.6, 4944.9, 4948.2, 4951.6, 4954.9, 4958.3,
          4961.6, 4964.9, 4968.3, 4971.6, 4975.0, 4978.3, 4981.6, 4985.0, 4988.3, 4991.7, 4995.0, 4998.3,
          5001.7, 5005.0, 5008.3, 5011.7, 5015.0, 5018.4, 5021.7, 5025.0, 5028.4, 5031.7, 5035.1, 5038.4,
          5041.7, 5045.1, 5048.4, 5051.8, 5055.1, 5058.4, 5061.8, 5065.1, 5068.4, 5071.8, 5075.1, 5078.5,
          5081.8, 5085.1, 5088.5, 5091.8, 5095.2, 5098.5, 5101.8, 5105.2, 5108.5, 5111.9, 5115.2, 5118.5,
          5121.9, 5125.2, 5128.5, 5131.9, 5135.2, 5138.6, 5141.9, 5145.2, 5148.6, 5151.9, 5155.3, 5158.6,
          5161.9, 5165.3, 5168.6, 5172.0, 5175.3, 5178.6, 5182.0, 5185.3, 5188.6, 5192.0, 5195.3, 5198.7,
          5202.0, 5205.3, 5208.7, 5212.0, 5215.4, 5218.7, 5222.0, 5225.4, 5228.7, 5232.1, 5235.4, 5238.7,
          5242.1, 5245.4, 5248.7, 5252.1, 5255.4, 5258.8, 5262.1, 5265.4, 5268.8, 5272.1, 5275.5, 5278.8,
          5282.1, 5285.5, 5288.8, 5292.2, 5295.5, 5298.8, 5302.2, 5305.5, 5308.8, 5312.2, 5315.5, 5318.9,
          5322.2, 5325.5, 5328.9, 5332.2, 5335.6, 5338.9, 5342.2, 5345.6, 5348.9, 5352.3, 5355.6, 5358.9,
          5362.3, 5365.6, 5368.9, 5372.3, 5375.6, 5379.0, 5382.3, 5385.6, 5389.0, 5392.3, 5395.7, 5399.0,
          5402.3, 5405.7, 5409.0, 5412.4, 5415.7, 5419.0, 5422.4, 5425.7, 5429.0, 5432.4, 5435.7, 5439.1,
          5442.4, 5445.7, 5449.1, 5452.4, 5455.8, 5459.1, 5462.4, 5465.8, 5469.1, 5472.5, 5475.8, 5479.1,
          5482.5, 5485.8, 5489.1, 5492.5, 5495.8, 5499.2, 5502.5, 5505.8, 5509.2, 5512.5, 5515.9, 5519.2,
          5522.5, 5525.9, 5529.2, 5532.6, 5535.9, 5539.2, 5542.6, 5545.9, 5549.2, 5552.6, 5555.9, 5559.3,
          5562.6, 5565.9, 5569.3, 5572.6, 5576.0, 5579.3, 5582.6, 5586.0, 5589.3, 5592.7, 5596.0, 5599.3,
          5602.7, 5606.0, 5609.3, 5612.7, 5616.0, 5619.4, 5622.7, 5626.0, 5629.4, 5632.7, 5636.1, 5639.4,
          5642.7, 5646.1, 5649.4, 5652.8, 5656.1, 5659.4, 5662.8, 5666.1, 5669.4, 5672.8, 5676.1, 5679.5,
          5682.8, 5686.1, 5689.5, 5692.8, 5696.2, 5699.5, 5702.8, 5706.2, 5709.5, 5712.9, 5716.2, 5719.5,
          5722.9, 5726.2, 5729.5, 5732.9, 5736.2, 5739.6, 5742.9, 5746.2, 5749.6, 5752.9, 5756.3, 5759.6,
          5762.9, 5766.3, 5769.6, 5773.0, 5776.3, 5779.6, 5783.0, 5786.3, 5789.6, 5793.0, 5796.3, 5799.7,
          5803.0, 5806.3, 5809.7, 5813.0, 5816.4, 5819.7, 5823.0, 5826.4, 5829.7, 5833.1, 5836.4, 5839.7,
          5843.1, 5846.4, 5849.7, 5853.1, 5856.4, 5859.8, 5863.1, 5866.4, 5869.8, 5873.1, 5876.5, 5879.8,
          5883.1, 5886.5, 5889.8, 5893.2, 5896.5, 5899.8, 5903.2, 5906.5, 5909.8, 5913.2, 5916.5, 5919.9,
          5923.2, 5926.5, 5929.9, 5933.2, 5936.6, 5939.9, 5943.2, 5946.6, 5949.9, 5953.3, 5956.6, 5959.9,
          5963.3, 5966.6, 5970.0, 5973.3, 5976.6, 5980.0, 5983.3, 5986.6, 5990.0, 5993.3, 5996.7, 6000.0
        };

        const double XVALUES[600] = {\
          0.00000, 0.00000, 0.00000, 0.00002, 0.00003, 0.00003, 0.00004, 0.00005, 0.00005, 0.00006, 0.00007, 0.00007,
          0.00008, 0.00009, 0.00010, 0.00010, 0.00011, 0.00012, 0.00013, 0.00014, 0.00015, 0.00015, 0.00016, 0.00017,
          0.00018, 0.00019, 0.00020, 0.00021, 0.00022, 0.00023, 0.00024, 0.00025, 0.00026, 0.00027, 0.00028, 0.00029,
          0.00030, 0.00031, 0.00032, 0.00033, 0.00034, 0.00035, 0.00037, 0.00038, 0.00039, 0.00040, 0.00041, 0.00043,
          0.00044, 0.00045, 0.00047, 0.00048, 0.00049, 0.00051, 0.00052, 0.00054, 0.00055, 0.00057, 0.00058, 0.00060,
          0.00061, 0.00063, 0.00065, 0.00066, 0.00068, 0.00070, 0.00072, 0.00074, 0.00075, 0.00077, 0.00079, 0.00081,
          0.00083, 0.00085, 0.00087, 0.00089, 0.00092, 0.00094, 0.00096, 0.00098, 0.00101, 0.00103, 0.00106, 0.00108,
          0.00111, 0.00113, 0.00116, 0.00118, 0.00121, 0.00124, 0.00127, 0.00130, 0.00133, 0.00136, 0.00139, 0.00142,
          0.00146, 0.00149, 0.00152, 0.00156, 0.00159, 0.00163, 0.00167, 0.00171, 0.00174, 0.00178, 0.00182, 0.00187,
          0.00191, 0.00195, 0.00200, 0.00204, 0.00209, 0.00214, 0.00219, 0.00224, 0.00229, 0.00235, 0.00240, 0.00246,
          0.00251, 0.00257, 0.00263, 0.00269, 0.00276, 0.00282, 0.00289, 0.00296, 0.00303, 0.00310, 0.00318, 0.00325,
          0.00333, 0.00341, 0.00349, 0.00358, 0.00367, 0.00376, 0.00385, 0.00394, 0.00404, 0.00414, 0.00425, 0.00435,
          0.00446, 0.00458, 0.00469, 0.00481, 0.00494, 0.00507, 0.00520, 0.00533, 0.00548, 0.00562, 0.00577, 0.00592,
          0.00608, 0.00625, 0.00642, 0.00659, 0.00677, 0.00696, 0.00716, 0.00736, 0.00757, 0.00778, 0.00800, 0.00823,
          0.00847, 0.00872, 0.00898, 0.00924, 0.00952, 0.00980, 0.01010, 0.01041, 0.01073, 0.01106, 0.01141, 0.01177,
          0.01214, 0.01253, 0.01293, 0.01335, 0.01379, 0.01425, 0.01472, 0.01522, 0.01573, 0.01627, 0.01683, 0.01742,
          0.01803, 0.01867, 0.01934, 0.02004, 0.02077, 0.02154, 0.02234, 0.02317, 0.02405, 0.02497, 0.02594, 0.02695,
          0.02801, 0.02913, 0.03030, 0.03153, 0.03282, 0.03419, 0.03561, 0.03712, 0.03871, 0.04037, 0.04213, 0.04398,
          0.04594, 0.04799, 0.05017, 0.05246, 0.05488, 0.05743, 0.06013, 0.06297, 0.06598, 0.06915, 0.07251, 0.07605,
          0.07979, 0.08374, 0.08791, 0.09230, 0.09694, 0.10183, 0.10698, 0.11241, 0.11812, 0.12411, 0.13041, 0.13703,
          0.14395, 0.15119, 0.15877, 0.16667, 0.17490, 0.18347, 0.19237, 0.20159, 0.21114, 0.22100, 0.23117, 0.24164,
          0.25240, 0.26344, 0.27473, 0.28628, 0.29807, 0.31007, 0.32228, 0.33468, 0.34725, 0.35999, 0.37286, 0.38586,
          0.39898, 0.41219, 0.42549, 0.43886, 0.45228, 0.46575, 0.47925, 0.49277, 0.50628, 0.51980, 0.53329, 0.54674,
          0.56016, 0.57350, 0.58677, 0.59996, 0.61304, 0.62600, 0.63883, 0.65152, 0.66403, 0.67638, 0.68853, 0.70048,
          0.71220, 0.72369, 0.73492, 0.74590, 0.75659, 0.76700, 0.77711, 0.78691, 0.79640, 0.80557, 0.81442, 0.82294,
          0.83113, 0.83900, 0.84654, 0.85376, 0.86067, 0.86726, 0.87355, 0.87954, 0.88525, 0.89067, 0.89583, 0.90073,
          0.90537, 0.90979, 0.91398, 0.91794, 0.92170, 0.92527, 0.92865, 0.93185, 0.93489, 0.93776, 0.94049, 0.94307,
          0.94552, 0.94784, 0.95005, 0.95213, 0.95412, 0.95600, 0.95779, 0.95949, 0.96110, 0.96264, 0.96410, 0.96549,
          0.96681, 0.96807, 0.96927, 0.97041, 0.97150, 0.97254, 0.97353, 0.97447, 0.97538, 0.97624, 0.97706, 0.97785,
          0.97860, 0.97933, 0.98002, 0.98068, 0.98131, 0.98192, 0.98250, 0.98306, 0.98359, 0.98411, 0.98460, 0.98507,
          0.98553, 0.98596, 0.98638, 0.98679, 0.98718, 0.98755, 0.98791, 0.98826, 0.98859, 0.98892, 0.98923, 0.98953,
          0.98981, 0.99009, 0.99036, 0.99062, 0.99087, 0.99111, 0.99135, 0.99158, 0.99179, 0.99201, 0.99221, 0.99241,
          0.99260, 0.99279, 0.99296, 0.99314, 0.99331, 0.99347, 0.99363, 0.99378, 0.99393, 0.99408, 0.99422, 0.99435,
          0.99448, 0.99461, 0.99473, 0.99486, 0.99497, 0.99509, 0.99520, 0.99530, 0.99541, 0.99551, 0.99561, 0.99571,
          0.99580, 0.99589, 0.99598, 0.99607, 0.99615, 0.99623, 0.99631, 0.99639, 0.99647, 0.99654, 0.99661, 0.99668,
          0.99675, 0.99682, 0.99688, 0.99694, 0.99701, 0.99707, 0.99713, 0.99718, 0.99724, 0.99729, 0.99735, 0.99740,
          0.99745, 0.99750, 0.99755, 0.99760, 0.99764, 0.99769, 0.99773, 0.99778, 0.99782, 0.99786, 0.99790, 0.99794,
          0.99798, 0.99802, 0.99806, 0.99809, 0.99813, 0.99816, 0.99820, 0.99823, 0.99827, 0.99830, 0.99833, 0.99836,
          0.99839, 0.99842, 0.99845, 0.99848, 0.99850, 0.99853, 0.99856, 0.99859, 0.99861, 0.99864, 0.99866, 0.99869,
          0.99871, 0.99873, 0.99876, 0.99878, 0.99880, 0.99882, 0.99884, 0.99886, 0.99889, 0.99891, 0.99893, 0.99895,
          0.99896, 0.99898, 0.99900, 0.99902, 0.99904, 0.99906, 0.99907, 0.99909, 0.99911, 0.99912, 0.99914, 0.99915,
          0.99917, 0.99919, 0.99920, 0.99922, 0.99923, 0.99924, 0.99926, 0.99927, 0.99929, 0.99930, 0.99931, 0.99933,
          0.99934, 0.99935, 0.99936, 0.99938, 0.99939, 0.99940, 0.99941, 0.99942, 0.99943, 0.99944, 0.99946, 0.99947,
          0.99948, 0.99949, 0.99950, 0.99951, 0.99952, 0.99953, 0.99954, 0.99955, 0.99956, 0.99956, 0.99957, 0.99958,
          0.99959, 0.99960, 0.99961, 0.99962, 0.99963, 0.99963, 0.99964, 0.99965, 0.99966, 0.99967, 0.99967, 0.99968,
          0.99969, 0.99970, 0.99970, 0.99971, 0.99972, 0.99973, 0.99973, 0.99974, 0.99975, 0.99975, 0.99976, 0.99977,
          0.99977, 0.99978, 0.99979, 0.99979, 0.99980, 0.99980, 0.99981, 0.99982, 0.99982, 0.99983, 0.99983, 0.99984,
          0.99984, 0.99985, 0.99985, 0.99986, 0.99986, 0.99987, 0.99988, 0.99988, 0.99989, 0.99989, 0.99990, 0.99990,
          0.99990, 0.99991, 0.99991, 0.99992, 0.99992, 0.99993, 0.99993, 0.99994, 0.99994, 0.99994, 0.99995, 0.99995,
          0.99996, 0.99996, 0.99997, 0.99997, 0.99997, 0.99998, 0.99998, 0.99998, 0.99999, 0.99999, 1.00000, 1.00000
        };

        for(size_t i = 0; i < 599; ++i)
        {
          if(XVALUES[i] < randv && XVALUES[i+1] > randv)
          {
           return ENERGIES[i] + (randv - XVALUES[i])*(ENERGIES[i+1] - ENERGIES[i])/(XVALUES[i+1] - XVALUES[i]);
          }
        }
        return 0.0;
      }

      /**
       * Generate the final energy of a neutron for gold foil analyser at 293K
       * with number density of 7.35E19 atoms/cm^2 in double-difference mode.
       * @param randv A random number between 0.0 & 1.0, sample from a flat distribution
       * @return A value to use for the final energy
       */
      double finalEnergyUranium(const double randv)
      {
        const double ENERGIES[201] = {\
          5959.0, 5967.7, 5976.4, 5985.1, 5993.8, 6002.5, 6011.2, 6019.9, 6028.6, 6037.3, 6046.0, 6054.8,
          6063.5, 6072.2, 6080.9, 6089.6, 6098.3, 6107.0, 6115.7, 6124.4, 6133.1, 6141.8, 6150.5, 6159.2,
          6167.9, 6176.6, 6185.3, 6194.0, 6202.7, 6211.4, 6220.1, 6228.9, 6237.6, 6246.3, 6255.0, 6263.7,
          6272.4, 6281.1, 6289.8, 6298.5, 6307.2, 6315.9, 6324.6, 6333.3, 6342.0, 6350.7, 6359.4, 6368.1,
          6376.8, 6385.5, 6394.3, 6403.0, 6411.7, 6420.4, 6429.1, 6437.8, 6446.5, 6455.2, 6463.9, 6472.6,
          6481.3, 6490.0, 6498.7, 6507.4, 6516.1, 6524.8, 6533.5, 6542.2, 6550.9, 6559.6, 6568.4, 6577.1,
          6585.8, 6594.5, 6603.2, 6611.9, 6620.6, 6629.3, 6638.0, 6646.7, 6655.4, 6664.1, 6672.8, 6681.5,
          6690.2, 6698.9, 6707.6, 6716.3, 6725.0, 6733.7, 6742.5, 6751.2, 6759.9, 6768.6, 6777.3, 6786.0,
          6794.7, 6803.4, 6812.1, 6820.8, 6829.5, 6838.2, 6846.9, 6855.6, 6864.3, 6873.0, 6881.7, 6890.4,
          6899.1, 6907.8, 6916.5, 6925.3, 6934.0, 6942.7, 6951.4, 6960.1, 6968.8, 6977.5, 6986.2, 6994.9,
          7003.6, 7012.3, 7021.0, 7029.7, 7038.4, 7047.1, 7055.8, 7064.5, 7073.2, 7081.9, 7090.6, 7099.4,
          7108.1, 7116.8, 7125.5, 7134.2, 7142.9, 7151.6, 7160.3, 7169.0, 7177.7, 7186.4, 7195.1, 7203.8,
          7212.5, 7221.2, 7229.9, 7238.6, 7247.3, 7256.0, 7264.8, 7273.5, 7282.2, 7290.9, 7299.6, 7308.3,
          7317.0, 7325.7, 7334.4, 7343.1, 7351.8, 7360.5, 7369.2, 7377.9, 7386.6, 7395.3, 7404.0, 7412.7,
          7421.4, 7430.1, 7438.9, 7447.6, 7456.3, 7465.0, 7473.7, 7482.4, 7491.1, 7499.8, 7508.5, 7517.2,
          7525.9, 7534.6, 7543.3, 7552.0, 7560.7, 7569.4, 7578.1, 7586.8, 7595.5, 7604.2, 7613.0, 7621.7,
          7630.4, 7639.1, 7647.8, 7656.5, 7665.2, 7673.9, 7682.6, 7691.3, 7700.0
        };

        const double XVALUES[201] = {\
          0.00000, 0.00000, 0.00000, 0.00020, 0.00030, 0.00040, 0.00050, 0.00060, 0.00070, 0.00080, 0.00090, 0.00110,
          0.00120, 0.00140, 0.00150, 0.00170, 0.00190, 0.00210, 0.00230, 0.00250, 0.00270, 0.00290, 0.00310, 0.00340,
          0.00360, 0.00390, 0.00410, 0.00440, 0.00470, 0.00500, 0.00530, 0.00560, 0.00590, 0.00620, 0.00650, 0.00690,
          0.00720, 0.00760, 0.00800, 0.00840, 0.00880, 0.00920, 0.00960, 0.01010, 0.01050, 0.01100, 0.01150, 0.01210,
          0.01270, 0.01330, 0.01390, 0.01460, 0.01530, 0.01610, 0.01690, 0.01780, 0.01870, 0.01970, 0.02090, 0.02210,
          0.02350, 0.02500, 0.02660, 0.02850, 0.03070, 0.03320, 0.03620, 0.03990, 0.04440, 0.05020, 0.05780, 0.06790,
          0.08120, 0.09880, 0.12150, 0.15020, 0.18520, 0.22640, 0.27340, 0.32510, 0.38050, 0.43830, 0.49720, 0.55580,
          0.61290, 0.66710, 0.71740, 0.76250, 0.80190, 0.83510, 0.86220, 0.88380, 0.90050, 0.91340, 0.92340, 0.93100,
          0.93710, 0.94200, 0.94600, 0.94940, 0.95230, 0.95490, 0.95710, 0.95920, 0.96100, 0.96270, 0.96430, 0.96580,
          0.96710, 0.96840, 0.96950, 0.97060, 0.97170, 0.97270, 0.97360, 0.97450, 0.97540, 0.97620, 0.97700, 0.97770,
          0.97840, 0.97910, 0.97980, 0.98040, 0.98100, 0.98160, 0.98220, 0.98280, 0.98330, 0.98390, 0.98440, 0.98490,
          0.98540, 0.98590, 0.98630, 0.98680, 0.98720, 0.98770, 0.98810, 0.98850, 0.98890, 0.98930, 0.98970, 0.99010,
          0.99050, 0.99090, 0.99130, 0.99160, 0.99200, 0.99230, 0.99270, 0.99300, 0.99330, 0.99360, 0.99400, 0.99430,
          0.99460, 0.99480, 0.99510, 0.99540, 0.99560, 0.99590, 0.99610, 0.99640, 0.99660, 0.99680, 0.99710, 0.99730,
          0.99750, 0.99770, 0.99780, 0.99800, 0.99820, 0.99840, 0.99850, 0.99870, 0.99880, 0.99890, 0.99910, 0.99920,
          0.99930, 0.99940, 0.99950, 0.99960, 0.99960, 0.99970, 0.99980, 0.99980, 0.99990, 0.99990, 0.99990, 1.00000,
          1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000
        };

        for(size_t i = 0; i < 299; ++i)
        {
          if(XVALUES[i] < randv && XVALUES[i+1] > randv)
          {
           return ENERGIES[i] + (randv - XVALUES[i])*(ENERGIES[i+1] - ENERGIES[i])/(XVALUES[i+1] - XVALUES[i]);
          }
        }

        return 0.0;
      }


    } // end anonymous namespace

    //-------------------------------------------------------------------------
    // RandomNumberGenerator helper
    //-------------------------------------------------------------------------

    /**
     * Produces random numbers with various probability distributions
     */
    CalculateMSVesuvio::
      RandomNumberGenerator::RandomNumberGenerator(const int seed) : m_generator()
    {
      m_generator.seed(static_cast<boost::mt19937::result_type>(seed));
    }
    /// Returns a flat random number between 0.0 & 1.0
    double CalculateMSVesuvio::
      RandomNumberGenerator::flat()
    {
      return uniform_double()(m_generator,
                              uniform_double::param_type(0.0, 1.0));
    }
    /// Returns a random number distributed  by a normal distribution
    double CalculateMSVesuvio::
      RandomNumberGenerator::gaussian(const double mean, const double sigma)
    {
      return gaussian_double()(m_generator,
                               gaussian_double::param_type(mean, sigma));
    }

    //-------------------------------------------------------------------------
    // Simulation helper
    //-------------------------------------------------------------------------
    /**
     * Stores counts for each scatter order
     * for a "run" of a given number of events
     */
    CalculateMSVesuvio::Simulation::
      Simulation(const size_t order, const size_t ntimes) :
        counts(order, std::vector<double>(ntimes)),
        maxorder(order)
    {}
    //-------------------------------------------------------------------------
    // SimulationAggreator
    //-------------------------------------------------------------------------
    /**
     * Accumulates and averages the results
     * of each simulation
     * @param nruns The number of runs that will be computed
     */
    CalculateMSVesuvio::SimulationAggregator::
      SimulationAggregator(const size_t nruns)
    {
      results.reserve(nruns);
    }

    /**
     * @param order The number of requested scatterings
     * @param ntimes The number of times on input workspace
     * @return A reference to a new Simulation object
     */
    CalculateMSVesuvio::Simulation &
    CalculateMSVesuvio::SimulationAggregator::
      newSimulation(const size_t order, const size_t ntimes)
    {
      results.push_back(Simulation(order, ntimes));
      return results.back();
    }

    /**
     * @return The mean and standard deviation of the current set of simulations
     */
    CalculateMSVesuvio::SimulationWithErrors
    CalculateMSVesuvio::SimulationAggregator::average() const
    {
      const size_t maxorder(results[0].maxorder), ntimes(results[0].counts[0].size()),
          nruns(results.size());
      SimulationWithErrors retval(maxorder, ntimes);

      for(size_t i = 0; i < maxorder; ++i)
      {
        auto & orderCounts = retval.sim.counts[i];
        auto & orderErrors = retval.errors[i];
        for(size_t j = 0; j < ntimes; ++j)
        {
          double mean(0.0);
          size_t npoints(0);
          for(size_t k = 0; k < nruns; ++k)
          {
            const double val = results[k].counts[i][j];
            if(val > 0.0)
            {
              mean += val;
              npoints +=1;
            }
          }
          if(npoints < 2)
          {
            orderCounts[j] = 0.0;
            orderErrors[j] = 0.0;
          }
          else
          {
            const double dblPts = static_cast<double>(npoints);
            orderCounts[j] = mean/dblPts;
            // error is std dev
            double sumsq(0.0);
            for(size_t k = 0; k < nruns; ++k)
            {
              const double val = results[k].counts[i][j];
              if(val > 0.0)
              {
                const double diff = (val - mean);
                sumsq += diff*diff;
              }
            }
            orderErrors[j] = sqrt(sumsq/(dblPts*(dblPts-1)));
          }
        }
      }

      return retval;
    }
    //-------------------------------------------------------------------------
    // SimulationWithErrors
    //-------------------------------------------------------------------------
    /**
     * Normalise the counts so that the integral over the single-scatter
     * events is 1.
     */
    void CalculateMSVesuvio::SimulationWithErrors::normalise()
    {
      const double sumSingle = std::accumulate(sim.counts.front().begin(),
                                               sim.counts.front().end(), 0.0);
      if(sumSingle > 0.0)
      {
        const double invSum = 1.0/sumSingle; // multiply is faster
        // Divide everything by the sum
        const size_t nscatters = sim.counts.size();
        for(size_t i = 0; i < nscatters; ++i)
        {
          auto & counts = sim.counts[i];
          auto & scerrors = this->errors[i];
          for(auto cit = counts.begin(), eit = scerrors.begin(); cit != counts.end();
              ++cit, ++eit)
          {
            (*cit) *= invSum;
            (*eit) *= invSum;
          }
        }
      }
    }

    //-------------------------------------------------------------------------
    // Algorithm definitions
    //-------------------------------------------------------------------------

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(CalculateMSVesuvio)

    /// Constructor
    CalculateMSVesuvio::CalculateMSVesuvio() : Algorithm(),
      m_randgen(NULL),
      m_acrossIdx(0), m_upIdx(1), m_beamIdx(3), m_beamDir(), m_srcR1(0.0), m_srcR2(0.0),
      m_halfSampleHeight(0.0), m_halfSampleWidth(0.0), m_halfSampleThick(0.0),
      m_maxWidthSampleFrame(0.0), m_goniometer(NULL), m_sampleShape(NULL),
      m_sampleProps(NULL),
      m_detHeight(-1.0), m_detWidth(-1.0), m_detThick(-1.0),
      m_tmin(-1.0), m_tmax(-1.0), m_delt(-1.0), m_foilRes(-1.0),
      m_progress(NULL), m_inputWS()
    {
    }

    /// Destructor
    CalculateMSVesuvio::~CalculateMSVesuvio()
    {
      delete m_randgen;
      delete m_progress;
      delete m_sampleProps;
    }

    /**
     * Initialize the algorithm's properties.
     */
    void CalculateMSVesuvio::init()
    {
      // Inputs
      auto inputWSValidator = boost::make_shared<CompositeValidator>();
      inputWSValidator->add<WorkspaceUnitValidator>("TOF");
      inputWSValidator->add<SampleShapeValidator>();
      declareProperty(new WorkspaceProperty<>("InputWorkspace","",
                                              Direction::Input, inputWSValidator),
                      "Input workspace to be corrected, in units of TOF.");

      // -- Sample --
      auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
      positiveInt->setLower(1);
      declareProperty("NoOfMasses", -1, positiveInt,
                      "The number of masses contained within the sample");

      auto positiveNonZero = boost::make_shared<BoundedValidator<double>>();
      positiveNonZero->setLower(0.0);
      positiveNonZero->setLowerExclusive(true);
      declareProperty("SampleDensity", -1.0, positiveNonZero,
                      "The density of the sample in gm/cm^3");

      auto nonEmptyArray = boost::make_shared<ArrayLengthValidator<double>>();
      nonEmptyArray->setLengthMin(3);
      declareProperty(new ArrayProperty<double>("AtomicProperties", nonEmptyArray),
                      "Atomic properties of masses within the sample. "
                      "The expected format is 3 consecutive values per mass: "
                      "mass(amu), cross-section (barns) & s.d of Compton profile.");
      setPropertyGroup("NoOfMasses", "Sample");
      setPropertyGroup("SampleDensity", "Sample");
      setPropertyGroup("AtomicProperties", "Sample");

      // -- Beam --
      declareProperty("BeamUmbraRadius", -1.0, positiveNonZero,
                      "Radius, in cm, of part in total shadow.");
      declareProperty("BeamPenumbraRadius", -1.0, positiveNonZero,
                      "Radius, in cm, of part in partial shadow.");
      setPropertyGroup("BeamUmbraRadius", "Beam");
      setPropertyGroup("BeamPenumbraRadius", "Beam");

      // -- Algorithm --
      declareProperty("Seed", 123456789, positiveInt,
                      "Seed the random number generator with this value");

      // Outputs
      declareProperty(new WorkspaceProperty<>("TotalScatteringWS","", Direction::Output),
                      "Workspace to store the calculated total scattering counts");
      declareProperty(new WorkspaceProperty<>("MultipleScatteringWS","", Direction::Output),
                      "Workspace to store the calculated total scattering counts");
    }

    /**
     * Execute the algorithm.
     */
    void CalculateMSVesuvio::exec()
    {
      m_inputWS = getProperty("InputWorkspace");
      cacheInputs();

      // Create new workspaces
      MatrixWorkspace_sptr totalsc = WorkspaceFactory::Instance().create(m_inputWS);
      MatrixWorkspace_sptr multsc = WorkspaceFactory::Instance().create(m_inputWS);

      // Initialize random number generator
      m_randgen = new RandomNumberGenerator(getProperty("Seed"));

      // Setup progress
      const int64_t nhist = static_cast<int64_t>(m_inputWS->getNumberHistograms());
      m_progress = new API::Progress(this, 0.0, 1.0, nhist);
      for(int64_t i = 0; i < nhist; ++i)
      {
        m_progress->report("Calculating corrections");

        // Copy over the X-values
        const MantidVec & xValues = m_inputWS->readX(i);
        totalsc->dataX(i) = xValues;
        multsc->dataX(i) = xValues;

        // Final detector position
        Geometry::IDetector_const_sptr detector;
        try
        {
          detector = m_inputWS->getDetector(i);
        }
        catch(Kernel::Exception::NotFoundError&)
        {
          // intel compiler doesn't like continue in a catch inside an OMP loop
        }
        if(!detector)
        {
          std::ostringstream os;
          os << "No valid detector object found for spectrum at workspace index '" << i << "'. No correction calculated.";
          g_log.information(os.str());
          continue;
        }

        // the output spectrum objects have references to where the data will be stored
        calculateMS(i, *totalsc->getSpectrum(i), *multsc->getSpectrum(i));
      }

      setProperty("TotalScatteringWS", totalsc);
      setProperty("MultipleScatteringWS", multsc);
   }

    /**
     * Caches inputs insuitable form for speed in later calculations
     */
    void CalculateMSVesuvio::cacheInputs()
    {
      // -- Geometry --
      const auto instrument = m_inputWS->getInstrument();
      m_beamDir = instrument->getSample()->getPos() - instrument->getSource()->getPos();

      const auto rframe = instrument->getReferenceFrame();
      m_acrossIdx = rframe->pointingHorizontal();
      m_upIdx = rframe->pointingUp();
      m_beamIdx = rframe->pointingAlongBeam();

      m_srcR1 = getProperty("BeamUmbraRadius");
      m_srcR2 = getProperty("BeamPenumbraRadius");
      if(m_srcR2 < m_srcR1)
      {
        std::ostringstream os;
        os << "Invalid beam radius parameters. Penumbra value="
           << m_srcR2 << " < Umbra value="
           << m_srcR1;
        throw std::invalid_argument(os.str());
      }

      // Sample rotation specified by a goniometer
      m_goniometer = &(m_inputWS->run().getGoniometerMatrix());
      // Sample shape
      m_sampleShape = &(m_inputWS->sample().getShape());
      // We know the shape is valid from the property validator
      // Use the bounding box as an approximation to determine the extents
      // as this will match in both height and width for a cuboid & cylinder
      // sample shape
      Geometry::BoundingBox bounds = m_sampleShape->getBoundingBox();
      V3D boxWidth = bounds.width();
      // Use half-width/height for easier calculation later
      m_halfSampleWidth = 0.5*boxWidth[m_acrossIdx];
      m_halfSampleHeight = 0.5*boxWidth[m_upIdx];
      m_halfSampleThick = 0.5*boxWidth[m_beamIdx];

      // -- Workspace --
      const auto & inX = m_inputWS->readX(0);
      m_tmin = inX.front()*1e-06;
      m_tmax = inX.back()*1e-06;
      m_delt = (inX[1] - m_tmin)*1e-06;

      // -- Sample --
      int nmasses = getProperty("NoOfMasses");
      std::vector<double> sampleInfo = getProperty("AtomicProperties");
      const int nInputAtomProps = static_cast<int>(sampleInfo.size());
      const int nExptdAtomProp(3);
      if(nInputAtomProps != nExptdAtomProp*nmasses)
      {
        std::ostringstream os;
        os << "Inconsistent AtomicProperties list defined. Expected " << nExptdAtomProp*nmasses
           << " values, however, only " << sampleInfo.size() << " have been given.";
        throw std::invalid_argument(os.str());
      }
      const int natoms = nInputAtomProps/3;
      m_sampleProps = new SampleComptonProperties(natoms);
      m_sampleProps->density = getProperty("SampleDensity");

      double totalMass(0.0); // total mass in grams
      m_sampleProps->totalxsec = 0.0;
      for(int i = 0; i < natoms; ++i)
      {
        auto & comptonAtom = m_sampleProps->atoms[i];
        comptonAtom.mass = sampleInfo[nExptdAtomProp*i];
        totalMass += comptonAtom.mass*PhysicalConstants::AtomicMassUnit*1000;

        const double xsec = sampleInfo[nExptdAtomProp*i + 1];
        comptonAtom.sclength = sqrt(xsec/4.0*M_PI);
        const double factor = 1.0 + (PhysicalConstants::NeutronMassAMU/comptonAtom.mass);
        m_sampleProps->totalxsec += (xsec/(factor*factor));

        comptonAtom.profile = sampleInfo[nExptdAtomProp*i + 2];
      }
      const double numberDensity = m_sampleProps->density*1e6/totalMass; // formula units/m^3
      m_sampleProps->mu = numberDensity*m_sampleProps->totalxsec*1e-28;

      // -- Detector geometry -- choose first detector that is not a monitor
      Geometry::IDetector_const_sptr detPixel;
      for(size_t i = 0; i < m_inputWS->getNumberHistograms(); ++i)
      {
        try
        {
          detPixel = m_inputWS->getDetector(i);
        }
        catch(Exception::NotFoundError &)
        {
          continue;
        }
        if(!detPixel->isMonitor()) break;
      }
      // Bounding box in detector frame
      Geometry::Object_const_sptr pixelShape = detPixel->shape();
      if(!pixelShape || !pixelShape->hasValidShape())
      {
        throw std::invalid_argument("Detector pixel has no defined shape!");
      }
      Geometry::BoundingBox detBounds = pixelShape->getBoundingBox();
      V3D detBoxWidth = detBounds.width();
      m_detWidth = detBoxWidth[m_acrossIdx];
      m_detHeight = detBoxWidth[m_upIdx];
      m_detThick = detBoxWidth[m_beamIdx];

      // Foil resolution
      auto foil = instrument->getComponentByName("foil-pos0");
      if(!foil)
      {
        throw std::runtime_error("Workspace has no gold foil component defined.");
      }
      auto param = m_inputWS->instrumentParameters().get(foil.get(), "hwhm_lorentz");
      if(!param)
      {
        throw std::runtime_error("Foil component has no hwhm_lorentz parameter defined.");
      }
      m_foilRes = param->value<double>();
    }

    /**
     * Calculate the total scattering and contributions from higher-order scattering for given
     * spectrum
     * @param wsIndex The index on the input workspace for the chosen spectrum
     * @param totalsc A non-const reference to the spectrum that will contain the total scattering calculation
     * @param multsc A non-const reference to the spectrum that will contain the multiple scattering contribution
     */
    void CalculateMSVesuvio::calculateMS(const size_t wsIndex, API::ISpectrum & totalsc,
                                         API::ISpectrum & multsc) const
    {
      // Detector information
      DetectorParams detpar = ConvertToYSpace::getDetectorParameters(m_inputWS, wsIndex);
      // t0 is stored in seconds here, whereas here we want microseconds
      detpar.t0 *= 1e6;

      const Geometry::IDetector_const_sptr detector = m_inputWS->getDetector(wsIndex);
      const auto & pmap = m_inputWS->instrumentParameters();
      // Resolution information
      ResolutionParams respar;
      respar.dl1 = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_l1");
      respar.dl2 = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_l2");
      respar.dthe = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_theta"); //radians
      respar.dEnLorentz = ConvertToYSpace::getComponentParameter(detector, pmap, "hwhm_lorentz");
      respar.dEnGauss = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_gauss");

      // Final counts averaged over all simulations
      const size_t nruns(NSIMULATIONS), nscatters(NSCATTERS), nevents(NEVENTS);
      SimulationAggregator accumulator(nruns);
      for(size_t i = 0; i < nruns; ++i)
      {
        simulate(nevents, nscatters, detpar, respar,
                 accumulator.newSimulation(nscatters, m_inputWS->blocksize()));
      }

      SimulationWithErrors avgCounts = accumulator.average();
      avgCounts.normalise();

      // assign to output spectrum
      auto & msscatY = multsc.dataY();
      auto & msscatE = multsc.dataE();
      for(size_t i = 0; i < nscatters; ++i)
      {
        const auto & counts = avgCounts.sim.counts[i];
        // equivalent to msscatY[j] += counts[j]
        std::transform(counts.begin(), counts.end(), msscatY.begin(), msscatY.begin(),
                       std::plus<double>());
        const auto & scerrors = avgCounts.errors[i];
        // sum errors in quadrature
        std::transform(scerrors.begin(), scerrors.end(), msscatE.begin(), msscatE.begin(),
                       VectorHelper::SumGaussError<double>());
      }
      // for total scattering add on single-scatter events
      auto & totalscY = totalsc.dataY();
      auto & totalscE = totalsc.dataE();
      const auto & counts0 = avgCounts.sim.counts.front();
      std::transform(counts0.begin(), counts0.end(), msscatY.begin(), totalscY.begin(),
                     std::plus<double>());
      const auto & errors0 = avgCounts.errors.front();
      std::transform(errors0.begin(), errors0.end(), msscatE.begin(), totalscE.begin(),
                     VectorHelper::SumGaussError<double>());
    }

    /**
     * Perform a single simulation of a given number of events for up to a maximum number of
     * scatterings on a chosen detector
     * @param nevents The number of neutron events to simulate
     * @param nscatters Maximum order of scattering that should be simulated
     * @param detpar Detector information describing the final detector position
     * @param respar Resolution information on the intrument as a whole
     * @param simulCounts Simulation object used to storing the calculated number of counts
     */
    void CalculateMSVesuvio::simulate(const size_t nevents, const size_t nscatters,
                                      const DetectorParams & detpar,
                                      const ResolutionParams &respar,
                                      Simulation & simulCounts) const
    {
      for(size_t i = 0; i < nevents; ++i)
      {
        calculateCounts(nscatters, detpar, respar, simulCounts);
      }
    }

    /**
     *
     * @param nscatters Maximum order of scattering that should be simulated
     * @param detpar Detector information describing the final detector position
     * @param respar Resolution information on the intrument as a whole
     * @param simulation [Output] Store the calculated counts here
     * @return The sum of the weights for all scatters
     */
    double CalculateMSVesuvio::calculateCounts(const size_t nscatters, const DetectorParams &detpar,
                                               const ResolutionParams &respar,
                                               Simulation &simulation) const
    {
      double weightSum(0.0);

      // moderator coord in lab frame
      V3D srcPos = generateSrcPos(detpar.l1);
      // transform to sample frame
      srcPos.rotate(*m_goniometer);
      if(fabs(srcPos[0]) > m_halfSampleWidth ||
         fabs(srcPos[1]) > m_halfSampleHeight) return 0.0; // misses sample

      // track various variables during calculation
      std::vector<double> weights(nscatters, 1.0), // start at 1.0
        tofs(nscatters, 0.0),
        scAngs(nscatters, 0.0), // scattering angles between each order
        en1(nscatters, 0.0);

      const double vel2 = sqrt(detpar.efixed/MASS_TO_MEV);
      const double t2 = detpar.l2/vel2;
      en1[0] = generateE0(detpar.l1, t2, weights[0]);
      tofs[0] = generateTOF(en1[0], detpar.t0, respar.dl1); // correction for resolution in l1

      // Neutron path
      // Algorithm has initial direction pointing to origin
      V3D particleDir = V3D() - srcPos;
      particleDir.normalize();

      // first scatter
      std::vector<V3D> scatterPts(nscatters); // track origin of each scatter
      V3D startPos(srcPos);
      generateScatter(startPos, particleDir, weights[0], scatterPts[0]);
      double distFromStart = startPos.distance(scatterPts[0]);
      // Compute TOF for first scatter event
      const double vel0 = sqrt(en1[0]/MASS_TO_MEV);
      tofs[0] += (distFromStart*1e6/vel0);

      // multiple scatter events within sample, i.e not including zeroth
      for(size_t i = 1; i < nscatters; ++i)
      {
        weights[i] = weights[i-1];
        tofs[i] = tofs[i-1];

        // Generate a new direction of travel
        V3D oldDir = particleDir;
        size_t ntries(0);
        do
        {
          double randth = acos(2.0*m_randgen->flat() - 1.0);
          double randphi = 2.0*M_PI*m_randgen->flat();
          particleDir.spherical_rad(1.0, randth, randphi);
          particleDir.normalize();
          scAngs[i-1] = particleDir.angle(oldDir);
          // Update weight
          const double wgt = weights[i];
          if(generateScatter(scatterPts[i-1], particleDir, weights[i], scatterPts[i]))
            break;
          else
          {
            weights[i] = wgt; // put it back to what it was
            ++ntries;
          }
        }
        while(ntries < MAX_SCATTER_PT_TRIES);
        if(ntries == MAX_SCATTER_PT_TRIES)
        {
          throw std::runtime_error("Unable to generate scatter point in sample. Check sample shape.");
        }

        auto e1range = calculateE1Range(scAngs[i-1], en1[i-1]);
        en1[i] = e1range.first + m_randgen->flat()*(e1range.second - e1range.first);
        const double d2sig = partialDiffXSec(en1[i-1], en1[i], scAngs[i-1]);
        double weight = d2sig*4.0*M_PI*(e1range.second - e1range.first)/m_sampleProps->totalxsec;
        // accumulate total weight
        weightSum += weight;
        weights[i] *= weight; // account for this scatter on top of previous

        // Increment time of flight...
        const double veli = sqrt(en1[i]/MASS_TO_MEV);
        tofs[i] += (scatterPts[i].distance(scatterPts[i-1])*1e6/veli);
      }

      // force all orders in to current detector
      const auto & inX = m_inputWS->readX(0);
      for(size_t i = 0; i < nscatters; ++i)
      {
        V3D detPos = generateDetectorPos(detpar.l2, detpar.theta, en1[i]);
        // transform to sample frame
        detPos.rotate(*m_goniometer);
        // Distance to exit the sample for this order
        V3D detDirection = detPos - scatterPts[i];
        detDirection.normalize();
        Geometry::Track scatterToDet(scatterPts[i], detDirection);
        if(m_sampleShape->interceptSurface(scatterToDet) == 0)
        {
          throw std::logic_error("CalculateMSVesuvio::calculateCounts() - "
                                 "Logical error. No intersection with sample, despite track "
                                 "originating from with sample.");
        }
        const auto & link = scatterToDet.begin();
        double distToExit = link->distInsideObject;
        // Weight by probability neutron leaves sample
        weights[i] *= exp(-m_sampleProps->mu*distToExit);

        // Weight by cross-section for the final energy
        const double efinal = generateE1(detpar.theta, detpar.efixed, m_foilRes);
        weights[i] *= partialDiffXSec(en1[i], efinal, scAngs[i])/m_sampleProps->totalxsec;

        // final TOF
        const double veli = sqrt(efinal/MASS_TO_MEV);
        tofs[i] += detpar.t0 + (scatterPts[i].distance(detPos)*1e6)/veli;

        // "Bin" weight into appropriate place
        std::vector<double> &counts = simulation.counts[i];
        const double finalTOF = tofs[i];
        auto uppIter = std::upper_bound(inX.begin(), inX.end(), finalTOF);
        if(uppIter != inX.begin())
        {
          // See which side of line between us and previous value it should fall
          auto prevIter = uppIter - 1;
          if(finalTOF < *uppIter - 0.5*(*uppIter - *prevIter)) --uppIter;
        }
        size_t idx = std::distance(inX.begin(), uppIter);
        counts[idx] += weights[i];
      }

      return weightSum;
    }

    /**
      * Sample from the moderator assuming it can be seen
      * as a cylindrical ring with inner and outer radius
      * @param l1 Src-sample distance (m)
      * @returns Position on the moderator of the generated point
      */
    V3D CalculateMSVesuvio::generateSrcPos(const double l1) const
    {
      double radius(-1.0), widthPos(0.0), heightPos(0.0);
      do
      {
        widthPos = -m_srcR2 + 2.0*m_srcR2*m_randgen->flat();
        heightPos = -m_srcR2 + 2.0*m_srcR2*m_randgen->flat();
        using std::sqrt;
        radius = sqrt(widthPos*widthPos + heightPos*heightPos);
      }
      while(radius > m_srcR2);
      // assign to output
      V3D srcPos;
      srcPos[m_acrossIdx] = widthPos;
      srcPos[m_upIdx] = heightPos;
      srcPos[m_beamIdx] = -l1;

      return srcPos;
    }

    /**
     * Generate an incident energy based on a randomly-selected TOF value
     * It is assigned a weight = (2.0*E0/(T-t2))/E0^0.9.
     * @param l1 Distance from src to sample (metres)
     * @param t2 Nominal time from sample to detector (seconds)
     * @param weight [Out] Weight factor to modify for the generated energy value
     * @return
     */
    double CalculateMSVesuvio::generateE0(const double l1, const double t2, double &weight) const
    {
      const double tof = m_tmin + (m_tmax - m_tmin)*m_randgen->flat();
      const double t1 = (tof - t2);
      const double vel0 = l1/t1;
      const double en0 = MASS_TO_MEV*vel0*vel0;

      weight = 2.0*en0/t1/pow(en0, 0.9);
      weight *= 1e-4; // Reduce weight to ~1

      return en0;
    }

    /**
     * Generate an initial tof from this distribution:
     * 1-(0.5*X**2/T0**2+X/T0+1)*EXP(-X/T0), where x is the time and t0
     * is the src-sample time.
     * @param dt0 Error in time resolution (us)
     * @param en0 Value of the incident energy
     * @param dl1 S.d of moderator to sample distance
     * @return tof Guass TOF modified for asymmetric pulse
     */
    double CalculateMSVesuvio::generateTOF(const double en0, const double dt0,
                                           const double dl1) const
    {
      const double vel1 = sqrt(en0/MASS_TO_MEV);
      const double dt1 = (dl1/vel1)*1e6;
      const double xmin(0.0), xmax(15.0*dt1);
      double dx = 0.5*(xmax - xmin);
      // Generate a random y position in th distribution
      const double yv = m_randgen->flat();

      double xt(xmin);
      double tof = m_randgen->gaussian(0.0, dt0);
      while(true)
      {
        xt += dx;
        //Y=1-(0.5*X**2/T0**2+X/T0+1)*EXP(-X/T0)
        double y = 1.0 - (0.5*xt*xt/(dt1*dt1) + xt/dt1 + 1)*exp(-xt/dt1);
        if(fabs(y - yv) < 1e-4)
        {
          tof += xt - 3*dt1;
          break;
        }
        if(y > yv)
        {
          dx = -fabs(0.5*dx);
        }
        else
        {
          dx = fabs(0.5*dx);
        }
      }
      return tof;
    }

    /**
     * Generate a scatter event and update the weight according to the
     * amount the beam would be attenuted by the sample
     * @param startPos Starting position
     * @param direc Direction of travel for the neutron
     * @param weight [InOut] Multiply the incoming weight by the attenuation factor
     * @param scatterPt [Out] Generated scattering point
     * @return True if the scatter event was generated, false otherwise
     */
    bool CalculateMSVesuvio::generateScatter(const Kernel::V3D &startPos, const Kernel::V3D &direc,
                                             double &weight, V3D &scatterPt) const
    {
      Track particleTrack(startPos, direc);
      if(m_sampleShape->interceptSurface(particleTrack) != 1)
      {
        return false;
      }
      // Find distance inside object and compute probability of scattering
      const auto & link = particleTrack.begin();
      double totalObjectDist = link->distInsideObject;
      const double scatterProb = 1.0 - exp(-m_sampleProps->mu*totalObjectDist);
      // Select a random point on the track that is the actual scatter point
      // from the scattering probability distribution
      const double dist = -log(1.0 - m_randgen->flat()*scatterProb)/m_sampleProps->mu;
      // From start point advance in direction of travel by computed distance to find scatter point
      // Track is defined as set of links and exit point of first link is entry to sample!
      scatterPt = link->entryPoint;
      scatterPt += direc*dist;
      // Update weight
      weight *= scatterProb;

      return true;
    }

    /**
     * @param theta Neutron scattering angle (radians)
     * @param en0 Computed incident energy
     * @return The range of allowed final energies for the neutron
     */
    std::pair<double, double> CalculateMSVesuvio::calculateE1Range(const double theta, const double en0) const
    {
      const double k0 = sqrt(en0/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      const double sth(sin(theta)), cth(cos(theta));

      double e1min(1e10), e1max(-1e10); // large so that anything else is smaller
      const auto & atoms = m_sampleProps->atoms;
      for(size_t i = 0; i < atoms.size(); ++i)
      {
        const double mass = atoms[i].mass;

        const double fraction = (cth + sqrt(mass*mass - sth*sth))/(1.0 + mass);
        const double k1 = fraction*k0;
        const double en1 = PhysicalConstants::E_mev_toNeutronWavenumberSq*k1*k1;
        const double qr = sqrt(k0*k0 + k1*k1 - 2.0*k0*k1*cth);
        const double wr = en0 - en1;
        const double width = PhysicalConstants::E_mev_toNeutronWavenumberSq*atoms[i].profile*qr/mass;
        const double e1a = en0 - wr - 10.0*width;
        const double e1b = en0 - wr + 10.0*width;
        if(e1a < e1min) e1min = e1a;
        if(e1b > e1min) e1max = e1b;
      }
      if(e1min < 0.0) e1min = 0.0;
      return std::make_pair(e1min, e1max);
    }

    /**
     * Compute the partial differential cross section for this energy and theta.
     * @param en0 Initial energy (meV)
     * @param en1 Final energy (meV)
     * @param theta Scattering angle
     * @return Value of differential cross section
     */
    double CalculateMSVesuvio::partialDiffXSec(const double en0, const double en1, const double theta) const
    {
      const double rt2pi = sqrt(2.0*M_PI);

      const double k0 = sqrt(en0/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      const double k1 = sqrt(en1/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      const double q = sqrt(k0*k0 + k1*k1 - 2.0*k0*k1*cos(theta));
      const double w = en0 - en1;

      double pdcs(0.0);
      const auto & atoms = m_sampleProps->atoms;
      if(q > 0.0) // avoid continuous checking in loop
      {
        for(size_t i = 0; i < atoms.size(); ++i)
        {
          const double jstddev = atoms[i].profile;
          const double mass = atoms[i].mass;
          const double y = mass*w/(4.18036*q) - 0.5*q;
          const double jy = exp(-0.5*y*y/(jstddev*jstddev))/(jstddev*rt2pi);
          const double sqw = mass*jy/(4.18036*q);

          const double sclength = atoms[i].sclength;
          pdcs += sclength*sclength*(k1/k0)*sqw;
        }
      }
      else
      {
        for(size_t i = 0; i < atoms.size(); ++i)
        {
          const double sclength = atoms[i].sclength;
          pdcs += sclength*sclength;
        }
      }

      return pdcs;
    }

    /**
     * Generate a random position within the final detector in the lab frame
     * @param l2 The nominal distance from sample to detector
     * @param angle The The scattering angle from the sample
     * @param energy The final energy of the neutron
     * @return A new position in the detector
     */
    V3D CalculateMSVesuvio::generateDetectorPos(const double l2, const double angle, const double energy) const
    {
      const double mu = 7430.0/sqrt(energy); // Inverse attenuation length (m-1) for vesuvio det.
      const double ps = 1.0 - exp(-mu*m_detThick); // Probability of detection in path length YD.

      const double width = -0.5*m_detWidth + m_detWidth*m_randgen->flat();
      const double beam = -log(1.0 - m_randgen->flat()*ps)/mu;
      const double height = -0.5*m_detHeight + m_detHeight*m_randgen->flat();
      const double widthLab = (l2 + beam)*sin(angle) + width*cos(angle);
      const double beamLab = (l2 + beam)*cos(angle) - width*sin(angle);
      V3D detPos;
      detPos[m_beamIdx] = beamLab;
      detPos[m_acrossIdx] = widthLab;
      detPos[m_upIdx] = height;

      return detPos;
    }

    /**
     * Generate the final energy of the analyser
     * @param angle Detector angle from sample
     * @param e1nom The nominal final energy of the analyzer
     * @param e1res The resoltion in energy of the analyser
     * @return A value for the final energy of the neutron
     */
    double CalculateMSVesuvio::generateE1(const double angle, const double e1nom,
                                          const double e1res) const
    {
      if(e1res == 0.0) return e1nom;

      const double randv = m_randgen->flat();
      if(e1nom < 5000.0)
      {
        if(angle > 90.0) return finalEnergyAuDD(randv);
        else return finalEnergyAuYap(randv);
      }
      else
      {
        return finalEnergyUranium(randv);
      }
    }

  } // namespace Algorithms
} // namespace Mantid

#ifndef __Mt_Motionmask_H__
#define __Mt_Motionmask_H__

#include "../../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

typedef bool (Processor)(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nMotionThreshold, int nSceneChange, int nSceneChangeValue, int nWidth, int nHeight, IScriptEnvironment* env);
typedef bool (Processor16)(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nMotionThreshold, int nSceneChange, int nSceneChangeValue, int nWidth, int nHeight, IScriptEnvironment* env);
typedef bool (Processor32)(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, Float nLowThreshold, Float nHighThreshold, Float nMotionThreshold, int nSceneChange, Float nSceneChangeValue_f, int nWidth, int nHeight, IScriptEnvironment* env);

extern Processor *mask_c;
extern Processor *mask_sse2;
extern Processor *mask_asse2;

extern Processor16 *mask10_16_c;
extern Processor16 *mask10_16_sse2;
extern Processor16 *mask10_16_asse2;

extern Processor32 *mask32_c;
extern Processor32 *mask32_sse2;
extern Processor32 *mask32_asse2;

class MotionMask : public MaskTools::Filter
{
   int nLowThresholds[4];
   int nHighThresholds[4];
   int nMotionThreshold;

   float nLowThresholds_f[4];
   float nHighThresholds_f[4];
   float nMotionThreshold_f;

   ProcessorList<Processor> processors;
   ProcessorList<Processor16> processors16;
   ProcessorList<Processor32> processors32;

   int nSceneChange;
   
   int nSceneChangeValue;
   Float nSceneChangeValue_f;

   int bits_per_pixel;

protected:

    virtual void process(int n, const Plane<Byte> &dst, int nPlane, const Filtering::Frame<const Byte> frames[4], const Constraint constraints[4], IScriptEnvironment* env) override
    {
        UNUSED(n);
        if (bits_per_pixel == 8) {
          if (nPlane == 0)
            nSceneChange = processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              nLowThresholds[nPlane], nHighThresholds[nPlane], nMotionThreshold, 0, nSceneChangeValue, dst.width(), dst.height(), env) ? 3 : 2;
          else
            processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              nLowThresholds[nPlane], nHighThresholds[nPlane], nMotionThreshold, nSceneChange, nSceneChangeValue, dst.width(), dst.height(), env);
        }
        else if (bits_per_pixel <= 16) {
          if (nPlane == 0)
            nSceneChange = processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              nLowThresholds[nPlane], nHighThresholds[nPlane], nMotionThreshold, 0, nSceneChangeValue, dst.width(), dst.height(), env) ? 3 : 2;
          else
            processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              nLowThresholds[nPlane], nHighThresholds[nPlane], nMotionThreshold, nSceneChange, nSceneChangeValue, dst.width(), dst.height(), env);
        }
        else {
          if (nPlane == 0)
            nSceneChange = processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              nLowThresholds_f[nPlane], nHighThresholds_f[nPlane], nMotionThreshold_f, 0, nSceneChangeValue_f, dst.width(), dst.height(), env) ? 3 : 2;
          else
            processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
              frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
              nLowThresholds_f[nPlane], nHighThresholds_f[nPlane], nMotionThreshold_f, nSceneChange, nSceneChangeValue_f, dst.width(), dst.height(), env);
        }
    }

public:
  MotionMask(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
  {
    bits_per_pixel = bit_depths[C];
    bool isFloat = bits_per_pixel == 32;

    bool fullscale = planes_isRGB[C];

    String scalemode = parameters["paramscale"].toString();

    int thY1, thC1, thY2, thC2, thT, scvalue;
    float thY1_f, thC1_f, thY2_f, thC2_f, thT_f, scvalue_f;

    // defaults
    thY1_f = thC1_f = thY2_f = thC2_f = thT_f = 10.0f / 255;
    thY1 = thC1 = thY2 = thC2 = thT = 10 << (bits_per_pixel - 8);
    scvalue = 0;
    scvalue_f = 0.0f;

    const char *errortxt = "invalid parameter: paramscale. Use i8, i10, i12, i14, i16, f32 for scale or none/empty to disable scaling";

     // Y threshold
    if (parameters["thY1"].is_defined()) {
      thY1_f = (float)parameters["thY1"].toFloat();
      if (!ScaleParam(scalemode, thY1_f, bits_per_pixel, thY1_f, thY1, fullscale, false))
      {
        error = errortxt;
        return;
      }
    }

    if (parameters["thY2"].is_defined()) {
      thY2_f = (float)parameters["thY2"].toFloat();
      if (!ScaleParam(scalemode, thY2_f, bits_per_pixel, thY2_f, thY2, fullscale, false))
      {
        error = errortxt;
        return;
      }
    }

   // chroma threshold
    if (parameters["thC1"].is_defined()) {
      thC1_f = (float)parameters["thC1"].toFloat();
      if (!ScaleParam(scalemode, thC1_f, bits_per_pixel, thC1_f, thC1, fullscale, false))
      {
        error = errortxt;
        return;
      }
    }

    if (parameters["thC2"].is_defined()) {
      thC2_f = (float)parameters["thC2"].toFloat();
      if (!ScaleParam(scalemode, thC2_f, bits_per_pixel, thC2_f, thC2, fullscale, false))
      {
        error = errortxt;
        return;
      }
    }

    // motion threshold
    if (parameters["thT"].is_defined()) {
      thT_f = (float)parameters["thT"].toFloat();
      if (!ScaleParam(scalemode, thT_f, bits_per_pixel, thT_f, thT, fullscale, false))
      {
        error = errortxt;
        return;
      }
    }

    // scvalue: value copied into the whole frame when scenechange is detected
    if (parameters["sc_value"].is_defined()) {
      scvalue_f = (float)parameters["sc_value"].toFloat();
      if (!ScaleParam(scalemode, scvalue_f, bits_per_pixel, scvalue_f, scvalue, fullscale, false))
      {
        error = errortxt;
        return;
      }
    }

    if (isFloat) {
      nLowThresholds_f[0] = thY1_f;
      nLowThresholds_f[1] = nLowThresholds_f[2] = thC1_f;
      nHighThresholds_f[0] = thY2_f;
      nHighThresholds_f[1] = nHighThresholds_f[2] = thC2_f;
      nMotionThreshold_f = thT_f;
      nSceneChangeValue_f = scvalue_f;
      nLowThresholds_f[3] = thY1_f; // no separate param for alpha
      nHighThresholds_f[3] = thY2_f;
    }
    else {
      nLowThresholds[0] = thY1;
      nLowThresholds[1] = nLowThresholds[2] = thC1;
      nHighThresholds[0] = thY2;
      nHighThresholds[1] = nHighThresholds[2] = thC2;
      nMotionThreshold = thT;
      nSceneChangeValue = scvalue;
      nLowThresholds[3] = thY1; // no separate param for alpha
      nHighThresholds[3] = thY2;
    }

    /* add the processors */
    switch (bits_per_pixel) {
    case 8:
      processors.push_back(Filtering::Processor<Processor>(mask_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      processors.push_back(Filtering::Processor<Processor>(mask_sse2, Constraint(CPU_SSE2, 1, 1, 1, 1), 1));
      processors.push_back(Filtering::Processor<Processor>(mask_asse2, Constraint(CPU_SSE2, 1, 1, 16, 16), 2));
      break;
    case 10:
    case 12:
    case 14:
    case 16:
      processors16.push_back(Filtering::Processor<Processor16>(mask10_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      processors16.push_back(Filtering::Processor<Processor16>(mask10_16_sse2, Constraint(CPU_SSE2, 1, 1, 1, 1), 1));
      processors16.push_back(Filtering::Processor<Processor16>(mask10_16_asse2, Constraint(CPU_SSE2, 1, 1, 16, 16), 2));
      break;
    case 32:
      processors32.push_back(Filtering::Processor<Processor32>(mask32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      processors32.push_back(Filtering::Processor<Processor32>(mask32_sse2, Constraint(CPU_SSE2, 1, 1, 1, 1), 1)); // only sad is sse2
      processors32.push_back(Filtering::Processor<Processor32>(mask32_asse2, Constraint(CPU_SSE2, 1, 1, 16, 16), 2));
      break;
    }
  }

   InputConfiguration &input_configuration() const { return InPlaceTemporalOneFrame(); }
	 InputConfiguration &input_configuration_cuda() const { return TemporalOneFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "kmt_motion";

      signature.add(Parameter(TYPE_CLIP, "", false));
      signature.add(Parameter(10.0f, "thY1", true));
      signature.add(Parameter(10.0f, "thY2", true));
      signature.add(Parameter(10.0f, "thC1", true));
      signature.add(Parameter(10.0f, "thC2", true));
      signature.add(Parameter(10.0f, "thT", true));
      signature.add(Parameter(0.0f, "sc_value", true));

      add_defaults( signature );

      return signature;
   }

};

} } } } }

#endif
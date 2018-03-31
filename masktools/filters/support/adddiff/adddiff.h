#ifndef __Mt_AddDiff_H__
#define __Mt_AddDiff_H__

#include "../../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Support { namespace AddDiff {
/* 8 bit */
typedef void(Processor)(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight);
typedef void(Processor16)(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int nOrigHeight);
typedef void(Processor32)(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight);

Processor adddiff_c;
extern Processor *adddiff_sse2;
extern Processor *adddiff_asse2;

/* 16 bit */

Processor16 adddiff16_stacked_c;
Processor16 adddiff16_stacked_sse2;

extern Processor16 *adddiff16_native_10_c;
extern Processor16 *adddiff16_native_12_c;
extern Processor16 *adddiff16_native_14_c;
extern Processor16 *adddiff16_native_16_c;

extern Processor16 *adddiff16_native_10_sse2;
extern Processor16 *adddiff16_native_12_sse2;
extern Processor16 *adddiff16_native_14_sse2;
extern Processor16 *adddiff16_native_16_sse2;

extern Processor16 *adddiff16_native_10_sse4_1;
extern Processor16 *adddiff16_native_12_sse4_1;
extern Processor16 *adddiff16_native_14_sse4_1;
extern Processor16 *adddiff16_native_16_sse4_1;

Processor32 adddiff32_c;
extern Processor32 *adddiff32_sse2;
extern Processor32 *adddiff32_asse2;

class AddDiff : public MaskTools::Filter
{
    ProcessorList<Processor> processors;
    ProcessorList<Processor16> processors16;
    ProcessorList<Processor32> processors32;

protected:
    virtual void process(int n, const Plane<Byte> &dst, int nPlane, const Filtering::Frame<const Byte> frames[4], const Constraint constraints[4], IScriptEnvironment2* env) override
    {
        UNUSED(n); UNUSED(env);
        int bits_per_pixel = bit_depths[C];
        if (parameters["stacked"].toBool())
          bits_per_pixel = 16;
        if (bits_per_pixel == 8)
          processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            dst.width(), dst.height());
        else if(bits_per_pixel<=16)
          processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(), 
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            dst.width(), dst.height(), dst.origheight());
        else
          processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),
            frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(),
            dst.width(), dst.height());
    }

public:
    AddDiff(const Parameters &parameters, CpuFlags cpuFlags, IScriptEnvironment2* env)
       : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
    {
       UNUSED(env);
      bool isStacked = parameters["stacked"].toBool();
      int bits_per_pixel = bit_depths[C];

      if (isStacked && bits_per_pixel != 8) {
        error = "Stacked specified for a non-8 bit clip";
        return;
      }

      if (isStacked)
        bits_per_pixel = 16;

      /* add the processors */
      if (parameters["stacked"].toBool() == true) {
        processors16.push_back(Filtering::Processor<Processor16>(adddiff16_stacked_c, Constraint(CPU_NONE, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 0));
        processors16.push_back(Filtering::Processor<Processor16>(adddiff16_stacked_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
      }
      else {
        switch (bits_per_pixel) {
        case 8:
          processors.push_back(Filtering::Processor<Processor>(&adddiff_c, Constraint(CPU_NONE, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 0));
          processors.push_back(Filtering::Processor<Processor>(adddiff_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
          processors.push_back(Filtering::Processor<Processor>(adddiff_asse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 2));
          break;
        case 10:
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_10_c, Constraint(CPU_NONE, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 0));
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_10_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_10_sse4_1, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 2));
          break;
        case 12:
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_12_c, Constraint(CPU_NONE, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 0));
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_12_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_12_sse4_1, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 2));
          break;
        case 14:
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_14_c, Constraint(CPU_NONE, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 0));
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_14_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_14_sse4_1, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 2));
          break;
        case 16:
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_16_c, Constraint(CPU_NONE, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 0));
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_16_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
          processors16.push_back(Filtering::Processor<Processor16>(adddiff16_native_16_sse4_1, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 2));
          break;
        case 32:
          processors32.push_back(Filtering::Processor<Processor32>(&adddiff32_c, Constraint(CPU_NONE, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 0));
          processors32.push_back(Filtering::Processor<Processor32>(adddiff32_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1));
          processors32.push_back(Filtering::Processor<Processor32>(adddiff32_asse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 2));
          break;
        }
      }

    }

    InputConfiguration &input_configuration() const { return InPlaceTwoFrame(); }
		InputConfiguration &input_configuration_cuda() const { return TwoFrame(); }

    static Signature filter_signature()
    {
        Signature signature = "kmt_adddiff";

        signature.add(Parameter(TYPE_CLIP, "", false));
        signature.add(Parameter(TYPE_CLIP, "", false));

        add_defaults(signature);

        signature.add(Parameter(false, "stacked", false));
        return signature;
    }

};

} } } } }

#endif
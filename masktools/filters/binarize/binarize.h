#ifndef __Mt_Binarize_H__
#define __Mt_Binarize_H__

#include "../../common/base/filter.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

typedef void(Processor)(Byte *pDst, ptrdiff_t nDstPitch, Byte nThreshold, int nWidth, int nHeight);
typedef void(Processor16)(Byte *pDst, ptrdiff_t nDstPitch, Word nThreshold, int nWidth, int nHeight, int nOrigHeight);
typedef void(Processor32)(Byte *pDst, ptrdiff_t nDstPitch, Float nThreshold, int nWidth, int nHeight);

#define DEFINE_PROCESSOR(name) \
   extern Processor *binarize_##name##_c; \
   extern Processor *binarize_##name##_sse2; \
   extern Processor *binarize_##name##_asse2; \
   extern Processor *binarize_##name##_avx2

DEFINE_PROCESSOR(upper);
DEFINE_PROCESSOR(lower);
DEFINE_PROCESSOR(0_x);
DEFINE_PROCESSOR(t_x);
DEFINE_PROCESSOR(x_0);
DEFINE_PROCESSOR(x_t);
DEFINE_PROCESSOR(t_0);
DEFINE_PROCESSOR(0_t);
DEFINE_PROCESSOR(x_255);
DEFINE_PROCESSOR(t_255);
DEFINE_PROCESSOR(255_x);
DEFINE_PROCESSOR(255_t);

#undef DEFINE_PROCESSOR

/* 16 bits */

#define DEFINE_PROCESSOR(name) \
extern Processor16 *binarize_##name##_stacked_16_c; \
extern Processor16 *binarize_##name##_stacked_16_sse2; \
extern Processor16 *binarize_##name##_native_10_c; \
extern Processor16 *binarize_##name##_native_10_sse2; \
extern Processor16 *binarize_##name##_native_12_c; \
extern Processor16 *binarize_##name##_native_12_sse2; \
extern Processor16 *binarize_##name##_native_14_c; \
extern Processor16 *binarize_##name##_native_14_sse2; \
extern Processor16 *binarize_##name##_native_16_c; \
extern Processor16 *binarize_##name##_native_16_sse2;

DEFINE_PROCESSOR(upper);
DEFINE_PROCESSOR(lower);
DEFINE_PROCESSOR(0_x);
DEFINE_PROCESSOR(t_x);
DEFINE_PROCESSOR(x_0);
DEFINE_PROCESSOR(x_t);
DEFINE_PROCESSOR(t_0);
DEFINE_PROCESSOR(0_t);
DEFINE_PROCESSOR(x_255);
DEFINE_PROCESSOR(t_255);
DEFINE_PROCESSOR(255_x);
DEFINE_PROCESSOR(255_t);

#undef DEFINE_PROCESSOR

/* 32 bits */
#define DEFINE_PROCESSOR(name) \
   extern Processor32 *binarize32_##name##_c; \
   extern Processor32 *binarize32_##name##_sse2; \
   extern Processor32 *binarize32_##name##_asse2

DEFINE_PROCESSOR(upper);
DEFINE_PROCESSOR(lower);
DEFINE_PROCESSOR(0_x);
DEFINE_PROCESSOR(t_x);
DEFINE_PROCESSOR(x_0);
DEFINE_PROCESSOR(x_t);
DEFINE_PROCESSOR(t_0);
DEFINE_PROCESSOR(0_t);
DEFINE_PROCESSOR(x_255);
DEFINE_PROCESSOR(t_255);
DEFINE_PROCESSOR(255_x);
DEFINE_PROCESSOR(255_t);

#undef DEFINE_PROCESSOR


class Binarize : public MaskTools::Filter
{
   ProcessorList<Processor> processors;
   ProcessorList<Processor16> processors16;
   ProcessorList<Processor32> processors32;

   int nThreshold;
   float nThreshold_f;

   int bits_per_pixel;
protected:
    virtual void process(int n, const Plane<Byte> &dst, int nPlane, const Frame<const Byte> frames[4], const Constraint constraints[4], IScriptEnvironment2* env) override
    {
        UNUSED(n); UNUSED(frames); UNUSED(env);
        if(bits_per_pixel == 8)
          processors.best_processor(constraints[nPlane])(dst.data(), dst.pitch(), (Byte)nThreshold, dst.width(), dst.height());
        else if(bits_per_pixel <= 16)
          processors16.best_processor(constraints[nPlane])(dst.data(), dst.pitch(),(Word)nThreshold, dst.width(), dst.height(), dst.origheight());
        else
          processors32.best_processor(constraints[nPlane])(dst.data(), dst.pitch(), nThreshold_f, dst.width(), dst.height());
    }

   bool isMode(const char *mode) {
       return parameters["mode"].is_defined() && parameters["mode"].toString() == mode;
   }

public:
   Binarize(const Parameters &parameters, CpuFlags cpuFlags, IScriptEnvironment2 *env)
      : MaskTools::Filter(parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
   {
     UNUSED(env);
     bool isStacked = parameters["stacked"].toBool();
     bits_per_pixel = bit_depths[C];

     if (isStacked && bits_per_pixel != 8) {
       error = "Stacked specified for a non-8 bit clip";
       return;
     }

     if (isStacked)
       bits_per_pixel = 16;

     bool isFloat = bits_per_pixel == 32;
     
     if (parameters["threshold"].is_defined()) {
       nThreshold_f = (float)parameters["threshold"].toFloat();
       nThreshold = parameters["threshold"].toInt();
       String scalemode = parameters["paramscale"].toString();
       bool fullscale = planes_isRGB[C];
       if (!ScaleParam(scalemode, nThreshold_f, bits_per_pixel, nThreshold_f, nThreshold, fullscale, false))
       {
         error = "invalid parameter: paramscale. Use i8, i10, i12, i14, i16, f32 for scale or none/empty to disable scaling";
         return;
       }
     }
     else {
       if(isFloat)
         nThreshold_f = 0.5f; 
       else 
         nThreshold = (1 << (bits_per_pixel - 1)); // bit-depth adaptive default half value
     }

     if (bits_per_pixel == 8) {
#define SET_MODE(mode) \
      processors.push_back(Filtering::Processor<Processor>(binarize_##mode##_c, Constraint(CPU_NONE, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 0)); \
      processors.push_back(Filtering::Processor<Processor>(binarize_##mode##_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1)); \
      processors.push_back(Filtering::Processor<Processor>(binarize_##mode##_asse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 2)); \
      /*processors.push_back(Filtering::Processor<Processor>(binarize_##mode##_avx2, Constraint(CPU_AVX2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 3));*/
      // avx2: not any faster than 128bit sse2, don't go live

       if (isMode("0 x")) { SET_MODE(0_x); }
       else if (isMode("t x")) { SET_MODE(t_x); }
       else if (isMode("x 0")) { SET_MODE(x_0); }
       else if (isMode("x t")) { SET_MODE(x_t); }
       else if (isMode("t 0")) { SET_MODE(t_0); }
       else if (isMode("0 t")) { SET_MODE(0_t); }
       else if (isMode("x 255")) { SET_MODE(x_255); }
       else if (isMode("t 255")) { SET_MODE(t_255); }
       else if (isMode("255 x")) { SET_MODE(255_x); }
       else if (isMode("255 t")) { SET_MODE(255_t); }
       else if ((parameters["mode"].is_defined() && (parameters["mode"].toString() == "upper" || parameters["mode"].toString() == "0 255")) || (!parameters["mode"].is_defined() && parameters["upper"].toBool())) {
         SET_MODE(upper);
       }
       else {
         SET_MODE(lower);
       }
#undef SET_MODE
     
     }
     else if (bits_per_pixel <= 16) {

#define SET_MODE(mode) \
    if (isStacked) { \
        processors16.push_back( Filtering::Processor<Processor16>( binarize_##mode##_stacked_16_c, Constraint( CPU_NONE, 1, 1, 1, 1 ), 0 ) ); \
        processors16.push_back( Filtering::Processor<Processor16>( binarize_##mode##_stacked_16_sse2, Constraint( CPU_SSE2 , 1, 1, 1, 1 ), 1 ) ); \
    } else { \
        switch(bit_depths[C]) { \
        case 10: processors16.push_back(Filtering::Processor<Processor16>(binarize_##mode##_native_10_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); \
                 processors16.push_back( Filtering::Processor<Processor16>( binarize_##mode##_native_10_sse2, Constraint( CPU_SSE2 , 1, 1, 1, 1 ), 1 ) ); \
                 break; \
        case 12: processors16.push_back(Filtering::Processor<Processor16>(binarize_##mode##_native_12_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); \
                 processors16.push_back( Filtering::Processor<Processor16>( binarize_##mode##_native_12_sse2, Constraint( CPU_SSE2 , 1, 1, 1, 1 ), 1 ) ); \
                 break; \
        case 14: processors16.push_back(Filtering::Processor<Processor16>(binarize_##mode##_native_14_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); \
                 processors16.push_back( Filtering::Processor<Processor16>( binarize_##mode##_native_14_sse2, Constraint( CPU_SSE2 , 1, 1, 1, 1 ), 1 ) ); \
                 break; \
        case 16: processors16.push_back(Filtering::Processor<Processor16>(binarize_##mode##_native_16_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0)); \
                 processors16.push_back( Filtering::Processor<Processor16>( binarize_##mode##_native_16_sse2, Constraint( CPU_SSE2 , 1, 1, 1, 1 ), 1 ) ); \
                 break; \
        }\
    }
       if (isMode("0 x")) { SET_MODE(0_x); } // e.g. binarize_0_x_10_native_c instead of binarize_0_x_native_c
       else if (isMode("t x")) { SET_MODE(t_x); }
       else if (isMode("x 0")) { SET_MODE(x_0); }
       else if (isMode("x t")) { SET_MODE(x_t); }
       else if (isMode("t 0")) { SET_MODE(t_0); }
       else if (isMode("0 t")) { SET_MODE(0_t); }
       else if (isMode("x 255")) { SET_MODE(x_255); }
       else if (isMode("t 255")) { SET_MODE(t_255); }
       else if (isMode("255 x")) { SET_MODE(255_x); }
       else if (isMode("255 t")) { SET_MODE(255_t); }
       else if ((parameters["mode"].is_defined() && (parameters["mode"].toString() == "upper" || parameters["mode"].toString() == "0 255")) || (!parameters["mode"].is_defined() && parameters["upper"].toBool())) {
         SET_MODE(upper);
       }
       else {
         SET_MODE(lower);
       }
#undef SET_MODE

     }
     else { // float
#define SET_MODE(mode) \
      processors32.push_back(Filtering::Processor<Processor32>(binarize32_##mode##_c, Constraint(CPU_NONE, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 0)); \
      processors32.push_back(Filtering::Processor<Processor32>(binarize32_##mode##_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 1), 1)); \
      processors32.push_back(Filtering::Processor<Processor32>(binarize32_##mode##_asse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 2));

       if (isMode("0 x")) { SET_MODE(0_x); }
       else if (isMode("t x")) { SET_MODE(t_x); }
       else if (isMode("x 0")) { SET_MODE(x_0); }
       else if (isMode("x t")) { SET_MODE(x_t); }
       else if (isMode("t 0")) { SET_MODE(t_0); }
       else if (isMode("0 t")) { SET_MODE(0_t); }
       else if (isMode("x 255")) { SET_MODE(x_255); }
       else if (isMode("t 255")) { SET_MODE(t_255); }
       else if (isMode("255 x")) { SET_MODE(255_x); }
       else if (isMode("255 t")) { SET_MODE(255_t); }
       else if ((parameters["mode"].is_defined() && (parameters["mode"].toString() == "upper" || parameters["mode"].toString() == "0 255")) || (!parameters["mode"].is_defined() && parameters["upper"].toBool())) {
         SET_MODE(upper);
       }
       else {
         SET_MODE(lower);
       }
#undef SET_MODE


     }
   }

	 InputConfiguration &input_configuration() const { return InPlaceOneFrame(); }
	 InputConfiguration &input_configuration_cuda() const { return OneFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "kmt_binarize";

      signature.add( Parameter( TYPE_CLIP, "", false) );
      signature.add( Parameter( 128.0f, "threshold", true) ); // bit depth dependent, see above.
      signature.add( Parameter( false, "upper", false));
      signature.add( Parameter( String("lower"), "mode", false) );

      add_defaults( signature );

      signature.add(Parameter(false, "stacked", false));
      return signature;
   }
};

} } } } // namespace Binarize, Filter, MaskTools, Filtering

#endif
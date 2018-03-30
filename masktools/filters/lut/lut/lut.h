#ifndef __Mt_Lut_H__
#define __Mt_Lut_H__

#include "../../../common/base/filter.h"
#include "../../../../common/parser/parser.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace Single {

typedef void(Processor)(Byte *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, const Byte lut[256]);
typedef void(Processor16)(Byte *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, const Word lut[65536], int nOrigHeightForStacked);
typedef void(ProcessorCtx)(Byte *pDst, ptrdiff_t nDstPitch, int nWidth, int nHeight, Parser::Context &ctx);

Processor lut_c;

Processor16 lut16_c_native;
Processor16 lut16_c_stacked;

ProcessorCtx realtime8_c;
extern ProcessorCtx *realtime10_c;
extern ProcessorCtx *realtime12_c;
extern ProcessorCtx *realtime14_c;
extern ProcessorCtx *realtime16_c;
ProcessorCtx realtime32_c;

class Lut : public MaskTools::Filter
{
   Byte luts[4][256];
   Word *luts16[4]; // full size, even for 10 bits (avoid over addressing by invalid pixel values)

   // for realtime
   std::deque<Filtering::Parser::Symbol> *parsed_expressions[4];

   Processor *processor;
   Processor16 *processor16;
   ProcessorCtx *processorCtx;
   ProcessorCtx *processorCtx16;
   ProcessorCtx *processorCtx32;
   int bits_per_pixel;
   bool isStacked;
   bool realtime;

protected:
    virtual void process(int n, const Plane<Byte> &dst, int nPlane, const ::Filtering::Frame<const Byte> frames[4], const Constraint constraints[4], IScriptEnvironment* env) override
    {
        UNUSED(n);
        UNUSED(constraints);
        UNUSED(frames);
        UNUSED(env);
        if (realtime) {
          // thread safety
          Parser::Context ctx(*parsed_expressions[nPlane]);
          processorCtx(dst.data(), dst.pitch(), dst.width(), dst.height(), ctx);
        }
        else if (bits_per_pixel == 8)
          processor(dst.data(), dst.pitch(), dst.width(), dst.height(), luts[nPlane]);
        else if (bits_per_pixel <= 16)
          processor16(dst.data(), dst.pitch(), dst.width(), dst.height(), luts16[nPlane], dst.origheight());
    }

public:
   Lut(const Parameters &parameters, CpuFlags cpuFlags) : MaskTools::Filter( parameters, FilterProcessingType::INPLACE, (CpuFlags)cpuFlags)
   {
     for (int i = 0; i < 4; i++) {
        luts16[i] = nullptr;
        parsed_expressions[i] = nullptr;
      }

      static const char *expr_strs[] = { "yExpr", "uExpr", "vExpr", "aExpr" };

      Parser::Parser parser = Parser::getDefaultParser().addSymbol(Parser::Symbol::X);

      isStacked = parameters["stacked"].toBool();
      bits_per_pixel = bit_depths[C];
      int max_pixel_value = (1 << bits_per_pixel) - 1;

      if (isStacked && bits_per_pixel != 8) {
        error = "Stacked specified for a non-8 bit clip";
        return;
      }

      if (isStacked)
        bits_per_pixel = 16;

      realtime = parameters["realtime"].toBool();

      if (bits_per_pixel == 32) { // no lookup for float
        realtime = true;
      }

      if (realtime && isStacked) {
        error = "realtime calculation not supported for stacked clip";
        return;
      }

      /* compute the luts16 */
      for (int i = 0; i < 4; i++)
      {
        if (operators[i] != PROCESS) {
          continue;
        }

        if (parameters[expr_strs[i]].undefinedOrEmptyString() && parameters["expr"].undefinedOrEmptyString()) {
          operators[i] = NONE; //inplace
          continue;
        }

        if (parameters[expr_strs[i]].is_defined())
          parser.parse(parameters[expr_strs[i]].toString(), " ");
        else
          parser.parse(parameters["expr"].toString(), " ");

        Parser::Context ctx(parser.getExpression());

        if (!ctx.check())
        {
          error = "invalid expression in the lut";
          return;
        }

        if (realtime) {
          parsed_expressions[i] = new std::deque<Parser::Symbol>(parser.getExpression());

          switch (bits_per_pixel) {
          case 8: processorCtx = realtime8_c; break;
          case 10: processorCtx = realtime10_c; break;
          case 12: processorCtx = realtime12_c; break;
          case 14: processorCtx = realtime14_c; break;
          case 16: processorCtx = realtime16_c; break;
          case 32: processorCtx = realtime32_c; break;
          }
          continue;
        }

        // real lut
        if(bits_per_pixel >=8 && bits_per_pixel <= 16) 
          luts16[i] = reinterpret_cast<Word*>(_aligned_malloc(65536*sizeof(uint16_t), 16));

        switch(bits_per_pixel) {
        case 8: 
          for (int x = 0; x < 256; x++)
            luts[i][x] = ctx.compute_byte_x(x);
          break;
        case 10:
          for (int x = 0; x < 1024; x++)
            luts16[i][x] = ctx.compute_word_x<10>(x);
          break;
        case 12:
          for (int x = 0; x < 4096; x++)
            luts16[i][x] = ctx.compute_word_x<12>(x);
          break;
        case 14:
          for (int x = 0; x < 16384; x++)
            luts16[i][x] = ctx.compute_word_x<14>(x);
          break;
        case 16:
          // real or stacked 16 bit
          for (int x = 0; x < 65536; x++)
            luts16[i][x] = ctx.compute_word_x<16>(x);
          break;
        }
        // fill the rest against invalid input values for 10-14 bits
        if (bits_per_pixel > 8 && bits_per_pixel < 16) {
          for (int x = max_pixel_value; x < 65536; x++)
            luts16[i][x] = Word(max_pixel_value);
        }

        if (bits_per_pixel == 8) {
          processor = lut_c;
        }
        else {
          if (isStacked)
            processor16 = lut16_c_stacked;
          else
            processor16 = lut16_c_native;
        }
      }
   }

   ~Lut()
   {
     for (int i = 0; i < 4; i++) {
       _aligned_free(luts16[i]);
       delete parsed_expressions[i];
     }
   }

   InputConfiguration &input_configuration() const { return InPlaceOneFrame(); }
   InputConfiguration &input_configuration_cuda() const { return OneFrame(); }
   bool is_cuda_available() { return true; }

   static Signature filter_signature()
   {
      Signature signature = "kmt_lut";

      signature.add(Parameter(TYPE_CLIP, "", false));
      signature.add(Parameter(String("x"), "expr", false));
      signature.add(Parameter(String("x"), "yExpr", false));
      signature.add(Parameter(String("x"), "uExpr", false));
      signature.add(Parameter(String("x"), "vExpr", false));
      signature.add(Parameter(String("x"), "aExpr", false));

      add_defaults( signature );

      signature.add(Parameter(false, "stacked", false));
      signature.add(Parameter(false, "realtime", false));
      return signature;
   }
};


} } } } }

#endif
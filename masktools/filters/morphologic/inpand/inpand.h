#ifndef __Mt_Inpand_H__
#define __Mt_Inpand_H__

#include "../morphologic.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inpand {


extern Processor *inpand_square_c;
extern Processor *inpand_square_sse2;
extern Processor *inpand_square_asse2;

extern Processor *inpand_horizontal_c;
extern Processor *inpand_horizontal_sse2;
extern Processor *inpand_horizontal_asse2;

extern Processor *inpand_vertical_c;
extern Processor *inpand_vertical_sse2;
extern Processor *inpand_vertical_asse2;

extern Processor *inpand_both_c;
extern Processor *inpand_both_sse2;
extern Processor *inpand_both_asse2;

extern Processor *inpand_custom_c;

/* 16 bit */
extern StackedProcessor *inpand_square_stacked_c;
extern StackedProcessor *inpand_horizontal_stacked_c;
extern StackedProcessor *inpand_vertical_stacked_c;
extern StackedProcessor *inpand_both_stacked_c;
extern StackedProcessor *inpand_custom_stacked_c;

extern Processor16 *inpand_square_native_c;
extern Processor16 *inpand_horizontal_native_c;
extern Processor16 *inpand_vertical_native_c;
extern Processor16 *inpand_both_native_c;
extern Processor16 *inpand_custom_native_c;

extern Processor16 *inpand_square_sse4_16;
extern Processor16 *inpand_square_asse4_16;
extern Processor16 *inpand_horizontal_sse4_16;
extern Processor16 *inpand_horizontal_asse4_16;
extern Processor16 *inpand_vertical_sse4_16;
extern Processor16 *inpand_vertical_asse4_16;
extern Processor16 *inpand_both_sse4_16;
extern Processor16 *inpand_both_asse4_16;

/* 32 bit */
extern Processor32 *inpand32_square_c;
extern Processor32 *inpand32_horizontal_c;
extern Processor32 *inpand32_vertical_c;
extern Processor32 *inpand32_both_c;
extern Processor32 *inpand32_custom_c;

class Inpand : public Morphologic::MorphologicFilter
{
public:
  Inpand(const Parameters&parameters, CpuFlags cpuFlags, IScriptEnvironment2* env)
     : Morphologic::MorphologicFilter(parameters, (CpuFlags)cpuFlags, env)
  {
    int _bits_per_pixel = bit_depths[C];
    bool _isStacked = parameters["stacked"].toBool();
    if (_isStacked)
      _bits_per_pixel = 16;
    /* add the processors */
    if (parameters["mode"].toString() == "square")
    {
      if (_bits_per_pixel == 8) {
        processors.push_back(Filtering::Processor<Processor>(inpand_square_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
        processors.push_back(Filtering::Processor<Processor>(inpand_square_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 3));
        processors.push_back(Filtering::Processor<Processor>(inpand_square_asse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 4));
      }
      else if (_isStacked) {
        stackedProcessors.push_back(Filtering::Processor<StackedProcessor>(inpand_square_stacked_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
      else if (_bits_per_pixel <= 16) {
        processors16.push_back(Filtering::Processor<Processor16>(inpand_square_native_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_square_sse4_16, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 3));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_square_asse4_16, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 4));
      }
      else {
        processors32.push_back(Filtering::Processor<Processor32>(inpand32_square_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
    }
    else if (parameters["mode"].toString() == "horizontal")
    {
      if (_bits_per_pixel == 8) {
        processors.push_back(Filtering::Processor<Processor>(inpand_horizontal_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
        processors.push_back(Filtering::Processor<Processor>(inpand_horizontal_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 3));
        processors.push_back(Filtering::Processor<Processor>(inpand_horizontal_asse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 4));
      }
      else if (_isStacked) {
        stackedProcessors.push_back(Filtering::Processor<StackedProcessor>(inpand_horizontal_stacked_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
      else if (_bits_per_pixel <= 16) {
        processors16.push_back(Filtering::Processor<Processor16>(inpand_horizontal_native_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_horizontal_sse4_16, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 3));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_horizontal_asse4_16, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 4));
      }
      else {
        processors32.push_back(Filtering::Processor<Processor32>(inpand32_horizontal_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
    }
    else if (parameters["mode"].toString() == "vertical")
    {
      if (_bits_per_pixel == 8) {
        processors.push_back(Filtering::Processor<Processor>(inpand_vertical_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
        processors.push_back(Filtering::Processor<Processor>(inpand_vertical_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 3));
        processors.push_back(Filtering::Processor<Processor>(inpand_vertical_asse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 4));
      }
      else if (_isStacked) {
        stackedProcessors.push_back(Filtering::Processor<StackedProcessor>(inpand_vertical_stacked_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
      else if (_bits_per_pixel <= 16) {
        processors16.push_back(Filtering::Processor<Processor16>(inpand_vertical_native_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_vertical_sse4_16, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 3));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_vertical_asse4_16, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 4));
      }
      else {
        processors32.push_back(Filtering::Processor<Processor32>(inpand32_vertical_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
    }
    else if (parameters["mode"].toString() == "both")
    {
      if (_bits_per_pixel == 8) {
        processors.push_back(Filtering::Processor<Processor>(inpand_both_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
        processors.push_back(Filtering::Processor<Processor>(inpand_both_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 3));
        processors.push_back(Filtering::Processor<Processor>(inpand_both_asse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 4));
      }
      else if (_isStacked) {
        stackedProcessors.push_back(Filtering::Processor<StackedProcessor>(inpand_both_stacked_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
      else if (_bits_per_pixel <= 16) {
        processors16.push_back(Filtering::Processor<Processor16>(inpand_both_native_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_both_sse4_16, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 3));
        processors16.push_back(Filtering::Processor<Processor16>(inpand_both_asse4_16, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 4));
      }
      else {
        processors32.push_back(Filtering::Processor<Processor32>(inpand32_both_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
    }
    else
    {
      if (_bits_per_pixel == 8) {
        processors.push_back(Filtering::Processor<Processor>(inpand_custom_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
      else if (_isStacked) {
        stackedProcessors.push_back(Filtering::Processor<StackedProcessor>(inpand_custom_stacked_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
      else if (_bits_per_pixel <= 16) {
        processors16.push_back(Filtering::Processor<Processor16>(inpand_custom_native_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
      else {
        processors32.push_back(Filtering::Processor<Processor32>(inpand32_custom_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      }
      FillCoordinates(parameters["mode"].toString());
    }
  }

  static Signature Inpand::filter_signature()
  {
    Signature signature = "kmt_inpand";

    signature.add(Parameter(TYPE_CLIP, "", false));
    signature.add(Parameter(TYPE_FLOAT, "thY", true)); // overwritten to default 255..65535 in morphologic.h
    signature.add(Parameter(TYPE_FLOAT, "thC", true));
    signature.add(Parameter(String("square"), "mode", false));

    add_defaults(signature);

    signature.add(Parameter(false, "stacked", false));
    return signature;
  }
};

} } } } } // namespace Inpand, Morphologic, Filter, MaskTools, Filtering

#endif
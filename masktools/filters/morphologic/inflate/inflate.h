#ifndef __Mt_Inflate_H__
#define __Mt_Inflate_H__

#include "../morphologic.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Morphologic { namespace Inflate {

extern Processor *inflate_c;
extern Processor *inflate_sse2;
extern Processor *inflate_asse2;

/* 16 bit */
extern StackedProcessor *inflate_stacked_c;
extern Processor16 *inflate_native_c;

extern Processor16 *inflate_sse4_16;
extern Processor16 *inflate_asse4_16;

/*32 bit */
extern Processor32 *inflate32_c;

class Inflate : public Morphologic::MorphologicFilter
{
public:
  Inflate(const Parameters &parameters, CpuFlags cpuFlags, PNeoEnv env)
     : Morphologic::MorphologicFilter(parameters, (CpuFlags)cpuFlags, env)
  {
    int _bits_per_pixel = bit_depths[C];
    bool _isStacked = parameters["stacked"].toBool();
    if (_isStacked)
      _bits_per_pixel = 16;
    /* add the processors */
    if (_bits_per_pixel == 8) {
      processors.push_back(Filtering::Processor<Processor>(inflate_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      processors.push_back(Filtering::Processor<Processor>(inflate_sse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
      processors.push_back(Filtering::Processor<Processor>(inflate_asse2, Constraint(CPU_SSE2, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 2));
    }
    else if (_isStacked) {
      stackedProcessors.push_back(Filtering::Processor<StackedProcessor>(inflate_stacked_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
    }
    else if (_bits_per_pixel <= 16){
      processors16.push_back(Filtering::Processor<Processor16>(inflate_native_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
      processors16.push_back(Filtering::Processor<Processor16>(inflate_sse4_16, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_NONE, 16), 1));
      processors16.push_back(Filtering::Processor<Processor16>(inflate_asse4_16, Constraint(CPU_SSE4_1, MODULO_NONE, MODULO_NONE, ALIGNMENT_16, 16), 2));
    }
    else {
      processors32.push_back(Filtering::Processor<Processor32>(inflate32_c, Constraint(CPU_NONE, 1, 1, 1, 1), 0));
    }
  }

  static Signature Inflate::filter_signature()
  {
    Signature signature = "kmt_inflate";

    signature.add(Parameter(TYPE_CLIP, "", false));
    signature.add(Parameter(TYPE_FLOAT, "thY", true)); // overwritten to default 255..65535 in morphologic.h
    signature.add(Parameter(TYPE_FLOAT, "thC", true));

    add_defaults(signature);

    signature.add(Parameter(false, "stacked", false));
    return signature;
  }
};

} } } } } // namespace Inflate, Morphologic, Filter, MaskTools, Filtering

#endif
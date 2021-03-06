#include "symbol.h"
#include <math.h>

using namespace Filtering;
using namespace Filtering::Parser;

// basic math
static double addition        (double x, double y) { return x + y; }
static double multiplication  (double x, double y) { return x * y; }
static double division        (double x, double y) { return x / y; }
static double substraction    (double x, double y) { return x - y; }
static double power           (double x, double y) { return pow(x, y); }
static double modulo          (double x, double y) { return double(convert<Int64, double>( x ) % convert<Int64, double>( y )); }
// Ternary operator helper
static double interrogation   (double x, double y, double z) { return x > 0 ? y : z; }
// comparison
static double equal           (double x, double y) { return abs(x - y) < 0.000001 ? 1 : -1; }
static double notEqual        (double x, double y) { return abs(x - y) >= 0.000001 ? 1 : -1; }
static double inferior        (double x, double y) { return x <= y ? 1 : -1; }
static double inferiorStrict  (double x, double y) { return x < y ? 1 : -1; }
static double superior        (double x, double y) { return x >= y ? 1 : -1; }
static double superiorStrict  (double x, double y) { return x > y ? 1 : -1; }
// bool
static double and             (double x, double y) { return x > 0 && y > 0 ? 1 : -1; }
static double or              (double x, double y) { return x > 0 || y > 0 ? 1 : -1; }
static double andNot          (double x, double y) { return x > 0 && y <= 0 ? 1 : -1; }
static double xor             (double x, double y) { return (x > 0 && y <= 0) || (x <= 0 && y > 0)? 1 : -1; }
// Unsigned Bit arithmetic
static double andUB           (double x, double y) { return double(clip<Uint64, double>(x) & clip<Uint64, double>(y)); }
static double orUB            (double x, double y) { return double(clip<Uint64, double>(x) | clip<Uint64, double>(y)); }
static double xorUB           (double x, double y) { return double(clip<Uint64, double>(x) ^ clip<Uint64, double>(y)); }
static double negateUB        (double x) { return double(~clip<Uint64, double>(x)); }
static double posshiftUB      (double x, double y) { return y >= 0 ? double(clip<Uint64, double>(x) << clip<Int64, double>(y)) : double(clip<Uint64, double>(x) >> clip<Int64, double>(-y)); }
static double negshiftUB      (double x, double y) { return y >= 0 ? double(clip<Uint64, double>(x) >> clip<Int64, double>(y)) : double(clip<Uint64, double>(x) << clip<Int64, double>(-y)); }
// Signed Bit arithmetic
static double andSB           (double x, double y) { return double(clip<Int64, double>(x) & clip<Int64, double>(y)); }
static double orSB            (double x, double y) { return double(clip<Int64, double>(x) | clip<Int64, double>(y)); }
static double xorSB           (double x, double y) { return double(clip<Int64, double>(x) ^ clip<Int64, double>(y)); }
static double negateSB        (double x) { return double(~clip<Int64, double>(x)); }
static double posshiftSB      (double x, double y) { return y >= 0 ? double(clip<Int64, double>(x) << clip<Int64, double>(y)) : double(clip<Int64, double>(x) >> clip<Int64, double>(-y)); }
static double negshiftSB      (double x, double y) { return y >= 0 ? double(clip<Int64, double>(x) >> clip<Int64, double>(y)) : double(clip<Int64, double>(x) << clip<Int64, double>(-y)); }
// Math
static double mtcos             (double x) { return cos(x); }
static double mtsin             (double x) { return sin(x); }
static double mttan             (double x) { return tan(x); }
static double mtexp             (double x) { return exp(x); }
static double mtlog             (double x) { return log(x); }
static double mtmabs            (double x) { return abs(x); }
static double mtacos            (double x) { return acos(x); }
static double mtasin            (double x) { return asin(x); }
static double mtatan            (double x) { return atan(x); }
static double mtround           (double x) { return double(convert<Int64, double>( x )); }
static double mtclip            (double x, double y, double z) { return clip<double, double>( x, y, z ); }
static double mtmin             (double x, double y) { return min<double>( x, y ); }
static double mtmax             (double x, double y) { return max<double>( x, y ); }
static double mtfloor           (double x) { return floor(x); }
static double mtceil            (double x) { return ceil(x); }
static double mttrunc           (double x) { return double(Int64(x)); }
// bit depth conversion helpers. x:value on 8 bit scale y: target bit depth 8-32 z:base bit depth
static double upscaleByShift(double x, int y, int z)
{
  const int target_bit_depth = y;
  const int source_bit_depth = z;
  if (target_bit_depth == source_bit_depth) return x; // same bit 
  if (target_bit_depth == 32) { // target float
    const int max_pixel_value_source = (1 << source_bit_depth) - 1;
    return x / max_pixel_value_source;
  }
  if (source_bit_depth == 32) { // treat original as float, target 8-16 bits
    const int max_pixel_value_target = (1 << target_bit_depth) - 1;
    return x * max_pixel_value_target;
  }
  // 8-16 <-> 8-16
  if (target_bit_depth > source_bit_depth) // e.g. 8-->10
    return double(x * (1 << (target_bit_depth - source_bit_depth))); // upscale, mul by 2^N bits
  else
    return double(x / (1 << (source_bit_depth - target_bit_depth))); // downscale: div by 2^N bits
}
static double upscaleByStretch(double x, int y, int z) // e.g. 8->10 bit rgb: x/255*1023
{
  const int target_bit_depth = y;
  const int source_bit_depth = z;
  if (target_bit_depth == source_bit_depth) return x; // same bit 
  
  if (target_bit_depth == 32) { // target float
    const int max_pixel_value_source = (1 << source_bit_depth) - 1;
    return x / max_pixel_value_source;
  }
  if (source_bit_depth == 32) { // treat original as float, target 8-16 bits
    const int max_pixel_value_target = (1 << target_bit_depth) - 1;
    return x * max_pixel_value_target;
  }
  // 8-16 <-> 8-16
  const int max_pixel_value_source = (1 << source_bit_depth) - 1;
  const int max_pixel_value_target = (1 << target_bit_depth) - 1;
  return double(x * max_pixel_value_target / max_pixel_value_source); // upscale or downscale
}

Symbol Symbol::Addition       ("+" , OPERATOR, addition);
Symbol Symbol::Multiplication ("*" , OPERATOR, multiplication);
Symbol Symbol::Division       ("/" , OPERATOR, division);
Symbol Symbol::Substraction   ("-" , OPERATOR, substraction);
Symbol Symbol::Power          ("^" , OPERATOR, power);
Symbol Symbol::Modulo         ("%" , OPERATOR, modulo);
Symbol Symbol::Interrogation  ("?" , TERNARY , interrogation);
Symbol Symbol::Equal          ("==", OPERATOR, equal);
Symbol Symbol::Equal2         ("=", OPERATOR, equal);
Symbol Symbol::NotEqual       ("!=", OPERATOR, notEqual);
Symbol Symbol::Inferior       ("<=", OPERATOR, inferior);
Symbol Symbol::InferiorStrict ("<" , OPERATOR, inferiorStrict);
Symbol Symbol::Superior       (">=", OPERATOR, superior);
Symbol Symbol::SuperiorStrict (">" , OPERATOR, superiorStrict);
Symbol Symbol::And            ("&" , OPERATOR, and);
Symbol Symbol::Or             ("|" , OPERATOR, or);
Symbol Symbol::AndNot         ("&!", OPERATOR, andNot);
Symbol Symbol::Xor            ("�" , "@", OPERATOR, xor);
Symbol Symbol::AndUB          ("&u" , OPERATOR, andUB);
Symbol Symbol::OrUB           ("|u" , OPERATOR, orUB);
Symbol Symbol::XorUB          ("�u" , "@u", OPERATOR, xorUB);
Symbol Symbol::NegateUB       ("~u" , FUNCTION, negateUB);
Symbol Symbol::PosShiftUB     ("<<", "<<u", OPERATOR, posshiftUB);
Symbol Symbol::NegShiftUB     (">>", ">>u", OPERATOR, negshiftUB);
Symbol Symbol::AndSB          ("&s" , OPERATOR, andSB);
Symbol Symbol::OrSB           ("|s" , OPERATOR, orSB);
Symbol Symbol::XorSB          ("�s" , "@s", OPERATOR, xorSB);
Symbol Symbol::NegateSB       ("~s" , FUNCTION, negateSB);
Symbol Symbol::PosShiftSB     ("<<s", OPERATOR, posshiftSB);
Symbol Symbol::NegShiftSB     (">>s", OPERATOR, negshiftSB);
Symbol Symbol::Pi             ("pi", 3.1415927, NUMBER  , 0, NULL);
// Lut variables
Symbol Symbol::X              ("x" , VARIABLE, VARIABLE_X);  
Symbol Symbol::Y              ("y" , VARIABLE, VARIABLE_Y);
Symbol Symbol::Z              ("z" , VARIABLE, VARIABLE_Z);
// preliminary lut variable: A(lpha)
Symbol Symbol::A              ("a" , VARIABLE, VARIABLE_A);
// global bitdepth parameter for autoscale, since v2.2.1
Symbol Symbol::BITDEPTH       ("bitdepth" , VARIABLE, VARIABLE_BITDEPTH);
Symbol Symbol::SCRIPT_BITDEPTH("sbitdepth", VARIABLE, VARIABLE_SCRIPT_BITDEPTH);
// bit-depth adaptive constants, since v2.2.1
Symbol Symbol::RANGE_HALF     ("range_half", VARIABLE, VARIABLE_RANGE_HALF); // 128  scaled
Symbol Symbol::RANGE_MAX      ("range_max", VARIABLE, VARIABLE_RANGE_MAX);   // 255, 4095, .. 65535
Symbol Symbol::RANGE_SIZE     ("range_size", VARIABLE, VARIABLE_RANGE_SIZE); // 256, 1024, 4096, 16384, 65536
Symbol Symbol::YMIN           ("ymin", VARIABLE, VARIABLE_YMIN); // 16 scaled
Symbol Symbol::YMAX           ("ymax", VARIABLE, VARIABLE_YMAX); // 235 or scaled
Symbol Symbol::CMIN           ("cmin", VARIABLE, VARIABLE_CMIN); // 16 scaled = LIMITED_YMIN
Symbol Symbol::CMAX           ("cmax", VARIABLE, VARIABLE_CMAX); // 240 scaled
// Math
Symbol Symbol::Cos            ("cos", FUNCTION, mtcos);
Symbol Symbol::Sin            ("sin", FUNCTION, mtsin);
Symbol Symbol::Tan            ("tan", FUNCTION, mttan);
Symbol Symbol::Log            ("log", FUNCTION, mtlog);
Symbol Symbol::Exp            ("exp", FUNCTION, mtexp);
Symbol Symbol::Abs            ("abs", FUNCTION, mtmabs);
Symbol Symbol::Atan           ("atan", FUNCTION, mtatan);
Symbol Symbol::Acos           ("acos", FUNCTION, mtacos);
Symbol Symbol::Asin           ("asin", FUNCTION, mtasin);
Symbol Symbol::Round          ("round", FUNCTION, mtround);
Symbol Symbol::Clip           ("clip", FUNCTION, mtclip);
Symbol Symbol::Min            ("min", FUNCTION, mtmin);
Symbol Symbol::Max            ("max", FUNCTION, mtmax);
Symbol Symbol::Ceil           ("ceil", FUNCTION, mtceil);
Symbol Symbol::Floor          ("floor", FUNCTION, mtfloor);
Symbol Symbol::Trunc          ("trunc", FUNCTION, mttrunc);
// automatic bit-depth scaling helpers, since v2.2.1
Symbol Symbol::ScaleByShift   ("@B", "scaleb", FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM, upscaleByShift); // v 2.2.5: #B, #F -> @B, @F
Symbol Symbol::ScaleByStretch ("@F", "scalef", FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM, upscaleByStretch); // with optinal scaleb and scalef aliases
// admin config
Symbol Symbol::SetScriptBitDepthI8("i8", 8.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetScriptBitDepthI10("i10", 10.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetScriptBitDepthI12("i12", 12.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetScriptBitDepthI14("i14", 14.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetScriptBitDepthI16("i16", 16.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetScriptBitDepthF32("f32", 32.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseI8Range("clamp_f_i8", -8.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseI10Range("clamp_f_i10", -10.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseI12Range("clamp_f_i12", -12.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseI14Range("clamp_f_i14", -14.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseI16Range("clamp_f_i16", -16.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseF32Range("clamp_f_f32", -32.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);
Symbol Symbol::SetFloatToClampUseF32Range_2("clamp_f", -32.0, FUNCTION_CONFIG_SCRIPT_BITDEPTH, 0, NULL);

Symbol Symbol::Dup("dup", DUP);
Symbol Symbol::Swap("swap", SWAP);

Symbol::Symbol() :
type(UNDEFINED), value(""), value2("")
{
}

// dup, swap, direct numeric literals from parser
Symbol::Symbol(String value, Type type) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(0), process0(NULL), process1(NULL), process2(NULL), process3(NULL)
{
  if (type == NUMBER)
    dValue = atof(value.c_str());
}

// function, operator, ternary

Symbol::Symbol(String value, Type type, Process0 process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(0), process0(process)
{
}

Symbol::Symbol(String value, Type type, Process1 process) :
type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(1), process1(process) 
{
}

Symbol::Symbol(String value, Type type, Process2 process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(2), process2(process)
{
}

Symbol::Symbol(String value, Type type, Process3 process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(3), process3(process)
{
}

// two tokens
Symbol::Symbol(String value, String value2, Type type, Process1 process) :
type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(value2), nParameter(1), process1(process)
{
}

Symbol::Symbol(String value, String value2, Type type, Process2 process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(value2), nParameter(2), process2(process)
{
}

Symbol::Symbol(String value, String value2, Type type, Process3 process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(value2), nParameter(3), process3(process)
{
}

Symbol::Symbol(String value, String value2, Type type, ProcessScale process) :
  type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(value2), nParameter(3), processScale(process)
{
}

// variables
// Type=VARIABLE always
Symbol::Symbol(String value, Type type, VarType vartype) :
  type(type), vartype(vartype), value(value), value2(""), nParameter(0)
{
}

// predefined numeric constants, like "Pi"
// Type=NUMBER always
Symbol::Symbol(String value, double dValue, Type type, int nParameter, Process1 process) :
type(type), vartype(VARIABLE_UNDEFINED), value(value), value2(""), nParameter(nParameter), dValue(dValue), process1(process)
{
}

/* dead code
void Symbol::setValue(double _dValue)
{
   this->dValue = _dValue;
}
*/

// called from rec_compute_old, why
double Symbol::getValue(double x, double y, double z) const
{
   switch ( type )
   {
   case VARIABLE: // ?
   case NUMBER:
     return dValue;
   default:
      return process3(x, y, z);
   }
}

Context::Context(const std::deque<Symbol> &expression)
{
   nPos = -1;
   nSymbols = expression.size();
   pSymbols = new Symbol[nSymbols];
   exprstack = new double[nSymbols];

   nSymbols_control = expression.size();
   pSymbols_control = new Symbol[nSymbols];

   auto it = expression.begin();

   int default_sbitdepth = 8;
   int default_float_autoscale_bitdepth = 0; // no clamp and autoscale

   int symbolCount = 0;
   int symbolCount_control = 0;
   for (int i = 0; i < nSymbols; i++, it++) {
     // control mnemonics are filtered here, they are not put in the expression
     if (it->type == Symbol::FUNCTION_CONFIG_SCRIPT_BITDEPTH) {
       pSymbols_control[symbolCount_control++] = *it;
       if (it->dValue > 0) {
         // i8, i10, i12, i14, i16, f32
         default_sbitdepth = (int)it->dValue;
       }
       else {
         // negative values show the bitdepth that inputs and output should autoscale.
         // clamp_f
         // clamp_f_i8..clamp_f_i16..clamp_f_f32
         default_float_autoscale_bitdepth = -(int)it->dValue;
       }
     }
     else {
       pSymbols[symbolCount++] = *it;
     }
   }
   nSymbols = symbolCount;
   nSymbols_control = symbolCount_control;

   sbitdepth = default_sbitdepth;
   sbitdepth_f = default_sbitdepth; // copy to double type for direct variable use

   // fill predefined constants for faster rec_compute access
   for (int bits = 8; bits <= 16; bits++) {
     a_range_half[bits - 8] = 128 << (bits - 8); // or 0.0 for float chroma in the future?
     a_range_max[bits - 8] = (1 << bits) - 1; // max_pixel_value. 255, 1023, 4095, 16383, 65535 (1.0 for float)
     a_range_size[bits - 8] = (1 << bits); // 256, 1024, 4096, 16384, 65536 (1.0 for float)
     a_ymin[bits - 8] = 16 << (bits - 8); // 16 scaled
     a_ymax[bits - 8] = 235 << (bits - 8);// 235 scaled
     a_cmin[bits - 8] = 16 << (bits - 8); // 16 scaled
     a_cmax[bits - 8] = 240 << (bits - 8);// 240 scaled
   }
   range_half_f = 0.5; // or 0.0 for float chroma in the future?
   range_max_f = 1.0;
   range_size_f = 1.0;
   ymin_f = 16.0 / 255;
   ymax_f = 235.0 / 255;
   cmin_f = 16.0 / 255;
   cmax_f = 235.0 / 255;

   float_autoscale_bitdepth = default_float_autoscale_bitdepth;

   // precalculate scale factor and its inverse
   float_input_scalefactor = 1.0; // for default and 32 bit
   if (float_autoscale_bitdepth >= 8 && float_autoscale_bitdepth <= 16)
     float_input_scalefactor = double((1 << float_autoscale_bitdepth) - 1);
   float_input_invscalefactor = 1.0 / float_input_scalefactor;

}

Context::~Context()
{
   delete[] pSymbols;
   delete[] exprstack;
   delete[] pSymbols_control;
}

double Context::rec_compute()
{
  double last = 0;

  Symbol &s_first = pSymbols[0];

  int p = 0;

  switch (s_first.type)
  {
  case Symbol::NUMBER: { last = s_first.dValue; break; }
  case Symbol::VARIABLE: {
    switch (s_first.vartype) {
    case Symbol::VARIABLE_X: { last = x; break; }
    case Symbol::VARIABLE_Y: { last = y; break; }
    case Symbol::VARIABLE_Z: { last = z; break; }
    case Symbol::VARIABLE_A: { last = a; break; }
    case Symbol::VARIABLE_BITDEPTH: { last = bitdepth; break; } // bit-depth for autoscale
    case Symbol::VARIABLE_SCRIPT_BITDEPTH: { last = sbitdepth_f; break; } // source bit depth for autoscale

    case Symbol::VARIABLE_RANGE_HALF: { last = bitdepth == 32 ? range_half_f : a_range_half[bitdepth - 8]; break; } // or 0.0 for float in the future?
    case Symbol::VARIABLE_RANGE_MAX: { last = bitdepth == 32 ? range_max_f : a_range_max[bitdepth-8]; break; }// max_pixel_value. 255, 1023, 4095, 16383, 65535 (1.0 for float)
    case Symbol::VARIABLE_RANGE_SIZE: { last = bitdepth == 32 ? range_size_f : a_range_size[bitdepth-8]; break; } // 256, 1024, 4096, 16384, 65536 (1.0 for float)
    case Symbol::VARIABLE_YMIN: { last = bitdepth == 32 ? ymin_f : a_ymin[bitdepth - 8]; break;  }   // 16 scaled
    case Symbol::VARIABLE_YMAX: { last = bitdepth == 32 ? ymax_f : a_ymax[bitdepth - 8]; break; } // 235 scaled
    case Symbol::VARIABLE_CMIN: { last = bitdepth == 32 ? cmin_f : a_cmin[bitdepth - 8]; break;  } // 16 scaled
    case Symbol::VARIABLE_CMAX: { last = bitdepth == 32 ? cmax_f : a_cmax[bitdepth - 8]; break;  } // 240 scaled
    default: assert(0);
    }
    break;
  }
  default: return 0; // assert
  }

  for (int i = 1; i < nPos; i++) {
    Symbol &s = pSymbols[i];

    switch (s.type)
    {
    case Symbol::NUMBER: { exprstack[p++] = last; last=s.dValue; break; }
    case Symbol::VARIABLE: {
      switch (s.vartype) {
      case Symbol::VARIABLE_X: { exprstack[p++] = last; last = x; break; }
      case Symbol::VARIABLE_Y: { exprstack[p++] = last; last = y; break; }
      case Symbol::VARIABLE_Z: { exprstack[p++] = last; last = z; break; }
      case Symbol::VARIABLE_A: { exprstack[p++] = last; last = a; break; }
      case Symbol::VARIABLE_BITDEPTH: { exprstack[p++] = last; last = bitdepth; break; } // bit-depth for autoscale
      case Symbol::VARIABLE_SCRIPT_BITDEPTH: { exprstack[p++] = last; last = sbitdepth_f; break; } // source bit depth for autoscale

      case Symbol::VARIABLE_RANGE_HALF: { exprstack[p++] = last; last = bitdepth == 32 ? range_half_f : a_range_half[bitdepth - 8]; break; } // or 0.0 for float in the future?
      case Symbol::VARIABLE_RANGE_MAX: { exprstack[p++] = last; last = bitdepth == 32 ? range_max_f : a_range_max[bitdepth - 8]; break; }// max_pixel_value. 255, 1023, 4095, 16383, 65535 (1.0 for float)
      case Symbol::VARIABLE_RANGE_SIZE: { exprstack[p++] = last; last = bitdepth == 32 ? range_size_f : a_range_size[bitdepth - 8]; break; } // 256, 1024, 4096, 16384, 65536 (1.0 for float)
      case Symbol::VARIABLE_YMIN: { exprstack[p++] = last; last = bitdepth == 32 ? ymin_f : a_ymin[bitdepth - 8]; break;  }   // 16 scaled
      case Symbol::VARIABLE_YMAX: { exprstack[p++] = last; last = bitdepth == 32 ? ymax_f : a_ymax[bitdepth - 8]; break; } // 235 scaled
      case Symbol::VARIABLE_CMIN: { exprstack[p++] = last; last = bitdepth == 32 ? cmin_f : a_cmin[bitdepth - 8]; break;  } // 16 scaled
      case Symbol::VARIABLE_CMAX: { exprstack[p++] = last; last = bitdepth == 32 ? cmax_f : a_cmax[bitdepth - 8]; break;  } // 240 scaled
      default: assert(0);
      }
      break;
    }
    case Symbol::DUP: { exprstack[p++] = last; break;  }

    case Symbol::SWAP:
    {
      double p1 = exprstack[p-1];
      exprstack[p-1] = last;
      last = p1;
      break;
    }

    case Symbol::FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM: // silent bit-depth parameter for autoscale
      // only exists with one user parameter, plus two silent params
      last = s.processScale(last, bitdepth, sbitdepth);
      break;

    // OPERATOR, FUNCTION, TERNARY
    default:
      switch (s.nParameter)
      {
      case 2:
      {
        double xx = exprstack[--p];
        last = s.process2(xx, last); // two-operand function/operator
        break;
      }
      case 1: {
        last = s.process1(last); // one-operand function/operator
        break;
      }
      case 3:
      {
        double yy = exprstack[--p];  // three-operand
        double xx = exprstack[--p];
        last = s.process3(xx,yy,last);
        break;
      }
      default: // function with zero parameters, none
      {
        exprstack[p++] = last;
        last = s.process0(); 
        break; 
      }
      }
    }
  }

  return last;
}

double Context::rec_compute_old()
{
  const Symbol &s = pSymbols[--nPos];

  switch (s.type)
  {
  case Symbol::NUMBER: return s.dValue;
  case Symbol::VARIABLE:
    switch (s.vartype) {
    case Symbol::VARIABLE_X: return x;
    case Symbol::VARIABLE_Y: return y;
    case Symbol::VARIABLE_Z: return z;
    case Symbol::VARIABLE_A: return a;
    case Symbol::VARIABLE_BITDEPTH: return bitdepth; // bit-depth for autoscale
    case Symbol::VARIABLE_SCRIPT_BITDEPTH: return sbitdepth_f; // source bit depth for autoscale

    case Symbol::VARIABLE_RANGE_HALF: return bitdepth == 32 ? 0.5 : (128 << (bitdepth - 8)); // or 0.0 for float in the future?
    case Symbol::VARIABLE_RANGE_MAX: return bitdepth == 32 ? 1.0 : ((1 << bitdepth) - 1); // max_pixel_value. 255, 1023, 4095, 16383, 65535 (1.0 for float)
    case Symbol::VARIABLE_RANGE_SIZE: return bitdepth == 32 ? 1.0 : (1 << bitdepth); // 256, 1024, 4096, 16384, 65536 (1.0 for float)
    case Symbol::VARIABLE_YMIN: return bitdepth == 32 ? 16.0 / 255 : (16 << (bitdepth - 8));    // 16 scaled
    case Symbol::VARIABLE_YMAX: return bitdepth == 32 ? 235.0 / 255 : (235 << (bitdepth - 8));  // 235 scaled
    case Symbol::VARIABLE_CMIN: return bitdepth == 32 ? 16.0 / 255 : (16 << (bitdepth - 8));    // 16 scaled
    case Symbol::VARIABLE_CMAX: return bitdepth == 32 ? 240.0 / 255 : (240 << (bitdepth - 8));  // 240 scaled
    default: 
      assert(0);
      return 0;
    }
    break;
  case Symbol::FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM: // silent bit-depth parameter for autoscale
    switch (s.nParameter) // only exists with one user parameters
    {
    case 1: return s.process3(rec_compute_old(), bitdepth, sbitdepth); // automatic bit-depth parameters
    default:
      assert(0);
    }

  default:
    switch (s.nParameter)
    {
    case 2:
    {
      double yy = rec_compute_old();
      double xx = rec_compute_old();
      return s.process2(xx, yy);
    }
    case 1: return s.process1(rec_compute_old());
    case 3:
    {
      double zz = rec_compute_old();
      double yy = rec_compute_old();
      double xx = rec_compute_old();
      return s.process3(xx, yy, zz);
    }
    default: return s.process0();
    }
  }
}

double Context::compute(double _x, double _y, double _z, double _a, int _bitdepth)
{
   nPos = nSymbols;
   this->x = _x;
   this->y = _y;
   this->z = _z;
   this->a = _a;

   this->bitdepth = _bitdepth;
   // all other expr constants are calculated from bitdepth

   return rec_compute(); // check x86 rec_compute_old with x,y,z,a is faster
}

double Context::compute_4(double _x, double _y, double _z, double _a, int _bitdepth)
{
  nPos = nSymbols;
  this->x = _x;
  this->y = _y;
  this->z = _z;
  this->a = _a;

  this->bitdepth = _bitdepth;
  // all other expr constants are calculated from bitdepth
  
  return rec_compute(); // on x86 rec_compute_old is MUCH faster
}

double Context::compute_3(double _x, double _y, double _z, int _bitdepth)
{
  nPos = nSymbols;
  this->x = _x;
  this->y = _y;
  this->z = _z;

  this->bitdepth = _bitdepth;
  // all other expr constants are calculated from bitdepth

  return rec_compute();
}

double Context::compute_2(double _x, double _y, int _bitdepth)
{
  nPos = nSymbols;
  this->x = _x;
  this->y = _y;

  this->bitdepth = _bitdepth;
  // all other expr constants are calculated from bitdepth

  return rec_compute();
}

double Context::compute_1(double _x, int _bitdepth)
{
  nPos = nSymbols;
  this->x = _x;

  this->bitdepth = _bitdepth;
  // all other expr constants are calculated from bitdepth

  return rec_compute();
}

String Context::rec_infix()
{
    const Symbol &s = pSymbols[--nPos];

    switch ( s.type )
    {
    case Symbol::VARIABLE:
      switch (s.vartype) {
      case Symbol::VARIABLE_X:
      case Symbol::VARIABLE_Y:
      case Symbol::VARIABLE_Z:
      case Symbol::VARIABLE_A:
      case Symbol::VARIABLE_BITDEPTH:
      case Symbol::VARIABLE_SCRIPT_BITDEPTH:
      case Symbol::VARIABLE_RANGE_HALF:
      case Symbol::VARIABLE_RANGE_MAX:
      case Symbol::VARIABLE_RANGE_SIZE:
      case Symbol::VARIABLE_YMIN:
      case Symbol::VARIABLE_YMAX:
      case Symbol::VARIABLE_CMIN:
      case Symbol::VARIABLE_CMAX:
        return s.value;
      }
    case Symbol::NUMBER: return s.value;
    case Symbol::FUNCTION:
        if (s.nParameter == 1) {
            return s.value + "(" + rec_infix() + ")";
        } else if (s.nParameter == 2) {
            auto op2 = rec_infix();
            return s.value + "(" + rec_infix() + "," + op2 + ")";
        } else {
            auto op3 = rec_infix();
            auto op2 = rec_infix();
            return s.value + "(" + rec_infix() + "," + op2 + "," + op3 + ")";
        }
    case Symbol::OPERATOR:
        {
            auto op2 = rec_infix();
            return "(" + rec_infix() + s.value + op2 + ")";
        }
    case Symbol::TERNARY:
        {
            auto op3 = rec_infix();
            auto op2 = rec_infix();
            return "((" + rec_infix() + ") ? " + op2 + " : " + op3 + ")";
        }
    case Symbol::FUNCTION_WITH_BITDEPTH_AS_AUTOPARAM:
    {
      if (s.nParameter == 1) {
        return s.value + "(" + rec_infix() + ")";
      }
    }

    case Symbol::DUP: {
      return ""; // cannot convert to infix
    }

    case Symbol::SWAP: {
      return ""; // cannot convert to infix
    }

    default:
        assert(0);
        return "";
    }
}

String Context::infix()
{
   nPos = nSymbols;

   return rec_infix();
}

bool Context::check()
{
   return true;
}
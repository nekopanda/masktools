#include "inputconfig.h"

namespace Filtering { namespace MaskTools {

InputConfiguration &InPlaceOneFrame() { 
    static InputConfiguration ic; 
    return ic; 
}

InputConfiguration &OneFrame() { 
    static InputConfiguration ic(Input(0, 0)); 
    return ic; 
}

InputConfiguration &InPlaceTwoFrame() { 
    static InputConfiguration ic(Input(1, 0)); 
    return ic; 
}

InputConfiguration &TwoFrame() { 
    static InputConfiguration ic(Input(0, 0), Input(1, 0)); 
    return ic; 
}

InputConfiguration &InPlaceThreeFrame() { 
    static InputConfiguration ic(Input(1, 0), Input(2, 0)); 
    return ic;
}

InputConfiguration &ThreeFrame() {
	static InputConfiguration ic(Input(0, 0), Input(1, 0), Input(2, 0));
	return ic;
}

InputConfiguration &InPlaceFourFrame() {
  static InputConfiguration ic(Input(1, 0), Input(2, 0), Input(3, 0));
  return ic;
}

InputConfiguration &FourFrame() {
	static InputConfiguration ic(Input(0, 0), Input(1, 0), Input(2, 0), Input(3, 0));
	return ic;
}

InputConfiguration &InPlaceTemporalOneFrame() { 
    static InputConfiguration ic(Input(0, -1)); 
    return ic; 
}

InputConfiguration &TemporalOneFrame() { 
    static InputConfiguration ic(Input(0, 0), Input(0, -1)); 
    return ic; 
}

} }

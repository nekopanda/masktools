#include "motionmask.h"
#include "../../../common/simd.h"
#include "../../../../common/functions/functions.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Mask { namespace Motion {

  static uint64_t sad_c_op(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
  {
    uint64_t nSad = 0;
    for (int y = 0; y < nHeight; y++)
    {
      unsigned int nSad32 = 0;
      for (int x = 0; x < nWidth; x++)
        nSad32 += abs(pDst[x] - pSrc[x]);
      nSad += nSad32; // avoid overflow for big frames
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }
    return nSad;
  }

  static void mask_c_op(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
  {
    for (int y = 0; y < nHeight; y++)
    {
      for (int x = 0; x < nWidth; x++)
        pDst[x] = threshold<Byte, int>(abs<int>(pDst[x] - pSrc[x]), nLowThreshold, nHighThreshold);
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }
  }


  template <MemoryMode mem_mode>
  static uint64_t sad_sse2_op(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight)
  {
    uint64_t sad = 0; // avoid overflow
    int wMod16 = (nWidth / 16) * 16;
    auto pDstSave = pDst;
    auto pSrcSave = pSrc;
    for (int j = 0; j < nHeight; ++j) {
      __m128i acc = _mm_setzero_si128();
      for (int i = 0; i < wMod16; i += 16) {
        auto dst1 = simd_load_si128<mem_mode>(pDst + i);
        auto src1 = simd_load_si128<mem_mode>(pSrc + i);

        auto sad1 = _mm_sad_epu8(dst1, src1);

        acc = _mm_add_epi32(acc, sad1);
      }
      auto idk = _mm_castps_si128(_mm_movehl_ps(_mm_setzero_ps(), _mm_castsi128_ps(acc)));
      auto sum = _mm_add_epi32(acc, idk);
      auto sad32 = _mm_cvtsi128_si32(sum);
      sad += sad32;
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }

    if (nWidth > wMod16) {
      sad += sad_c_op(pDstSave + wMod16, nDstPitch, pSrcSave + wMod16, nSrcPitch, nWidth - wMod16, nHeight);
    }
    return sad;
  }

  template <MemoryMode mem_mode>
  static void mask_sse2_op(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nWidth, int nHeight)
  {
    int wMod16 = (nWidth / 16) * 16;
    auto pDst2 = pDst;
    auto pSrc2 = pSrc;

    auto v128 = _mm_set1_epi32(0x80808080);
    auto lowThresh = _mm_set1_epi8(Byte(nLowThreshold));
    auto highThresh = _mm_set1_epi8(Byte(nHighThreshold));
    lowThresh = _mm_sub_epi8(lowThresh, v128);
    highThresh = _mm_sub_epi8(highThresh, v128);

    for (int j = 0; j < nHeight; ++j) {
      for (int i = 0; i < wMod16; i += 16) {
        auto dst1 = simd_load_si128<mem_mode>(pDst + i);
        auto src1 = simd_load_si128<mem_mode>(pSrc + i);

        auto greater = _mm_subs_epu8(dst1, src1);
        auto smaller = _mm_subs_epu8(src1, dst1);
        auto diff = _mm_add_epi8(greater, smaller);

        auto mask = threshold_sse2(diff, lowThresh, highThresh, v128);

        simd_store_si128<mem_mode>(pDst + i, mask);
      }
      pDst += nDstPitch;
      pSrc += nSrcPitch;
    }

    if (nWidth > wMod16) {
      mask_c_op(pDst2 + wMod16, nDstPitch, pSrc2 + wMod16, nSrcPitch, nLowThreshold, nHighThreshold, nWidth - wMod16, nHeight);
    }
  }


template <decltype(sad_c_op) sad, decltype(mask_c_op) mask>
bool mask_t(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nLowThreshold, int nHighThreshold, int nMotionThreshold, int nSceneChange, int nSceneChangeValue, int nWidth, int nHeight, IScriptEnvironment* env)
{
    bool scenechange = nSceneChange >= 2 ? nSceneChange == 3 : sad(pDst, nDstPitch, pSrc, nSrcPitch, nWidth, nHeight) > (uint64_t)(nMotionThreshold * nWidth * nHeight);

    if (scenechange) {
        Functions::memset_plane(pDst, nDstPitch, nWidth, nHeight, static_cast<Byte>(nSceneChangeValue), env);
    } else {
        mask(pDst, nDstPitch, pSrc, nSrcPitch, nLowThreshold, nHighThreshold, nWidth, nHeight);
    }
    return scenechange;
}


Processor *mask_c    = &mask_t<sad_c_op, mask_c_op>;
Processor *mask_sse2 = &mask_t<sad_sse2_op<MemoryMode::SSE2_UNALIGNED>, mask_sse2_op<MemoryMode::SSE2_UNALIGNED>>;
Processor *mask_asse2 = &mask_t<sad_sse2_op<MemoryMode::SSE2_ALIGNED>, mask_sse2_op<MemoryMode::SSE2_ALIGNED>>;

} } } } }
//---------------------------------------------------------------------------

#pragma hdrstop
//---------------------------------------------------------------------------
#include <utility>
//---------------------------------------------------------------------------


#include <vector>
#include <cstdio>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "hahaha_isa_image_process_color.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

//---------------------------------------------------------------------------
namespace hahahaISAlib
{
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
namespace image_process
{
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
namespace color
{
//---------------------------------------------------------------------------


void hahaha_isa_image_process_color::Convert(
	const uint8_t* src,
	int src_stride,
	uint8_t* dst,
	int dst_stride,
	int width,
	int height
) {
#ifdef __AVX2__
    Convert_AVX2(src, src_stride, dst, dst_stride, width, height);
#else
    Convert_Scalar(src, src_stride, dst, dst_stride, width, height);
#endif
}

static bool hasAVX2()
{
#if defined(__GNUC__) || defined(__clang__)
	return __builtin_cpu_supports("avx2");
#elif defined(_MSC_VER)
	int cpuInfo[4];
	__cpuid(cpuInfo, 7);
	return (cpuInfo[1] & (1 << 5)) != 0;
#else
	return false;
#endif
}
//---------------------------------------------------------------------------
// ---------------- Scalar（正確基準版） ----------------
void hahaha_isa_image_process_color::Convert_Scalar(
	const uint8_t* src,
	int src_stride,
	uint8_t* dst,
	int dst_stride,
	int width,
	int height
) {
	for (int y = 0; y < height; y++)
	{
		const uint8_t* s = src + y * src_stride;
		uint8_t* d = dst + y * dst_stride;

		for (int x = 0; x < width; x += 2)
		{
			uint8_t Y0 = s[0];
			uint8_t U  = s[1];
			uint8_t Y1 = s[2];
			uint8_t V  = s[3];

			int C0 = Y0 - 16;
			int C1 = Y1 - 16;
			int D  = U  - 128;
			int E  = V  - 128;

			int R0 = (298 * C0 + 409 * E + 128) >> 8;
			int G0 = (298 * C0 - 100 * D - 208 * E + 128) >> 8;
			int B0 = (298 * C0 + 516 * D + 128) >> 8;

			int R1 = (298 * C1 + 409 * E + 128) >> 8;
			int G1 = (298 * C1 - 100 * D - 208 * E + 128) >> 8;
			int B1 = (298 * C1 + 516 * D + 128) >> 8;

			R0 = R0 < 0 ? 0 : (R0 > 255 ? 255 : R0);
			G0 = G0 < 0 ? 0 : (G0 > 255 ? 255 : G0);
			B0 = B0 < 0 ? 0 : (B0 > 255 ? 255 : B0);

			R1 = R1 < 0 ? 0 : (R1 > 255 ? 255 : R1);
			G1 = G1 < 0 ? 0 : (G1 > 255 ? 255 : G1);
			B1 = B1 < 0 ? 0 : (B1 > 255 ? 255 : B1);

			d[0] = B0; d[1] = G0; d[2] = R0; d[3] = 255;
			d[4] = B1; d[5] = G1; d[6] = R1; d[7] = 255;

			s += 4;
			d += 8;
		}
	}
}
//---------------------------------------------------------------------------
// ---------------- AVX2（與 Scalar 顏色對齊的版本） ----------------
#ifdef __AVX2__
void hahaha_isa_image_process_color::Convert_AVX2(
	const uint8_t* src,
	int src_stride,
	uint8_t* dst,
	int dst_stride,
	int width,
	int height
) {
	const int step = 16; // 一次處理 16 pixels

	const __m256i y_shuffle = _mm256_set_epi8(
		14,12,10,8,6,4,2,0,
		14,12,10,8,6,4,2,0,
		14,12,10,8,6,4,2,0,
		14,12,10,8,6,4,2,0
	);

	const __m256i u_shuffle = _mm256_set_epi8(
		13,13, 9, 9, 5, 5, 1, 1,
		13,13, 9, 9, 5, 5, 1, 1,
		13,13, 9, 9, 5, 5, 1, 1,
		13,13, 9, 9, 5, 5, 1, 1
	);

	const __m256i v_shuffle = _mm256_set_epi8(
		15,15,11,11, 7, 7, 3, 3,
		15,15,11,11, 7, 7, 3, 3,
		15,15,11,11, 7, 7, 3, 3,
		15,15,11,11, 7, 7, 3, 3
	);

	const __m256i c16  = _mm256_set1_epi16(16);
	const __m256i c128 = _mm256_set1_epi16(128);

	for (int y = 0; y < height; y++)
	{
		const uint8_t* s = src + y * src_stride;
		uint8_t* d = dst + y * dst_stride;

		int x = 0;

		for (; x <= width - step; x += step)
		{
			const uint8_t* sp = s + x * 2; // YUY2: 2 bytes per pixel
			__m256i yuy2 = _mm256_loadu_si256((const __m256i*)sp);

			__m256i y_bytes = _mm256_shuffle_epi8(yuy2, y_shuffle);
			__m256i u_bytes = _mm256_shuffle_epi8(yuy2, u_shuffle);
			__m256i v_bytes = _mm256_shuffle_epi8(yuy2, v_shuffle);

			__m128i y_lo_8 = _mm256_castsi256_si128(y_bytes);
			__m128i y_hi_8 = _mm256_extracti128_si256(y_bytes, 1);

			__m128i u_lo_8 = _mm256_castsi256_si128(u_bytes);
			__m128i u_hi_8 = _mm256_extracti128_si256(u_bytes, 1);

			__m128i v_lo_8 = _mm256_castsi256_si128(v_bytes);
			__m128i v_hi_8 = _mm256_extracti128_si256(v_bytes, 1);

			__m256i y_lo = _mm256_sub_epi16(_mm256_cvtepu8_epi16(y_lo_8), c16);
			__m256i y_hi = _mm256_sub_epi16(_mm256_cvtepu8_epi16(y_hi_8), c16);

			__m256i u_lo = _mm256_sub_epi16(_mm256_cvtepu8_epi16(u_lo_8), c128);
			__m256i u_hi = _mm256_sub_epi16(_mm256_cvtepu8_epi16(u_hi_8), c128);

			__m256i v_lo = _mm256_sub_epi16(_mm256_cvtepu8_epi16(v_lo_8), c128);
			__m256i v_hi = _mm256_sub_epi16(_mm256_cvtepu8_epi16(v_hi_8), c128);

			process8(d + x * 4 + 0,  y_lo, u_lo, v_lo);
			process8(d + x * 4 + 32, y_hi, u_hi, v_hi);
		}

		if (x < width)
		{
			Convert_Scalar(
				s + x * 2,
				src_stride,
				d + x * 4,
				dst_stride,
				width - x,
				1
			);
		}
	}
}

//---------------------------------------------------------------------------
// 8 pixels，32-bit 精準版，BGRA（與 Scalar 對齊）

void hahaha_isa_image_process_color::process8(
	uint8_t* dst,
	__m256i y16,
	__m256i u16,
	__m256i v16
) {
	static const __m256i k298 = _mm256_set1_epi32(298);
	static const __m256i k409 = _mm256_set1_epi32(409);
	static const __m256i k100 = _mm256_set1_epi32(100);
	static const __m256i k208 = _mm256_set1_epi32(208);
	static const __m256i k516 = _mm256_set1_epi32(516);
	static const __m256i bias = _mm256_set1_epi32(128);
	static const __m256i zero32 = _mm256_set1_epi32(0);
	static const __m256i max32  = _mm256_set1_epi32(255);

	__m128i y_lo16 = _mm256_castsi256_si128(y16);
	__m128i u_lo16 = _mm256_castsi256_si128(u16);
	__m128i v_lo16 = _mm256_castsi256_si128(v16);

	__m256i y32 = _mm256_cvtepi16_epi32(y_lo16);
	__m256i u32 = _mm256_cvtepi16_epi32(u_lo16);
	__m256i v32 = _mm256_cvtepi16_epi32(v_lo16);

	__m256i c = _mm256_mullo_epi32(y32, k298);

	__m256i r32 = _mm256_srai_epi32(
		_mm256_add_epi32(
			_mm256_add_epi32(c, _mm256_mullo_epi32(v32, k409)),
			bias
		), 8);

	__m256i g32 = _mm256_srai_epi32(
		_mm256_add_epi32(
			_mm256_sub_epi32(
				_mm256_sub_epi32(c, _mm256_mullo_epi32(u32, k100)),
				_mm256_mullo_epi32(v32, k208)
			),
			bias
		), 8);

	__m256i b32 = _mm256_srai_epi32(
		_mm256_add_epi32(
			_mm256_add_epi32(c, _mm256_mullo_epi32(u32, k516)),
			bias
		), 8);

	r32 = _mm256_min_epi32(_mm256_max_epi32(r32, zero32), max32);
	g32 = _mm256_min_epi32(_mm256_max_epi32(g32, zero32), max32);
	b32 = _mm256_min_epi32(_mm256_max_epi32(b32, zero32), max32);

	__m128i r16 = _mm_packus_epi32(_mm256_castsi256_si128(r32),
								   _mm256_extracti128_si256(r32, 1));
	__m128i g16 = _mm_packus_epi32(_mm256_castsi256_si128(g32),
								   _mm256_extracti128_si256(g32, 1));
	__m128i b16 = _mm_packus_epi32(_mm256_castsi256_si128(b32),
								   _mm256_extracti128_si256(b32, 1));

	__m128i r8 = _mm_packus_epi16(r16, r16);
	__m128i g8 = _mm_packus_epi16(g16, g16);
	__m128i b8 = _mm_packus_epi16(b16, b16);

	__m128i a8 = _mm_set1_epi8((char)255);

	__m128i bg = _mm_unpacklo_epi8(b8, g8);
	__m128i ra = _mm_unpacklo_epi8(r8, a8);

	__m128i bgra0 = _mm_unpacklo_epi16(bg, ra);
	__m128i bgra1 = _mm_unpackhi_epi16(bg, ra);

	_mm_storeu_si128((__m128i*)(dst +  0), bgra0);
	_mm_storeu_si128((__m128i*)(dst + 16), bgra1);
}

//---------------------------------------------------------------------------
// ---------------- 驗證：AVX2 與 Scalar 是否完全一致 ----------------
void hahaha_isa_image_process_color::Verify(const uint8_t* src, int src_stride,
				   int width, int height)
{
		std::vector<uint8_t> ref(height * width * 4);
        std::vector<uint8_t> test(height * width * 4);

        Convert_Scalar(src, src_stride,
                       ref.data(), width * 4,
                       width, height);

        Convert_AVX2(src, src_stride,
                     test.data(), width * 4,
                     width, height);

        for (int i = 0; i < width * height * 4; i++)
        {
            if (ref[i] != test[i])
            {
                std::printf("Mismatch at %d: ref=%d test=%d\n",
                            i, ref[i], test[i]);
                return;
            }
        }

        std::printf("OK: AVX2 matches Scalar\n");
}

#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
} // color
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
} // image_process
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
} // hahahaISAlib
//---------------------------------------------------------------------------
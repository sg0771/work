
#include "windows.h"
#include "utils.hpp"

#include "math.h"

#include <avisynth/avisynth.h>

typedef  struct { unsigned __int8 c[4]; }   PixelRGB32;

inline PixelRGB32 operator - (const PixelRGB32 a, PixelRGB32 b)
{
	PixelRGB32 d(a);
	d.c[0] -= b.c[0];
	d.c[1] -= b.c[1];
	d.c[2] -= b.c[2];
	d.c[3] -= b.c[3];
	return d;
}

inline PixelRGB32 operator * (const PixelRGB32 a, unsigned __int8 w)
{
	PixelRGB32 d;
	d.c[0] = (int)a.c[0] * w / 256;
	d.c[1] = (int)a.c[1] * w / 256;
	d.c[2] = (int)a.c[2] * w / 256;
	d.c[3] = (int)a.c[3] * w / 256;
	return d;
}


static
unsigned __int8 interp(unsigned __int8 a, unsigned __int8 b, unsigned __int8 w)
{
	return b + (a - b) * w / 256;
}

static
PixelRGB32 interp(PixelRGB32 a, PixelRGB32 b, unsigned __int8 w)
{
	PixelRGB32 dst;
	dst.c[0] = interp(a.c[0], b.c[0], w);
	dst.c[1] = interp(a.c[1], b.c[1], w);
	dst.c[2] = interp(a.c[2], b.c[2], w);
	dst.c[3] = interp(a.c[3], b.c[3], w);
	return dst;
}



// AllocatePolicy

class AllocatePolicyStdNew
{
public:
	static void* allocate(size_t size)
		//	{ return std::malloc(size); 	}
	{
		return malloc(size);
	} // Fizick
//	static void free(void *t) { std::free (t); }
	static void mfree(void* t) { free(t); } // Fizick
protected:
	~AllocatePolicyStdNew() {} // to prohibit destruction by client
};


// AlignmentPolicy

class AlignmentPolicyBmp
{
public:
	static unsigned aligned_width(unsigned width) { return (sizeof(PixelRGB32) * width + 3) & (~3); }
protected:
	~AlignmentPolicyBmp() {} // to prohibit destruction by client
};

class AlignmentPolicyUnaligned
{
public:
	static unsigned aligned_width(unsigned width) { return sizeof(PixelRGB32) * width; }
protected:
	~AlignmentPolicyUnaligned() {} // to prohibit destruction by client
};

// Callback

typedef bool (*CallbackFn)(double percent_complete);
#ifdef USE_LOKI
typedef Loki::Functor<bool, TYPELIST_1(double)> CallbackFtr;
#endif

template<
	typename ProgressAnbAbortCallBack = CallbackFn,
	class AllocatorPolicy = AllocatePolicyStdNew,
	class AlignmentPolicy = AlignmentPolicyBmp,
	class PixelType = PixelRGB32
>
class ImageRotate :
	public AllocatorPolicy
{
public:
	class Img :
		public AllocatorPolicy,
		public AlignmentPolicy
	{
	public:
		Img() :
			width(0), height(0), width_pad(0), pixel(0)
		{}
		Img(const Img& s) :
			width(s.width), height(s.height), width_pad(s.width_pad), pixel(s.pixel)
		{}
		Img(int w, int h)
		{
			Allocate(w, h);
		}
		void Allocate(int w, int h)
		{
			width = w;
			height = h;
			width_pad = aligned_width(width);
			pixel = (PixelType*)allocate(width_pad * height);
		}
		void Free()
		{
			mfree(pixel);
			pixel = 0;
		}

		PixelType* pixel;
		unsigned width;
		unsigned height;
		unsigned width_pad;	// in bytes

		PixelType& RGBVAL(int x, int y)
		{
			return *((PixelType*)((char*)pixel + width_pad * y) + x);
		}

		const PixelType& RGBVAL(int x, int y) const
		{
			return *((PixelType*)((char*)pixel + width_pad * y) + x);
		}

		void PrevLine(PixelType*& p) const
		{
			p = (PixelType*)((char*)p - width_pad);
		}

		void NextLine(PixelType*& p) const
		{
			p = (PixelType*)((char*)p + width_pad);
		}
	};

	static
		Img AllocAndRotate(
			const Img& src,
			PixelType clrBack,			// Background color
			double      angle,      // Rotation angle
			double aspect, // aspect factor = 2.0 for YV16 chroma planes
			ProgressAnbAbortCallBack* cb = 0
		)
	{
		if (!src.pixel)
			return src;

		Img mid_image(src);

		// mod by Fizick: change intervals from 0...360 to -180..180
		// to prevent any extra rotation for angles -45...45
		// Bring angle to range of (-INF .. 180.0)
		while (angle >= 180.0)
			angle -= 360.0;

		// Bring angle to range of [-180.0 .. 180.0)
		while (angle < -180.0)
			angle += 360.0;

		if (aspect != 1.0)
		{
			if (angle > 90.0)
			{
				// Angle in (90.0 .. 180.0]
				// Rotate image by 180 degrees into temporary image,
				// so it requires only an extra rotation angle
				// of -90.0 .. 0.0 to complete rotation.
				mid_image = Rotate180(src, cb);
				angle -= 180.0;
			}
			else if (angle < -90.0)
			{
				// Angle in [-180.0 .. -90.0)
				// Rotate image by 180 degrees into temporary image,
				// so it requires only an extra rotation angle
				// of 0.0 .. +90.0 to complete rotation.
				mid_image = Rotate180(src, cb);
				angle += 180.0;
			}
		}
		else // aspect == 1, so we may use rotation90
		{
			if (angle > 135.0)
			{
				mid_image = Rotate180(src, cb);
				angle -= 180.0;
			}
			else if (angle > 45.0)
			{
				mid_image = Rotate90(src, cb);
				angle -= 90.0;
			}
			else if (angle < -135.0)
			{
				mid_image = Rotate180(src, cb);
				angle += 180.0;
			}
			else if (angle < -45.0)
			{
				mid_image = Rotate270(src, cb);
				angle += 90.0;
			}
		}

		// check for abort
		if (!mid_image.pixel)
			return mid_image;

		// If we got here, angle is in [-90.0 .. +90.0]

		Img dst(Rotate45(mid_image, clrBack, angle, aspect, src.pixel != mid_image.pixel, cb));

		if (src.pixel != mid_image.pixel)
		{
			// Middle image was required, free it now.
			mid_image.Free();
		}

		return dst;
	}

	static
		Img AllocAndHShear( // added by Fizick
			const Img& src,
			PixelType clrBack,			// Background color
			double      angle,      // Rotation angle
			double aspect, // aspect factor = 2.0 for YV16 chroma planes
			ProgressAnbAbortCallBack* cb = 0
		)
	{
		if (!src.pixel)
			return src;

		Img mid_image(src);

		// mod by Fizick: change intervals from 0...360 to -180..180
		// to prevent any extra rotation for angles -45...45
		// Bring angle to range of (-INF .. 180.0)
		while (angle >= 180.0)
			angle -= 360.0;

		// Bring angle to range of [-180.0 .. 180.0)
		while (angle < -180.0)
			angle += 360.0;

		if (angle > 90.0)
		{
			// Rotate image by 180 degrees into temporary image,
			// so it requires only an extra rotation angle
			// of -90.0 .. +0.0 to complete rotation.
			mid_image = Rotate180(src, cb);
			angle -= 180.0;
		}
		else if (angle < -90.0)
		{
			// Rotate image by -180 degrees into temporary image,
			// so it requires only an extra rotation angle
			// of 0.0 .. 90.0 to complete rotation.
			mid_image = Rotate180(src, cb);
			angle += 180.0;
		}

		// check for abort
		if (!mid_image.pixel)
			return mid_image;

		// If we got here, angle is in (-90.0 .. +90.0]

		Img dst(HShearUpTo90(mid_image, clrBack, angle, aspect, src.pixel != mid_image.pixel, cb));

		if (src.pixel != mid_image.pixel)
		{
			// Middle image was required, free it now.
			mid_image.Free();
		}

		return dst;
	}

	static
		Img AllocAndVShear( // added by Fizick
			const Img& src,
			PixelType clrBack,			// Background color
			double      angle,      // Rotation angle
			double aspect, // aspect factor = 2.0 for YV16 chroma planes
			ProgressAnbAbortCallBack* cb = 0
		)
	{
		if (!src.pixel)
			return src;

		Img mid_image(src);

		// mod by Fizick: change intervals from 0...360 to -180..180
		// to prevent any extra rotation for angles -45...45
		// Bring angle to range of (-INF .. 180.0)
		while (angle >= 180.0)
			angle -= 360.0;

		// Bring angle to range of [-180.0 .. 180.0)
		while (angle < -180.0)
			angle += 360.0;

		if (angle > 90.0)
		{
			// Rotate image by 180 degrees into temporary image,
			// so it requires only an extra rotation angle
			// of -90.0 .. +0.0 to complete rotation.
			mid_image = Rotate180(src, cb);
			angle -= 180.0;
		}
		else if (angle < -90.0)
		{
			// Rotate image by -180 degrees into temporary image,
			// so it requires only an extra rotation angle
			// of 0.0 .. 90.0 to complete rotation.
			mid_image = Rotate180(src, cb);
			angle += 180.0;
		}

		// check for abort
		if (!mid_image.pixel)
			return mid_image;

		// If we got here, angle is in (-90.0 .. +90.0]

		Img dst(VShearUpTo90(mid_image, clrBack, angle, aspect, src.pixel != mid_image.pixel, cb));

		if (src.pixel != mid_image.pixel)
		{
			// Middle image was required, free it now.
			mid_image.Free();
		}

		return dst;
	}

private:

	static
		Img Rotate90(const Img& src, ProgressAnbAbortCallBack* cb = 0)
	{
		Img dst(src.height, src.width);
		if (!dst.pixel)
			return dst;

		for (unsigned uY = 0; uY < src.height; uY++)
		{
			const PixelType* psrc = &src.RGBVAL(0, uY);
			PixelType* pdst = &dst.RGBVAL(uY, dst.height - 1);
			for (unsigned uX = 0; uX < src.width; uX++)
			{
				*pdst = *psrc;
				psrc++;
				dst.PrevLine(pdst);
			}
			// Report progress
			if (cb && !((*cb) (50.0 * uY / src.height)))
			{
				dst.Free();
				break;
			}
		}
		return dst;
	}

	static
		Img Rotate180(const Img& src, ProgressAnbAbortCallBack* cb = 0)
	{
		Img dst(src.width, src.height);
		if (!dst.pixel)
			return dst;

		for (unsigned uY = 0; uY < src.height; uY++)
		{
			const PixelType* psrc = &src.RGBVAL(0, uY);
			PixelType* pdst = &dst.RGBVAL(dst.width - 1, dst.height - uY - 1);
			for (unsigned uX = 0; uX < src.width; uX++)
			{
				*pdst = *psrc;
				psrc++;
				pdst--;
			}
			// Report progress
			if (cb && !((*cb) (50.0 * uY / src.height)))
			{
				dst.Free();
				break;
			}
		}
		return dst;
	}

	static
		Img Rotate270(const Img& src, ProgressAnbAbortCallBack* cb = 0)
	{
		Img dst(src.height, src.width);
		if (!dst.pixel)
			return dst;

		for (unsigned uY = 0; uY < src.height; uY++)
		{
			const PixelType* psrc = &src.RGBVAL(0, uY);
			PixelType* pdst = &dst.RGBVAL(dst.width - uY - 1, 0);
			for (unsigned uX = 0; uX < src.width; uX++)
			{
				*pdst = *psrc;
				psrc++;
				dst.NextLine(pdst);
			}
			// Report progress
			if (cb && !((*cb) (50.0 * uY / src.height)))
			{
				dst.Free();
				break;
			}
		}
		return dst;
	}

	static
		Img Rotate45(
			const Img& src,		// Img to rotate (+dimensions)
			PixelType clrBack,			// Background color
			double dAngle,			// Degree of rotation
			double dAspect, // aspect factor = 2.0 for YV16 chroma planes
			bool bMidImage,			// Was middle image used (for correct progress report)
			ProgressAnbAbortCallBack* cb = 0
		)
	{
		const double ROTATE_PI = 3.1415926535897932384626433832795;
		double dRadAngle = dAngle * ROTATE_PI / double(180); // Angle in radians
		double dSinE = sin(dRadAngle);
		double dTan = tan(dRadAngle / 2.0);

		// Calc first shear (horizontal) destination image dimensions
		int dst1_width_delta = (int(double(src.height) * fabs(dTan) / dAspect + 0.5) + 1) & 0xFFFFFFFE; // delta  must be even! - Fizick
		Img dst1(src.width + dst1_width_delta, src.height); // Fizick
		if (!dst1.pixel)
			return dst1;


		// Perform 1st shear (horizontal)
		for (unsigned u = 0; u < dst1.height; u++)
		{
			double dShear = dst1_width_delta / 2 + ((int)u - ((int)dst1.height - 1) / 2.0) * dTan / dAspect; // Fizick
			int iShear = (int)floor(dShear);
			HorizSkew(src, dst1, u, iShear, uint8_t(255 * (dShear - double(iShear)) + 1), clrBack);
			// Report progress
			if (cb && !((*cb) (
				bMidImage ? 50.0 + (50.0 / 3) * u / dst1.height :
				(100.0 / 3) * u / dst1.height
				)))
			{
				dst1.Free();
				return dst1;
			}
		}

		// Perform 2nd shear  (vertical)
		int ivertex = dst1.width / 2 - int((src.height - 1) * fabs(dTan) / dAspect + 0.5); // Fizick
		int dst2_height_delta = int(2 * ivertex * fabs(dSinE) * dAspect + 1) & 0xFFFFFFFE; // Fizick
		int dst2_height_delta2 = int(dst1.width * fabs(dSinE) * dAspect - dst1.height) & 0xFFFFFFFE;
		if (dst2_height_delta2 > dst2_height_delta)
			dst2_height_delta = dst2_height_delta2; // use max

		Img dst2(dst1.width, src.height + dst2_height_delta); // Fizick
		if (!dst2.pixel)
		{
			dst1.Free();
			return dst2;
		}

		double dOffset = (dst2_height_delta + (dst2.width - 1) * dSinE * dAspect) / 2.0; // Fizick
		// Variable skew offset
		for (unsigned u = 0; u < dst2.width; u++, dOffset -= (dSinE * dAspect))
		{
			int iShear = int(floor(dOffset));
			VertSkew(dst1, dst2, u, iShear, uint8_t(255 * (dOffset - double(iShear)) + 1), clrBack);
			// Report progress
			if (cb && !((*cb) (
				bMidImage ? 66.0 + (50.0 / 3) * u / dst2.height :
				33.0 + (100.0 / 3) * u / dst2.height
				)))
			{
				dst1.Free();
				dst2.Free();
				return dst2;
			}
		}
		// Free result of 1st shear
		dst1.Free();

		int dst3_width_delta = (int(double(src.height) * fabs(dSinE) / dAspect + double(src.width) * cos(dRadAngle)) + 1 - dst2.width) & 0xFFFFFFFE; // Fizick
		// Perform 3rd shear (horizontal)
		Img dst3(dst2.width + dst3_width_delta, dst2.height); // Fizick
		if (!dst3.pixel)
		{
			dst2.Free();
			return dst3;
		}
		dOffset = dst3_width_delta / 2 + (((int)dst3.height - 1) / 2.0) * (-dTan) / dAspect; // Fizick
		for (unsigned u = 0; u < dst3.height; u++, dOffset += (dTan / dAspect))
		{
			int iShear = int(floor(dOffset));
			HorizSkew(dst2, dst3, u, iShear, uint8_t(255 * (dOffset - double(iShear)) + 1), clrBack);
			if (cb && !((*cb) (
				bMidImage ? 83.0 + (50.0 / 3) * u / dst3.height :
				66.0 + (100.0 / 3) * u / dst3.height
				)))
			{
				dst2.Free();
				dst3.Free();
				return dst3;
			}
		}
		// Free result of 2nd shear
		dst2.Free();

		return dst3;
	}

	static
		Img HShearUpTo90(
			const Img& src,		// Img to shear (+dimensions)
			PixelType clrBack,			// Background color
			double dAngle,			// Degree of rotation
			double dAspect, // aspect factor = 2.0 for YV16 chroma planes
			bool bMidImage,			// Was middle image used (for correct progress report)
			ProgressAnbAbortCallBack* cb = 0
		)
	{
		const double ROTATE_PI = 3.1415926535897932384626433832795;
		double dRadAngle = dAngle * ROTATE_PI / double(180); // Angle in radians
		double dSin = sin(dRadAngle);
		double dCos = cos(dRadAngle);
		if (fabs(dCos) < 0.01)
		{ // singular
			Img dst1(src.width * 2, 0); // 2 is reserve
			return dst1;
		}
		double dTan = dSin / dCos;

		// Calc first shear (horizontal) destination image dimensions
		int dst1_width_delta = (int(double(src.height) * fabs(dTan) / dAspect + 0.5) + 1) & 0xFFFFFFFE;
		Img dst1(src.width + dst1_width_delta, src.height);
		if (!dst1.pixel)
			return dst1;


		// Perform 1st shear (horizontal)
		for (unsigned u = 0; u < dst1.height; u++)
		{
			double dShear = dst1_width_delta / 2 + ((int)u - ((int)dst1.height - 1) / 2.0) * dTan / dAspect;
			int iShear = (int)floor(dShear);
			HorizSkew(src, dst1, u, iShear, uint8_t(255 * (dShear - double(iShear)) + 1), clrBack);
			// Report progress
			if (cb && !((*cb) (
				bMidImage ? 50.0 + (50.0 / 3) * u / dst1.height :
				(100.0 / 3) * u / dst1.height
				)))
			{
				dst1.Free();
				return dst1;
			}
		}

		return dst1;

	}

	static
		Img VShearUpTo90(
			const Img& src,		// Img to shear (+dimensions)
			PixelType clrBack,			// Background color
			double dAngle,			// Degree of rotation
			double dAspect, // aspect factor = 2.0 for YV16 chroma planes
			bool bMidImage,			// Was middle image used (for correct progress report)
			ProgressAnbAbortCallBack* cb = 0
		)
	{
		const double ROTATE_PI = 3.1415926535897932384626433832795;
		double dRadAngle = dAngle * ROTATE_PI / double(180); // Angle in radians
		double dSin = sin(dRadAngle);
		double dCos = cos(dRadAngle);
		if (fabs(dCos) < 0.01)
		{ // singular
			Img dst2(0, src.height * 2); // 2 is reserve
			return dst2;
		}
		double dTan = dSin / dCos;

		// Calc  shear (vertical) destination image dimensions

		int dst2_height_delta = (int(double(src.width) * fabs(dTan) * dAspect + 0.5) + 1) & 0xFFFFFFFE;
		Img dst2(src.width, src.height + dst2_height_delta); // Fizick
		if (!dst2.pixel)
		{
			dst2.Free();
			return dst2;
		}

		double dOffset = (dst2_height_delta + (dst2.width - 1) * dTan * dAspect) / 2.0;
		// Variable skew offset
		for (int u = 0; u < (int)dst2.width; u++, dOffset -= (dTan * dAspect))
		{
			int iShear = int(floor(dOffset));
			VertSkew(src, dst2, u, iShear, uint8_t(255 * (dOffset - double(iShear)) + 1), clrBack);
			// Report progress
			if (cb && !((*cb) (
				bMidImage ? 66.0 + (50.0 / 3) * u / dst2.height :
				33.0 + (100.0 / 3) * u / dst2.height
				)))
			{
				dst2.Free();
				return dst2;
			}
		}

		return dst2;

	}

	static
		void HorizSkew(
			const Img& src,		// Img to skew (+ dimensions)
			Img& dst,				// Destination of skewed image (+ dimensions)
			unsigned uRow,			// Row index
			int iOffset,			// Skew offset
			unsigned __int8 Weight,			// Relative weight of right pixel
			PixelType clrBack			// Background color
		)
	{
		// Fill gap left of skew with background
		PixelType* p = &dst.RGBVAL(0, uRow);
		for (int i = 0u; i < iOffset; i++)
			*p++ = clrBack;


		PixelType pxlOldLeft;

#ifdef _M_XI86
		if (MMXpresnt() && sizeof(PixelType) == 4) // PixelRGB32 only - Fizick
		{
			int iw = (Weight << 24) | (Weight << 16) | (Weight << 8) | Weight;
			int iw1 = ~iw & 0xFFFFFFFF;
			short* sptr = (short*)&src.RGBVAL(0, uRow);
			short* dptr = (short*)&dst.RGBVAL(iOffset, uRow);
			int swidth = src.width;
			int dwidth = dst.width;

			_asm pxor mm0, mm0	// mm0 -zero
			_asm movd mm1, clrBack
			_asm PUNPCKLBW mm1, mm0 // mm1 - back
			_asm movq mm5, mm1      // mm5 - pxlOldLeft
			_asm mov edx, iw
			_asm movd mm2, edx
			_asm PUNPCKLBW mm2, mm0 // mm2 - weight
			_asm mov edx, iw1
			_asm movd mm3, edx
			_asm PUNPCKLBW mm3, mm0 // mm3 - 1-weight
			_asm PMULLW mm3, mm1	// mm3 - back*(1-weight)
			_asm mov esi, dword ptr[sptr]	// src
				_asm mov edi, dword ptr[dptr]	// dst

				_asm xor ecx, ecx		// ecx - loop counter
			_asm mov ebx, iOffset	// ebx - i+iOffset
			loop : _asm cmp ecx, swidth
				_asm jge endloop

			_asm MOVD mm6, word ptr[esi]
				_asm PUNPCKLBW mm6, mm0	// mm6 - pxlSrc
			_asm movq mm4, mm6   	// mm4 - pixel
			_asm PMULLW mm4, mm2	// mm4 - pixel*weight
			_asm PADDW mm4, mm3		// mm4 - pixel*weight + back*(1-weight)
			_asm PSRLW mm4, 8		// mm4 - (pixel*weight + back*(1-weight)) / 256 - pxlLeft

			_asm cmp ebx, 0			// if ((i + iOffset >= 0) && (i + iOffset < dst.width))
			_asm jl noupdate
			_asm cmp ebx, dwidth
			_asm jge noupdate

			// dst.RGBVAL (i + iOffset, uRow) = pxlSrc - (pxlLeft - pxlOldLeft);
			_asm PSUBW mm6, mm4
			_asm PADDW mm6, mm5
			_asm PACKUSWB mm6, mm0	// mm6 - packed (pxlSrc - (pxlLeft - pxlOldLeft))
			_asm MOVD word ptr[edi], mm6

			noupdate :						// end of if

			_asm MOVQ mm5, mm4		// pxlOldLeft = pxlLeft

			_asm inc ebx			// i+iOffset
			_asm add esi, 4
			_asm add edi, 4

			_asm inc ecx			// end of for
			goto loop;
		endloop:
			_asm PACKUSWB mm5, mm0
			_asm MOVD pxlOldLeft, mm5			// pxlOldLeft
			_asm EMMS							// restore FPU status

			// update i, iOffset, pxlOldLeft
		}
		else
#endif
		{
			pxlOldLeft = clrBack;

			for (unsigned i = 0; i < src.width; i++)
			{
				// Loop through row pixels
				PixelType pxlSrc = src.RGBVAL(i, uRow);
				// Calculate weights
				PixelType pxlLeft = interp(pxlSrc, clrBack, Weight);
				// Check boundries
				if ((i + iOffset >= 0) && (i + iOffset < dst.width))
				{
					// Update left over on source
					dst.RGBVAL(i + iOffset, uRow) = pxlSrc - (pxlLeft - pxlOldLeft);
				}
				// Save leftover for next pixel in scan
				pxlOldLeft = pxlLeft;
			}
		}
		// Go to rightmost point of skew
		int i = src.width + iOffset;
		// If still in image bounds, put leftovers there
		if (i < (int)dst.width)
			dst.RGBVAL(i, uRow) = pxlOldLeft;
		p = &dst.RGBVAL(i, uRow);
		while (++i < (int)dst.width)
		{   // Clear to the right of the skewed line with background
			*++p = clrBack;
		}
	}

	// Skews a column vertically (with filtered weights)
	// Limited to 45 degree skewing only. Filters two adjacent pixels.
	static
		void VertSkew(
			const Img& src,		// Img to skew (+ dimensions)
			Img& dst,				// Destination of skewed image (+ dimensions)
			unsigned uCol,				// Column index
			int iOffset,			// Skew offset
			unsigned __int8 Weight,			// Relative weight of right pixel
			PixelType clrBack			// Background color
		)
	{
		PixelType* p = &dst.RGBVAL(uCol, 0);
		for (int i = 0; i < iOffset; i++)
		{
			// Fill gap above skew with background
			*p = clrBack;
			dst.NextLine(p);
		}

		PixelType pxlOldLeft;

#ifdef _M_XI86
		if (MMXpresnt() && sizeof(PixelType) == 4) // PixelRGB32 only - Fizick
		{
			int iw = (Weight << 24) | (Weight << 16) | (Weight << 8) | Weight;
			int iw1 = ~iw & 0xFFFFFFFF;
			short* sptr = (short*)&src.RGBVAL(uCol, 0);
			short* dptr = (short*)&dst.RGBVAL(uCol, iOffset);
			int sheight = src.height;
			int dheight = dst.height;
			int swidthpad = src.width_pad;
			int dwidthpad = dst.width_pad;

			_asm pxor mm0, mm0	// mm0 -zero
			_asm movd mm1, clrBack
			_asm PUNPCKLBW mm1, mm0 // mm1 - back
			_asm movq mm5, mm1      // mm5 - pxlOldLeft
			_asm mov edx, iw
			_asm movd mm2, edx
			_asm PUNPCKLBW mm2, mm0 // mm2 - weight
			_asm mov edx, iw1
			_asm movd mm3, edx
			_asm PUNPCKLBW mm3, mm0 // mm3 - 1-weight
			_asm PMULLW mm3, mm1	// mm3 - back*(1-weight)
			_asm mov esi, dword ptr[sptr]	// src
				_asm mov edi, dword ptr[dptr]	// dst

				_asm xor ecx, ecx		// ecx - loop counter
			_asm mov ebx, iOffset	// ebx - i+iOffset
			loop : _asm cmp ecx, sheight
				_asm jge endloop

			//_asm mov ax, word ptr[esi]
			//_asm MOVD mm6, eax
			_asm MOVD mm6, word ptr[esi]
				_asm PUNPCKLBW mm6, mm0	// mm6 - pxlSrc
			_asm movq mm4, mm6   	// mm4 - pixel
			_asm PMULLW mm4, mm2	// mm4 - pixel*weight
			_asm PADDW mm4, mm3		// mm4 - pixel*weight + back*(1-weight)
			_asm PSRLW mm4, 8		// mm4 - (pixel*weight + back*(1-weight)) / 256 - pxlLeft

			_asm cmp ebx, 0			// if ((i + iOffset >= 0) && (i + iOffset < dst.height))
			_asm jl noupdate
			_asm cmp ebx, dheight
			_asm jge noupdate

			// dst.RGBVAL (i + iOffset, uRow) = pxlSrc - (pxlLeft - pxlOldLeft);
			_asm PSUBW mm6, mm4
			_asm PADDW mm6, mm5
			_asm PACKUSWB mm6, mm0	// mm6 - packed (pxlSrc - (pxlLeft - pxlOldLeft))
			_asm MOVD word ptr[edi], mm6

			noupdate :						// end of if

			_asm MOVQ mm5, mm4		// pxlOldLeft = pxlLeft

			_asm inc ebx			// i+iOffset
			_asm add esi, swidthpad
			_asm add edi, dwidthpad

			_asm inc ecx			// end of for
			goto loop;
		endloop:
			_asm PACKUSWB mm5, mm0
			_asm MOVD pxlOldLeft, mm5			// pxlOldLeft
			_asm EMMS							// restore FPU status

			// update i, iOffset, pxlOldLeft
		}
		else
#endif
		{
			pxlOldLeft = clrBack;

			for (int i = 0; i < (int)src.height; i++)
			{
				// Loop through column pixels
				PixelType pxlSrc = src.RGBVAL(uCol, i);
				// Calculate weights
				PixelType pxlLeft = interp(pxlSrc, clrBack, Weight);
				// Update left over on source
				// Check boundries
				int iYPos = i + iOffset;
				if ((iYPos >= 0) && (iYPos < (int)dst.height))
					dst.RGBVAL(uCol, iYPos) = pxlSrc - (pxlLeft - pxlOldLeft);
				// Save leftover for next pixel in scan
				pxlOldLeft = pxlLeft;
			}
		}
		// Go to bottom point of skew
		int i = src.height + iOffset;
		// If still in image bounds, put leftovers there
		if (i < (int)dst.height)
			dst.RGBVAL(uCol, i) = pxlOldLeft;
		// Clear below skewed line with background
		p = &dst.RGBVAL(uCol, i);
		while (++i < (int)dst.height)
		{
			dst.NextLine(p);
			*p = clrBack;
		}
	}

	static
		bool MMXpresnt()
	{

#ifdef _M_XI86
		//		return false; //debug
		__asm mov EAX, 1; request CPU feature flags
		__asm cpuid; 0Fh, 0A2h CPUID instruction
		__asm test EDX, 800000h; test bit 23 to see if MMX available
		__asm jnz yes//	             ;yes
		return false;
	yes:
		return true;
#else
		return false;
#endif
	}


};



// declare ImageRotate class from template with default implementations for PixelRGB32
class ImageRotateRGB32 : public ImageRotate<CallbackFn, AllocatePolicyStdNew, AlignmentPolicyBmp, PixelRGB32> {};

// AlignmentPolicy (for bytes)

class AlignmentPolicyPlanar
{
public:
	static unsigned aligned_width(unsigned width) { return (sizeof(BYTE)*width + 7) & (~7); }
protected:
	~AlignmentPolicyPlanar(){} // to prohibit destruction by client
};

// declare ImageRotate class from template with implementations for PixelByte
class ImageRotatePlanar : public ImageRotate<CallbackFn, AllocatePolicyStdNew, AlignmentPolicyPlanar, BYTE> {};


// several functions, copy from avisynth

inline int RR_PixelClip(int b)
{
    int a = b;
   return (a<0) ? 0 : ((a>255) ? 255 : a);
}

static __inline BYTE RR_ScaledPixelClip(int i) {
  return RR_PixelClip((i+32768) >> 16);
}

// not used here, but useful to other filters
inline int RGB2YUV(int rgb)
{
  const int cyb = int(0.114*219/255*65536+0.5);
  const int cyg = int(0.587*219/255*65536+0.5);
  const int cyr = int(0.299*219/255*65536+0.5);

//  _PixelClip PixelClip;

  // y can't overflow
  int y = (cyb*(rgb&255) + cyg*((rgb>>8)&255) + cyr*((rgb>>16)&255) + 0x108000) >> 16;
  int scaled_y = (y - 16) * int(255.0/219.0*65536+0.5);
  int b_y = ((rgb&255) << 16) - scaled_y;
  int u = RR_ScaledPixelClip((b_y >> 10) * int(1/2.018*1024+0.5) + 0x800000);
  int r_y = (rgb & 0xFF0000) - scaled_y;
  int v = RR_ScaledPixelClip((r_y >> 10) * int(1/1.596*1024+0.5) + 0x800000);
  return (y*256+u)*256+v;
}


void fillInt( unsigned int *start, int count, int pitch, int height, unsigned int value)
{
    for (int h=0; h<height; h++) {
        for (int w=0; w<count; w++)
            start[w] = value;
        start += pitch/4;
    }
}

void fillByte( BYTE *start, int count, int pitch, int height, BYTE value)
{
    for (int h=0; h<height; h++) {
        memset(start, value, count); // was stupid bug
        start += pitch;
    }

}



class Rotate : public GenericVideoFilter {
  // Rotate defines the name of your filter class.
  // This name is only used internally, and does not affect the name of your filter or similar.
  // This filter extends GenericVideoFilter, which incorporates basic functionality.
  // All functions present in the filter must also be present here.

	double startangle; // degrees
	unsigned int backcolor; // color
	int startframe;
	int endframe;
	double endangle;
	int newwidth;
	int newheight;
	double aspectratio; // pixel aspect ratio
	int shearmode;

	PixelRGB32 clrBack;			// Background color
	int oldwidth;
	int oldheight;

public:
  // This defines that these functions are present in your class.
  // These functions must be that same as those actually implemented.
  // Since the functions are "public" they are accessible to other classes.
  // Otherwise they can only be called from functions within the class itself.

	Rotate(PClip _child,	double _angle,  unsigned int _backcolor,
	int _startframe, int _endframe, double _endangle,
	int _newwidth, int _newheight, double _aspectratio, int _shearmode, IScriptEnvironment* env);
  // This is the constructor. It does not return any value, and is always used,
  //  when an instance of the class is created.
  // Since there is no code in this, this is the definition.

  ~Rotate();
  // The is the destructor definition. This is called when the filter is destroyed.


	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  // This is the function that AviSynth calls to get a given frame.
  // So when this functions gets called, the filter is supposed to return frame n.
};


/***************************
 * The following is the implementation
 * of the defined functions.
 ***************************/


//Here is the acutal constructor code used
Rotate::Rotate(PClip _child, double _angle,  unsigned int _backcolor,
    int _startframe, int _endframe, double _endangle, int _newwidth,
    int _newheight, double _aspectratio, int _shearmode, IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ) {
  // This is the implementation of the constructor.
  // The child clip (source clip) is inherited by the GenericVideoFilter,
  //  where the following variables gets defined:
  //   PClip child;   // Contains the source clip.
  //   VideoInfo vi;  // Contains videoinfo on the source clip.

    oldwidth = vi.width;
    oldheight = vi.height;
  if (_newwidth>0) {
    newwidth = _newwidth;
    vi.width = newwidth;
  }
  else
    newwidth = vi.width;

  if (_newheight>0) {
    newheight = _newheight;
    vi.height = newheight;
  }
  else
    newheight = vi.height;

  aspectratio = _aspectratio;

  if(!vi.IsRGB32() && !vi.IsPlanar() )
    env->ThrowError("Rotate: image format must be RGB32 or planar!");

    shearmode = _shearmode;

    if (!vi.IsRGB32()) {
        startangle = - _angle; // negative: frame top is bottom
        endangle = - _endangle; // negative: frame top is bottom
        backcolor = RGB2YUV(_backcolor);
    }
    else { // YV12
        startangle = _angle;
        backcolor = _backcolor;
        endangle = _endangle;
    }

    startframe = _startframe;
    endframe = (_endframe < 0) ? (vi.num_frames - 1) : _endframe;

		clrBack.c[0] = backcolor & 0xFF; // B or V
		clrBack.c[1] = (backcolor & 0xFF00) >> 8; // G or U
		clrBack.c[2] = (backcolor & 0xFF0000) >> 16; // R or Y
		clrBack.c[3] = (backcolor & 0xFF000000) >> 24; // alpha



}

// This is where any actual destructor code used goes
Rotate::~Rotate() {
  // This is where you can deallocate any memory you might have used.
}


PVideoFrame __stdcall Rotate::GetFrame(int n, IScriptEnvironment* env) {

   double angle = (endframe == startframe) ? startangle : startangle + (n-startframe)*(endangle - startangle)/(endframe - startframe);
	
	PVideoFrame src = child->GetFrame(n, env);
	if (src == nullptr || src.m_ptr == nullptr) {
		return nullptr;
	}
   // Request frame 'n' from the child (source) clip.

   if ((n<startframe || n>endframe))
   {
	   angle = 0; // do not rotate frames out of range
	   if (oldwidth==newwidth && oldheight==newheight) 
        return src; // nothing to do
   }

	PVideoFrame dst = env->NewVideoFrame(vi);

	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}
	if (vi.IsRGB32())
	{

	const unsigned char* srcp = src->GetReadPtr();

	unsigned char* dstp = dst->GetWritePtr();

	const int dst_pitch = dst->GetPitch();

	const int dst_width = dst->GetRowSize();

	const int dst_height = dst->GetHeight();

	const int src_pitch = src->GetPitch();
	const int src_width = src->GetRowSize();
	const int src_height = src->GetHeight();


		// This code deals with RGB32 colourspace where each pixel is represented by
		// 4 bytes, Blue, Green and Red and "spare" byte that could/should be used for alpha
		// keying but usually isn't.

        ImageRotateRGB32::Img srcimg (src_width/4, src_height);

		BYTE *srcimgp = reinterpret_cast<BYTE *>(srcimg.pixel);

		// copy source to img
        env->BitBlt(srcimgp, srcimg.width_pad, srcp, src_pitch, src_width, src_height);

//		Do not use ProgressAnbAbortCallBack *cb = 0
        ImageRotateRGB32::Img dstimg;
        if(shearmode==1)
            dstimg = ImageRotateRGB32::AllocAndHShear(srcimg, clrBack, angle, aspectratio, 0);
        else if(shearmode==2)
            dstimg = ImageRotateRGB32::AllocAndVShear(srcimg, clrBack, angle, aspectratio, 0);
        else //(shearmode == 0)
            dstimg = ImageRotateRGB32::AllocAndRotate(srcimg, clrBack, angle, aspectratio, 0);

		srcimg.Free();

		BYTE *dstimgp = reinterpret_cast<BYTE *>(dstimg.pixel);

		// copy dest img to dst frame

//            char buf[80];
//            wsprintf(buf, "Rotate: %d %d %d, %d", dstimg.width, dstimg.height, dstimg.width_pad, dst_width);
//            OutputDebugString(buf);

        int hdif = (int)dstimg.height - dst_height; // height difference
        int hhdif = hdif/2; // half height difference
        int hoffset = abs((int)dstimg.width_pad*hhdif) & (0xFFFFFFFC); // multiple 4
        int hoffsetdst = abs((int)dst_pitch*hhdif) & (0xFFFFFFFC); // multiple 4

        int wdif = (int)dstimg.width*4 - dst_width;
        int woffset = abs(wdif/2) & (0xFFFFFFFC); // multiple 4

       if ( wdif >= 0 && hdif >= 0 ) {
            env->BitBlt(dstp, dst_pitch, dstimgp + hoffset + woffset, dstimg.width_pad, dst_width, dst_height);
        }
        else if ( wdif < 0 && hdif >= 0 ) {
            fillInt( reinterpret_cast<unsigned int *>(dstp), woffset/4, dst_pitch, dst_height, backcolor);
            env->BitBlt(dstp + woffset, dst_pitch, dstimgp + hoffset, dstimg.width_pad, dstimg.width*4, dst_height);
            fillInt( reinterpret_cast<unsigned int *>(dstp+woffset+dstimg.width*4), woffset/4, dst_pitch, dst_height, backcolor);
        }
        else if ( wdif >= 0 && hdif < 0 ) {
            fillInt( reinterpret_cast<unsigned int *>(dstp), dst_width/4, dst_pitch, -hhdif, backcolor);
            env->BitBlt(dstp + hoffsetdst, dst_pitch, dstimgp + woffset, dstimg.width_pad, dst_width, dstimg.height);
            fillInt( reinterpret_cast<unsigned int *>(dstp+hoffsetdst+dstimg.height*dst_pitch), dst_width/4, dst_pitch, -hhdif, backcolor);
        }
        else //if ( wdif < 0 && hdif < 0 )
        {
            fillInt( reinterpret_cast<unsigned int *>(dstp), dst_width/4, dst_pitch, -hhdif, backcolor);
            fillInt( reinterpret_cast<unsigned int *>(dstp+hoffsetdst), woffset/4, dst_pitch, dstimg.height, backcolor);
            env->BitBlt(dstp + hoffsetdst + woffset, dst_pitch, dstimgp, dstimg.width_pad, dstimg.width*4, dstimg.height);
            fillInt( reinterpret_cast<unsigned int *>(dstp+hoffsetdst+woffset+dstimg.width*4), woffset/4, dst_pitch, dstimg.height, backcolor);
            fillInt( reinterpret_cast<unsigned int *>(dstp+hoffsetdst+dstimg.height*dst_pitch), dst_width/4, dst_pitch, -hhdif, backcolor);
        }

		dstimg.Free();
	}

	if (vi.IsPlanar()) // Planar
	{
		int planes = vi.IsY8() ? 1 : 3;
	    for (int i = 0; i<planes; i++)
	    {
	        int plane;
	        if (i == 0) plane = PLANAR_Y;
	        else if (i == 1) plane = PLANAR_U;
	        else if (i == 2) plane = PLANAR_V;

        const unsigned char* srcp = src->GetReadPtr(plane);
        unsigned char* dstp = dst->GetWritePtr(plane);

        const int dst_pitch = dst->GetPitch(plane);
        const int dst_width = dst->GetRowSize(plane);
        const int dst_height = dst->GetHeight(plane);
        const int src_pitch = src->GetPitch(plane);
        const int src_width = src->GetRowSize(plane);
        const int src_height = src->GetHeight(plane);

		double aspectSS = pow(2.0, (double) (vi.GetPlaneWidthSubsampling(plane) - vi.GetPlaneHeightSubsampling(plane))); // 2.6

         ImageRotatePlanar::Img srcimg (src_width, src_height);

		BYTE *srcimgp = reinterpret_cast<BYTE *>(srcimg.pixel);

		// copy source to img
        env->BitBlt(srcimgp, srcimg.width_pad, srcp, src_pitch, src_width, src_height);

        BYTE color = clrBack.c[2-i]; // specific plane color
//		Do not use ProgressAnbAbortCallBack *cb = 0

        ImageRotatePlanar::Img dstimg;
        if (shearmode==1)
             dstimg = ImageRotatePlanar::AllocAndHShear(srcimg, color, angle, aspectratio*aspectSS, 0);
        else if (shearmode==2)
             dstimg = ImageRotatePlanar::AllocAndVShear(srcimg, color, angle, aspectratio*aspectSS, 0);
        else //(shearmode == 0)
             dstimg = ImageRotatePlanar::AllocAndRotate(srcimg, color, angle, aspectratio*aspectSS, 0);

		srcimg.Free();

		BYTE *dstimgp = reinterpret_cast<BYTE *>(dstimg.pixel);

		// copy rotated dest img to dst frame and fill empty space
        int hdif = (int)dstimg.height - dst_height; // height difference
        int hhdif = hdif/2; // half height difference ???
        int hoffset = (int)dstimg.width_pad*abs(hhdif);
        int hoffsetdst = (int)dst_pitch*abs(hhdif);

        int wdif = (int)dstimg.width - dst_width;
        int woffset = abs(wdif/2);
 
        if ( wdif >= 0 && hdif >= 0 ) {
            env->BitBlt(dstp, dst_pitch, dstimgp + hoffset + woffset, dstimg.width_pad, dst_width, dst_height);
        }
        else if ( wdif < 0 && hdif >= 0 ) {
            fillByte( dstp, woffset, dst_pitch, dst_height, color);
            env->BitBlt(dstp + woffset, dst_pitch, dstimgp + hoffset, dstimg.width_pad, dstimg.width, dst_height);
            fillByte(dstp+woffset+dstimg.width, woffset, dst_pitch, dst_height, color);
        }
        else if ( wdif >= 0 && hdif < 0 ) {
            fillByte( dstp, dst_width, dst_pitch, -hhdif, color);
            env->BitBlt(dstp + hoffsetdst, dst_pitch, dstimgp + woffset, dstimg.width_pad, dst_width, dstimg.height);
            fillByte(dstp+hoffsetdst+dstimg.height*dst_pitch, dst_width, dst_pitch, -hhdif, color);
        }
        else //if ( wdif < 0 && hdif < 0 )
        {
           fillByte( dstp, dst_width, dst_pitch, -hhdif, color);
            fillByte( dstp+hoffsetdst, woffset, dst_pitch, dstimg.height, color);
            env->BitBlt(dstp + hoffsetdst + woffset, dst_pitch, dstimgp, dstimg.width_pad, dstimg.width, dstimg.height);
            fillByte(dstp+hoffsetdst+woffset+dstimg.width, woffset, dst_pitch, dstimg.height, color);
            fillByte( dstp+hoffsetdst+dstimg.height*dst_pitch, dst_width, dst_pitch, -hhdif, color);
        }

		dstimg.Free();
	    }
	}

  // As we now are finished processing the image, we return the destination image.
	return dst;
}


// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,
// based on the arguments recieved.

AVSValue __cdecl Create_Rotate(AVSValue args, void* user_data, IScriptEnvironment* env) {
    double angle = args[1].AsFloat(0);
    return new Rotate(args[0].AsClip(), // the 0th parameter is the source clip
		 angle, // rotation angle in degrees.
		 args[2].AsInt(0), // background color (integer or hex or global color constant like color_gray).
		 args[3].AsInt(0), // start frame
		 args[4].AsInt(-1), // end frame (-1 as latest)
		 args[5].AsFloat(angle), // end rotation angle in degrees.
		 args[6].AsInt(0), // destination width. 0 - same as source
		 args[7].AsInt(0), // destination height.
		 args[8].AsFloat(1.0), // pixel aspect ratio.
		 0, // rotaion or shear mode
		 env);
    // Calls the constructor with the arguments provied.
}
AVSValue __cdecl Create_HShear(AVSValue args, void* user_data, IScriptEnvironment* env) {
    double angle = args[1].AsFloat(0);
    return new Rotate(args[0].AsClip(), // the 0th parameter is the source clip
		 angle, // rotation angle in degrees.
		 args[2].AsInt(0), // background color (integer or hex or global color constant like color_gray).
		 args[3].AsInt(0), // start frame
		 args[4].AsInt(-1), // end frame (-1 as latest)
		 args[5].AsFloat(angle), // end rotation angle in degrees.
		 args[6].AsInt(0), // destination width. 0 - same as source
		 args[7].AsInt(0), // destination height.
		 args[8].AsFloat(1.0), // pixel aspect ratio.
		 1, // rotaion or shear mode
		 env);
}
AVSValue __cdecl Create_VShear(AVSValue args, void* user_data, IScriptEnvironment* env) {
    double angle = args[1].AsFloat(0);
    return new Rotate(args[0].AsClip(), // the 0th parameter is the source clip
		 angle, // rotation angle in degrees.
		 args[2].AsInt(0), // background color (integer or hex or global color constant like color_gray).
		 args[3].AsInt(0), // start frame
		 args[4].AsInt(-1), // end frame (-1 as latest)
		 args[5].AsFloat(angle), // end rotation angle in degrees.
		 args[6].AsInt(0), // destination width. 0 - same as source
		 args[7].AsInt(0), // destination height.
		 args[8].AsFloat(1.0), // pixel aspect ratio.
		 2, // rotaion or shear mode
		 env);
}


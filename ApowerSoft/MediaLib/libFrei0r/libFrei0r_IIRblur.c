//stdio samo za debug izpise
//#include <stdio.h>
#include <libfrei0r/frei0r.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <inttypes.h>

typedef struct
	{
	float r;
	float g;
	float b;
	float a;
	} float_rgba;


//edge compensation average size
#define EDGEAVG 8

#include <sys/types.h>


//---------------------------------------------------------
//koeficienti za biquad lowpass  iz f in q
// f v Nyquistih    0.0 < f < 0.5
static void calcab_lp1(float f, float q, float* a0, float* a1, float* a2, float* b0, float* b1, float* b2)
{
	float a, b;

	a = sinf(PI * f) / 2.0 / q;
	b = cosf(PI * f);
	*b0 = (1.0 - b) / 2.0;
	*b1 = 1.0 - b;
	*b2 = (1.0 - b) / 2.0;
	*a0 = 1.0 + a;
	*a1 = -2.0 * b;
	*a2 = 1.0 - a;
}

//---------------------------------------------------------
//3tap iir coefficients for Gauss approximation according to:
//Ian T. Young, Lucas J. van Vliet:
//Recursive implementation of the Gaussian filter
//Signal Processing 44 (1995) 139-151
// s=sigma    0.5 < s < 200.0
static void young_vliet(float s, float* a0, float* a1, float* a2, float* a3)
{
	float q;

	q = 0.0;
	if (s > 2.5)
	{
		q = 0.98711 * s - 0.96330;
	}
	else
	{	//to velja za s>0.5 !!!!
		q = 3.97156 - 4.14554 * sqrtf(1.0 - 0.26891 * s);
	}

	*a0 = 1.57825 + 2.44413 * q + 1.4281 * q * q + 0.422205 * q * q * q;
	*a1 = 2.44413 * q + 2.85619 * q * q + 1.26661 * q * q * q;
	*a2 = -1.4281 * q * q - 1.26661 * q * q * q;
	*a3 = 0.422205 * q * q * q;
}

//---------------------------------------------------
//kompenzacija na desni
//c=0.0 "odziv na zacetno stanje" (zunaj crno)
//gain ni kompenziran
static void rep(float v1, float v2, float c, float* i1, float* i2, int n, float a1, float a2)
{
	int i;
	float lb[8192];

	lb[0] = v1; lb[1] = v2;
	for (i = 2; i < n - 2; i++)
	{
		lb[i] = c - a1 * lb[i - 1] - a2 * lb[i - 2];
	}

	lb[n - 2] = 0.0; lb[n - 1] = 0.0;
	for (i = n - 3; i >= 0; i--)
	{
		lb[i] = lb[i] - a1 * lb[i + 1] - a2 * lb[i + 2];
	}

	*i1 = lb[0]; *i2 = lb[1];
}

//---------------------------------------------------------
// 1-tap IIR v 4 smereh
//optimized for speed
//loops rearanged for more locality (better cache hit ratio)
//outer (vertical) loop 2x unroll to break dependency chain
//simplified indexes
static void fibe1o_8(const uint32_t* inframe, uint32_t* outframe, float_rgba* s, int w, int h, float a, int ec)
{
	int i, j;
	float b, g, g4, avg, avg1, cr, cg, cb, g4a, g4b;
	int p, pw, pj, pwj, pww, pmw;

	avg = EDGEAVG;	//koliko vzorcev za povprecje pri edge comp
	avg1 = 1.0 / avg;

	g = 1.0 / (1.0 - a);
	g4 = 1.0 / g / g / g / g;

	//predpostavimo, da je "zunaj" crnina (nicle)
	b = 1.0 / (1.0 - a) / (1.0 + a);

	//prvih avg vrstic
	for (i = 0; i < avg; i++)
	{
		p = i * w; pw = p + w;
		if (ec != 0)
		{
			cr = 0.0; cg = 0.0; cb = 0.0;
			for (j = 0; j < avg; j++)
			{
				s[p + j].r = (float)(inframe[p + j] & 0xFF);
				s[p + j].g = (float)((inframe[p + j] & 0xFF00) >> 8);
				s[p + j].b = (float)((inframe[p + j] & 0xFF0000) >> 16);
				cr = cr + s[p + j].r;
				cg = cg + s[p + j].g;
				cb = cb + s[p + j].b;
			}
			cr = cr * avg1; cg = cg * avg1; cb = cb * avg1;
			s[p].r = cr * g + b * (s[p].r - cr);
			s[p].g = cg * g + b * (s[p].g - cg);
			s[p].b = cb * g + b * (s[p].b - cb);
		}
		else
			for (j = 0; j < avg; j++)
			{
				s[p + j].r = (float)(inframe[p + j] & 0xFF);
				s[p + j].g = (float)((inframe[p + j] & 0xFF00) >> 8);
				s[p + j].b = (float)((inframe[p + j] & 0xFF0000) >> 16);
			}

		for (j = 1; j < avg; j++)	//tja  (ze pretvorjeni)
		{
			s[p + j].r = s[p + j].r + a * s[p + j - 1].r;
			s[p + j].g = s[p + j].g + a * s[p + j - 1].g;
			s[p + j].b = s[p + j].b + a * s[p + j - 1].b;
		}
		for (j = avg; j < w; j++)	//tja  (s pretvorbo)
		{
			s[p + j].r = (float)(inframe[p + j] & 0xFF);
			s[p + j].g = (float)((inframe[p + j] & 0xFF00) >> 8);
			s[p + j].b = (float)((inframe[p + j] & 0xFF0000) >> 16);
			s[p + j].r = s[p + j].r + a * s[p + j - 1].r;
			s[p + j].g = s[p + j].g + a * s[p + j - 1].g;
			s[p + j].b = s[p + j].b + a * s[p + j - 1].b;
		}

		if (ec != 0)
		{
			cr = 0.0; cg = 0.0; cb = 0.0;
			for (j = w - avg; j < w; j++)
			{
				cr = cr + s[p + j].r;
				cg = cg + s[p + j].g;
				cb = cb + s[p + j].b;
			}
			cr = cr * avg1; cg = cg * avg1; cb = cb * avg1;
			s[pw - 1].r = cr * g + b * (s[pw - 1].r - cr);
			s[pw - 1].g = cg * g + b * (s[pw - 1].g - cg);
			s[pw - 1].b = cb * g + b * (s[pw - 1].b - cb);
		}
		else
		{
			s[pw - 1].r = b * s[pw - 1].r;
			s[pw - 1].g = b * s[pw - 1].g;
			s[pw - 1].b = b * s[pw - 1].b;
		}
		for (j = w - 2; j >= 0; j--)	//nazaj
		{
			s[p + j].r = a * s[p + j + 1].r + s[p + j].r;
			s[p + j].g = a * s[p + j + 1].g + s[p + j].g;
			s[p + j].b = a * s[p + j + 1].b + s[p + j].b;
		}
	}

	//prvih avg vrstic samo navzdol (nazaj so ze)
	for (i = 0; i < w; i++)
	{
		if (ec != 0)
		{
			cr = 0.0; cg = 0.0; cb = 0.0;
			for (j = 0; j < avg; j++)
			{
				cr = cr + s[i + w * j].r;
				cg = cg + s[i + w * j].g;
				cb = cb + s[i + w * j].b;
			}
			cr = cr * avg1; cg = cg * avg1; cb = cb * avg1;
			s[i].r = cr * g + b * (s[i].r - cr);
			s[i].g = cg * g + b * (s[i].g - cg);
			s[i].b = cb * g + b * (s[i].b - cb);
		}
		for (j = 1; j < avg; j++)	//dol
		{
			s[i + j * w].r = s[i + j * w].r + a * s[i + w * (j - 1)].r;
			s[i + j * w].g = s[i + j * w].g + a * s[i + w * (j - 1)].g;
			s[i + j * w].b = s[i + j * w].b + a * s[i + w * (j - 1)].b;
		}
	}

	for (i = avg; i < h - 1; i = i + 2)	//po vrsticah navzdol
	{
		p = i * w; pw = p + w; pww = pw + w; pmw = p - w;
		if (ec != 0)
		{
			cr = 0.0; cg = 0.0; cb = 0.0;
			for (j = 0; j < avg; j++)
			{
				s[p + j].r = (float)(inframe[p + j] & 0xFF);
				s[p + j].g = (float)((inframe[p + j] & 0xFF00) >> 8);
				s[p + j].b = (float)((inframe[p + j] & 0xFF0000) >> 16);
				cr = cr + s[p + j].r;
				cg = cg + s[p + j].g;
				cb = cb + s[p + j].b;
			}
			cr = cr * avg1; cg = cg * avg1; cb = cb * avg1;
			s[p].r = cr * g + b * (s[p].r - cr);
			s[p].g = cg * g + b * (s[p].g - cg);
			s[p].b = cb * g + b * (s[p].b - cb);
			cr = 0.0; cg = 0.0; cb = 0.0;
			for (j = 0; j < avg; j++)
			{
				s[pw + j].r = (float)(inframe[pw + j] & 0xFF);
				s[pw + j].g = (float)((inframe[pw + j] & 0xFF00) >> 8);
				s[pw + j].b = (float)((inframe[pw + j] & 0xFF0000) >> 16);
				cr = cr + s[pw + j].r;
				cg = cg + s[pw + j].g;
				cb = cb + s[pw + j].b;
			}
			cr = cr * avg1; cg = cg * avg1; cb = cb * avg1;
			s[pw].r = cr * g + b * (s[pw].r - cr);
			s[pw].g = cg * g + b * (s[pw].g - cg);
			s[pw].b = cb * g + b * (s[pw].b - cb);
		}
		else
		{
			for (j = 0; j < avg; j++)
			{
				s[p + j].r = (float)(inframe[p + j] & 0xFF);
				s[p + j].g = (float)((inframe[p + j] & 0xFF00) >> 8);
				s[p + j].b = (float)((inframe[p + j] & 0xFF0000) >> 16);
			}
			for (j = 0; j < avg; j++)
			{
				s[pw + j].r = (float)(inframe[pw + j] & 0xFF);
				s[pw + j].g = (float)((inframe[pw + j] & 0xFF00) >> 8);
				s[pw + j].b = (float)((inframe[pw + j] & 0xFF0000) >> 16);
			}
		}
		for (j = 1; j < avg; j++)	//tja  (ze pretvojeni)
		{
			pj = p + j; pwj = pw + j;
			s[pj].r = s[pj].r + a * s[pj - 1].r;
			s[pj].g = s[pj].g + a * s[pj - 1].g;
			s[pj].b = s[pj].b + a * s[pj - 1].b;
			s[pwj].r = s[pwj].r + a * s[pwj - 1].r;
			s[pwj].g = s[pwj].g + a * s[pwj - 1].g;
			s[pwj].b = s[pwj].b + a * s[pwj - 1].b;
		}
		for (j = avg; j < w; j++)	//tja  (s pretvorbo)
		{
			pj = p + j; pwj = pw + j;
			s[pj].r = (float)(inframe[pj] & 0xFF);
			s[pj].g = (float)((inframe[pj] & 0xFF00) >> 8);
			s[pj].b = (float)((inframe[pj] & 0xFF0000) >> 16);
			s[pj].r = s[pj].r + a * s[pj - 1].r;
			s[pj].g = s[pj].g + a * s[pj - 1].g;
			s[pj].b = s[pj].b + a * s[pj - 1].b;
			s[pwj].r = (float)(inframe[pwj] & 0xFF);
			s[pwj].g = (float)((inframe[pwj] & 0xFF00) >> 8);
			s[pwj].b = (float)((inframe[pwj] & 0xFF0000) >> 16);
			s[pwj].r = s[pwj].r + a * s[pwj - 1].r;
			s[pwj].g = s[pwj].g + a * s[pwj - 1].g;
			s[pwj].b = s[pwj].b + a * s[pwj - 1].b;
		}

		if (ec != 0)
		{
			cr = 0.0; cg = 0.0; cb = 0.0;
			for (j = w - avg; j < w; j++)
			{
				cr = cr + s[p + j].r;
				cg = cg + s[p + j].g;
				cb = cb + s[p + j].b;
			}
			cr = cr * avg1; cg = cg * avg1; cb = cb * avg1;
			s[pw - 1].r = cr * g + b * (s[pw - 1].r - cr);
			s[pw - 1].g = cg * g + b * (s[pw - 1].g - cg);
			s[pw - 1].b = cb * g + b * (s[pw - 1].b - cb);
			cr = 0.0; cg = 0.0; cb = 0.0;
			for (j = w - avg; j < w; j++)
			{
				cr = cr + s[pw + j].r;
				cg = cg + s[pw + j].g;
				cb = cb + s[pw + j].b;
			}
			cr = cr * avg1; cg = cg * avg1; cb = cb * avg1;
			s[pww - 1].r = cr * g + b * (s[pww - 1].r - cr);
			s[pww - 1].g = cg * g + b * (s[pww - 1].g - cg);
			s[pww - 1].b = cb * g + b * (s[pww - 1].b - cb);
		}
		else
		{
			s[pw - 1].r = b * s[pw - 1].r;	//rep H
			s[pw - 1].g = b * s[pw - 1].g;
			s[pw - 1].b = b * s[pw - 1].b;
			s[pww - 1].r = b * s[pww - 1].r;
			s[pww - 1].g = b * s[pww - 1].g;
			s[pww - 1].b = b * s[pww - 1].b;
		}

		//zacetek na desni
		s[pw - 2].r = s[pw - 2].r + a * s[pw - 1].r;	//nazaj
		s[pw - 2].g = s[pw - 2].g + a * s[pw - 1].g;
		s[pw - 2].b = s[pw - 2].b + a * s[pw - 1].b;
		s[pw - 1].r = s[pw - 1].r + a * s[p - 1].r;		//dol
		s[pw - 1].g = s[pw - 1].g + a * s[p - 1].g;
		s[pw - 1].b = s[pw - 1].b + a * s[p - 1].b;

		for (j = w - 2; j >= 1; j--)	//nazaj
		{
			pj = p + j; pwj = pw + j;
			s[pj - 1].r = a * s[pj].r + s[pj - 1].r;
			s[pj - 1].g = a * s[pj].g + s[pj - 1].g;
			s[pj - 1].b = a * s[pj].b + s[pj - 1].b;
			s[pwj].r = a * s[pwj + 1].r + s[pwj].r;
			s[pwj].g = a * s[pwj + 1].g + s[pwj].g;
			s[pwj].b = a * s[pwj + 1].b + s[pwj].b;
			//zdaj naredi se en piksel vertikalno dol, za vse stolpce
			//dva nazaj, da ne vpliva na H nazaj
			s[pj].r = s[pj].r + a * s[pmw + j].r;
			s[pj].g = s[pj].g + a * s[pmw + j].g;
			s[pj].b = s[pj].b + a * s[pmw + j].b;
			s[pwj + 1].r = s[pwj + 1].r + a * s[pj + 1].r;
			s[pwj + 1].g = s[pwj + 1].g + a * s[pj + 1].g;
			s[pwj + 1].b = s[pwj + 1].b + a * s[pj + 1].b;
		}
		//konec levo
		s[pw].r = s[pw].r + a * s[pw + 1].r;	//nazaj
		s[pw].g = s[pw].g + a * s[pw + 1].g;
		s[pw].b = s[pw].b + a * s[pw + 1].b;
		s[p].r = s[p].r + a * s[pmw].r;	//dol
		s[p].g = s[p].g + a * s[pmw].g;
		s[p].b = s[p].b + a * s[pmw].b;
		s[pw + 1].r = s[pw + 1].r + a * s[p + 1].r;	//dol
		s[pw + 1].g = s[pw + 1].g + a * s[p + 1].g;
		s[pw + 1].b = s[pw + 1].b + a * s[p + 1].b;
		s[pw].r = s[pw].r + a * s[p].r;	//dol
		s[pw].g = s[pw].g + a * s[p].g;
		s[pw].b = s[pw].b + a * s[p].b;
	}

	//ce je sodo stevilo vrstic, moras zadnjo posebej
	if (i != h)
	{
		p = i * w; pw = p + w;
		for (j = 1; j < w; j++)	//tja
		{
			s[p + j].r = s[p + j].r + a * s[p + j - 1].r;
			s[p + j].g = s[p + j].g + a * s[p + j - 1].g;
			s[p + j].b = s[p + j].b + a * s[p + j - 1].b;
		}

		s[pw - 1].r = b * s[pw - 1].r;	//rep H
		s[pw - 1].g = b * s[pw - 1].g;
		s[pw - 1].b = b * s[pw - 1].b;

		for (j = w - 2; j >= 0; j--)	//nazaj in dol
		{
			s[p + j].r = a * s[p + j + 1].r + s[p + j].r;
			s[p + j].g = a * s[p + j + 1].g + s[p + j].g;
			s[p + j].b = a * s[p + j + 1].b + s[p + j].b;

			//zdaj naredi se en piksel vertikalno dol, za vse stolpce
			//dva nazaj, da ne vpliva na H nazaj
			s[p + j + 1].r = s[p + j + 1].r + a * s[p - w + j + 1].r;
			s[p + j + 1].g = s[p + j + 1].g + a * s[p - w + j + 1].g;
			s[p + j + 1].b = s[p + j + 1].b + a * s[p - w + j + 1].b;
		}
		//levi piksel vert
		s[p].r = s[p].r + a * s[p - w].r;
		s[p].g = s[p].g + a * s[p - w].g;
		s[p].b = s[p].b + a * s[p - w].b;
	}

	//zadnja vrstica (h-1)
	g4b = g4 * b;
	g4a = g4 / (1.0 - a);
	p = (h - 1) * w;
	if (ec != 0)
	{
		for (i = 0; i < w; i++)	//po stolpcih
		{
			cr = 0.0; cg = 0.0; cb = 0.0;
			for (j = h - avg; j < h; j++)
			{
				cr = cr + s[i + w * j].r;
				cg = cg + s[i + w * j].g;
				cb = cb + s[i + w * j].b;
			}
			cr = cr * avg1; cg = cg * avg1; cb = cb * avg1;
			s[i + p].r = g4a * cr + g4b * (s[i + p].r - cr);
			s[i + p].g = g4a * cg + g4b * (s[i + p].g - cg);
			s[i + p].b = g4a * cb + g4b * (s[i + p].b - cb);
			outframe[p + i] = ((uint32_t)s[p + i].r & 0xFF) + (((uint32_t)s[p + i].g & 0xFF) << 8) + (((uint32_t)s[p + i].b & 0xFF) << 16);
		}
	}
	else
	{
		for (j = 0; j < w; j++)	//po stolpcih
		{
			s[j + p].r = g4b * s[j + p].r;	//rep V
			s[j + p].g = g4b * s[j + p].g;
			s[j + p].b = g4b * s[j + p].b;
			outframe[p + j] = ((uint32_t)s[p + j].r & 0xFF) + (((uint32_t)s[p + j].g & 0xFF) << 8) + (((uint32_t)s[p + j].b & 0xFF) << 16);
		}
	}

	for (i = h - 2; i >= 0; i--)	//po vrsticah navzgor
	{
		p = i * w; pw = p + w;
		for (j = 0; j < w; j++)	//po stolpcih
		{
			s[p + j].r = a * s[pw + j].r + g4 * s[p + j].r;
			s[p + j].g = a * s[pw + j].g + g4 * s[p + j].g;
			s[p + j].b = a * s[pw + j].b + g4 * s[p + j].b;
			outframe[p + j] = ((uint32_t)s[p + j].r & 0xFF) + (((uint32_t)s[p + j].g & 0xFF) << 8) + (((uint32_t)s[p + j].b & 0xFF) << 16);
		}
	}

}

//-------------------------------------------------------
// 2-tap IIR v stirih smereh   a only verzija, a0=1.0
//desno kompenzacijo izracuna direktno (rdx,rsx,rcx)
//optimized for speed
static void fibe2o_8(const uint32_t* inframe, uint32_t* outframe, float_rgba s[], int w, int h, float a1, float a2, float rd1, float rd2, float rs1, float rs2, float rc1, float rc2, int ec)
{
	float cr, cg, cb, g, g4, avg, gavg, avgg, iavg;
	float_rgba rep1, rep2;
	int i, j;
	int jw, jww, h1w, h2w, iw, i1w, i2w;

	g = 1.0 / (1.0 + a1 + a2);
	g4 = 1.0 / g / g / g / g;
	avg = EDGEAVG;	//koliko vzorcev za povprecje pri edge comp
	gavg = g4 / avg;
	avgg = 1.0 / g / avg;
	iavg = 1.0 / avg;

	for (j = 0; j < avg; j++)	//prvih avg vrstic tja in nazaj
	{
		jw = j * w; jww = jw + w;
		cr = 0.0; cg = 0.0; cb = 0.0;
		if (ec != 0)
		{	//edge comp (popvprecje prvih)
			for (i = 0; i < avg; i++)
			{
				s[jw + i].r = (float)(inframe[jw + i] & 0xFF);
				s[jw + i].g = (float)((inframe[jw + i] & 0xFF00) >> 8);
				s[jw + i].b = (float)((inframe[jw + i] & 0xFF0000) >> 16);
				cr = cr + s[jw + i].r;
				cg = cg + s[jw + i].g;
				cb = cb + s[jw + i].b;
			}
			cr = cr * gavg; cg = cg * gavg; cb = cb * gavg;
		}
		else
			for (i = 0; i < avg; i++)
			{
				s[jw + i].r = (float)(inframe[jw + i] & 0xFF);
				s[jw + i].g = (float)((inframe[jw + i] & 0xFF00) >> 8);
				s[jw + i].b = (float)((inframe[jw + i] & 0xFF0000) >> 16);
			}

		s[jw].r = g4 * s[jw].r - (a1 + a2) * g * cr;
		s[jw].g = g4 * s[jw].g - (a1 + a2) * g * cg;
		s[jw].b = g4 * s[jw].b - (a1 + a2) * g * cb;
		s[jw + 1].r = g4 * s[jw + 1].r - a1 * s[jw].r - a2 * g * cr;
		s[jw + 1].g = g4 * s[jw + 1].g - a1 * s[jw].g - a2 * g * cg;
		s[jw + 1].b = g4 * s[jw + 1].b - a1 * s[jw].b - a2 * g * cb;

		if (ec != 0)
		{	//edge comp za nazaj
			cr = 0.0; cg = 0.0; cb = 0.0;
			for (i = w - avg; i < w; i++)
			{
				s[jw + i].r = (float)(inframe[jw + i] & 0xFF);
				s[jw + i].g = (float)((inframe[jw + i] & 0xFF00) >> 8);
				s[jw + i].b = (float)((inframe[jw + i] & 0xFF0000) >> 16);
				cr = cr + s[jw + i].r;
				cg = cg + s[jw + i].g;
				cb = cb + s[jw + i].b;
			}
			cr = cr * gavg; cg = cg * gavg; cb = cb * gavg;
		}
		else
			for (i = w - avg; i < w; i++)
			{
				s[jw + i].r = (float)(inframe[jw + i] & 0xFF);
				s[jw + i].g = (float)((inframe[jw + i] & 0xFF00) >> 8);
				s[jw + i].b = (float)((inframe[jw + i] & 0xFF0000) >> 16);
			}

		for (i = 2; i < avg; i++)	//tja (ze pretv. levo)
		{
			s[jw + i].r = g4 * s[jw + i].r - a1 * s[jw + i - 1].r - a2 * s[jw + i - 2].r;
			s[jw + i].g = g4 * s[jw + i].g - a1 * s[jw + i - 1].g - a2 * s[jw + i - 2].g;
			s[jw + i].b = g4 * s[jw + i].b - a1 * s[jw + i - 1].b - a2 * s[jw + i - 2].b;
		}

		for (i = avg; i < w - avg; i++)	//tja (s pretvorbo)
		{
			s[jw + i].r = (float)(inframe[jw + i] & 0xFF);
			s[jw + i].g = (float)((inframe[jw + i] & 0xFF00) >> 8);
			s[jw + i].b = (float)((inframe[jw + i] & 0xFF0000) >> 16);
			s[jw + i].r = g4 * s[jw + i].r - a1 * s[jw + i - 1].r - a2 * s[jw + i - 2].r;
			s[jw + i].g = g4 * s[jw + i].g - a1 * s[jw + i - 1].g - a2 * s[jw + i - 2].g;
			s[jw + i].b = g4 * s[jw + i].b - a1 * s[jw + i - 1].b - a2 * s[jw + i - 2].b;
		}

		for (i = w - avg; i < w; i++)	//tja (ze pretv. desno)
		{
			s[jw + i].r = g4 * s[jw + i].r - a1 * s[jw + i - 1].r - a2 * s[jw + i - 2].r;
			s[jw + i].g = g4 * s[jw + i].g - a1 * s[jw + i - 1].g - a2 * s[jw + i - 2].g;
			s[jw + i].b = g4 * s[jw + i].b - a1 * s[jw + i - 1].b - a2 * s[jw + i - 2].b;
		}

		rep1.r = (s[jww - 1].r + s[jww - 2].r) * 0.5 * rs1 + (s[jww - 1].r - s[jww - 2].r) * rd1;
		rep1.g = (s[jww - 1].g + s[jww - 2].g) * 0.5 * rs1 + (s[jww - 1].g - s[jww - 2].g) * rd1;
		rep1.b = (s[jww - 1].b + s[jww - 2].b) * 0.5 * rs1 + (s[jww - 1].b - s[jww - 2].b) * rd1;
		rep2.r = (s[jww - 1].r + s[jww - 2].r) * 0.5 * rs2 + (s[jww - 1].r - s[jww - 2].r) * rd2;
		rep2.g = (s[jww - 1].g + s[jww - 2].g) * 0.5 * rs2 + (s[jww - 1].g - s[jww - 2].g) * rd2;
		rep2.b = (s[jww - 1].b + s[jww - 2].b) * 0.5 * rs2 + (s[jww - 1].b - s[jww - 2].b) * rd2;

		if (ec != 0)
		{
			rep1.r = rep1.r + rc1 * cr;
			rep1.g = rep1.g + rc1 * cg;
			rep1.b = rep1.b + rc1 * cb;
			rep2.r = rep2.r + rc2 * cr;
			rep2.g = rep2.g + rc2 * cg;
			rep2.b = rep2.b + rc2 * cb;
		}

		s[jww - 1].r = s[jww - 1].r - a1 * rep1.r - a2 * rep2.r;
		s[jww - 1].g = s[jww - 1].g - a1 * rep1.g - a2 * rep2.g;
		s[jww - 1].b = s[jww - 1].b - a1 * rep1.b - a2 * rep2.b;
		s[jww - 2].r = s[jww - 2].r - a1 * s[jww - 1].r - a2 * rep1.r;
		s[jww - 2].g = s[jww - 2].g - a1 * s[jww - 1].g - a2 * rep1.g;
		s[jww - 2].b = s[jww - 2].b - a1 * s[jww - 1].b - a2 * rep1.b;

		for (i = w - 3; i >= 0; i--)		//nazaj
		{
			s[jw + i].r = s[jw + i].r - a1 * s[jw + i + 1].r - a2 * s[jw + i + 2].r;
			s[jw + i].g = s[jw + i].g - a1 * s[jw + i + 1].g - a2 * s[jw + i + 2].g;
			s[jw + i].b = s[jw + i].b - a1 * s[jw + i + 1].b - a2 * s[jw + i + 2].b;
		}
	}	//prvih avg vrstic

//edge comp zgoraj za navzdol
	for (j = 0; j < w; j++)	//po stolpcih
	{
		cr = 0.0; cg = 0.0; cb = 0.0;
		if (ec != 0)
		{	//edge comp (popvprecje prvih)
			for (i = 0; i < avg; i++)
			{
				cr = cr + s[j + w * i].r;
				cg = cg + s[j + w * i].g;
				cb = cb + s[j + w * i].b;
			}
			cr = cr * iavg; cg = cg * iavg; cb = cb * iavg;
		}

		//zgornji vrstici
		s[j].r = s[j].r - (a1 + a2) * g * cr;
		s[j].g = s[j].g - (a1 + a2) * g * cg;
		s[j].b = s[j].b - (a1 + a2) * g * cb;
		s[j + w].r = s[j + w].r - a1 * s[j].r - a2 * g * cr;
		s[j + w].g = s[j + w].g - a1 * s[j].g - a2 * g * cg;
		s[j + w].b = s[j + w].b - a1 * s[j].b - a2 * g * cb;
	}

	//tretja do avg, samo navzdol (nazaj so ze)
	for (i = 2; i < avg; i++)
	{
		iw = i * w; i1w = iw - w;
		for (j = 0; j < w; j++)	//po stolpcih
		{
			s[j + iw].r = s[j + iw].r - a1 * s[j + i1w].r - a2 * s[j + i1w - w].r;
			s[j + iw].g = s[j + iw].g - a1 * s[j + i1w].g - a2 * s[j + i1w - w].g;
			s[j + iw].b = s[j + iw].b - a1 * s[j + i1w].b - a2 * s[j + i1w - w].b;
		}
	}

	for (j = avg; j < h; j++)	//po vrsticah tja, nazaj in dol
	{
		jw = j * w; jww = jw + w;
		cr = 0.0; cg = 0.0; cb = 0.0;
		if (ec != 0)
		{	//edge comp (popvprecje prvih)
			for (i = 0; i < avg; i++)
			{
				s[jw + i].r = (float)(inframe[jw + i] & 0xFF);
				s[jw + i].g = (float)((inframe[jw + i] & 0xFF00) >> 8);
				s[jw + i].b = (float)((inframe[jw + i] & 0xFF0000) >> 16);
				cr = cr + s[jw + i].r;
				cg = cg + s[jw + i].g;
				cb = cb + s[jw + i].b;
			}
			cr = cr * gavg; cg = cg * gavg; cb = cb * gavg;
		}
		else
			for (i = 0; i < avg; i++)
			{
				s[jw + i].r = (float)(inframe[jw + i] & 0xFF);
				s[jw + i].g = (float)((inframe[jw + i] & 0xFF00) >> 8);
				s[jw + i].b = (float)((inframe[jw + i] & 0xFF0000) >> 16);
			}
		s[jw].r = g4 * s[jw].r - (a1 + a2) * g * cr;
		s[jw].g = g4 * s[jw].g - (a1 + a2) * g * cg;
		s[jw].b = g4 * s[jw].b - (a1 + a2) * g * cb;
		s[jw + 1].r = g4 * s[jw + 1].r - a1 * s[jw].r - a2 * g * cr;
		s[jw + 1].g = g4 * s[jw + 1].g - a1 * s[jw].g - a2 * g * cg;
		s[jw + 1].b = g4 * s[jw + 1].b - a1 * s[jw].b - a2 * g * cb;

		if (ec != 0)
		{	//edge comp za nazaj
			cr = 0.0; cg = 0.0; cb = 0.0;
			for (i = w - avg; i < w; i++)
			{
				s[jw + i].r = (float)(inframe[jw + i] & 0xFF);
				s[jw + i].g = (float)((inframe[jw + i] & 0xFF00) >> 8);
				s[jw + i].b = (float)((inframe[jw + i] & 0xFF0000) >> 16);
				cr = cr + s[jw + i].r;
				cg = cg + s[jw + i].g;
				cb = cb + s[jw + i].b;
			}
			cr = cr * gavg; cg = cg * gavg; cb = cb * gavg;
		}
		else
			for (i = w - avg; i < w; i++)
			{
				s[jw + i].r = (float)(inframe[jw + i] & 0xFF);
				s[jw + i].g = (float)((inframe[jw + i] & 0xFF00) >> 8);
				s[jw + i].b = (float)((inframe[jw + i] & 0xFF0000) >> 16);
			}

		for (i = 2; i < avg; i++)	//tja  (ze pretv. levo)
		{
			s[jw + i].r = g4 * s[jw + i].r - a1 * s[jw + i - 1].r - a2 * s[jw + i - 2].r;
			s[jw + i].g = g4 * s[jw + i].g - a1 * s[jw + i - 1].g - a2 * s[jw + i - 2].g;
			s[jw + i].b = g4 * s[jw + i].b - a1 * s[jw + i - 1].b - a2 * s[jw + i - 2].b;
		}

		for (i = avg; i < w - avg; i++)	//tja  (s retvorbo)
		{
			s[jw + i].r = (float)(inframe[jw + i] & 0xFF);
			s[jw + i].g = (float)((inframe[jw + i] & 0xFF00) >> 8);
			s[jw + i].b = (float)((inframe[jw + i] & 0xFF0000) >> 16);
			s[jw + i].r = g4 * s[jw + i].r - a1 * s[jw + i - 1].r - a2 * s[jw + i - 2].r;
			s[jw + i].g = g4 * s[jw + i].g - a1 * s[jw + i - 1].g - a2 * s[jw + i - 2].g;
			s[jw + i].b = g4 * s[jw + i].b - a1 * s[jw + i - 1].b - a2 * s[jw + i - 2].b;
		}

		for (i = w - avg; i < w; i++)	//tja  (ze pretv. desno)
		{
			s[jw + i].r = g4 * s[jw + i].r - a1 * s[jw + i - 1].r - a2 * s[jw + i - 2].r;
			s[jw + i].g = g4 * s[jw + i].g - a1 * s[jw + i - 1].g - a2 * s[jw + i - 2].g;
			s[jw + i].b = g4 * s[jw + i].b - a1 * s[jw + i - 1].b - a2 * s[jw + i - 2].b;
		}

		rep1.r = (s[jww - 1].r + s[jww - 2].r) * 0.5 * rs1 + (s[jww - 1].r - s[jww - 2].r) * rd1;
		rep1.g = (s[jww - 1].g + s[jww - 2].g) * 0.5 * rs1 + (s[jww - 1].g - s[jww - 2].g) * rd1;
		rep1.b = (s[jww - 1].b + s[jww - 2].b) * 0.5 * rs1 + (s[jww - 1].b - s[jww - 2].b) * rd1;
		rep2.r = (s[jww - 1].r + s[jww - 2].r) * 0.5 * rs2 + (s[jww - 1].r - s[jww - 2].r) * rd2;
		rep2.g = (s[jww - 1].g + s[jww - 2].g) * 0.5 * rs2 + (s[jww - 1].g - s[jww - 2].g) * rd2;
		rep2.b = (s[jww - 1].b + s[jww - 2].b) * 0.5 * rs2 + (s[jww - 1].b - s[jww - 2].b) * rd2;

		if (ec != 0)
		{
			rep1.r = rep1.r + rc1 * cr;
			rep1.g = rep1.g + rc1 * cg;
			rep1.b = rep1.b + rc1 * cb;
			rep2.r = rep2.r + rc2 * cr;
			rep2.g = rep2.g + rc2 * cg;
			rep2.b = rep2.b + rc2 * cb;
		}

		s[jww - 1].r = s[jww - 1].r - a1 * rep1.r - a2 * rep2.r;
		s[jww - 1].g = s[jww - 1].g - a1 * rep1.g - a2 * rep2.g;
		s[jww - 1].b = s[jww - 1].b - a1 * rep1.b - a2 * rep2.b;
		s[jww - 2].r = s[jww - 2].r - a1 * s[jww - 1].r - a2 * rep1.r;
		s[jww - 2].g = s[jww - 2].g - a1 * s[jww - 1].g - a2 * rep1.g;
		s[jww - 2].b = s[jww - 2].b - a1 * s[jww - 1].b - a2 * rep1.b;

		for (i = w - 3; i >= 0; i--)	//po stolpcih
		{ //nazaj
			s[jw + i].r = s[jw + i].r - a1 * s[jw + i + 1].r - a2 * s[jw + i + 2].r;
			s[jw + i].g = s[jw + i].g - a1 * s[jw + i + 1].g - a2 * s[jw + i + 2].g;
			s[jw + i].b = s[jw + i].b - a1 * s[jw + i + 1].b - a2 * s[jw + i + 2].b;
			//dol
			s[jw + i + 2].r = s[jw + i + 2].r - a1 * s[jw - w + i + 2].r - a2 * s[jw - w - w + i + 2].r;
			s[jw + i + 2].g = s[jw + i + 2].g - a1 * s[jw - w + i + 2].g - a2 * s[jw - w - w + i + 2].g;
			s[jw + i + 2].b = s[jw + i + 2].b - a1 * s[jw - w + i + 2].b - a2 * s[jw - w - w + i + 2].b;
		}

		//se leva stolpca dol
		s[jw + 1].r = s[jw + 1].r - a1 * s[jw - w + 1].r - a2 * s[jw - w - w + 1].r;
		s[jw + 1].g = s[jw + 1].g - a1 * s[jw - w + 1].g - a2 * s[jw - w - w + 1].g;
		s[jw + 1].b = s[jw + 1].b - a1 * s[jw - w + 1].b - a2 * s[jw - w - w + 1].b;
		s[jw].r = s[jw].r - a1 * s[jw - w].r - a2 * s[jw - w - w].r;
		s[jw].g = s[jw].g - a1 * s[jw - w].g - a2 * s[jw - w - w].g;
		s[jw].b = s[jw].b - a1 * s[jw - w].b - a2 * s[jw - w - w].b;

	}	//po vrsticah

//pa se navzgor
//spodnji dve vrstici
	h1w = (h - 1) * w; h2w = (h - 2) * w;
	for (j = 0; j < w; j++)	//po stolpcih
	{
		if (ec != 0)
		{	//edge comp za gor
			cr = 0.0; cg = 0.0; cb = 0.0;
			for (i = h - avg; i < h; i++)
			{
				cr = cr + s[j + w * i].r;
				cg = cg + s[j + w * i].g;
				cb = cb + s[j + w * i].b;
			}
			cr = cr * avgg; cg = cg * avgg; cb = cb * avgg;
		}

		rep1.r = (s[j + h1w].r + s[j + h2w].r) * 0.5 * rs1 + (s[j + h1w].r - s[j + h2w].r) * rd1;
		rep1.g = (s[j + h1w].g + s[j + h2w].g) * 0.5 * rs1 + (s[j + h1w].g - s[j + h2w].g) * rd1;
		rep1.b = (s[j + h1w].b + s[j + h2w].b) * 0.5 * rs1 + (s[j + h1w].b - s[j + h2w].b) * rd1;
		rep2.r = (s[j + h1w].r + s[j + h2w].r) * 0.5 * rs2 + (s[j + h1w].r - s[j + h2w].r) * rd2;
		rep2.g = (s[j + h1w].g + s[j + h2w].g) * 0.5 * rs2 + (s[j + h1w].g - s[j + h2w].g) * rd2;
		rep2.b = (s[j + h1w].b + s[j + h2w].b) * 0.5 * rs2 + (s[j + h1w].b - s[j + h2w].b) * rd2;

		if (ec != 0)
		{	//edge comp
			rep1.r = rep1.r + rc1 * cr;
			rep1.g = rep1.g + rc1 * cg;
			rep1.b = rep1.b + rc1 * cb;
			rep2.r = rep2.r + rc2 * cr;
			rep2.g = rep2.g + rc2 * cg;
			rep2.b = rep2.b + rc2 * cb;
		}

		s[j + h1w].r = s[j + h1w].r - a1 * rep1.r - a2 * rep2.r;
		s[j + h1w].g = s[j + h1w].g - a1 * rep1.g - a2 * rep2.g;
		s[j + h1w].b = s[j + h1w].b - a1 * rep1.b - a2 * rep2.b;
		if (s[j + h1w].r > 255) s[j + h1w].r = 255.0;
		if (s[j + h1w].r < 0.0) s[j + h1w].r = 0.0;
		if (s[j + h1w].g > 255) s[j + h1w].g = 255.0;
		if (s[j + h1w].g < 0.0) s[j + h1w].g = 0.0;
		if (s[j + h1w].b > 255) s[j + h1w].b = 255.0;
		if (s[j + h1w].b < 0.0) s[j + h1w].b = 0.0;
		outframe[j + h1w] = ((uint32_t)s[j + h1w].r & 0xFF) + (((uint32_t)s[j + h1w].g & 0xFF) << 8) + (((uint32_t)s[j + h1w].b & 0xFF) << 16);
		s[j + h2w].r = s[j + h2w].r - a1 * s[j + h1w].r - a2 * rep1.r;
		s[j + h2w].g = s[j + h2w].g - a1 * s[j + h1w].g - a2 * rep1.g;
		s[j + h2w].b = s[j + h2w].b - a1 * s[j + h1w].b - a2 * rep1.b;
		if (s[j + h2w].r > 255) s[j + h2w].r = 255.0;
		if (s[j + h2w].r < 0.0) s[j + h2w].r = 0.0;
		if (s[j + h2w].g > 255) s[j + h2w].g = 255.0;
		if (s[j + h2w].g < 0.0) s[j + h2w].g = 0.0;
		if (s[j + h2w].b > 255) s[j + h2w].b = 255.0;
		if (s[j + h2w].b < 0.0) s[j + h2w].b = 0.0;
		outframe[j + h2w] = ((uint32_t)s[j + h2w].r & 0xFF) + (((uint32_t)s[j + h2w].g & 0xFF) << 8) + (((uint32_t)s[j + h2w].b & 0xFF) << 16);
	}

	//ostale vrstice
	for (i = h - 3; i >= 0; i--)		//gor
	{
		iw = i * w; i1w = iw + w; i2w = i1w + w;
		for (j = 0; j < w; j++)
		{
			s[j + iw].r = s[j + iw].r - a1 * s[j + i1w].r - a2 * s[j + i2w].r;
			s[j + iw].g = s[j + iw].g - a1 * s[j + i1w].g - a2 * s[j + i2w].g;
			s[j + iw].b = s[j + iw].b - a1 * s[j + i1w].b - a2 * s[j + i2w].b;
			if (s[j + iw].r > 255) s[j + iw].r = 255.0;
			if (s[j + iw].r < 0.0) s[j + iw].r = 0.0;
			if (s[j + iw].g > 255) s[j + iw].g = 255.0;
			if (s[j + iw].g < 0.0) s[j + iw].g = 0.0;
			if (s[j + iw].b > 255) s[j + iw].b = 255.0;
			if (s[j + iw].b < 0.0) s[j + iw].b = 0.0;
			outframe[j + iw] = ((uint32_t)s[j + iw].r & 0xFF) + (((uint32_t)s[j + iw].g & 0xFF) << 8) + (((uint32_t)s[j + iw].b & 0xFF) << 16);
		}
	}

}

//-------------------------------------------------------
// 3-tap IIR v stirih smereh
//a only verzija, a0=1.0
//edge efekt na desni kompenzira tako, da racuna 256 vzorcev
//cez rob in in gre potem nazaj
static void fibe3_8(const uint32_t* inframe, uint32_t* outframe, float_rgba s[], int w, int h, float a1, float a2, float a3, int ec)
{
	float cr, cg, cb, g, g4, avg;
	int i, j;
	float_rgba lb[4096];
	int cez;

	g = 1.0 / (1.0 + a1 + a2 + a3); g4 = 1.0 / g / g / g / g;

	avg = EDGEAVG;	//koliko vzorcev za povprecje pri edge comp
	cez = 256;	//koliko vzorcev gre cez na desni

	for (j = 0; j < h; j++)	//po vrsticah
	{
		cr = 0.0; cg = 0.0; cb = 0.0;
		if (ec != 0)
		{	//edge comp (popvprecje prvih)
			for (i = 0; i < avg; i++)
			{
				s[j * w + i].r = (float)(inframe[j * w + i] & 0xFF);
				s[j * w + i].g = (float)((inframe[j * w + i] & 0xFF00) >> 8);
				s[j * w + i].b = (float)((inframe[j * w + i] & 0xFF0000) >> 16);
				cr = cr + s[j * w + i].r;
				cg = cg + s[j * w + i].g;
				cb = cb + s[j * w + i].b;
			}
			cr = g4 * cr / avg; cg = g4 * cg / avg; cb = g4 * cb / avg;
		}
		else
			for (i = 0; i < avg; i++)
			{
				s[j * w + i].r = (float)(inframe[j * w + i] & 0xFF);
				s[j * w + i].g = (float)((inframe[j * w + i] & 0xFF00) >> 8);
				s[j * w + i].b = (float)((inframe[j * w + i] & 0xFF0000) >> 16);
			}
		lb[0].r = g4 * s[j * w].r - (a1 + a2 + a3) * g * cr;
		lb[0].g = g4 * s[j * w].g - (a1 + a2 + a3) * g * cg;
		lb[0].b = g4 * s[j * w].b - (a1 + a2 + a3) * g * cb;
		lb[1].r = g4 * s[j * w + 1].r - a1 * lb[0].r - (a2 + a3) * g * cr;
		lb[1].g = g4 * s[j * w + 1].g - a1 * lb[0].g - (a2 + a3) * g * cg;
		lb[1].b = g4 * s[j * w + 1].b - a1 * lb[0].b - (a2 + a3) * g * cb;
		lb[2].r = g4 * s[j * w + 2].r - a1 * lb[1].r - a2 * lb[0].r - a3 * g * cr;
		lb[2].g = g4 * s[j * w + 2].g - a1 * lb[1].g - a2 * lb[0].g - a3 * g * cg;
		lb[2].b = g4 * s[j * w + 2].b - a1 * lb[1].b - a2 * lb[0].b - a3 * g * cb;

		for (i = 3; i < avg; i++)	//tja  (ze pretvorjeni)
		{
			lb[i].r = g4 * s[j * w + i].r - a1 * lb[i - 1].r - a2 * lb[i - 2].r - a3 * lb[i - 3].r;
			lb[i].g = g4 * s[j * w + i].g - a1 * lb[i - 1].g - a2 * lb[i - 2].g - a3 * lb[i - 3].g;
			lb[i].b = g4 * s[j * w + i].b - a1 * lb[i - 1].b - a2 * lb[i - 2].b - a3 * lb[i - 3].b;
		}

		for (i = avg; i < w; i++)	//tja  (s pretvorbo)
		{
			s[j * w + i].r = (float)(inframe[j * w + i] & 0xFF);
			s[j * w + i].g = (float)((inframe[j * w + i] & 0xFF00) >> 8);
			s[j * w + i].b = (float)((inframe[j * w + i] & 0xFF0000) >> 16);
			lb[i].r = g4 * s[j * w + i].r - a1 * lb[i - 1].r - a2 * lb[i - 2].r - a3 * lb[i - 3].r;
			lb[i].g = g4 * s[j * w + i].g - a1 * lb[i - 1].g - a2 * lb[i - 2].g - a3 * lb[i - 3].g;
			lb[i].b = g4 * s[j * w + i].b - a1 * lb[i - 1].b - a2 * lb[i - 2].b - a3 * lb[i - 3].b;
		}

		cr = 0.0; cg = 0.0; cb = 0.0;
		if (ec != 0)
		{	//edge comp
			for (i = w - avg; i < w; i++)
			{
				cr = cr + s[j * w + i].r;
				cg = cg + s[j * w + i].g;
				cb = cb + s[j * w + i].b;
			}
			cr = g4 * cr / avg; cg = g4 * cg / avg; cb = g4 * cb / avg;
		}

		for (i = w; i < (w + cez); i++)	//naprej cez rob
		{
			lb[i].r = cr - a1 * lb[i - 1].r - a2 * lb[i - 2].r - a3 * lb[i - 3].r;
			lb[i].g = cr - a1 * lb[i - 1].g - a2 * lb[i - 2].g - a3 * lb[i - 3].g;
			lb[i].b = cr - a1 * lb[i - 1].b - a2 * lb[i - 2].b - a3 * lb[i - 3].b;
		}
		//nazaj do roba
		lb[w + cez - 2].r = lb[w + cez - 2].r - a1 * lb[w + cez - 1].r;
		lb[w + cez - 2].g = lb[w + cez - 2].g - a1 * lb[w + cez - 1].g;
		lb[w + cez - 2].b = lb[w + cez - 2].b - a1 * lb[w + cez - 1].b;
		lb[w + cez - 3].r = lb[w + cez - 3].r - a1 * lb[w + cez - 2].r - a2 * lb[w + cez - 1].r;
		lb[w + cez - 3].g = lb[w + cez - 3].g - a1 * lb[w + cez - 2].g - a2 * lb[w + cez - 1].g;
		lb[w + cez - 3].b = lb[w + cez - 3].b - a1 * lb[w + cez - 2].b - a2 * lb[w + cez - 1].b;
		for (i = (w + cez - 4); i >= w; i--)
		{
			lb[i].r = lb[i].r - a1 * lb[i + 1].r - a2 * lb[i + 2].r - a3 * lb[i + 3].r;
			lb[i].g = lb[i].g - a1 * lb[i + 1].g - a2 * lb[i + 2].g - a3 * lb[i + 3].g;
			lb[i].b = lb[i].b - a1 * lb[i + 1].b - a2 * lb[i + 2].b - a3 * lb[i + 3].b;
		}

		s[j * w + w - 1].r = lb[w - 1].r - a1 * lb[w].r - a2 * lb[w + 1].r - a3 * lb[w + 2].r;
		s[j * w + w - 1].g = lb[w - 1].g - a1 * lb[w].g - a2 * lb[w + 1].g - a3 * lb[w + 2].g;
		s[j * w + w - 1].b = lb[w - 1].b - a1 * lb[w].b - a2 * lb[w + 1].b - a3 * lb[w + 2].b;
		s[j * w + w - 2].r = lb[w - 2].r - a1 * s[j * w + w - 1].r - a2 * lb[w].r - a3 * lb[w + 1].r;
		s[j * w + w - 2].g = lb[w - 2].g - a1 * s[j * w + w - 1].g - a2 * lb[w].g - a3 * lb[w + 1].g;
		s[j * w + w - 2].b = lb[w - 2].b - a1 * s[j * w + w - 1].b - a2 * lb[w].b - a3 * lb[w + 1].b;
		s[j * w + w - 3].r = lb[w - 3].r - a1 * s[j * w + w - 2].r - a2 * s[j * w + w - 1].r - a3 * lb[w].r;
		s[j * w + w - 3].g = lb[w - 3].g - a1 * s[j * w + w - 2].g - a2 * s[j * w + w - 1].g - a3 * lb[w].g;
		s[j * w + w - 3].b = lb[w - 3].b - a1 * s[j * w + w - 2].b - a2 * s[j * w + w - 1].b - a3 * lb[w].b;

		for (i = w - 4; i >= 0; i--)		//nazaj
		{
			s[j * w + i].r = lb[i].r - a1 * s[j * w + i + 1].r - a2 * s[j * w + i + 2].r - a3 * s[j * w + i + 3].r;
			s[j * w + i].g = lb[i].g - a1 * s[j * w + i + 1].g - a2 * s[j * w + i + 2].g - a3 * s[j * w + i + 3].g;
			s[j * w + i].b = lb[i].b - a1 * s[j * w + i + 1].b - a2 * s[j * w + i + 2].b - a3 * s[j * w + i + 3].b;
		}
	}	//po vrsticah

	for (j = 0; j < w; j++)	//po stolpcih
	{
		cr = 0.0; cg = 0.0; cb = 0.0;
		if (ec != 0)
		{	//edge comp (popvprecje prvih)
			for (i = 0; i < avg; i++)
			{
				cr = cr + s[j + w * i].r;
				cg = cg + s[j + w * i].g;
				cb = cb + s[j + w * i].b;
			}
			cr = cr / avg; cg = cg / avg; cb = cb / avg;
		}
		lb[0].r = s[j].r - (a1 + a2 + a3) * g * cr;
		lb[0].g = s[j].g - (a1 + a2 + a3) * g * cg;
		lb[0].b = s[j].b - (a1 + a2 + a3) * g * cb;
		lb[1].r = s[j + w].r - a1 * lb[0].r - (a2 + a3) * g * cr;
		lb[1].g = s[j + w].g - a1 * lb[0].g - (a2 + a3) * g * cg;
		lb[1].b = s[j + w].b - a1 * lb[0].b - (a2 + a3) * g * cb;
		lb[2].r = s[j + 2 * w].r - a1 * lb[1].r - a2 * lb[0].r - a3 * g * cr;
		lb[2].g = s[j + 2 * w].g - a1 * lb[1].g - a2 * lb[0].g - a3 * g * cg;
		lb[2].b = s[j + 2 * w].b - a1 * lb[1].b - a2 * lb[0].b - a3 * g * cb;

		for (i = 3; i < h; i++)		//dol
		{
			lb[i].r = s[j + w * i].r - a1 * lb[i - 1].r - a2 * lb[i - 2].r - a3 * lb[i - 3].r;
			lb[i].g = s[j + w * i].g - a1 * lb[i - 1].g - a2 * lb[i - 2].g - a3 * lb[i - 3].g;
			lb[i].b = s[j + w * i].b - a1 * lb[i - 1].b - a2 * lb[i - 2].b - a3 * lb[i - 3].b;
		}

		cr = 0.0; cg = 0.0; cb = 0.0;
		if (ec != 0)
		{	//edge comp
			for (i = h - avg; i < h; i++)
			{
				cr = cr + s[j + w * i].r;
				cg = cg + s[j + w * i].g;
				cb = cb + s[j + w * i].b;
			}
			cr = cr / avg; cg = cg / avg; cb = cb / avg;
		}

		for (i = h; i < (h + cez); i++)	//naprej cez rob
		{
			lb[i].r = cr - a1 * lb[i - 1].r - a2 * lb[i - 2].r - a3 * lb[i - 3].r;
			lb[i].g = cg - a1 * lb[i - 1].g - a2 * lb[i - 2].g - a3 * lb[i - 3].g;
			lb[i].b = cb - a1 * lb[i - 1].b - a2 * lb[i - 2].b - a3 * lb[i - 3].b;
		}
		//nazaj do roba
		lb[h + cez - 2].r = lb[h + cez - 2].r - a1 * lb[h + cez - 1].r;
		lb[h + cez - 2].g = lb[h + cez - 2].g - a1 * lb[h + cez - 1].g;
		lb[h + cez - 2].b = lb[h + cez - 2].b - a1 * lb[h + cez - 1].b;
		lb[h + cez - 3].r = lb[h + cez - 3].r - a1 * lb[h + cez - 2].r - a2 * lb[h + cez - 1].r;
		lb[h + cez - 3].g = lb[h + cez - 3].g - a1 * lb[h + cez - 2].g - a2 * lb[h + cez - 1].g;
		lb[h + cez - 3].b = lb[h + cez - 3].b - a1 * lb[h + cez - 2].b - a2 * lb[h + cez - 1].b;
		for (i = (h + cez - 4); i >= h; i--)
		{
			lb[i].r = lb[i].r - a1 * lb[i + 1].r - a2 * lb[i + 2].r - a3 * lb[i + 3].r;
			lb[i].g = lb[i].g - a1 * lb[i + 1].g - a2 * lb[i + 2].g - a3 * lb[i + 3].g;
			lb[i].b = lb[i].b - a1 * lb[i + 1].b - a2 * lb[i + 2].b - a3 * lb[i + 3].b;
		}

		s[j + (h - 1) * w].r = lb[h - 1].r - a1 * lb[h].r - a2 * lb[h + 1].r - a3 * lb[h + 2].r;
		s[j + (h - 1) * w].g = lb[h - 1].g - a1 * lb[h].g - a2 * lb[h + 1].g - a3 * lb[h + 2].g;
		s[j + (h - 1) * w].b = lb[h - 1].b - a1 * lb[h].b - a2 * lb[h + 1].b - a3 * lb[h + 2].b;
		s[j + (h - 2) * w].r = lb[h - 2].r - a1 * s[j + (h - 1) * w].r - a2 * lb[h].r - a3 * lb[h + 1].r;
		s[j + (h - 2) * w].g = lb[h - 2].g - a1 * s[j + (h - 1) * w].g - a2 * lb[h].g - a3 * lb[h + 1].g;
		s[j + (h - 2) * w].b = lb[h - 2].b - a1 * s[j + (h - 1) * w].b - a2 * lb[h].b - a3 * lb[h + 1].b;
		s[j + (h - 3) * w].r = lb[h - 3].r - a1 * s[j + (h - 2) * w].r - a2 * s[j + (h - 1) * w].r - a3 * lb[h].r;
		s[j + (h - 3) * w].g = lb[h - 3].g - a1 * s[j + (h - 2) * w].g - a2 * s[j + (h - 1) * w].g - a3 * lb[h].g;
		s[j + (h - 3) * w].b = lb[h - 3].b - a1 * s[j + (h - 2) * w].b - a2 * s[j + (h - 1) * w].b - a3 * lb[h].b;

		for (i = h - 4; i >= 0; i--)		//gor
		{
			s[j + w * i].r = lb[i].r - a1 * s[j + w * (i + 1)].r - a2 * s[j + w * (i + 2)].r - a3 * s[j + w * (i + 3)].r;
			s[j + w * i].g = lb[i].g - a1 * s[j + w * (i + 1)].g - a2 * s[j + w * (i + 2)].g - a3 * s[j + w * (i + 3)].g;
			s[j + w * i].b = lb[i].b - a1 * s[j + w * (i + 1)].b - a2 * s[j + w * (i + 2)].b - a3 * s[j + w * (i + 3)].b;
			outframe[j + w * i] = ((uint32_t)s[j + w * i].r & 0xFF) + (((uint32_t)s[j + w * i].g & 0xFF) << 8) + (((uint32_t)s[j + w * i].b & 0xFF) << 16);
		}
	}	//po stolpcih

}




//----------------------------------------
//struktura za instanco efekta
typedef struct
{
//status
int h;
int w;

//parameters
float am;	//amount of blur
int ty;		//type of blur [0..2]
int ec;		//edge compensation (BOOL)

//video buffers
float_rgba *img;

//internal variables
float a1,a2,a3;
float rd1,rd2,rs1,rs2,rc1,rc2;

} inst;

//--------------------------------------------------------
//Aitken-Neville interpolacija iz 4 tock (tretjega reda)
//t = stevilo tock v arrayu
//array xt naj bo v rastocem zaporedju, lahko neekvidistanten
static float AitNev3(int t, float xt[], float yt[], float x)
{
float p[10];
int i,j,m;

if ((x<xt[0])||(x>xt[t-1]))
	{
//	printf("\n\n x=%f je izven mej tabele!",x);
#ifndef _MSC_VER
	return 1.0 / 0.0;
#else
	return 1.0 / 0.0000000000000001;
#endif
	}

//poisce, katere tocke bo uporabil
m=0; while (x>xt[m++]);
m=m-4/2-1; if (m<0) m=0; if ((m+4)>(t-1)) m=t-4;

for (i=0;i<4;i++)
	p[i]=yt[i+m];

for (j=1;j<4;j++)
	for (i=(4-1);i>=j;i--)
		{
		p[i]=p[i]+(x-xt[i+m])/(xt[i+m]-xt[i-j+m])*(p[i]-p[i-1]);
		}
return p[4-1];
}


//-----------------------------------------------------
//stretch [0...1] to parameter range [min...max] linear
static float map_value_forward(double v, float min, float max)
{
return min+(max-min)*v;
}

//-----------------------------------------------------
//collapse from parameter range [min...max] to [0...1] linear
static double map_value_backward(float v, float min, float max)
{
return (v-min)/(max-min);
}

//-----------------------------------------------------
//stretch [0...1] to parameter range [min...max] logarithmic
//min and max must be positive!
static float map_value_forward_log(double v, float min, float max)
{
	float sr,k;

	sr=sqrtf(min*max);
	k=2.0*log(max/sr);
	return sr*expf(k*(v-0.5));
}

//-----------------------------------------------------
//collapse from parameter range [min...max] to [0...1] logarithmic
//min and max must be positive!
static double map_value_backward_log(float v, float min, float max)
{
	float sr,k;

	sr=sqrtf(min*max);
	k=2.0*log(max/sr);
	return logf(v/sr)/k+0.5;
}

//***********************************************
// OBVEZNE FREI0R FUNKCIJE

//-----------------------------------------------
static int f0r_init()
{
return 1;
}

//------------------------------------------------
static void f0r_deinit()
{
}

//-----------------------------------------------
static void f0r_get_plugin_info(f0r_plugin_info_t* info)
{

}

//--------------------------------------------------
static void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
switch(param_index)
	{
	case 0:
		info->name = "Amount";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Amount of blur";
		break;
	case 1:
		info->name = "Type";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Blur type";
		break;
	case 2:
		info->name = "Edge";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Edge compensation";
		break;
	}
}

//----------------------------------------------
static f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
inst *in;

in=calloc(1,sizeof(inst));
in->w=width;
in->h=height;

in->img=calloc(width*height*4,sizeof(float));

in->am=map_value_forward_log(0.2, 0.5, 100.0);
in->a1=-0.796093; in->a2=0.186308;
in->ty=1;
in->ec=1;

return (f0r_instance_t)in;
}

//---------------------------------------------------
static void f0r_destruct(f0r_instance_t instance)
{
inst *in;

in=(inst*)instance;

free(in->img);

free(instance);
}

//-----------------------------------------------------
static void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
inst *p;
double tmpf;
int chg,tmpi;
float a0,b0,b1,b2,f,q,s;

float am1[]={0.499999,0.7,1.0,1.5,2.0,3.0,4.0,5.0,7.0,10.0,
	15.0,20.0,30.0,40.0,50.0,70.0,100.0,150.0,200.00001};
//float iir2f[]={0.448,0.4,0.31,0.25,0.21,0.15,0.1,0.075,
//	0.055,0.039,0.026,0.02,0.013,0.01,0.008,0.006,
//	0.0042,0.0029,0.00205};
//float iir2q[]={0.53,0.53,0.54,0.54,0.54,0.55,0.6,0.7,0.7,
//	0.7,0.7,0.7,0.7,0.7,0.7,0.7,0.7,0.7,0.7};
float iir2q[]={0.53,0.53,0.54,0.54,0.54,0.55,0.6,0.6,0.6,
	0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6};
//float iir1a1[]={0.167,0.3,0.5,0.65,0.7,0.8,0.88,0.92,0.95,
//	0.96,0.97,0.98,0.985,0.988,0.99,0.992,0.993,0.9955,
//	0.997};

float iir1a1[]={0.138,0.24,0.34,0.45,0.55, 0.65,0.728,0.775,0.834,0.88,
0.92,0.937,0.958,0.968,0.9745,
0.98,0.986,0.991,0.9931};		//po sigmi

float iir2f[]={0.475,0.39,0.325,0.26,0.21,
0.155,0.112,0.0905,0.065,0.0458,
0.031,0.0234,0.01575,0.0118,0.0093,
0.00725,0.00505,0.0033,0.0025};		//po sigmi

float iir3si[]={0.5,0.7,1.0,1.5,2.0,
3.0,4.0,5.0,7.0,10.0,
15.0,20.0,30.0,40.0,50.0,
70.0,100.0,150.0,186.5};


p=(inst*)instance;

chg=0;
switch(param_index)
	{
	case 0:
//		tmpf=map_value_forward(*((double*)parm), 0.5, 100.0); 
		tmpf=map_value_forward_log(*((double*)parm), 0.5, 100.0); 
		if (tmpf!=p->am) chg=1;
		p->am=tmpf;
		break;
	case 1:
		tmpf=*((double*)parm);
		if (tmpf>=1.0)
			tmpi=(int)tmpf;
		else
			tmpi = map_value_forward(tmpf, 0.0, 2.9999);
		if ((tmpi<0)||(tmpi>2.0)) break;
		if (p->ty != tmpi) chg=1;
		p->ty = tmpi;
		break;
	case 2:
                p->ec=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
		break;
	}

if (chg==0) return;

switch(p->ty)
	{
	case 0:		//FIBE-1
	  p->a1=AitNev3(19, am1, iir1a1, p->am);
//printf("Set parm FIBE-1 a1=%f (p->am=%f)\n",p->a1,p->am);
	  break;
	case 1:		//FIBE-2
	  f=AitNev3(19, am1, iir2f, p->am);
	  q=AitNev3(19, am1, iir2q, p->am);
	  calcab_lp1(f, q, &a0, &p->a1, &p->a2, &b0, &b1, &b2);
	  p->a1=p->a1/a0; p->a2=p->a2/a0;
	  rep(-0.5, 0.5, 0.0, &p->rd1, &p->rd2, 256, p->a1, p->a2);
	  rep(1.0, 1.0, 0.0, &p->rs1, &p->rs2, 256, p->a1, p->a2);
	  rep(0.0, 0.0, 1.0, &p->rc1, &p->rc2, 256, p->a1, p->a2);
//printf("Set parm FIBE-2 a1=%f a2=%f\n",p->a1,p->a2);
	  break;
	case 2:		//FIBE-3
	  s=AitNev3(19, am1, iir3si, p->am);
	  young_vliet(s, &a0, &p->a1, &p->a2, &p->a3);
	  p->a1=-p->a1/a0;
	  p->a2=-p->a2/a0;
	  p->a3=-p->a3/a0;
//printf("Set parm FIBE-3 a1=%f a2=%f a3=%f\n",p->a1,p->a2,p->a3);
	  break;
	}
}

//--------------------------------------------------
static void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
inst *p;

p=(inst*)instance;

switch(param_index)
	{
	case 0:
//		*((double*)param)=map_value_backward(p->am, 0.5, 100.0);
		*((double*)param)=map_value_backward_log(p->am, 0.5, 100.0);
		break;
	case 1:
		*((double*)param)=map_value_backward(p->ty, 0.0, 2.9999);
		break;
	case 2:
                *((double*)param)=map_value_backward(p->ec, 0.0, 1.0);//BOOL!!
		break;
	}
}

//-------------------------------------------------
static void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;
int i;

assert(instance);
in=(inst*)instance;

  if (in->am==0.0)	//zero blur, just copy and return
    {
    for (i=0;i<in->w*in->h;i++) outframe[i]=inframe[i];
    return;
    }
  //do the blur
  switch(in->ty)
    {
    case 0:
      fibe1o_8(inframe, outframe, in->img, in->w, in->h, in->a1, in->ec);
      break;
    case 1:
      fibe2o_8(inframe, outframe, in->img, in->w, in->h, in->a1, in->a2, in->rd1, in->rd2, in->rs1, in->rs2, in->rc1, in->rc2, in->ec);
      break;
    case 2:
      fibe3_8(inframe, outframe, in->img, in->w, in->h, in->a1, in->a2, in->a3, in->ec);
      break;
    }
  //copy alpha
  for (i=0;i<in->w*in->h;i++)
    {
    outframe[i]=(outframe[i]&0x00FFFFFF) | (inframe[i]&0xFF000000);
    }


}


//==================================================================================================
//export
filter_dest(IIRblur,
	F0R_PLUGIN_TYPE_FILTER,
	F0R_COLOR_MODEL_RGBA8888,
	0,
	1,
	3,
	f0r_update,
	NULL);

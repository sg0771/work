
/***************************************************************************************************
This set of FastResizer functions resizes a source window to the size of the destination window. Nearest point 
method is used for fast work.YV12,YUY2 RGB 24 and 
RGB32  formats are supported. The src and dst  buffers must be of adequate sizes. The buffer pointers
point to start of image (left top). The image can also be flipped
***************************************************************************************************/
#ifndef FastResizer_V_C_MOHAN
#define FastResizer_V_C_MOHAN
#include "FastResizer.h"

bool HFastResizerRGB32(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
					 int spitch, int dpitch,  bool horflip)
{
	if(!(srcht==dstht))
		return false; // these must be equal as only width is being resized
	if((dstwd & 1)==1 || (srcwd & 1)==1)		// must be even numbers
		return false;
	if (dstwd<2 || srcwd<2)
		return true; // no resizing or flip is necessasary
	
					//calculate magnification/shrink factor
	double hormag = (double)(srcwd-1)/(double)(dstwd-1); // srcwd-1 intervals to become dstwd-1 intervals
	int n;
	
	if(horflip)
	{
		
		for(int w=0; w<dstwd; w++) // we will take horizontal symmetry advantage Process byte by byte
										
		{
						// pixel first byte position
			n= (int)(w*hormag+0.5);	// since there are 4 bytes per pixel all 4 bytes have same factor
								// calculate source offset for dst offset of w
								// correct pointer position for RGBA bytes
								// process every line
			for (int h=0;h< srcht; h++)
			{
				*((int*)(dstptr+h*dpitch+4*w))=*((int *)(srcptr+h*spitch+4*srcwd-4-4*n));
						

			}
					
		}
		
			return true;
	}

		
			//The leftmost pixel and Rightmost pixels will be left and right most pixels of output
			// this could not be included in main loop due to access violations
		
	for(int w=0; w<dstwd; w++) // we will take horizontal symmetry advantage Process byte by byte
										
		{
						// pixel first byte position
			n= (int)(w*hormag+0.5);	// since there are 4 bytes per pixel all 4 bytes have same factor
								// calculate source offset for dst offset of w
								// correct pointer position for RGBA bytes
								// process every line
			for (int h=0;h< srcht; h++)
			{
				*((int*)(dstptr+h*dpitch+4*w))=*((int *)(srcptr+h*spitch+4*n));
						

			}
					
		}
	return true;

}
	

bool VFastResizerRGB32(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
				   int spitch, int dpitch,  bool vflip)
{
	if(!(srcwd==dstwd))
		return false; // these must be equal as only height is being resized

	if((dstht & 1)==1 || (srcht & 1)==1) // must be even numbers
		return false;
	if (dstht<2 ||srcht<2)
		return true;	// not possible to resize or flip
	//calculate magnification/shrinkage factor
	double vermag = (double)(srcht-1)/(double)(dstht-1);
	int n;
	if(vflip)		
	{

		for (int h=0;h< dstht; h++) 
		{
			n=(int)(h*vermag+0.5);
			for(int w=0; w<dstwd; w++)	// 4 bytes per pixel format . we are dealing 4 bytes at a time
			{
				// sample at 3.4 = sample at 3 
				*((int*)(dstptr+4*w+h*dpitch))=*((int*)(srcptr+(srcht-1-n)*spitch+4*w));
			}
		}
		return true;
	}

		// no flip
		for (int h=0;h< dstht; h++) 
		{
			n=(int)(h*vermag+0.5);
			for(int w=0; w<dstwd; w++)	// 4 bytes per pixel format . we are dealing 4 bytes at a time
			{
				// sample at 3.4 = sample at 3 
				*((int*)(dstptr+4*w+h*dpitch))=*((int*)(srcptr+n*spitch+4*w));
			}
		}
	
				 
	return true;
}

bool HFastResizerRGB24(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
					 int spitch, int dpitch,  bool horflip)
{
	if(!(srcht==dstht))
		return false; // these must be equal as only width is being resized
	if((dstwd & 1)==1 || (srcwd & 1)==1)		// must be even numbers
		return false;
	if (dstwd<2 || srcwd<2)
		return true; // no resizing or flip is necessasary
	
					//calculate magnification/shrink factor
	double hormag = (double)(srcwd-1)/(double)(dstwd-1); // srcwd-1 intervals to become dstwd-1 intervals
	
		if(horflip)
		{
		
			for(int w=0; w<dstwd; w++) // we will  Process byte by byte
										
			{
							// pixel first byte position
				int n=(int)(w*hormag+0.5);	// all 3 bytes of a pixel have same factor
							// calculate source offset for dst offset of w
							// correct pointer position for RGB bytes
							// process every line

				for (int h=0;h< srcht; h++)
				{	// sample at 3.4 = sample at 3
					for(int k=0;k<3;k++)
						*(dstptr+h*dpitch+3*w+k)=*(srcptr+h*spitch+3*(srcwd-1-n)+k);
						
				}
					
			}
			return true;
		}


// no horizontal flip		
			for(int w=0; w<dstwd; w++) // we will  Process byte by byte
										
			{
							// pixel first byte position
				int n=(int)(w*hormag+0.5);	// all 3 bytes of a pixel have same factor
							// calculate source offset for dst offset of w
							// correct pointer position for RGB bytes
							// process every line

				for (int h=0;h< srcht; h++)
				{	// sample at 3.4 = sample at 3
					for(int k=0;k<3;k++)
						*(dstptr+h*dpitch+3*w+k)=*(srcptr+h*spitch+3*n+k);
						
				}
					
			}
	return true;
}
	

bool VFastResizerRGB24(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
				   int spitch, int dpitch,  bool vflip)
{
	if(!(srcwd==dstwd))
		return false; // these must be equal as only height is being resized

	if((dstht & 1)==1 || (srcht & 1)==1) // must be even numbers
		return false;
	if (dstht<2 ||srcht<2)
		return true;	// not possible to resize or flip
	//calculate magnification/shrinkage factor
	double vermag = (double)(srcht-1)/(double)(dstht-1);
	if(vflip)		
	{
		for (int h=0;h< dstht; h++) 
		{
			int n=h*vermag;

			for(int w=0; w<3*dstwd; w++)	// 3 bytes per pixel format . we are dealing byte by byte
			{
				// sample at 3.4 = s3 
				*(dstptr+w+h*dpitch)=*(srcptr+w+(srcht-1-n)*spitch);
			}
		}
		return true;
	}

		// no flip
		for (int h=0;h< dstht; h++) 
		{
			int n=h*vermag;

			for(int w=0; w<3*dstwd; w++)	// 3 bytes per pixel format . we are dealing byte by byte
			{
				// sample at 3.4 = s3 
				*(dstptr+w+h*dpitch)=*(srcptr+w+n*spitch);
			}
		}
	return true;
}
/*************************************************************************************************************
//  YUY2 format has two pixels in a word or 4 bytes, Y-U-Y-V



************************************************************************************************************/

bool HFastResizerYUY2(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
					 int spitch, int dpitch,  bool horflip)
{
	if(!(srcht==dstht))
		return false; // these must be equal as only width is being resized
	if((dstwd & 1)==1 || (srcwd & 1)==1)		// must be even numbers
		return false;
	if (dstwd<2 || srcwd<2)
		return true; // no resizing or flip is necessasary
	
					//calculate magnification/shrink factor
	double hormag = (double)(srcwd-1)/(double)(dstwd-1); // srcwd-1 intervals to become dstwd-1 intervals
	int n;
	
	if(horflip)
	{
		
		for(int w=0; w<dstwd; w+=2) 
										
		{
						// pixel first byte position
			n= (int)(w*hormag+0.5)& 0xfffffffe;	// since there are 4 bytes per 2 pixel force all 4 bytes to have same factor
								// calculate source offset for dst offset of w
								// correct pointer position for RGBA bytes
								// process every line
			for (int h=0;h< srcht; h++)
			{
				*((int*)(dstptr+h*dpitch+2*w))=*((int *)(srcptr+h*spitch+2*(srcwd-2-n)));
						

			}
					
		}
		
			return true;
	}

		
			//The leftmost pixel and Rightmost pixels will be left and right most pixels of output
			// this could not be included in main loop due to access violations
		
	for(int w=0; w<dstwd; w+=2) // we will take horizontal symmetry advantage Process byte by byte
										
		{
						// pixel first byte position
			n= ((int)(w*hormag+0.5)) & 0xFFFFFFFE;	// since there are 4 bytes per pixel all 4 bytes have same factor
								// calculate source offset for dst offset of w
								
								// process every line
			for (int h=0;h< srcht; h++)
			{
				*((int*)(dstptr+h*dpitch+2*w))=*((int *)(srcptr+h*spitch+2*n));
						

			}
					
		}
	return true;

}
	

bool VFastResizerYUY2(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
				   int spitch, int dpitch,  bool vflip)
{
	if(!(srcwd==dstwd))
		return false; // these must be equal as only height is being resized

	if((dstht & 1)==1 || (srcht & 1)==1) // must be even numbers
		return false;
	if (dstht<2 ||srcht<2)
		return true;	// not possible to resize or flip
	//calculate magnification/shrinkage factor
	double vermag = (double)(srcht-1)/(double)(dstht-1);
	int n;
	if(vflip)		
	{

		for (int h=0;h< dstht; h++) 
		{
			n=(int)(h*vermag+0.5);
			for(int w=0; w<dstwd; w+=2)	// 4 bytes per pixel format . we are dealing 4 bytes at a time
			{
				// sample at 3.4 = sample at 3 
				*((int*)(dstptr+2*w+h*dpitch))=*((int*)(srcptr+(srcht-1-n)*spitch+2*w));
			}
		}
		return true;
	}

		// no flip
		for (int h=0;h< dstht; h++) 
		{
			n=(int)(h*vermag+0.5);
			for(int w=0; w<dstwd; w++)	// 4 bytes per pixel format . we are dealing 4 bytes at a time
			{
				// sample at 3.4 = sample at 3 
				*((int*)(dstptr+2*w+h*dpitch))=*((int*)(srcptr+n*spitch+2*w));
			}
		}
	
				 
	return true;
}

/************************************************************************************************************/

bool HFastResizerYV12(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
					 int spitch, int dpitch,  bool horflip)
{
	if(!(srcht==dstht))
		return false; // these must be equal as only width is being resized
	if((dstwd & 1)==1 || (srcwd & 1)==1)		// must be even numbers
		return false;
	if (dstwd<2 || srcwd<2)
		return true; // no resizing or flip is necessasary
	
					//calculate magnification/shrink factor
	double hormag = (double)(srcwd-1)/(double)(dstwd-1); // srcwd-1 intervals to become dstwd-1 intervals
	int n;
	
	if(horflip)
	{
		
		for(int w=1; w<dstwd; w++) 
										
		{
						// pixel first byte position
			n= (int)(w*hormag+0.5);	
								// calculate source offset for dst offset of w
			for (int h=0;h< srcht; h++)
			{
				*(dstptr+h*dpitch+w)=*(srcptr+h*spitch+(srcwd-n));
						

			}
					
		}
		
			return true;
	}

		
			//The leftmost pixel and Rightmost pixels will be left and right most pixels of output
			// this could not be included in main loop due to access violations
		
	for(int w=0; w<dstwd; w++) // we will take horizontal symmetry advantage Process byte by byte
										
		{
						// pixel first byte position
			n= (int)(w*hormag+0.5);	
								// calculate source offset for dst offset of w
			for (int h=0;h< srcht; h++)
			{
				*(dstptr+h*dpitch+w)=*(srcptr+h*spitch+n);
						

			}
					
		}
	return true;

}
	

bool VFastResizerYV12(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
				   int spitch, int dpitch,  bool vflip)
{
	if(!(srcwd==dstwd))
		return false; // these must be equal as only height is being resized

	if((dstht & 1)==1 || (srcht & 1)==1) // must be even numbers
		return false;
	if (dstht<2 ||srcht<2)
		return true;	// not possible to resize or flip
	//calculate magnification/shrinkage factor
	double vermag = (double)(srcht-1)/(double)(dstht-1);
	int n;
	if(vflip)		
	{

		for (int h=1;h< dstht; h++) 
		{
			n=(int)(h*vermag+0.5);
			for(int w=0; w<dstwd; w++)	
			{
				// sample at 3.4 = sample at 3 
				*(dstptr+w+h*dpitch)=*(srcptr+(srcht-n)*spitch+w);
			}
		}
		return true;
	}

		// no flip
		for (int h=0;h< dstht; h++) 
		{
			n=(int)(h*vermag+0.5);
			for(int w=0; w<dstwd; w++)
			{
				// sample at 3.4 = sample at 3 
				*(dstptr+w+h*dpitch)=*(srcptr+n*spitch+w);
			}
		}
	
				 
	return true;
}





//--------------------------------------------------------------------------------------------------------------
bool HFastResizerPlanar(const unsigned char* srcptr, unsigned char* dstptr, int srcht, int srcwd, int dstht, int dstwd,
	int spitch, int dpitch, bool horflip)
{
	if (!(srcht == dstht))
		return false; // these must be equal as only width is being resized
	dstwd &= 0xfffffffe;
	srcwd &= 0xfffffffe;

	if (dstwd < 2 || srcwd < 2)
		return true; // no resizing or flip is necessasary

	//calculate magnification/shrink factor
	double hormag = (double)(srcwd) / (double)(dstwd); // srcwd-1 intervals to become dstwd-1 intervals
	int n;

	if (horflip)
	{

		for (int w = 1; w < dstwd; w++)

		{
			// pixel first byte position
			n = (int)(w * hormag + 0.5);
			// calculate source offset for dst offset of w
			for (int h = 0; h < srcht; h++)
			{
				*(dstptr + h * dpitch + w) = *(srcptr + h * spitch + (srcwd - n));


			}

		}

		return true;
	}


	//The leftmost pixel and Rightmost pixels will be left and right most pixels of output
	// this could not be included in main loop due to access violations

	for (int w = 0; w < dstwd; w++) // we will take horizontal symmetry advantage Process byte by byte

	{
		// pixel first byte position
		n = (int)(w * hormag);
		// calculate source offset for dst offset of w
		for (int h = 0; h < srcht; h++)
		{
			*(dstptr + h * dpitch + w) = *(srcptr + h * spitch + n);


		}

	}
	return true;

}


bool VFastResizerPlanar(const unsigned char* srcptr, unsigned char* dstptr, int srcht, int srcwd, int dstht, int dstwd,
	int spitch, int dpitch, bool vflip)
{
	if (!(srcwd == dstwd))
		return false; // these must be equal as only height is being resized

	dstht &= 0xfffffffe;
	srcht &= 0xfffffffe;

	if (dstht < 2 || srcht < 2)
		return true;	// not possible to resize or flip
	//calculate magnification/shrinkage factor
	double vermag = (double)(srcht) / (double)(dstht);
	int n;
	if (vflip)
	{

		for (int h = 1; h < dstht; h++)
		{
			n = (int)(h * vermag + 0.5);
			for (int w = 0; w < dstwd; w++)
			{
				// sample at 3.4 = sample at 3 
				*(dstptr + w + h * dpitch) = *(srcptr + (srcht - n) * spitch + w);
			}
		}
		return true;
	}

	// no flip
	for (int h = 0; h < dstht; h++)
	{
		n = (int)(h * vermag);
		for (int w = 0; w < dstwd; w++)
		{
			// sample at 3.4 = sample at 3 
			*(dstptr + w + h * dpitch) = *(srcptr + n * spitch + w);
		}
	}


	return true;
}

#endif

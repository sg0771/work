

#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <stdio.h>
#include <algorithm>
#include "SoundTouch.h"

using namespace soundtouch;

/// test if two floating point numbers are equal
#define TEST_FLOAT_EQUAL(a, b)  (fabs(a - b) < 1e-10)


/// Print library version string for autoconf
extern "C" void soundtouch_ac_test()
{
    printf("SoundTouch Version: %s\n", SOUNDTOUCH_VERSION);
}


SoundTouch::SoundTouch()
{
    // Initialize rate transposer and tempo changer instances

    pRateTransposer = new RateTransposer();
    pTDStretch = TDStretch::newInstance();

    setOutPipe(pTDStretch);

    rate = tempo = 0;

    virtualPitch =
        virtualRate =
        virtualTempo = 1.0;

    calcEffectiveRateAndTempo();

    samplesExpectedOut = 0;
    samplesOutput = 0;

    channels = 0;
    bSrateSet = false;
}


SoundTouch::~SoundTouch()
{
    delete pRateTransposer;
    delete pTDStretch;
}


/// Get SoundTouch library version string
const char* SoundTouch::getVersionString()
{
    static const char* _version = SOUNDTOUCH_VERSION;

    return _version;
}


/// Get SoundTouch library version Id
uint SoundTouch::getVersionId()
{
    return SOUNDTOUCH_VERSION_ID;
}


// Sets the number of channels, 1 = mono, 2 = stereo
void SoundTouch::setChannels(uint numChannels)
{
    if (!verifyNumberOfChannels(numChannels)) return;

    channels = numChannels;
    pRateTransposer->setChannels((int)numChannels);
    pTDStretch->setChannels((int)numChannels);
}


// Sets new rate control value. Normal rate = 1.0, smaller values
// represent slower rate, larger faster rates.
void SoundTouch::setRate(double newRate)
{
    virtualRate = newRate;
    calcEffectiveRateAndTempo();
}


// Sets new rate control value as a difference in percents compared
// to the original rate (-50 .. +100 %)
void SoundTouch::setRateChange(double newRate)
{
    virtualRate = 1.0 + 0.01 * newRate;
    calcEffectiveRateAndTempo();
}


// Sets new tempo control value. Normal tempo = 1.0, smaller values
// represent slower tempo, larger faster tempo.
void SoundTouch::setTempo(double newTempo)
{
    virtualTempo = newTempo;
    calcEffectiveRateAndTempo();
}


// Sets new tempo control value as a difference in percents compared
// to the original tempo (-50 .. +100 %)
void SoundTouch::setTempoChange(double newTempo)
{
    virtualTempo = 1.0 + 0.01 * newTempo;
    calcEffectiveRateAndTempo();
}


// Sets new pitch control value. Original pitch = 1.0, smaller values
// represent lower pitches, larger values higher pitch.
void SoundTouch::setPitch(double newPitch)
{
    virtualPitch = newPitch;
    calcEffectiveRateAndTempo();
}


// Sets pitch change in octaves compared to the original pitch
// (-1.00 .. +1.00)
void SoundTouch::setPitchOctaves(double newPitch)
{
    virtualPitch = exp(0.69314718056 * newPitch);
    calcEffectiveRateAndTempo();
}


// Sets pitch change in semi-tones compared to the original pitch
// (-12 .. +12)
void SoundTouch::setPitchSemiTones(int newPitch)
{
    setPitchOctaves((double)newPitch / 12.0);
}


void SoundTouch::setPitchSemiTones(double newPitch)
{
    setPitchOctaves(newPitch / 12.0);
}


// Calculates 'effective' rate and tempo values from the
// nominal control values.
void SoundTouch::calcEffectiveRateAndTempo()
{
    double oldTempo = tempo;
    double oldRate = rate;

    tempo = virtualTempo / virtualPitch;
    rate = virtualPitch * virtualRate;

    if (!TEST_FLOAT_EQUAL(rate, oldRate)) pRateTransposer->setRate(rate);
    if (!TEST_FLOAT_EQUAL(tempo, oldTempo)) pTDStretch->setTempo(tempo);

#ifndef SOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER
    if (rate <= 1.0f)
    {
        if (output != pTDStretch)
        {
            FIFOSamplePipe* tempoOut;

            assert(output == pRateTransposer);
            // move samples in the current output buffer to the output of pTDStretch
            tempoOut = pTDStretch->getOutput();
            tempoOut->moveSamples(*output);
            // move samples in pitch transposer's store buffer to tempo changer's input
            // deprecated : pTDStretch->moveSamples(*pRateTransposer->getStore());

            output = pTDStretch;
        }
    }
    else
#endif
    {
        if (output != pRateTransposer)
        {
            FIFOSamplePipe* transOut;

            assert(output == pTDStretch);
            // move samples in the current output buffer to the output of pRateTransposer
            transOut = pRateTransposer->getOutput();
            transOut->moveSamples(*output);
            // move samples in tempo changer's input to pitch transposer's input
            pRateTransposer->moveSamples(*pTDStretch->getInput());

            output = pRateTransposer;
        }
    }
}


// Sets sample rate.
void SoundTouch::setSampleRate(uint srate)
{
    // set sample rate, leave other tempo changer parameters as they are.
    pTDStretch->setParameters((int)srate);
    bSrateSet = true;
    m_temp.resize(srate);
}


// Adds 'numSamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void SoundTouch::putSamples(const SAMPLETYPE* samples, uint nSamples)
{
    if (bSrateSet == false)
    {
        ST_THROW_RT_ERROR("SoundTouch : Sample rate not defined");
    }
    else if (channels == 0)
    {
        ST_THROW_RT_ERROR("SoundTouch : Number of channels not defined");
    }

    // accumulate how many samples are expected out from processing, given the current 
    // processing setting
    samplesExpectedOut += (double)nSamples / ((double)rate * (double)tempo);

#ifndef SOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER
    if (rate <= 1.0f)
    {
        // transpose the rate down, output the transposed sound to tempo changer buffer
        assert(output == pTDStretch);
        pRateTransposer->putSamples(samples, nSamples);
        pTDStretch->moveSamples(*pRateTransposer);
    }
    else
#endif
    {
        // evaluate the tempo changer, then transpose the rate up, 
        assert(output == pRateTransposer);
        pTDStretch->putSamples(samples, nSamples);
        pRateTransposer->moveSamples(*pTDStretch);
    }
}


// Flushes the last samples from the processing pipeline to the output.
// Clears also the internal processing buffers.
//
// Note: This function is meant for extracting the last samples of a sound
// stream. This function may introduce additional blank samples in the end
// of the sound stream, and thus it's not recommended to call this function
// in the middle of a sound stream.
void SoundTouch::flush()
{
    int i;
    int numStillExpected;
    SAMPLETYPE* buff = new SAMPLETYPE[128 * channels];

    // how many samples are still expected to output
    numStillExpected = (int)((long)(samplesExpectedOut + 0.5) - samplesOutput);
    if (numStillExpected < 0) numStillExpected = 0;

    memset(buff, 0, 128 * channels * sizeof(SAMPLETYPE));
    // "Push" the last active samples out from the processing pipeline by
    // feeding blank samples into the processing pipeline until new, 
    // processed samples appear in the output (not however, more than 
    // 24ksamples in any case)
    for (i = 0; (numStillExpected > (int)numSamples()) && (i < 200); i++)
    {
        putSamples(buff, 128);
    }

    adjustAmountOfSamples(numStillExpected);

    delete[] buff;

    // Clear input buffers
    pTDStretch->clearInput();
    // yet leave the output intouched as that's where the
    // flushed samples are!
}


// Changes a setting controlling the processing system behaviour. See the
// 'SETTING_...' defines for available setting ID's.
bool SoundTouch::setSetting(int settingId, int value)
{
    int sampleRate, sequenceMs, seekWindowMs, overlapMs;

    // read current tdstretch routine parameters
    pTDStretch->getParameters(&sampleRate, &sequenceMs, &seekWindowMs, &overlapMs);

    switch (settingId)
    {
    case SETTING_USE_AA_FILTER:
        // enables / disabless anti-alias filter
        pRateTransposer->enableAAFilter((value != 0) ? true : false);
        return true;

    case SETTING_AA_FILTER_LENGTH:
        // sets anti-alias filter length
        pRateTransposer->getAAFilter()->setLength(value);
        return true;

    case SETTING_USE_QUICKSEEK:
        // enables / disables tempo routine quick seeking algorithm
        pTDStretch->enableQuickSeek((value != 0) ? true : false);
        return true;

    case SETTING_SEQUENCE_MS:
        // change time-stretch sequence duration parameter
        pTDStretch->setParameters(sampleRate, value, seekWindowMs, overlapMs);
        return true;

    case SETTING_SEEKWINDOW_MS:
        // change time-stretch seek window length parameter
        pTDStretch->setParameters(sampleRate, sequenceMs, value, overlapMs);
        return true;

    case SETTING_OVERLAP_MS:
        // change time-stretch overlap length parameter
        pTDStretch->setParameters(sampleRate, sequenceMs, seekWindowMs, value);
        return true;

    default:
        return false;
    }
}


// Reads a setting controlling the processing system behaviour. See the
// 'SETTING_...' defines for available setting ID's.
//
// Returns the setting value.
int SoundTouch::getSetting(int settingId) const
{
    int temp;

    switch (settingId)
    {
    case SETTING_USE_AA_FILTER:
        return (uint)pRateTransposer->isAAFilterEnabled();

    case SETTING_AA_FILTER_LENGTH:
        return pRateTransposer->getAAFilter()->getLength();

    case SETTING_USE_QUICKSEEK:
        return (uint)pTDStretch->isQuickSeekEnabled();

    case SETTING_SEQUENCE_MS:
        pTDStretch->getParameters(NULL, &temp, NULL, NULL);
        return temp;

    case SETTING_SEEKWINDOW_MS:
        pTDStretch->getParameters(NULL, NULL, &temp, NULL);
        return temp;

    case SETTING_OVERLAP_MS:
        pTDStretch->getParameters(NULL, NULL, NULL, &temp);
        return temp;

    case SETTING_NOMINAL_INPUT_SEQUENCE:
    {
        int size = pTDStretch->getInputSampleReq();

#ifndef SOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER
        if (rate <= 1.0)
        {
            // transposing done before timestretch, which impacts latency
            return (int)(size * rate + 0.5);
        }
#endif
        return size;
    }

    case SETTING_NOMINAL_OUTPUT_SEQUENCE:
    {
        int size = pTDStretch->getOutputBatchSize();

        if (rate > 1.0)
        {
            // transposing done after timestretch, which impacts latency
            return (int)(size / rate + 0.5);
        }
        return size;
    }

    case SETTING_INITIAL_LATENCY:
    {
        double latency = pTDStretch->getLatency();
        int latency_tr = pRateTransposer->getLatency();

#ifndef SOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER
        if (rate <= 1.0)
        {
            // transposing done before timestretch, which impacts latency
            latency = (latency + latency_tr) * rate;
        }
        else
#endif
        {
            latency += (double)latency_tr / rate;
        }

        return (int)(latency + 0.5);
    }

    default:
        return 0;
    }
}


// Clears all the samples in the object's output and internal processing
// buffers.
void SoundTouch::clear()
{
    samplesExpectedOut = 0;
    samplesOutput = 0;
    pRateTransposer->clear();
    pTDStretch->clear();
}


/// Returns number of samples currently unprocessed.
uint SoundTouch::numUnprocessedSamples() const
{
    FIFOSamplePipe* psp;
    if (pTDStretch)
    {
        psp = pTDStretch->getInput();
        if (psp)
        {
            return psp->numSamples();
        }
    }
    return 0;
}


/// Output samples from beginning of the sample buffer. Copies requested samples to 
/// output buffer and removes them from the sample buffer. If there are less than 
/// 'numsample' samples in the buffer, returns all that available.
///
/// \return Number of samples returned.
uint SoundTouch::receiveSamples(SAMPLETYPE* output, uint maxSamples)
{
    uint ret = FIFOProcessor::receiveSamples(output, maxSamples);
    samplesOutput += (long)ret;
    return ret;
}


/// Adjusts book-keeping so that given number of samples are removed from beginning of the 
/// sample buffer without copying them anywhere. 
///
/// Used to reduce the number of samples in the buffer when accessing the sample buffer directly
/// with 'ptrBegin' function.
uint SoundTouch::receiveSamples(uint maxSamples)
{
    uint ret = FIFOProcessor::receiveSamples(maxSamples);
    samplesOutput += (long)ret;
    return ret;
}


/// Get ratio between input and output audio durations, useful for calculating
/// processed output duration: if you'll process a stream of N samples, then 
/// you can expect to get out N * getInputOutputSampleRatio() samples.
double SoundTouch::getInputOutputSampleRatio()
{
    return 1.0 / (tempo * rate);
}


#ifdef SOUNDTOUCH_ALLOW_SSE

#include <xmmintrin.h>
#include <math.h>

// Calculates cross correlation of two buffers
double TDStretchSSE::calcCrossCorr(const float* pV1, const float* pV2, double& anorm)
{
    int i;
    const float* pVec1;
    const __m128* pVec2;
    __m128 vSum, vNorm;

    // Note. It means a major slow-down if the routine needs to tolerate 
    // unaligned __m128 memory accesses. It's way faster if we can skip 
    // unaligned slots and use _mm_load_ps instruction instead of _mm_loadu_ps.
    // This can mean up to ~ 10-fold difference (incl. part of which is
    // due to skipping every second round for stereo sound though).
    //
    // Compile-time define SOUNDTOUCH_ALLOW_NONEXACT_SIMD_OPTIMIZATION is provided
    // for choosing if this little cheating is allowed.

#ifdef ST_SIMD_AVOID_UNALIGNED
    // Little cheating allowed, return valid correlation only for 
    // aligned locations, meaning every second round for stereo sound.

#define _MM_LOAD    _mm_load_ps

    if (((ulongptr)pV1) & 15) return -1e50;    // skip unaligned locations

#else
    // No cheating allowed, use unaligned load & take the resulting
    // performance hit.
#define _MM_LOAD    _mm_loadu_ps
#endif 

    // ensure overlapLength is divisible by 8
    assert((overlapLength % 8) == 0);

    // Calculates the cross-correlation value between 'pV1' and 'pV2' vectors
    // Note: pV2 _must_ be aligned to 16-bit boundary, pV1 need not.
    pVec1 = (const float*)pV1;
    pVec2 = (const __m128*)pV2;
    vSum = vNorm = _mm_setzero_ps();

    // Unroll the loop by factor of 4 * 4 operations. Use same routine for
    // stereo & mono, for mono it just means twice the amount of unrolling.
    for (i = 0; i < channels * overlapLength / 16; i++)
    {
        __m128 vTemp;
        // vSum += pV1[0..3] * pV2[0..3]
        vTemp = _MM_LOAD(pVec1);
        vSum = _mm_add_ps(vSum, _mm_mul_ps(vTemp, pVec2[0]));
        vNorm = _mm_add_ps(vNorm, _mm_mul_ps(vTemp, vTemp));

        // vSum += pV1[4..7] * pV2[4..7]
        vTemp = _MM_LOAD(pVec1 + 4);
        vSum = _mm_add_ps(vSum, _mm_mul_ps(vTemp, pVec2[1]));
        vNorm = _mm_add_ps(vNorm, _mm_mul_ps(vTemp, vTemp));

        // vSum += pV1[8..11] * pV2[8..11]
        vTemp = _MM_LOAD(pVec1 + 8);
        vSum = _mm_add_ps(vSum, _mm_mul_ps(vTemp, pVec2[2]));
        vNorm = _mm_add_ps(vNorm, _mm_mul_ps(vTemp, vTemp));

        // vSum += pV1[12..15] * pV2[12..15]
        vTemp = _MM_LOAD(pVec1 + 12);
        vSum = _mm_add_ps(vSum, _mm_mul_ps(vTemp, pVec2[3]));
        vNorm = _mm_add_ps(vNorm, _mm_mul_ps(vTemp, vTemp));

        pVec1 += 16;
        pVec2 += 4;
    }

    // return value = vSum[0] + vSum[1] + vSum[2] + vSum[3]
    float* pvNorm = (float*)&vNorm;
    float norm = (pvNorm[0] + pvNorm[1] + pvNorm[2] + pvNorm[3]);
    anorm = norm;

    float* pvSum = (float*)&vSum;
    return (double)(pvSum[0] + pvSum[1] + pvSum[2] + pvSum[3]) / sqrt(norm < 1e-9 ? 1.0 : norm);

    /* This is approximately corresponding routine in C-language yet without normalization:
    double corr, norm;
    uint i;

    // Calculates the cross-correlation value between 'pV1' and 'pV2' vectors
    corr = norm = 0.0;
    for (i = 0; i < channels * overlapLength / 16; i ++)
    {
        corr += pV1[0] * pV2[0] +
                pV1[1] * pV2[1] +
                pV1[2] * pV2[2] +
                pV1[3] * pV2[3] +
                pV1[4] * pV2[4] +
                pV1[5] * pV2[5] +
                pV1[6] * pV2[6] +
                pV1[7] * pV2[7] +
                pV1[8] * pV2[8] +
                pV1[9] * pV2[9] +
                pV1[10] * pV2[10] +
                pV1[11] * pV2[11] +
                pV1[12] * pV2[12] +
                pV1[13] * pV2[13] +
                pV1[14] * pV2[14] +
                pV1[15] * pV2[15];

    for (j = 0; j < 15; j ++) norm += pV1[j] * pV1[j];

        pV1 += 16;
        pV2 += 16;
    }
    return corr / sqrt(norm);
    */
}



double TDStretchSSE::calcCrossCorrAccumulate(const float* pV1, const float* pV2, double& norm)
{
    // call usual calcCrossCorr function because SSE does not show big benefit of 
    // accumulating "norm" value, and also the "norm" rolling algorithm would get 
    // complicated due to SSE-specific alignment-vs-nonexact correlation rules.
    return calcCrossCorr(pV1, pV2, norm);
}


//////////////////////////////////////////////////////////////////////////////
//
// implementation of SSE optimized functions of class 'FIRFilter'
//
//////////////////////////////////////////////////////////////////////////////



FIRFilterSSE::FIRFilterSSE() : FIRFilter()
{
    filterCoeffsAlign = NULL;
    filterCoeffsUnalign = NULL;
}


FIRFilterSSE::~FIRFilterSSE()
{
    delete[] filterCoeffsUnalign;
    filterCoeffsAlign = NULL;
    filterCoeffsUnalign = NULL;
}


// (overloaded) Calculates filter coefficients for SSE routine
void FIRFilterSSE::setCoefficients(const float* coeffs, uint newLength, uint uResultDivFactor)
{
    uint i;
    float fDivider;

    FIRFilter::setCoefficients(coeffs, newLength, uResultDivFactor);

    // Scale the filter coefficients so that it won't be necessary to scale the filtering result
    // also rearrange coefficients suitably for SSE
    // Ensure that filter coeffs array is aligned to 16-byte boundary
    delete[] filterCoeffsUnalign;
    filterCoeffsUnalign = new float[2 * newLength + 4];
    filterCoeffsAlign = (float*)SOUNDTOUCH_ALIGN_POINTER_16(filterCoeffsUnalign);

    fDivider = (float)resultDivider;

    // rearrange the filter coefficients for mmx routines 
    for (i = 0; i < newLength; i++)
    {
        filterCoeffsAlign[2 * i + 0] =
            filterCoeffsAlign[2 * i + 1] = coeffs[i + 0] / fDivider;
    }
}



// SSE-optimized version of the filter routine for stereo sound
uint FIRFilterSSE::evaluateFilterStereo(float* dest, const float* source, uint numSamples) const
{
    int count = (int)((numSamples - length) & (uint)-2);
    int j;

    assert(count % 2 == 0);

    if (count < 2) return 0;

    assert(source != NULL);
    assert(dest != NULL);
    assert((length % 8) == 0);
    assert(filterCoeffsAlign != NULL);
    assert(((ulongptr)filterCoeffsAlign) % 16 == 0);

    // filter is evaluated for two stereo samples with each iteration, thus use of 'j += 2'
#pragma omp parallel for
    for (j = 0; j < count; j += 2)
    {
        const float* pSrc;
        float* pDest;
        const __m128* pFil;
        __m128 sum1, sum2;
        uint i;

        pSrc = (const float*)source + j * 2;      // source audio data
        pDest = dest + j * 2;                     // destination audio data
        pFil = (const __m128*)filterCoeffsAlign;  // filter coefficients. NOTE: Assumes coefficients 
        // are aligned to 16-byte boundary
        sum1 = sum2 = _mm_setzero_ps();

        for (i = 0; i < length / 8; i++)
        {
            // Unroll loop for efficiency & calculate filter for 2*2 stereo samples 
            // at each pass

            // sum1 is accu for 2*2 filtered stereo sound data at the primary sound data offset
            // sum2 is accu for 2*2 filtered stereo sound data for the next sound sample offset.

            sum1 = _mm_add_ps(sum1, _mm_mul_ps(_mm_loadu_ps(pSrc), pFil[0]));
            sum2 = _mm_add_ps(sum2, _mm_mul_ps(_mm_loadu_ps(pSrc + 2), pFil[0]));

            sum1 = _mm_add_ps(sum1, _mm_mul_ps(_mm_loadu_ps(pSrc + 4), pFil[1]));
            sum2 = _mm_add_ps(sum2, _mm_mul_ps(_mm_loadu_ps(pSrc + 6), pFil[1]));

            sum1 = _mm_add_ps(sum1, _mm_mul_ps(_mm_loadu_ps(pSrc + 8), pFil[2]));
            sum2 = _mm_add_ps(sum2, _mm_mul_ps(_mm_loadu_ps(pSrc + 10), pFil[2]));

            sum1 = _mm_add_ps(sum1, _mm_mul_ps(_mm_loadu_ps(pSrc + 12), pFil[3]));
            sum2 = _mm_add_ps(sum2, _mm_mul_ps(_mm_loadu_ps(pSrc + 14), pFil[3]));

            pSrc += 16;
            pFil += 4;
        }

        // Now sum1 and sum2 both have a filtered 2-channel sample each, but we still need
        // to sum the two hi- and lo-floats of these registers together.

        // post-shuffle & add the filtered values and store to dest.
        _mm_storeu_ps(pDest, _mm_add_ps(
            _mm_shuffle_ps(sum1, sum2, _MM_SHUFFLE(1, 0, 3, 2)),   // s2_1 s2_0 s1_3 s1_2
            _mm_shuffle_ps(sum1, sum2, _MM_SHUFFLE(3, 2, 1, 0))    // s2_3 s2_2 s1_1 s1_0
        ));
    }

    // Ideas for further improvement:
    // 1. If it could be guaranteed that 'source' were always aligned to 16-byte 
    //    boundary, a faster aligned '_mm_load_ps' instruction could be used.
    // 2. If it could be guaranteed that 'dest' were always aligned to 16-byte 
    //    boundary, a faster '_mm_store_ps' instruction could be used.

    return (uint)count;

    /* original routine in C-language. please notice the C-version has differently
       organized coefficients though.
    double suml1, suml2;
    double sumr1, sumr2;
    uint i, j;

    for (j = 0; j < count; j += 2)
    {
        const float *ptr;
        const float *pFil;

        suml1 = sumr1 = 0.0;
        suml2 = sumr2 = 0.0;
        ptr = src;
        pFil = filterCoeffs;
        for (i = 0; i < lengthLocal; i ++)
        {
            // unroll loop for efficiency.

            suml1 += ptr[0] * pFil[0] +
                     ptr[2] * pFil[2] +
                     ptr[4] * pFil[4] +
                     ptr[6] * pFil[6];

            sumr1 += ptr[1] * pFil[1] +
                     ptr[3] * pFil[3] +
                     ptr[5] * pFil[5] +
                     ptr[7] * pFil[7];

            suml2 += ptr[8] * pFil[0] +
                     ptr[10] * pFil[2] +
                     ptr[12] * pFil[4] +
                     ptr[14] * pFil[6];

            sumr2 += ptr[9] * pFil[1] +
                     ptr[11] * pFil[3] +
                     ptr[13] * pFil[5] +
                     ptr[15] * pFil[7];

            ptr += 16;
            pFil += 8;
        }
        dest[0] = (float)suml1;
        dest[1] = (float)sumr1;
        dest[2] = (float)suml2;
        dest[3] = (float)sumr2;

        src += 4;
        dest += 4;
    }
    */
}

#endif  // SOUNDTOUCH_ALLOW_SSE



#if defined(SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS)

#if defined(__GNUC__) && defined(__i386__)
// gcc
#include "cpuid.h"
#elif defined(_M_IX86)
// windows non-gcc
#include <intrin.h>
#endif

#define bit_MMX     (1 << 23)
#define bit_SSE     (1 << 25)
#define bit_SSE2    (1 << 26)
#endif


//////////////////////////////////////////////////////////////////////////////
//
// processor instructions extension detection routines
//
//////////////////////////////////////////////////////////////////////////////

// Flag variable indicating whick ISA extensions are disabled (for debugging)
static uint _dwDisabledISA = 0x00;      // 0xffffffff; //<- use this to disable all extensions

// Disables given set of instruction extensions. See SUPPORT_... defines.
void disableExtensions(uint dwDisableMask)
{
    _dwDisabledISA = dwDisableMask;
}


/// Checks which instruction set extensions are supported by the CPU.
uint detectCPUextensions(void)
{
    /// If building for a 64bit system (no Itanium) and the user wants optimizations.
    /// Return the OR of SUPPORT_{MMX,SSE,SSE2}. 11001 or 0x19.
    /// Keep the _dwDisabledISA test (2 more operations, could be eliminated).
#if ((defined(__GNUC__) && defined(__x86_64__)) \
    || defined(_M_X64))  \
    && defined(SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS)
    return 0x19 & ~_dwDisabledISA;

    /// If building for a 32bit system and the user wants optimizations.
    /// Keep the _dwDisabledISA test (2 more operations, could be eliminated).
#elif ((defined(__GNUC__) && defined(__i386__)) \
    || defined(_M_IX86))  \
    && defined(SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS)

    if (_dwDisabledISA == 0xffffffff) return 0;

    uint res = 0;

#if defined(__GNUC__)
    // GCC version of cpuid. Requires GCC 4.3.0 or later for __cpuid intrinsic support.
    uint eax, ebx, ecx, edx;  // unsigned int is the standard type. uint is defined by the compiler and not guaranteed to be portable.

    // Check if no cpuid support.
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) return 0; // always disable extensions.

    if (edx & bit_MMX)  res = res | SUPPORT_MMX;
    if (edx & bit_SSE)  res = res | SUPPORT_SSE;
    if (edx & bit_SSE2) res = res | SUPPORT_SSE2;

#else
    // Window / VS version of cpuid. Notice that Visual Studio 2005 or later required 
    // for __cpuid intrinsic support.
    int reg[4] = { -1 };

    // Check if no cpuid support.
    __cpuid(reg, 0);
    if ((unsigned int)reg[0] == 0) return 0; // always disable extensions.

    __cpuid(reg, 1);
    if ((unsigned int)reg[3] & bit_MMX)  res = res | SUPPORT_MMX;
    if ((unsigned int)reg[3] & bit_SSE)  res = res | SUPPORT_SSE;
    if ((unsigned int)reg[3] & bit_SSE2) res = res | SUPPORT_SSE2;

#endif

    return res & ~_dwDisabledISA;

#else

/// One of these is true:
/// 1) We don't want optimizations.
/// 2) Using an unsupported compiler.
/// 3) Running on a non-x86 platform.
    return 0;

#endif
}


#ifdef SOUNDTOUCH_ALLOW_MMX
// MMX routines available only with integer sample type

using namespace soundtouch;

//////////////////////////////////////////////////////////////////////////////
//
// implementation of MMX optimized functions of class 'TDStretchMMX'
//
//////////////////////////////////////////////////////////////////////////////

#include "TDStretch.h"
#include <mmintrin.h>
#include <limits.h>
#include <math.h>


// Calculates cross correlation of two buffers
double TDStretchMMX::calcCrossCorr(const short* pV1, const short* pV2, double& dnorm)
{
    const __m64* pVec1, * pVec2;
    __m64 shifter;
    __m64 accu, normaccu;
    long corr, norm;
    int i;

    pVec1 = (__m64*)pV1;
    pVec2 = (__m64*)pV2;

    shifter = _m_from_int(overlapDividerBitsNorm);
    normaccu = accu = _mm_setzero_si64();

    // Process 4 parallel sets of 2 * stereo samples or 4 * mono samples 
    // during each round for improved CPU-level parallellization.
    for (i = 0; i < channels * overlapLength / 16; i++)
    {
        __m64 temp, temp2;

        // dictionary of instructions:
        // _m_pmaddwd   : 4*16bit multiply-add, resulting two 32bits = [a0*b0+a1*b1 ; a2*b2+a3*b3]
        // _mm_add_pi32 : 2*32bit add
        // _m_psrad     : 32bit right-shift

        temp = _mm_add_pi32(_mm_sra_pi32(_mm_madd_pi16(pVec1[0], pVec2[0]), shifter),
            _mm_sra_pi32(_mm_madd_pi16(pVec1[1], pVec2[1]), shifter));
        temp2 = _mm_add_pi32(_mm_sra_pi32(_mm_madd_pi16(pVec1[0], pVec1[0]), shifter),
            _mm_sra_pi32(_mm_madd_pi16(pVec1[1], pVec1[1]), shifter));
        accu = _mm_add_pi32(accu, temp);
        normaccu = _mm_add_pi32(normaccu, temp2);

        temp = _mm_add_pi32(_mm_sra_pi32(_mm_madd_pi16(pVec1[2], pVec2[2]), shifter),
            _mm_sra_pi32(_mm_madd_pi16(pVec1[3], pVec2[3]), shifter));
        temp2 = _mm_add_pi32(_mm_sra_pi32(_mm_madd_pi16(pVec1[2], pVec1[2]), shifter),
            _mm_sra_pi32(_mm_madd_pi16(pVec1[3], pVec1[3]), shifter));
        accu = _mm_add_pi32(accu, temp);
        normaccu = _mm_add_pi32(normaccu, temp2);

        pVec1 += 4;
        pVec2 += 4;
    }

    // copy hi-dword of mm0 to lo-dword of mm1, then sum mmo+mm1
    // and finally store the result into the variable "corr"

    accu = _mm_add_pi32(accu, _mm_srli_si64(accu, 32));
    corr = _m_to_int(accu);

    normaccu = _mm_add_pi32(normaccu, _mm_srli_si64(normaccu, 32));
    norm = _m_to_int(normaccu);

    // Clear MMS state
    _m_empty();

    if (norm > (long)maxnorm)
    {
        // modify 'maxnorm' inside critical section to avoid multi-access conflict if in OpenMP mode
#pragma omp critical
        if (norm > (long)maxnorm)
        {
            maxnorm = norm;
        }
    }

    // Normalize result by dividing by sqrt(norm) - this step is easiest 
    // done using floating point operation
    dnorm = (double)norm;

    return (double)corr / sqrt(dnorm < 1e-9 ? 1.0 : dnorm);
    // Note: Warning about the missing EMMS instruction is harmless
    // as it'll be called elsewhere.
}


/// Update cross-correlation by accumulating "norm" coefficient by previously calculated value
double TDStretchMMX::calcCrossCorrAccumulate(const short* pV1, const short* pV2, double& dnorm)
{
    const __m64* pVec1, * pVec2;
    __m64 shifter;
    __m64 accu;
    long corr, lnorm;
    int i;

    // cancel first normalizer tap from previous round
    lnorm = 0;
    for (i = 1; i <= channels; i++)
    {
        lnorm -= (pV1[-i] * pV1[-i]) >> overlapDividerBitsNorm;
    }

    pVec1 = (__m64*)pV1;
    pVec2 = (__m64*)pV2;

    shifter = _m_from_int(overlapDividerBitsNorm);
    accu = _mm_setzero_si64();

    // Process 4 parallel sets of 2 * stereo samples or 4 * mono samples 
    // during each round for improved CPU-level parallellization.
    for (i = 0; i < channels * overlapLength / 16; i++)
    {
        __m64 temp;

        // dictionary of instructions:
        // _m_pmaddwd   : 4*16bit multiply-add, resulting two 32bits = [a0*b0+a1*b1 ; a2*b2+a3*b3]
        // _mm_add_pi32 : 2*32bit add
        // _m_psrad     : 32bit right-shift

        temp = _mm_add_pi32(_mm_sra_pi32(_mm_madd_pi16(pVec1[0], pVec2[0]), shifter),
            _mm_sra_pi32(_mm_madd_pi16(pVec1[1], pVec2[1]), shifter));
        accu = _mm_add_pi32(accu, temp);

        temp = _mm_add_pi32(_mm_sra_pi32(_mm_madd_pi16(pVec1[2], pVec2[2]), shifter),
            _mm_sra_pi32(_mm_madd_pi16(pVec1[3], pVec2[3]), shifter));
        accu = _mm_add_pi32(accu, temp);

        pVec1 += 4;
        pVec2 += 4;
    }

    // copy hi-dword of mm0 to lo-dword of mm1, then sum mmo+mm1
    // and finally store the result into the variable "corr"

    accu = _mm_add_pi32(accu, _mm_srli_si64(accu, 32));
    corr = _m_to_int(accu);

    // Clear MMS state
    _m_empty();

    // update normalizer with last samples of this round
    pV1 = (short*)pVec1;
    for (int j = 1; j <= channels; j++)
    {
        lnorm += (pV1[-j] * pV1[-j]) >> overlapDividerBitsNorm;
    }
    dnorm += (double)lnorm;

    if (lnorm > (long)maxnorm)
    {
        maxnorm = lnorm;
    }

    // Normalize result by dividing by sqrt(norm) - this step is easiest 
    // done using floating point operation
    return (double)corr / sqrt((dnorm < 1e-9) ? 1.0 : dnorm);
}


void TDStretchMMX::clearCrossCorrState()
{
    // Clear MMS state
    _m_empty();
    //_asm EMMS;
}


// MMX-optimized version of the function overlapStereo
void TDStretchMMX::overlapStereo(short* output, const short* input) const
{
    const __m64* pVinput, * pVMidBuf;
    __m64* pVdest;
    __m64 mix1, mix2, adder, shifter;
    int i;

    pVinput = (const __m64*)input;
    pVMidBuf = (const __m64*)pMidBuffer;
    pVdest = (__m64*)output;

    // mix1  = mixer values for 1st stereo sample
    // mix1  = mixer values for 2nd stereo sample
    // adder = adder for updating mixer values after each round

    mix1 = _mm_set_pi16(0, overlapLength, 0, overlapLength);
    adder = _mm_set_pi16(1, -1, 1, -1);
    mix2 = _mm_add_pi16(mix1, adder);
    adder = _mm_add_pi16(adder, adder);

    // Overlaplength-division by shifter. "+1" is to account for "-1" deduced in
    // overlapDividerBits calculation earlier.
    shifter = _m_from_int(overlapDividerBitsPure + 1);

    for (i = 0; i < overlapLength / 4; i++)
    {
        __m64 temp1, temp2;

        // load & shuffle data so that input & mixbuffer data samples are paired
        temp1 = _mm_unpacklo_pi16(pVMidBuf[0], pVinput[0]);     // = i0l m0l i0r m0r
        temp2 = _mm_unpackhi_pi16(pVMidBuf[0], pVinput[0]);     // = i1l m1l i1r m1r

        // temp = (temp .* mix) >> shifter
        temp1 = _mm_sra_pi32(_mm_madd_pi16(temp1, mix1), shifter);
        temp2 = _mm_sra_pi32(_mm_madd_pi16(temp2, mix2), shifter);
        pVdest[0] = _mm_packs_pi32(temp1, temp2); // pack 2*2*32bit => 4*16bit

        // update mix += adder
        mix1 = _mm_add_pi16(mix1, adder);
        mix2 = _mm_add_pi16(mix2, adder);

        // --- second round begins here ---

        // load & shuffle data so that input & mixbuffer data samples are paired
        temp1 = _mm_unpacklo_pi16(pVMidBuf[1], pVinput[1]);       // = i2l m2l i2r m2r
        temp2 = _mm_unpackhi_pi16(pVMidBuf[1], pVinput[1]);       // = i3l m3l i3r m3r

        // temp = (temp .* mix) >> shifter
        temp1 = _mm_sra_pi32(_mm_madd_pi16(temp1, mix1), shifter);
        temp2 = _mm_sra_pi32(_mm_madd_pi16(temp2, mix2), shifter);
        pVdest[1] = _mm_packs_pi32(temp1, temp2); // pack 2*2*32bit => 4*16bit

        // update mix += adder
        mix1 = _mm_add_pi16(mix1, adder);
        mix2 = _mm_add_pi16(mix2, adder);

        pVinput += 2;
        pVMidBuf += 2;
        pVdest += 2;
    }

    _m_empty(); // clear MMS state
}


//////////////////////////////////////////////////////////////////////////////
//
// implementation of MMX optimized functions of class 'FIRFilter'
//
//////////////////////////////////////////////////////////////////////////////

#include "FIRFilter.h"


FIRFilterMMX::FIRFilterMMX() : FIRFilter()
{
    filterCoeffsAlign = NULL;
    filterCoeffsUnalign = NULL;
}


FIRFilterMMX::~FIRFilterMMX()
{
    delete[] filterCoeffsUnalign;
}


// (overloaded) Calculates filter coefficients for MMX routine
void FIRFilterMMX::setCoefficients(const short* coeffs, uint newLength, uint uResultDivFactor)
{
    uint i;
    FIRFilter::setCoefficients(coeffs, newLength, uResultDivFactor);

    // Ensure that filter coeffs array is aligned to 16-byte boundary
    delete[] filterCoeffsUnalign;
    filterCoeffsUnalign = new short[2 * newLength + 8];
    filterCoeffsAlign = (short*)SOUNDTOUCH_ALIGN_POINTER_16(filterCoeffsUnalign);

    // rearrange the filter coefficients for mmx routines 
    for (i = 0; i < length; i += 4)
    {
        filterCoeffsAlign[2 * i + 0] = coeffs[i + 0];
        filterCoeffsAlign[2 * i + 1] = coeffs[i + 2];
        filterCoeffsAlign[2 * i + 2] = coeffs[i + 0];
        filterCoeffsAlign[2 * i + 3] = coeffs[i + 2];

        filterCoeffsAlign[2 * i + 4] = coeffs[i + 1];
        filterCoeffsAlign[2 * i + 5] = coeffs[i + 3];
        filterCoeffsAlign[2 * i + 6] = coeffs[i + 1];
        filterCoeffsAlign[2 * i + 7] = coeffs[i + 3];
    }
}


// mmx-optimized version of the filter routine for stereo sound
uint FIRFilterMMX::evaluateFilterStereo(short* dest, const short* src, uint numSamples) const
{
    // Create stack copies of the needed member variables for asm routines :
    uint i, j;
    __m64* pVdest = (__m64*)dest;

    if (length < 2) return 0;

    for (i = 0; i < (numSamples - length) / 2; i++)
    {
        __m64 accu1;
        __m64 accu2;
        const __m64* pVsrc = (const __m64*)src;
        const __m64* pVfilter = (const __m64*)filterCoeffsAlign;

        accu1 = accu2 = _mm_setzero_si64();
        for (j = 0; j < lengthDiv8 * 2; j++)
        {
            __m64 temp1, temp2;

            temp1 = _mm_unpacklo_pi16(pVsrc[0], pVsrc[1]);  // = l2 l0 r2 r0
            temp2 = _mm_unpackhi_pi16(pVsrc[0], pVsrc[1]);  // = l3 l1 r3 r1

            accu1 = _mm_add_pi32(accu1, _mm_madd_pi16(temp1, pVfilter[0]));  // += l2*f2+l0*f0 r2*f2+r0*f0
            accu1 = _mm_add_pi32(accu1, _mm_madd_pi16(temp2, pVfilter[1]));  // += l3*f3+l1*f1 r3*f3+r1*f1

            temp1 = _mm_unpacklo_pi16(pVsrc[1], pVsrc[2]);  // = l4 l2 r4 r2

            accu2 = _mm_add_pi32(accu2, _mm_madd_pi16(temp2, pVfilter[0]));  // += l3*f2+l1*f0 r3*f2+r1*f0
            accu2 = _mm_add_pi32(accu2, _mm_madd_pi16(temp1, pVfilter[1]));  // += l4*f3+l2*f1 r4*f3+r2*f1

            // accu1 += l2*f2+l0*f0 r2*f2+r0*f0
            //       += l3*f3+l1*f1 r3*f3+r1*f1

            // accu2 += l3*f2+l1*f0 r3*f2+r1*f0
            //          l4*f3+l2*f1 r4*f3+r2*f1

            pVfilter += 2;
            pVsrc += 2;
        }
        // accu >>= resultDivFactor
        accu1 = _mm_srai_pi32(accu1, resultDivFactor);
        accu2 = _mm_srai_pi32(accu2, resultDivFactor);

        // pack 2*2*32bits => 4*16 bits
        pVdest[0] = _mm_packs_pi32(accu1, accu2);
        src += 4;
        pVdest++;
    }

    _m_empty();  // clear emms state

    return (numSamples & 0xfffffffe) - length;
}

#else

// workaround to not complain about empty module
bool _dontcomplain_mmx_empty;

#endif  // SOUNDTOUCH_ALLOW_MMX




#include <string.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#include <float.h>

// Table for the hierarchical mixing position seeking algorithm
const short _scanOffsets[5][24] = {
    { 124,  186,  248,  310,  372,  434,  496,  558,  620,  682,  744, 806,
      868,  930,  992, 1054, 1116, 1178, 1240, 1302, 1364, 1426, 1488,   0},
    {-100,  -75,  -50,  -25,   25,   50,   75,  100,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    { -20,  -15,  -10,   -5,    5,   10,   15,   20,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    {  -4,   -3,   -2,   -1,    1,    2,    3,    4,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    { 121,  114,   97,  114,   98,  105,  108,   32,  104,   99,  117,  111,
      116,  100,  110,  117,  111,  115,    0,    0,    0,    0,    0,   0} };

/*****************************************************************************
 *
 * Implementation of the class 'TDStretch'
 *
 *****************************************************************************/


TDStretch::TDStretch() : FIFOProcessor(&outputBuffer)
{
    bQuickSeek = false;
    channels = 2;

    pMidBuffer = NULL;
    pMidBufferUnaligned = NULL;
    overlapLength = 0;

    bAutoSeqSetting = true;
    bAutoSeekSetting = true;

    tempo = 1.0f;
    setParameters(44100, DEFAULT_SEQUENCE_MS, DEFAULT_SEEKWINDOW_MS, DEFAULT_OVERLAP_MS);
    setTempo(1.0f);

    clear();
}



TDStretch::~TDStretch()
{
    delete[] pMidBufferUnaligned;
}



// Sets routine control parameters. These control are certain time constants
// defining how the sound is stretched to the desired duration.
//
// 'sampleRate' = sample rate of the sound
// 'sequenceMS' = one processing sequence length in milliseconds (default = 82 ms)
// 'seekwindowMS' = seeking window length for scanning the best overlapping 
//      position (default = 28 ms)
// 'overlapMS' = overlapping length (default = 12 ms)

void TDStretch::setParameters(int aSampleRate, int aSequenceMS,
    int aSeekWindowMS, int aOverlapMS)
{
    // accept only positive parameter values - if zero or negative, use old values instead
    if (aSampleRate > 0)
    {
        if (aSampleRate > 192000) ST_THROW_RT_ERROR("Error: Excessive samplerate");
        this->sampleRate = aSampleRate;
    }

    if (aOverlapMS > 0) this->overlapMs = aOverlapMS;

    if (aSequenceMS > 0)
    {
        this->sequenceMs = aSequenceMS;
        bAutoSeqSetting = false;
    }
    else if (aSequenceMS == 0)
    {
        // if zero, use automatic setting
        bAutoSeqSetting = true;
    }

    if (aSeekWindowMS > 0)
    {
        this->seekWindowMs = aSeekWindowMS;
        bAutoSeekSetting = false;
    }
    else if (aSeekWindowMS == 0)
    {
        // if zero, use automatic setting
        bAutoSeekSetting = true;
    }

    calcSeqParameters();

    calculateOverlapLength(overlapMs);

    // set tempo to recalculate 'sampleReq'
    setTempo(tempo);
}



/// Get routine control parameters, see setParameters() function.
/// Any of the parameters to this function can be NULL, in such case corresponding parameter
/// value isn't returned.
void TDStretch::getParameters(int* pSampleRate, int* pSequenceMs, int* pSeekWindowMs, int* pOverlapMs) const
{
    if (pSampleRate)
    {
        *pSampleRate = sampleRate;
    }

    if (pSequenceMs)
    {
        *pSequenceMs = (bAutoSeqSetting) ? (USE_AUTO_SEQUENCE_LEN) : sequenceMs;
    }

    if (pSeekWindowMs)
    {
        *pSeekWindowMs = (bAutoSeekSetting) ? (USE_AUTO_SEEKWINDOW_LEN) : seekWindowMs;
    }

    if (pOverlapMs)
    {
        *pOverlapMs = overlapMs;
    }
}


// Overlaps samples in 'midBuffer' with the samples in 'pInput'
void TDStretch::overlapMono(SAMPLETYPE* pOutput, const SAMPLETYPE* pInput) const
{
    int i;
    SAMPLETYPE m1, m2;

    m1 = (SAMPLETYPE)0;
    m2 = (SAMPLETYPE)overlapLength;

    for (i = 0; i < overlapLength; i++)
    {
        pOutput[i] = (pInput[i] * m1 + pMidBuffer[i] * m2) / overlapLength;
        m1 += 1;
        m2 -= 1;
    }
}



void TDStretch::clearMidBuffer()
{
    memset(pMidBuffer, 0, channels * sizeof(SAMPLETYPE) * overlapLength);
}


void TDStretch::clearInput()
{
    inputBuffer.clear();
    clearMidBuffer();
    isBeginning = true;
    maxnorm = 0;
    maxnormf = 1e8;
    skipFract = 0;
}


// Clears the sample buffers
void TDStretch::clear()
{
    outputBuffer.clear();
    clearInput();
}



// Enables/disables the quick position seeking algorithm. Zero to disable, nonzero
// to enable
void TDStretch::enableQuickSeek(bool enable)
{
    bQuickSeek = enable;
}


// Returns nonzero if the quick seeking algorithm is enabled.
bool TDStretch::isQuickSeekEnabled() const
{
    return bQuickSeek;
}


// Seeks for the optimal overlap-mixing position.
int TDStretch::seekBestOverlapPosition(const SAMPLETYPE* refPos)
{
    if (bQuickSeek)
    {
        return seekBestOverlapPositionQuick(refPos);
    }
    else
    {
        return seekBestOverlapPositionFull(refPos);
    }
}


// Overlaps samples in 'midBuffer' with the samples in 'pInputBuffer' at position
// of 'ovlPos'.
inline void TDStretch::overlap(SAMPLETYPE* pOutput, const SAMPLETYPE* pInput, uint ovlPos) const
{
#ifndef USE_MULTICH_ALWAYS
    if (channels == 1)
    {
        // mono sound.
        overlapMono(pOutput, pInput + ovlPos);
    }
    else if (channels == 2)
    {
        // stereo sound
        overlapStereo(pOutput, pInput + 2 * ovlPos);
    }
    else
#endif // USE_MULTICH_ALWAYS
    {
        assert(channels > 0);
        overlapMulti(pOutput, pInput + channels * ovlPos);
    }
}


// Seeks for the optimal overlap-mixing position. The 'stereo' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period
int TDStretch::seekBestOverlapPositionFull(const SAMPLETYPE* refPos)
{
    int bestOffs;
    double bestCorr;
    int i;
    double norm;

    bestCorr = -FLT_MAX;
    bestOffs = 0;

    // Scans for the best correlation value by testing each possible position
    // over the permitted range.
    bestCorr = calcCrossCorr(refPos, pMidBuffer, norm);
    bestCorr = (bestCorr + 0.1) * 0.75;

#pragma omp parallel for
    for (i = 1; i < seekLength; i++)
    {
        double corr;
        // Calculates correlation value for the mixing position corresponding to 'i'
#if defined(_OPENMP) || defined(ST_SIMD_AVOID_UNALIGNED)
        // in parallel OpenMP mode, can't use norm accumulator version as parallel executor won't
        // iterate the loop in sequential order
        // in SIMD mode, avoid accumulator version to allow avoiding unaligned positions
        corr = calcCrossCorr(refPos + channels * i, pMidBuffer, norm);
#else
        // In non-parallel version call "calcCrossCorrAccumulate" that is otherwise same
        // as "calcCrossCorr", but saves time by reusing & updating previously stored 
        // "norm" value
        corr = calcCrossCorrAccumulate(refPos + channels * i, pMidBuffer, norm);
#endif
        // heuristic rule to slightly favour values close to mid of the range
        double tmp = (double)(2 * i - seekLength) / (double)seekLength;
        corr = ((corr + 0.1) * (1.0 - 0.25 * tmp * tmp));

        // Checks for the highest correlation value
        if (corr > bestCorr)
        {
            // For optimal performance, enter critical section only in case that best value found.
            // in such case repeat 'if' condition as it's possible that parallel execution may have
            // updated the bestCorr value in the mean time
#pragma omp critical
            if (corr > bestCorr)
            {
                bestCorr = corr;
                bestOffs = i;
            }
        }
    }

#ifdef SOUNDTOUCH_INTEGER_SAMPLES
    adaptNormalizer();
#endif

    // clear cross correlation routine state if necessary (is so e.g. in MMX routines).
    clearCrossCorrState();

    return bestOffs;
}


// Quick seek algorithm for improved runtime-performance: First roughly scans through the 
// correlation area, and then scan surroundings of two best preliminary correlation candidates
// with improved precision
//
// Based on testing:
// - This algorithm gives on average 99% as good match as the full algorithm
// - this quick seek algorithm finds the best match on ~90% of cases
// - on those 10% of cases when this algorithm doesn't find best match, 
//   it still finds on average ~90% match vs. the best possible match
int TDStretch::seekBestOverlapPositionQuick(const SAMPLETYPE* refPos)
{
#define _MIN(a, b)   (((a) < (b)) ? (a) : (b))
#define SCANSTEP    16
#define SCANWIND    8

    int bestOffs;
    int i;
    int bestOffs2;
    float bestCorr, corr;
    float bestCorr2;
    double norm;

    // note: 'float' types used in this function in case that the platform would need to use software-fp

    bestCorr =
        bestCorr2 = -FLT_MAX;
    bestOffs =
        bestOffs2 = SCANWIND;

    // Scans for the best correlation value by testing each possible position
    // over the permitted range. Look for two best matches on the first pass to
    // increase possibility of ideal match.
    //
    // Begin from "SCANSTEP" instead of SCANWIND to make the calculation
    // catch the 'middlepoint' of seekLength vector as that's the a-priori 
    // expected best match position
    //
    // Roughly:
    // - 15% of cases find best result directly on the first round,
    // - 75% cases find better match on 2nd round around the best match from 1st round
    // - 10% cases find better match on 2nd round around the 2nd-best-match from 1st round
    for (i = SCANSTEP; i < seekLength - SCANWIND - 1; i += SCANSTEP)
    {
        // Calculates correlation value for the mixing position corresponding
        // to 'i'
        corr = (float)calcCrossCorr(refPos + channels * i, pMidBuffer, norm);
        // heuristic rule to slightly favour values close to mid of the seek range
        float tmp = (float)(2 * i - seekLength - 1) / (float)seekLength;
        corr = ((corr + 0.1f) * (1.0f - 0.25f * tmp * tmp));

        // Checks for the highest correlation value
        if (corr > bestCorr)
        {
            // found new best match. keep the previous best as 2nd best match
            bestCorr2 = bestCorr;
            bestOffs2 = bestOffs;
            bestCorr = corr;
            bestOffs = i;
        }
        else if (corr > bestCorr2)
        {
            // not new best, but still new 2nd best match
            bestCorr2 = corr;
            bestOffs2 = i;
        }
    }

    // Scans surroundings of the found best match with small stepping
    int end = _MIN(bestOffs + SCANWIND + 1, seekLength);
    for (i = bestOffs - SCANWIND; i < end; i++)
    {
        if (i == bestOffs) continue;    // this offset already calculated, thus skip

        // Calculates correlation value for the mixing position corresponding
        // to 'i'
        corr = (float)calcCrossCorr(refPos + channels * i, pMidBuffer, norm);
        // heuristic rule to slightly favour values close to mid of the range
        float tmp = (float)(2 * i - seekLength - 1) / (float)seekLength;
        corr = ((corr + 0.1f) * (1.0f - 0.25f * tmp * tmp));

        // Checks for the highest correlation value
        if (corr > bestCorr)
        {
            bestCorr = corr;
            bestOffs = i;
        }
    }

    // Scans surroundings of the 2nd best match with small stepping
    end = _MIN(bestOffs2 + SCANWIND + 1, seekLength);
    for (i = bestOffs2 - SCANWIND; i < end; i++)
    {
        if (i == bestOffs2) continue;    // this offset already calculated, thus skip

        // Calculates correlation value for the mixing position corresponding
        // to 'i'
        corr = (float)calcCrossCorr(refPos + channels * i, pMidBuffer, norm);
        // heuristic rule to slightly favour values close to mid of the range
        float tmp = (float)(2 * i - seekLength - 1) / (float)seekLength;
        corr = ((corr + 0.1f) * (1.0f - 0.25f * tmp * tmp));

        // Checks for the highest correlation value
        if (corr > bestCorr)
        {
            bestCorr = corr;
            bestOffs = i;
        }
    }

    // clear cross correlation routine state if necessary (is so e.g. in MMX routines).
    clearCrossCorrState();

#ifdef SOUNDTOUCH_INTEGER_SAMPLES
    adaptNormalizer();
#endif

    return bestOffs;
}




/// For integer algorithm: adapt normalization factor divider with music so that 
/// it'll not be pessimistically restrictive that can degrade quality on quieter sections
/// yet won't cause integer overflows either
void TDStretch::adaptNormalizer()
{
    // Do not adapt normalizer over too silent sequences to avoid averaging filter depleting to
    // too low values during pauses in music
    if ((maxnorm > 1000) || (maxnormf > 40000000))
    {
        //norm averaging filter
        maxnormf = 0.9f * maxnormf + 0.1f * (float)maxnorm;

        if ((maxnorm > 800000000) && (overlapDividerBitsNorm < 16))
        {
            // large values, so increase divider
            overlapDividerBitsNorm++;
            if (maxnorm > 1600000000) overlapDividerBitsNorm++; // extra large value => extra increase
        }
        else if ((maxnormf < 1000000) && (overlapDividerBitsNorm > 0))
        {
            // extra small values, decrease divider
            overlapDividerBitsNorm--;
        }
    }

    maxnorm = 0;
}


/// clear cross correlation routine state if necessary 
void TDStretch::clearCrossCorrState()
{
    // default implementation is empty.
}


/// Calculates processing sequence length according to tempo setting
void TDStretch::calcSeqParameters()
{
    // Adjust tempo param according to tempo, so that variating processing sequence length is used
    // at various tempo settings, between the given low...top limits
#define AUTOSEQ_TEMPO_LOW   0.5     // auto setting low tempo range (-50%)
#define AUTOSEQ_TEMPO_TOP   2.0     // auto setting top tempo range (+100%)

// sequence-ms setting values at above low & top tempo
#define AUTOSEQ_AT_MIN      90.0
#define AUTOSEQ_AT_MAX      40.0
#define AUTOSEQ_K           ((AUTOSEQ_AT_MAX - AUTOSEQ_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
#define AUTOSEQ_C           (AUTOSEQ_AT_MIN - (AUTOSEQ_K) * (AUTOSEQ_TEMPO_LOW))

// seek-window-ms setting values at above low & top tempoq
#define AUTOSEEK_AT_MIN     20.0
#define AUTOSEEK_AT_MAX     15.0
#define AUTOSEEK_K          ((AUTOSEEK_AT_MAX - AUTOSEEK_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
#define AUTOSEEK_C          (AUTOSEEK_AT_MIN - (AUTOSEEK_K) * (AUTOSEQ_TEMPO_LOW))

#define CHECK_LIMITS(x, mi, ma) (((x) < (mi)) ? (mi) : (((x) > (ma)) ? (ma) : (x)))

    double seq, seek;

    if (bAutoSeqSetting)
    {
        seq = AUTOSEQ_C + AUTOSEQ_K * tempo;
        seq = CHECK_LIMITS(seq, AUTOSEQ_AT_MAX, AUTOSEQ_AT_MIN);
        sequenceMs = (int)(seq + 0.5);
    }

    if (bAutoSeekSetting)
    {
        seek = AUTOSEEK_C + AUTOSEEK_K * tempo;
        seek = CHECK_LIMITS(seek, AUTOSEEK_AT_MAX, AUTOSEEK_AT_MIN);
        seekWindowMs = (int)(seek + 0.5);
    }

    // Update seek window lengths
    seekWindowLength = (sampleRate * sequenceMs) / 1000;
    if (seekWindowLength < 2 * overlapLength)
    {
        seekWindowLength = 2 * overlapLength;
    }
    seekLength = (sampleRate * seekWindowMs) / 1000;
}



// Sets new target tempo. Normal tempo = 'SCALE', smaller values represent slower 
// tempo, larger faster tempo.
void TDStretch::setTempo(double newTempo)
{
    int intskip;

    tempo = newTempo;

    // Calculate new sequence duration
    calcSeqParameters();

    // Calculate ideal skip length (according to tempo value) 
    nominalSkip = tempo * (seekWindowLength - overlapLength);
    intskip = (int)(nominalSkip + 0.5);

    // Calculate how many samples are needed in the 'inputBuffer' to 
    // process another batch of samples
    //sampleReq = max(intskip + overlapLength, seekWindowLength) + seekLength / 2;
    sampleReq = std::max(intskip + overlapLength, seekWindowLength) + seekLength;
}



// Sets the number of channels, 1 = mono, 2 = stereo
void TDStretch::setChannels(int numChannels)
{
    if (!verifyNumberOfChannels(numChannels) ||
        (channels == numChannels)) return;

    channels = numChannels;
    inputBuffer.setChannels(channels);
    outputBuffer.setChannels(channels);

    // re-init overlap/buffer
    overlapLength = 0;
    setParameters(sampleRate);
}


// nominal tempo, no need for processing, just pass the samples through
// to outputBuffer
/*
void TDStretch::processNominalTempo()
{
    assert(tempo == 1.0f);

    if (bMidBufferDirty)
    {
        // If there are samples in pMidBuffer waiting for overlapping,
        // do a single sliding overlapping with them in order to prevent a
        // clicking distortion in the output sound
        if (inputBuffer.numSamples() < overlapLength)
        {
            // wait until we've got overlapLength input samples
            return;
        }
        // Mix the samples in the beginning of 'inputBuffer' with the
        // samples in 'midBuffer' using sliding overlapping
        overlap(outputBuffer.ptrEnd(overlapLength), inputBuffer.ptrBegin(), 0);
        outputBuffer.putSamples(overlapLength);
        inputBuffer.receiveSamples(overlapLength);
        clearMidBuffer();
        // now we've caught the nominal sample flow and may switch to
        // bypass mode
    }

    // Simply bypass samples from input to output
    outputBuffer.moveSamples(inputBuffer);
}
*/


// Processes as many processing frames of the samples 'inputBuffer', store
// the result into 'outputBuffer'
void TDStretch::processSamples()
{
    int ovlSkip;
    int offset = 0;
    int temp;

    /* Removed this small optimization - can introduce a click to sound when tempo setting
       crosses the nominal value
    if (tempo == 1.0f)
    {
        // tempo not changed from the original, so bypass the processing
        processNominalTempo();
        return;
    }
    */

    // Process samples as long as there are enough samples in 'inputBuffer'
    // to form a processing frame.
    while ((int)inputBuffer.numSamples() >= sampleReq)
    {
        if (isBeginning == false)
        {
            // apart from the very beginning of the track, 
            // scan for the best overlapping position & do overlap-add
            offset = seekBestOverlapPosition(inputBuffer.ptrBegin());

            // Mix the samples in the 'inputBuffer' at position of 'offset' with the 
            // samples in 'midBuffer' using sliding overlapping
            // ... first partially overlap with the end of the previous sequence
            // (that's in 'midBuffer')
            overlap(outputBuffer.ptrEnd((uint)overlapLength), inputBuffer.ptrBegin(), (uint)offset);
            outputBuffer.putSamples((uint)overlapLength);
            offset += overlapLength;
        }
        else
        {
            // Adjust processing offset at beginning of track by not perform initial overlapping
            // and compensating that in the 'input buffer skip' calculation
            isBeginning = false;
            int skip = (int)(tempo * overlapLength + 0.5 * seekLength + 0.5);

#ifdef ST_SIMD_AVOID_UNALIGNED
            // in SIMD mode, round the skip amount to value corresponding to aligned memory address
            if (channels == 1)
            {
                skip &= -4;
            }
            else if (channels == 2)
            {
                skip &= -2;
            }
#endif
            skipFract -= skip;
            if (skipFract <= -nominalSkip)
            {
                skipFract = -nominalSkip;
            }
        }

        // ... then copy sequence samples from 'inputBuffer' to output:

        // crosscheck that we don't have buffer overflow...
        if ((int)inputBuffer.numSamples() < (offset + seekWindowLength - overlapLength))
        {
            continue;    // just in case, shouldn't really happen
        }

        // length of sequence
        temp = (seekWindowLength - 2 * overlapLength);
        outputBuffer.putSamples(inputBuffer.ptrBegin() + channels * offset, (uint)temp);

        // Copies the end of the current sequence from 'inputBuffer' to 
        // 'midBuffer' for being mixed with the beginning of the next 
        // processing sequence and so on
        assert((offset + temp + overlapLength) <= (int)inputBuffer.numSamples());
        memcpy(pMidBuffer, inputBuffer.ptrBegin() + channels * (offset + temp),
            channels * sizeof(SAMPLETYPE) * overlapLength);

        // Remove the processed samples from the input buffer. Update
        // the difference between integer & nominal skip step to 'skipFract'
        // in order to prevent the error from accumulating over time.
        skipFract += nominalSkip;   // real skip size
        ovlSkip = (int)skipFract;   // rounded to integer skip
        skipFract -= ovlSkip;       // maintain the fraction part, i.e. real vs. integer skip
        inputBuffer.receiveSamples((uint)ovlSkip);
    }
}


// Adds 'numsamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void TDStretch::putSamples(const SAMPLETYPE* samples, uint nSamples)
{
    // Add the samples into the input buffer
    inputBuffer.putSamples(samples, nSamples);
    // Process the samples in input buffer
    processSamples();
}



/// Set new overlap length parameter & reallocate RefMidBuffer if necessary.
void TDStretch::acceptNewOverlapLength(int newOverlapLength)
{
    int prevOvl;

    assert(newOverlapLength >= 0);
    prevOvl = overlapLength;
    overlapLength = newOverlapLength;

    if (overlapLength > prevOvl)
    {
        delete[] pMidBufferUnaligned;

        pMidBufferUnaligned = new SAMPLETYPE[overlapLength * channels + 16 / sizeof(SAMPLETYPE)];
        // ensure that 'pMidBuffer' is aligned to 16 byte boundary for efficiency
        pMidBuffer = (SAMPLETYPE*)SOUNDTOUCH_ALIGN_POINTER_16(pMidBufferUnaligned);

        clearMidBuffer();
    }
}


// Operator 'new' is overloaded so that it automatically creates a suitable instance 
// depending on if we've a MMX/SSE/etc-capable CPU available or not.
void* TDStretch::operator new(size_t s)
{
    // Notice! don't use "new TDStretch" directly, use "newInstance" to create a new instance instead!
    ST_THROW_RT_ERROR("Error in TDStretch::new: Don't use 'new TDStretch' directly, use 'newInstance' member instead!");
    return newInstance();
}


TDStretch* TDStretch::newInstance()
{
    uint uExtensions;

    uExtensions = detectCPUextensions();

    // Check if MMX/SSE instruction set extensions supported by CPU

#ifdef SOUNDTOUCH_ALLOW_MMX
    // MMX routines available only with integer sample types
    if (uExtensions & SUPPORT_MMX)
    {
        return ::new TDStretchMMX;
    }
    else
#endif // SOUNDTOUCH_ALLOW_MMX


#ifdef SOUNDTOUCH_ALLOW_SSE
        if (uExtensions & SUPPORT_SSE)
        {
            // SSE support
            return ::new TDStretchSSE;
        }
        else
#endif // SOUNDTOUCH_ALLOW_SSE

        {
            // ISA optimizations not supported, use plain C version
            return ::new TDStretch;
        }
}


//////////////////////////////////////////////////////////////////////////////
//
// Integer arithmetic specific algorithm implementations.
//
//////////////////////////////////////////////////////////////////////////////

#ifdef SOUNDTOUCH_INTEGER_SAMPLES

// Overlaps samples in 'midBuffer' with the samples in 'input'. The 'Stereo' 
// version of the routine.
void TDStretch::overlapStereo(short* poutput, const short* input) const
{
    int i;
    short temp;
    int cnt2;

    for (i = 0; i < overlapLength; i++)
    {
        temp = (short)(overlapLength - i);
        cnt2 = 2 * i;
        poutput[cnt2] = (input[cnt2] * i + pMidBuffer[cnt2] * temp) / overlapLength;
        poutput[cnt2 + 1] = (input[cnt2 + 1] * i + pMidBuffer[cnt2 + 1] * temp) / overlapLength;
    }
}


// Overlaps samples in 'midBuffer' with the samples in 'input'. The 'Multi'
// version of the routine.
void TDStretch::overlapMulti(short* poutput, const short* input) const
{
    short m1;
    int i = 0;

    for (m1 = 0; m1 < overlapLength; m1++)
    {
        short m2 = (short)(overlapLength - m1);
        for (int c = 0; c < channels; c++)
        {
            poutput[i] = (input[i] * m1 + pMidBuffer[i] * m2) / overlapLength;
            i++;
        }
    }
}

// Calculates the x having the closest 2^x value for the given value
static int _getClosest2Power(double value)
{
    return (int)(log(value) / log(2.0) + 0.5);
}


/// Calculates overlap period length in samples.
/// Integer version rounds overlap length to closest power of 2
/// for a divide scaling operation.
void TDStretch::calculateOverlapLength(int aoverlapMs)
{
    int newOvl;

    assert(aoverlapMs >= 0);

    // calculate overlap length so that it's power of 2 - thus it's easy to do
    // integer division by right-shifting. Term "-1" at end is to account for 
    // the extra most significatnt bit left unused in result by signed multiplication 
    overlapDividerBitsPure = _getClosest2Power((sampleRate * aoverlapMs) / 1000.0) - 1;
    if (overlapDividerBitsPure > 9) overlapDividerBitsPure = 9;
    if (overlapDividerBitsPure < 3) overlapDividerBitsPure = 3;
    newOvl = (int)pow(2.0, (int)overlapDividerBitsPure + 1);    // +1 => account for -1 above

    acceptNewOverlapLength(newOvl);

    overlapDividerBitsNorm = overlapDividerBitsPure;

    // calculate sloping divider so that crosscorrelation operation won't 
    // overflow 32-bit register. Max. sum of the crosscorrelation sum without 
    // divider would be 2^30*(N^3-N)/3, where N = overlap length
    slopingDivider = (newOvl * newOvl - 1) / 3;
}


double TDStretch::calcCrossCorr(const short* mixingPos, const short* compare, double& norm)
{
    long corr;
    unsigned long lnorm;
    int i;

#ifdef ST_SIMD_AVOID_UNALIGNED
    // in SIMD mode skip 'mixingPos' positions that aren't aligned to 16-byte boundary
    if (((ulongptr)mixingPos) & 15) return -1e50;
#endif

    // hint compiler autovectorization that loop length is divisible by 8
    int ilength = (channels * overlapLength) & -8;

    corr = lnorm = 0;
    // Same routine for stereo and mono
    for (i = 0; i < ilength; i += 2)
    {
        corr += (mixingPos[i] * compare[i] +
            mixingPos[i + 1] * compare[i + 1]) >> overlapDividerBitsNorm;
        lnorm += (mixingPos[i] * mixingPos[i] +
            mixingPos[i + 1] * mixingPos[i + 1]) >> overlapDividerBitsNorm;
        // do intermediate scalings to avoid integer overflow
    }

    if (lnorm > maxnorm)
    {
        // modify 'maxnorm' inside critical section to avoid multi-access conflict if in OpenMP mode
#pragma omp critical
        if (lnorm > maxnorm)
        {
            maxnorm = lnorm;
        }
    }
    // Normalize result by dividing by sqrt(norm) - this step is easiest 
    // done using floating point operation
    norm = (double)lnorm;
    return (double)corr / sqrt((norm < 1e-9) ? 1.0 : norm);
}


/// Update cross-correlation by accumulating "norm" coefficient by previously calculated value
double TDStretch::calcCrossCorrAccumulate(const short* mixingPos, const short* compare, double& norm)
{
    long corr;
    long lnorm;
    int i;

    // hint compiler autovectorization that loop length is divisible by 8
    int ilength = (channels * overlapLength) & -8;

    // cancel first normalizer tap from previous round
    lnorm = 0;
    for (i = 1; i <= channels; i++)
    {
        lnorm -= (mixingPos[-i] * mixingPos[-i]) >> overlapDividerBitsNorm;
    }

    corr = 0;
    // Same routine for stereo and mono.
    for (i = 0; i < ilength; i += 2)
    {
        corr += (mixingPos[i] * compare[i] +
            mixingPos[i + 1] * compare[i + 1]) >> overlapDividerBitsNorm;
    }

    // update normalizer with last samples of this round
    for (int j = 0; j < channels; j++)
    {
        i--;
        lnorm += (mixingPos[i] * mixingPos[i]) >> overlapDividerBitsNorm;
    }

    norm += (double)lnorm;
    if (norm > maxnorm)
    {
        maxnorm = (unsigned long)norm;
    }

    // Normalize result by dividing by sqrt(norm) - this step is easiest 
    // done using floating point operation
    return (double)corr / sqrt((norm < 1e-9) ? 1.0 : norm);
}

#endif // SOUNDTOUCH_INTEGER_SAMPLES

//////////////////////////////////////////////////////////////////////////////
//
// Floating point arithmetic specific algorithm implementations.
//

#ifdef SOUNDTOUCH_FLOAT_SAMPLES

// Overlaps samples in 'midBuffer' with the samples in 'pInput'
void TDStretch::overlapStereo(float* pOutput, const float* pInput) const
{
    int i;
    float fScale;
    float f1;
    float f2;

    fScale = 1.0f / (float)overlapLength;

    f1 = 0;
    f2 = 1.0f;

    for (i = 0; i < 2 * (int)overlapLength; i += 2)
    {
        pOutput[i + 0] = pInput[i + 0] * f1 + pMidBuffer[i + 0] * f2;
        pOutput[i + 1] = pInput[i + 1] * f1 + pMidBuffer[i + 1] * f2;

        f1 += fScale;
        f2 -= fScale;
    }
}


// Overlaps samples in 'midBuffer' with the samples in 'input'. 
void TDStretch::overlapMulti(float* pOutput, const float* pInput) const
{
    int i;
    float fScale;
    float f1;
    float f2;

    fScale = 1.0f / (float)overlapLength;

    f1 = 0;
    f2 = 1.0f;

    i = 0;
    for (int i2 = 0; i2 < overlapLength; i2++)
    {
        // note: Could optimize this slightly by taking into account that always channels > 2
        for (int c = 0; c < channels; c++)
        {
            pOutput[i] = pInput[i] * f1 + pMidBuffer[i] * f2;
            i++;
        }
        f1 += fScale;
        f2 -= fScale;
    }
}


/// Calculates overlapInMsec period length in samples.
void TDStretch::calculateOverlapLength(int overlapInMsec)
{
    int newOvl;

    assert(overlapInMsec >= 0);
    newOvl = (sampleRate * overlapInMsec) / 1000;
    if (newOvl < 16) newOvl = 16;

    // must be divisible by 8
    newOvl -= newOvl % 8;

    acceptNewOverlapLength(newOvl);
}


/// Calculate cross-correlation
double TDStretch::calcCrossCorr(const float* mixingPos, const float* compare, double& anorm)
{
    float corr;
    float norm;
    int i;

#ifdef ST_SIMD_AVOID_UNALIGNED
    // in SIMD mode skip 'mixingPos' positions that aren't aligned to 16-byte boundary
    if (((ulongptr)mixingPos) & 15) return -1e50;
#endif

    // hint compiler autovectorization that loop length is divisible by 8
    int ilength = (channels * overlapLength) & -8;

    corr = norm = 0;
    // Same routine for stereo and mono
    for (i = 0; i < ilength; i++)
    {
        corr += mixingPos[i] * compare[i];
        norm += mixingPos[i] * mixingPos[i];
    }

    anorm = norm;
    return corr / sqrt((norm < 1e-9 ? 1.0 : norm));
}


/// Update cross-correlation by accumulating "norm" coefficient by previously calculated value
double TDStretch::calcCrossCorrAccumulate(const float* mixingPos, const float* compare, double& norm)
{
    float corr;
    int i;

    corr = 0;

    // cancel first normalizer tap from previous round
    for (i = 1; i <= channels; i++)
    {
        norm -= mixingPos[-i] * mixingPos[-i];
    }

    // hint compiler autovectorization that loop length is divisible by 8
    int ilength = (channels * overlapLength) & -8;

    // Same routine for stereo and mono
    for (i = 0; i < ilength; i++)
    {
        corr += mixingPos[i] * compare[i];
    }

    // update normalizer with last samples of this round
    for (int j = 0; j < channels; j++)
    {
        i--;
        norm += mixingPos[i] * mixingPos[i];
    }

    return corr / sqrt((norm < 1e-9 ? 1.0 : norm));
}


#endif // SOUNDTOUCH_FLOAT_SAMPLES




#include <memory.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

// Define default interpolation algorithm here
TransposerBase::ALGORITHM TransposerBase::algorithm = TransposerBase::CUBIC;


// Constructor
RateTransposer::RateTransposer() : FIFOProcessor(&outputBuffer)
{
    bUseAAFilter =
#ifndef SOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER
        true;
#else
        // Disable Anti-alias filter if desirable to avoid click at rate change zero value crossover
        false;
#endif

    // Instantiates the anti-alias filter
    pAAFilter = new AAFilter(64);
    pTransposer = TransposerBase::newInstance();
    clear();
}


RateTransposer::~RateTransposer()
{
    delete pAAFilter;
    delete pTransposer;
}


/// Enables/disables the anti-alias filter. Zero to disable, nonzero to enable
void RateTransposer::enableAAFilter(bool newMode)
{
#ifndef SOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER
    // Disable Anti-alias filter if desirable to avoid click at rate change zero value crossover
    bUseAAFilter = newMode;
    clear();
#endif
}


/// Returns nonzero if anti-alias filter is enabled.
bool RateTransposer::isAAFilterEnabled() const
{
    return bUseAAFilter;
}


AAFilter* RateTransposer::getAAFilter()
{
    return pAAFilter;
}


// Sets new target iRate. Normal iRate = 1.0, smaller values represent slower 
// iRate, larger faster iRates.
void RateTransposer::setRate(double newRate)
{
    double fCutoff;

    pTransposer->setRate(newRate);

    // design a new anti-alias filter
    if (newRate > 1.0)
    {
        fCutoff = 0.5 / newRate;
    }
    else
    {
        fCutoff = 0.5 * newRate;
    }
    pAAFilter->setCutoffFreq(fCutoff);
}


// Adds 'nSamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void RateTransposer::putSamples(const SAMPLETYPE* samples, uint nSamples)
{
    processSamples(samples, nSamples);
}


// Transposes sample rate by applying anti-alias filter to prevent folding. 
// Returns amount of samples returned in the "dest" buffer.
// The maximum amount of samples that can be returned at a time is set by
// the 'set_returnBuffer_size' function.
void RateTransposer::processSamples(const SAMPLETYPE* src, uint nSamples)
{
    uint count;

    if (nSamples == 0) return;

    // Store samples to input buffer
    inputBuffer.putSamples(src, nSamples);

    // If anti-alias filter is turned off, simply transpose without applying
    // the filter
    if (bUseAAFilter == false)
    {
        count = pTransposer->transpose(outputBuffer, inputBuffer);
        return;
    }

    assert(pAAFilter);

    // Transpose with anti-alias filter
    if (pTransposer->rate < 1.0f)
    {
        // If the parameter 'Rate' value is smaller than 1, first transpose
        // the samples and then apply the anti-alias filter to remove aliasing.

        // Transpose the samples, store the result to end of "midBuffer"
        pTransposer->transpose(midBuffer, inputBuffer);

        // Apply the anti-alias filter for transposed samples in midBuffer
        pAAFilter->evaluate(outputBuffer, midBuffer);
    }
    else
    {
        // If the parameter 'Rate' value is larger than 1, first apply the
        // anti-alias filter to remove high frequencies (prevent them from folding
        // over the lover frequencies), then transpose.

        // Apply the anti-alias filter for samples in inputBuffer
        pAAFilter->evaluate(midBuffer, inputBuffer);

        // Transpose the AA-filtered samples in "midBuffer"
        pTransposer->transpose(outputBuffer, midBuffer);
    }
}


// Sets the number of channels, 1 = mono, 2 = stereo
void RateTransposer::setChannels(int nChannels)
{
    if (!verifyNumberOfChannels(nChannels) ||
        (pTransposer->numChannels == nChannels)) return;

    pTransposer->setChannels(nChannels);
    inputBuffer.setChannels(nChannels);
    midBuffer.setChannels(nChannels);
    outputBuffer.setChannels(nChannels);
}


// Clears all the samples in the object
void RateTransposer::clear()
{
    outputBuffer.clear();
    midBuffer.clear();
    inputBuffer.clear();
    pTransposer->resetRegisters();

    // prefill buffer to avoid losing first samples at beginning of stream
    int prefill = getLatency();
    inputBuffer.addSilent(prefill);
}


// Returns nonzero if there aren't any samples available for outputting.
int RateTransposer::isEmpty() const
{
    int res;

    res = FIFOProcessor::isEmpty();
    if (res == 0) return 0;
    return inputBuffer.isEmpty();
}


/// Return approximate initial input-output latency
int RateTransposer::getLatency() const
{
    return pTransposer->getLatency() +
        ((bUseAAFilter) ? (pAAFilter->getLength() / 2) : 0);
}


//////////////////////////////////////////////////////////////////////////////
//
// TransposerBase - Base class for interpolation
// 

// static function to set interpolation algorithm
void TransposerBase::setAlgorithm(TransposerBase::ALGORITHM a)
{
    TransposerBase::algorithm = a;
}


// Transposes the sample rate of the given samples using linear interpolation. 
// Returns the number of samples returned in the "dest" buffer
int TransposerBase::transpose(FIFOSampleBuffer& dest, FIFOSampleBuffer& src)
{
    int numSrcSamples = src.numSamples();
    int sizeDemand = (int)((double)numSrcSamples / rate) + 8;
    int numOutput;
    SAMPLETYPE* psrc = src.ptrBegin();
    SAMPLETYPE* pdest = dest.ptrEnd(sizeDemand);

#ifndef USE_MULTICH_ALWAYS
    if (numChannels == 1)
    {
        numOutput = transposeMono(pdest, psrc, numSrcSamples);
    }
    else if (numChannels == 2)
    {
        numOutput = transposeStereo(pdest, psrc, numSrcSamples);
    }
    else
#endif // USE_MULTICH_ALWAYS
    {
        assert(numChannels > 0);
        numOutput = transposeMulti(pdest, psrc, numSrcSamples);
    }
    dest.putSamples(numOutput);
    src.receiveSamples(numSrcSamples);
    return numOutput;
}


TransposerBase::TransposerBase()
{
    numChannels = 0;
    rate = 1.0f;
}


TransposerBase::~TransposerBase()
{
}


void TransposerBase::setChannels(int channels)
{
    numChannels = channels;
    resetRegisters();
}


void TransposerBase::setRate(double newRate)
{
    rate = newRate;
}


// static factory function
TransposerBase* TransposerBase::newInstance()
{
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
    // Notice: For integer arithmetic support only linear algorithm (due to simplest calculus)
    return ::new InterpolateLinearInteger;
#else
    switch (algorithm)
    {
    case LINEAR:
        return new InterpolateLinearFloat;

    case CUBIC:
        return new InterpolateCubic;

    case SHANNON:
        return new InterpolateShannon;

    default:
        assert(false);
        return NULL;
    }
#endif
}




#include <math.h>
#include <assert.h>


#define max(x, y) (((x) > (y)) ? (x) : (y))


PeakFinder::PeakFinder()
{
    minPos = maxPos = 0;
}


// Finds real 'top' of a peak hump from neighnourhood of the given 'peakpos'.
int PeakFinder::findTop(const float* data, int peakpos) const
{
    int i;
    int start, end;
    float refvalue;

    refvalue = data[peakpos];

    // seek within ±10 points
    start = peakpos - 10;
    if (start < minPos) start = minPos;
    end = peakpos + 10;
    if (end > maxPos) end = maxPos;

    for (i = start; i <= end; i++)
    {
        if (data[i] > refvalue)
        {
            peakpos = i;
            refvalue = data[i];
        }
    }

    // failure if max value is at edges of seek range => it's not peak, it's at slope.
    if ((peakpos == start) || (peakpos == end)) return 0;

    return peakpos;
}


// Finds 'ground level' of a peak hump by starting from 'peakpos' and proceeding
// to direction defined by 'direction' until next 'hump' after minimum value will 
// begin
int PeakFinder::findGround(const float* data, int peakpos, int direction) const
{
    int lowpos;
    int pos;
    int climb_count;
    float refvalue;
    float delta;

    climb_count = 0;
    refvalue = data[peakpos];
    lowpos = peakpos;

    pos = peakpos;

    while ((pos > minPos + 1) && (pos < maxPos - 1))
    {
        int prevpos;

        prevpos = pos;
        pos += direction;

        // calculate derivate
        delta = data[pos] - data[prevpos];
        if (delta <= 0)
        {
            // going downhill, ok
            if (climb_count)
            {
                climb_count--;  // decrease climb count
            }

            // check if new minimum found
            if (data[pos] < refvalue)
            {
                // new minimum found
                lowpos = pos;
                refvalue = data[pos];
            }
        }
        else
        {
            // going uphill, increase climbing counter
            climb_count++;
            if (climb_count > 5) break;    // we've been climbing too long => it's next uphill => quit
        }
    }
    return lowpos;
}


// Find offset where the value crosses the given level, when starting from 'peakpos' and
// proceeds to direction defined in 'direction'
int PeakFinder::findCrossingLevel(const float* data, float level, int peakpos, int direction) const
{
    float peaklevel;
    int pos;

    peaklevel = data[peakpos];
    assert(peaklevel >= level);
    pos = peakpos;
    while ((pos >= minPos) && (pos + direction < maxPos))
    {
        if (data[pos + direction] < level) return pos;   // crossing found
        pos += direction;
    }
    return -1;  // not found
}


// Calculates the center of mass location of 'data' array items between 'firstPos' and 'lastPos'
double PeakFinder::calcMassCenter(const float* data, int firstPos, int lastPos) const
{
    int i;
    float sum;
    float wsum;

    sum = 0;
    wsum = 0;
    for (i = firstPos; i <= lastPos; i++)
    {
        sum += (float)i * data[i];
        wsum += data[i];
    }

    if (wsum < 1e-6) return 0;
    return sum / wsum;
}


/// get exact center of peak near given position by calculating local mass of center
double PeakFinder::getPeakCenter(const float* data, int peakpos) const
{
    float peakLevel;            // peak level
    int crosspos1, crosspos2;   // position where the peak 'hump' crosses cutting level
    float cutLevel;             // cutting value
    float groundLevel;          // ground level of the peak
    int gp1, gp2;               // bottom positions of the peak 'hump'

    // find ground positions.
    gp1 = findGround(data, peakpos, -1);
    gp2 = findGround(data, peakpos, 1);

    peakLevel = data[peakpos];

    if (gp1 == gp2)
    {
        // avoid rounding errors when all are equal
        assert(gp1 == peakpos);
        cutLevel = groundLevel = peakLevel;
    }
    else {
        // get average of the ground levels
        groundLevel = 0.5f * (data[gp1] + data[gp2]);

        // calculate 70%-level of the peak
        cutLevel = 0.70f * peakLevel + 0.30f * groundLevel;
    }

    // find mid-level crossings
    crosspos1 = findCrossingLevel(data, cutLevel, peakpos, -1);
    crosspos2 = findCrossingLevel(data, cutLevel, peakpos, 1);

    if ((crosspos1 < 0) || (crosspos2 < 0)) return 0;   // no crossing, no peak..

    // calculate mass center of the peak surroundings
    return calcMassCenter(data, crosspos1, crosspos2);
}


double PeakFinder::detectPeak(const float* data, int aminPos, int amaxPos)
{

    int i;
    int peakpos;                // position of peak level
    double highPeak, peak;

    this->minPos = aminPos;
    this->maxPos = amaxPos;

    // find absolute peak
    peakpos = minPos;
    peak = data[minPos];
    for (i = minPos + 1; i < maxPos; i++)
    {
        if (data[i] > peak)
        {
            peak = data[i];
            peakpos = i;
        }
    }

    // Calculate exact location of the highest peak mass center
    highPeak = getPeakCenter(data, peakpos);
    peak = highPeak;

    // Now check if the highest peak were in fact harmonic of the true base beat peak 
    // - sometimes the highest peak can be Nth harmonic of the true base peak yet 
    // just a slightly higher than the true base

    for (i = 1; i < 3; i++)
    {
        double peaktmp, harmonic;
        int i1, i2;

        harmonic = (double)pow(2.0, i);
        peakpos = (int)(highPeak / harmonic + 0.5f);
        if (peakpos < minPos) break;
        peakpos = findTop(data, peakpos);   // seek true local maximum index
        if (peakpos == 0) continue;         // no local max here

        // calculate mass-center of possible harmonic peak
        peaktmp = getPeakCenter(data, peakpos);

        // accept harmonic peak if 
        // (a) it is found
        // (b) is within ±4% of the expected harmonic interval
        // (c) has at least half x-corr value of the max. peak

        double diff = harmonic * peaktmp / highPeak;
        if ((diff < 0.96) || (diff > 1.04)) continue;   // peak too afar from expected

        // now compare to highest detected peak
        i1 = (int)(highPeak + 0.5);
        i2 = (int)(peaktmp + 0.5);
        if (data[i2] >= 0.4 * data[i1])
        {
            // The harmonic is at least half as high primary peak,
            // thus use the harmonic peak instead
            peak = peaktmp;
        }
    }

    return peak;
}




// Constructor
FIFOSampleBuffer::FIFOSampleBuffer(int numChannels)
{
    assert(numChannels > 0);
    sizeInBytes = 0; // reasonable initial value
    buffer = NULL;
    bufferUnaligned = NULL;
    samplesInBuffer = 0;
    bufferPos = 0;
    channels = (uint)numChannels;
    ensureCapacity(32);     // allocate initial capacity 
}


// destructor
FIFOSampleBuffer::~FIFOSampleBuffer()
{
    delete[] bufferUnaligned;
    bufferUnaligned = NULL;
    buffer = NULL;
}


// Sets number of channels, 1 = mono, 2 = stereo
void FIFOSampleBuffer::setChannels(int numChannels)
{
    uint usedBytes;

    if (!verifyNumberOfChannels(numChannels)) return;

    usedBytes = channels * samplesInBuffer;
    channels = (uint)numChannels;
    samplesInBuffer = usedBytes / channels;
}


// if output location pointer 'bufferPos' isn't zero, 'rewinds' the buffer and
// zeroes this pointer by copying samples from the 'bufferPos' pointer 
// location on to the beginning of the buffer.
void FIFOSampleBuffer::rewind()
{
    if (buffer && bufferPos)
    {
        memmove(buffer, ptrBegin(), sizeof(SAMPLETYPE) * channels * samplesInBuffer);
        bufferPos = 0;
    }
}


// Adds 'numSamples' pcs of samples from the 'samples' memory position to 
// the sample buffer.
void FIFOSampleBuffer::putSamples(const SAMPLETYPE* samples, uint nSamples)
{
    memcpy(ptrEnd(nSamples), samples, sizeof(SAMPLETYPE) * nSamples * channels);
    samplesInBuffer += nSamples;
}


// Increases the number of samples in the buffer without copying any actual
// samples.
//
// This function is used to update the number of samples in the sample buffer
// when accessing the buffer directly with 'ptrEnd' function. Please be 
// careful though!
void FIFOSampleBuffer::putSamples(uint nSamples)
{
    uint req;

    req = samplesInBuffer + nSamples;
    ensureCapacity(req);
    samplesInBuffer += nSamples;
}


// Returns a pointer to the end of the used part of the sample buffer (i.e. 
// where the new samples are to be inserted). This function may be used for 
// inserting new samples into the sample buffer directly. Please be careful! 
//
// Parameter 'slackCapacity' tells the function how much free capacity (in
// terms of samples) there _at least_ should be, in order to the caller to
// successfully insert all the required samples to the buffer. When necessary, 
// the function grows the buffer size to comply with this requirement.
//
// When using this function as means for inserting new samples, also remember 
// to increase the sample count afterwards, by calling  the 
// 'putSamples(numSamples)' function.
SAMPLETYPE* FIFOSampleBuffer::ptrEnd(uint slackCapacity)
{
    ensureCapacity(samplesInBuffer + slackCapacity);
    return buffer + samplesInBuffer * channels;
}


// Returns a pointer to the beginning of the currently non-outputted samples. 
// This function is provided for accessing the output samples directly. 
// Please be careful!
//
// When using this function to output samples, also remember to 'remove' the
// outputted samples from the buffer by calling the 
// 'receiveSamples(numSamples)' function
SAMPLETYPE* FIFOSampleBuffer::ptrBegin()
{
    assert(buffer);
    return buffer + bufferPos * channels;
}


// Ensures that the buffer has enough capacity, i.e. space for _at least_
// 'capacityRequirement' number of samples. The buffer is grown in steps of
// 4 kilobytes to eliminate the need for frequently growing up the buffer,
// as well as to round the buffer size up to the virtual memory page size.
void FIFOSampleBuffer::ensureCapacity(uint capacityRequirement)
{
    SAMPLETYPE* tempUnaligned, * temp;

    if (capacityRequirement > getCapacity())
    {
        // enlarge the buffer in 4kbyte steps (round up to next 4k boundary)
        sizeInBytes = (capacityRequirement * channels * sizeof(SAMPLETYPE) + 4095) & (uint)-4096;
        assert(sizeInBytes % 2 == 0);
        tempUnaligned = new SAMPLETYPE[sizeInBytes / sizeof(SAMPLETYPE) + 16 / sizeof(SAMPLETYPE)];
        if (tempUnaligned == NULL)
        {
            ST_THROW_RT_ERROR("Couldn't allocate memory!\n");
        }
        // Align the buffer to begin at 16byte cache line boundary for optimal performance
        temp = (SAMPLETYPE*)SOUNDTOUCH_ALIGN_POINTER_16(tempUnaligned);
        if (samplesInBuffer)
        {
            memcpy(temp, ptrBegin(), samplesInBuffer * channels * sizeof(SAMPLETYPE));
        }
        delete[] bufferUnaligned;
        buffer = temp;
        bufferUnaligned = tempUnaligned;
        bufferPos = 0;
    }
    else
    {
        // simply rewind the buffer (if necessary)
        rewind();
    }
}


// Returns the current buffer capacity in terms of samples
uint FIFOSampleBuffer::getCapacity() const
{
    return sizeInBytes / (channels * sizeof(SAMPLETYPE));
}


// Returns the number of samples currently in the buffer
uint FIFOSampleBuffer::numSamples() const
{
    return samplesInBuffer;
}


// Output samples from beginning of the sample buffer. Copies demanded number
// of samples to output and removes them from the sample buffer. If there
// are less than 'numsample' samples in the buffer, returns all available.
//
// Returns number of samples copied.
uint FIFOSampleBuffer::receiveSamples(SAMPLETYPE* output, uint maxSamples)
{
    uint num;

    num = (maxSamples > samplesInBuffer) ? samplesInBuffer : maxSamples;

    memcpy(output, ptrBegin(), channels * sizeof(SAMPLETYPE) * num);
    return receiveSamples(num);
}


// Removes samples from the beginning of the sample buffer without copying them
// anywhere. Used to reduce the number of samples in the buffer, when accessing
// the sample buffer with the 'ptrBegin' function.
uint FIFOSampleBuffer::receiveSamples(uint maxSamples)
{
    if (maxSamples >= samplesInBuffer)
    {
        uint temp;

        temp = samplesInBuffer;
        samplesInBuffer = 0;
        return temp;
    }

    samplesInBuffer -= maxSamples;
    bufferPos += maxSamples;

    return maxSamples;
}


// Returns nonzero if the sample buffer is empty
int FIFOSampleBuffer::isEmpty() const
{
    return (samplesInBuffer == 0) ? 1 : 0;
}


// Clears the sample buffer
void FIFOSampleBuffer::clear()
{
    samplesInBuffer = 0;
    bufferPos = 0;
}


/// allow trimming (downwards) amount of samples in pipeline.
/// Returns adjusted amount of samples
uint FIFOSampleBuffer::adjustAmountOfSamples(uint numSamples)
{
    if (numSamples < samplesInBuffer)
    {
        samplesInBuffer = numSamples;
    }
    return samplesInBuffer;
}


/// Add silence to end of buffer
void FIFOSampleBuffer::addSilent(uint nSamples)
{
    memset(ptrEnd(nSamples), 0, sizeof(SAMPLETYPE) * nSamples * channels);
    samplesInBuffer += nSamples;
}



// cubic interpolation coefficients
static const float _coeffs[] =
{ -0.5f,  1.0f, -0.5f, 0.0f,
   1.5f, -2.5f,  0.0f, 1.0f,
  -1.5f,  2.0f,  0.5f, 0.0f,
   0.5f, -0.5f,  0.0f, 0.0f };


InterpolateCubic::InterpolateCubic()
{
    fract = 0;
}


void InterpolateCubic::resetRegisters()
{
    fract = 0;
}


/// Transpose mono audio. Returns number of produced output samples, and 
/// updates "srcSamples" to amount of consumed source samples
int InterpolateCubic::transposeMono(SAMPLETYPE* pdest,
    const SAMPLETYPE* psrc,
    int& srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 4;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        float out;
        const float x3 = 1.0f;
        const float x2 = (float)fract;    // x
        const float x1 = x2 * x2;           // x^2
        const float x0 = x1 * x2;           // x^3
        float y0, y1, y2, y3;

        assert(fract < 1.0);

        y0 = _coeffs[0] * x0 + _coeffs[1] * x1 + _coeffs[2] * x2 + _coeffs[3] * x3;
        y1 = _coeffs[4] * x0 + _coeffs[5] * x1 + _coeffs[6] * x2 + _coeffs[7] * x3;
        y2 = _coeffs[8] * x0 + _coeffs[9] * x1 + _coeffs[10] * x2 + _coeffs[11] * x3;
        y3 = _coeffs[12] * x0 + _coeffs[13] * x1 + _coeffs[14] * x2 + _coeffs[15] * x3;

        out = y0 * psrc[0] + y1 * psrc[1] + y2 * psrc[2] + y3 * psrc[3];

        pdest[i] = (SAMPLETYPE)out;
        i++;

        // update position fraction
        fract += rate;
        // update whole positions
        int whole = (int)fract;
        fract -= whole;
        psrc += whole;
        srcCount += whole;
    }
    srcSamples = srcCount;
    return i;
}


/// Transpose stereo audio. Returns number of produced output samples, and 
/// updates "srcSamples" to amount of consumed source samples
int InterpolateCubic::transposeStereo(SAMPLETYPE* pdest,
    const SAMPLETYPE* psrc,
    int& srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 4;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        const float x3 = 1.0f;
        const float x2 = (float)fract;    // x
        const float x1 = x2 * x2;           // x^2
        const float x0 = x1 * x2;           // x^3
        float y0, y1, y2, y3;
        float out0, out1;

        assert(fract < 1.0);

        y0 = _coeffs[0] * x0 + _coeffs[1] * x1 + _coeffs[2] * x2 + _coeffs[3] * x3;
        y1 = _coeffs[4] * x0 + _coeffs[5] * x1 + _coeffs[6] * x2 + _coeffs[7] * x3;
        y2 = _coeffs[8] * x0 + _coeffs[9] * x1 + _coeffs[10] * x2 + _coeffs[11] * x3;
        y3 = _coeffs[12] * x0 + _coeffs[13] * x1 + _coeffs[14] * x2 + _coeffs[15] * x3;

        out0 = y0 * psrc[0] + y1 * psrc[2] + y2 * psrc[4] + y3 * psrc[6];
        out1 = y0 * psrc[1] + y1 * psrc[3] + y2 * psrc[5] + y3 * psrc[7];

        pdest[2 * i] = (SAMPLETYPE)out0;
        pdest[2 * i + 1] = (SAMPLETYPE)out1;
        i++;

        // update position fraction
        fract += rate;
        // update whole positions
        int whole = (int)fract;
        fract -= whole;
        psrc += 2 * whole;
        srcCount += whole;
    }
    srcSamples = srcCount;
    return i;
}


/// Transpose multi-channel audio. Returns number of produced output samples, and 
/// updates "srcSamples" to amount of consumed source samples
int InterpolateCubic::transposeMulti(SAMPLETYPE* pdest,
    const SAMPLETYPE* psrc,
    int& srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 4;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        const float x3 = 1.0f;
        const float x2 = (float)fract;    // x
        const float x1 = x2 * x2;           // x^2
        const float x0 = x1 * x2;           // x^3
        float y0, y1, y2, y3;

        assert(fract < 1.0);

        y0 = _coeffs[0] * x0 + _coeffs[1] * x1 + _coeffs[2] * x2 + _coeffs[3] * x3;
        y1 = _coeffs[4] * x0 + _coeffs[5] * x1 + _coeffs[6] * x2 + _coeffs[7] * x3;
        y2 = _coeffs[8] * x0 + _coeffs[9] * x1 + _coeffs[10] * x2 + _coeffs[11] * x3;
        y3 = _coeffs[12] * x0 + _coeffs[13] * x1 + _coeffs[14] * x2 + _coeffs[15] * x3;

        for (int c = 0; c < numChannels; c++)
        {
            float out;
            out = y0 * psrc[c] + y1 * psrc[c + numChannels] + y2 * psrc[c + 2 * numChannels] + y3 * psrc[c + 3 * numChannels];
            pdest[0] = (SAMPLETYPE)out;
            pdest++;
        }
        i++;

        // update position fraction
        fract += rate;
        // update whole positions
        int whole = (int)fract;
        fract -= whole;
        psrc += numChannels * whole;
        srcCount += whole;
    }
    srcSamples = srcCount;
    return i;
}




//////////////////////////////////////////////////////////////////////////////
//
// InterpolateLinearInteger - integer arithmetic implementation
// 

/// fixed-point interpolation routine precision
#define SCALE    65536


// Constructor
InterpolateLinearInteger::InterpolateLinearInteger() : TransposerBase()
{
    // Notice: use local function calling syntax for sake of clarity, 
    // to indicate the fact that C++ constructor can't call virtual functions.
    resetRegisters();
    setRate(1.0f);
}


void InterpolateLinearInteger::resetRegisters()
{
    iFract = 0;
}


// Transposes the sample rate of the given samples using linear interpolation. 
// 'Mono' version of the routine. Returns the number of samples returned in 
// the "dest" buffer
int InterpolateLinearInteger::transposeMono(SAMPLETYPE* dest, const SAMPLETYPE* src, int& srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 1;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        LONG_SAMPLETYPE temp;

        assert(iFract < SCALE);

        temp = (SCALE - iFract) * src[0] + iFract * src[1];
        dest[i] = (SAMPLETYPE)(temp / SCALE);
        i++;

        iFract += iRate;

        int iWhole = iFract / SCALE;
        iFract -= iWhole * SCALE;
        srcCount += iWhole;
        src += iWhole;
    }
    srcSamples = srcCount;

    return i;
}


// Transposes the sample rate of the given samples using linear interpolation. 
// 'Stereo' version of the routine. Returns the number of samples returned in 
// the "dest" buffer
int InterpolateLinearInteger::transposeStereo(SAMPLETYPE* dest, const SAMPLETYPE* src, int& srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 1;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        LONG_SAMPLETYPE temp0;
        LONG_SAMPLETYPE temp1;

        assert(iFract < SCALE);

        temp0 = (SCALE - iFract) * src[0] + iFract * src[2];
        temp1 = (SCALE - iFract) * src[1] + iFract * src[3];
        dest[0] = (SAMPLETYPE)(temp0 / SCALE);
        dest[1] = (SAMPLETYPE)(temp1 / SCALE);
        dest += 2;
        i++;

        iFract += iRate;

        int iWhole = iFract / SCALE;
        iFract -= iWhole * SCALE;
        srcCount += iWhole;
        src += 2 * iWhole;
    }
    srcSamples = srcCount;

    return i;
}


int InterpolateLinearInteger::transposeMulti(SAMPLETYPE* dest, const SAMPLETYPE* src, int& srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 1;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        LONG_SAMPLETYPE temp, vol1;

        assert(iFract < SCALE);
        vol1 = (LONG_SAMPLETYPE)(SCALE - iFract);
        for (int c = 0; c < numChannels; c++)
        {
            temp = vol1 * src[c] + iFract * src[c + numChannels];
            dest[0] = (SAMPLETYPE)(temp / SCALE);
            dest++;
        }
        i++;

        iFract += iRate;

        int iWhole = iFract / SCALE;
        iFract -= iWhole * SCALE;
        srcCount += iWhole;
        src += iWhole * numChannels;
    }
    srcSamples = srcCount;

    return i;
}


// Sets new target iRate. Normal iRate = 1.0, smaller values represent slower 
// iRate, larger faster iRates.
void InterpolateLinearInteger::setRate(double newRate)
{
    iRate = (int)(newRate * SCALE + 0.5);
    TransposerBase::setRate(newRate);
}


//////////////////////////////////////////////////////////////////////////////
//
// InterpolateLinearFloat - floating point arithmetic implementation
// 
//////////////////////////////////////////////////////////////////////////////


// Constructor
InterpolateLinearFloat::InterpolateLinearFloat() : TransposerBase()
{
    // Notice: use local function calling syntax for sake of clarity, 
    // to indicate the fact that C++ constructor can't call virtual functions.
    resetRegisters();
    setRate(1.0);
}


void InterpolateLinearFloat::resetRegisters()
{
    fract = 0;
}


// Transposes the sample rate of the given samples using linear interpolation. 
// 'Mono' version of the routine. Returns the number of samples returned in 
// the "dest" buffer
int InterpolateLinearFloat::transposeMono(SAMPLETYPE* dest, const SAMPLETYPE* src, int& srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 1;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        double out;
        assert(fract < 1.0);

        out = (1.0 - fract) * src[0] + fract * src[1];
        dest[i] = (SAMPLETYPE)out;
        i++;

        // update position fraction
        fract += rate;
        // update whole positions
        int whole = (int)fract;
        fract -= whole;
        src += whole;
        srcCount += whole;
    }
    srcSamples = srcCount;
    return i;
}


// Transposes the sample rate of the given samples using linear interpolation. 
// 'Mono' version of the routine. Returns the number of samples returned in 
// the "dest" buffer
int InterpolateLinearFloat::transposeStereo(SAMPLETYPE* dest, const SAMPLETYPE* src, int& srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 1;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        double out0, out1;
        assert(fract < 1.0);

        out0 = (1.0 - fract) * src[0] + fract * src[2];
        out1 = (1.0 - fract) * src[1] + fract * src[3];
        dest[2 * i] = (SAMPLETYPE)out0;
        dest[2 * i + 1] = (SAMPLETYPE)out1;
        i++;

        // update position fraction
        fract += rate;
        // update whole positions
        int whole = (int)fract;
        fract -= whole;
        src += 2 * whole;
        srcCount += whole;
    }
    srcSamples = srcCount;
    return i;
}


int InterpolateLinearFloat::transposeMulti(SAMPLETYPE* dest, const SAMPLETYPE* src, int& srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 1;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        float temp, vol1, fract_float;

        vol1 = (float)(1.0 - fract);
        fract_float = (float)fract;
        for (int c = 0; c < numChannels; c++)
        {
            temp = vol1 * src[c] + fract_float * src[c + numChannels];
            *dest = (SAMPLETYPE)temp;
            dest++;
        }
        i++;

        fract += rate;

        int iWhole = (int)fract;
        fract -= iWhole;
        srcCount += iWhole;
        src += iWhole * numChannels;
    }
    srcSamples = srcCount;

    return i;
}




/// Kaiser window with beta = 2.0
/// Values scaled down by 5% to avoid overflows
static const double _kaiser8[8] =
{
   0.41778693317814,
   0.64888025049173,
   0.83508562409944,
   0.93887857733412,
   0.93887857733412,
   0.83508562409944,
   0.64888025049173,
   0.41778693317814
};


InterpolateShannon::InterpolateShannon()
{
    fract = 0;
}


void InterpolateShannon::resetRegisters()
{
    fract = 0;
}


#define PI 3.1415926536
#define sinc(x) (sin(PI * (x)) / (PI * (x)))

/// Transpose mono audio. Returns number of produced output samples, and 
/// updates "srcSamples" to amount of consumed source samples
int InterpolateShannon::transposeMono(SAMPLETYPE* pdest,
    const SAMPLETYPE* psrc,
    int& srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 8;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        double out;
        assert(fract < 1.0);

        out = psrc[0] * sinc(-3.0 - fract) * _kaiser8[0];
        out += psrc[1] * sinc(-2.0 - fract) * _kaiser8[1];
        out += psrc[2] * sinc(-1.0 - fract) * _kaiser8[2];
        if (fract < 1e-6)
        {
            out += psrc[3] * _kaiser8[3];     // sinc(0) = 1
        }
        else
        {
            out += psrc[3] * sinc(-fract) * _kaiser8[3];
        }
        out += psrc[4] * sinc(1.0 - fract) * _kaiser8[4];
        out += psrc[5] * sinc(2.0 - fract) * _kaiser8[5];
        out += psrc[6] * sinc(3.0 - fract) * _kaiser8[6];
        out += psrc[7] * sinc(4.0 - fract) * _kaiser8[7];

        pdest[i] = (SAMPLETYPE)out;
        i++;

        // update position fraction
        fract += rate;
        // update whole positions
        int whole = (int)fract;
        fract -= whole;
        psrc += whole;
        srcCount += whole;
    }
    srcSamples = srcCount;
    return i;
}


/// Transpose stereo audio. Returns number of produced output samples, and 
/// updates "srcSamples" to amount of consumed source samples
int InterpolateShannon::transposeStereo(SAMPLETYPE* pdest,
    const SAMPLETYPE* psrc,
    int& srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 8;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        double out0, out1, w;
        assert(fract < 1.0);

        w = sinc(-3.0 - fract) * _kaiser8[0];
        out0 = psrc[0] * w; out1 = psrc[1] * w;
        w = sinc(-2.0 - fract) * _kaiser8[1];
        out0 += psrc[2] * w; out1 += psrc[3] * w;
        w = sinc(-1.0 - fract) * _kaiser8[2];
        out0 += psrc[4] * w; out1 += psrc[5] * w;
        w = _kaiser8[3] * ((fract < 1e-5) ? 1.0 : sinc(-fract));   // sinc(0) = 1
        out0 += psrc[6] * w; out1 += psrc[7] * w;
        w = sinc(1.0 - fract) * _kaiser8[4];
        out0 += psrc[8] * w; out1 += psrc[9] * w;
        w = sinc(2.0 - fract) * _kaiser8[5];
        out0 += psrc[10] * w; out1 += psrc[11] * w;
        w = sinc(3.0 - fract) * _kaiser8[6];
        out0 += psrc[12] * w; out1 += psrc[13] * w;
        w = sinc(4.0 - fract) * _kaiser8[7];
        out0 += psrc[14] * w; out1 += psrc[15] * w;

        pdest[2 * i] = (SAMPLETYPE)out0;
        pdest[2 * i + 1] = (SAMPLETYPE)out1;
        i++;

        // update position fraction
        fract += rate;
        // update whole positions
        int whole = (int)fract;
        fract -= whole;
        psrc += 2 * whole;
        srcCount += whole;
    }
    srcSamples = srcCount;
    return i;
}


/// Transpose stereo audio. Returns number of produced output samples, and 
/// updates "srcSamples" to amount of consumed source samples
int InterpolateShannon::transposeMulti(SAMPLETYPE* pdest,
    const SAMPLETYPE* psrc,
    int& srcSamples)
{
    // not implemented
    assert(false);
    return 0;
}




/*****************************************************************************
 *
 * Implementation of the class 'FIRFilter'
 *
 *****************************************************************************/

FIRFilter::FIRFilter()
{
    resultDivFactor = 0;
    resultDivider = 0;
    length = 0;
    lengthDiv8 = 0;
    filterCoeffs = NULL;
    filterCoeffsStereo = NULL;
}


FIRFilter::~FIRFilter()
{
    delete[] filterCoeffs;
    delete[] filterCoeffsStereo;
}


// Usual C-version of the filter routine for stereo sound
uint FIRFilter::evaluateFilterStereo(SAMPLETYPE* dest, const SAMPLETYPE* src, uint numSamples) const
{
    int j, end;
#ifdef SOUNDTOUCH_FLOAT_SAMPLES
    // when using floating point samples, use a scaler instead of a divider
    // because division is much slower operation than multiplying.
    double dScaler = 1.0 / (double)resultDivider;
#endif
    // hint compiler autovectorization that loop length is divisible by 8
    int ilength = length & -8;

    assert((length != 0) && (length == ilength) && (src != NULL) && (dest != NULL) && (filterCoeffs != NULL));

    end = 2 * (numSamples - ilength);

#pragma omp parallel for
    for (j = 0; j < end; j += 2)
    {
        const SAMPLETYPE* ptr;
        LONG_SAMPLETYPE suml, sumr;

        suml = sumr = 0;
        ptr = src + j;

        for (int i = 0; i < ilength; i++)
        {
            suml += ptr[2 * i] * filterCoeffsStereo[2 * i];
            sumr += ptr[2 * i + 1] * filterCoeffsStereo[2 * i + 1];
        }

#ifdef SOUNDTOUCH_INTEGER_SAMPLES
        suml >>= resultDivFactor;
        sumr >>= resultDivFactor;
        // saturate to 16 bit integer limits
        suml = (suml < -32768) ? -32768 : (suml > 32767) ? 32767 : suml;
        // saturate to 16 bit integer limits
        sumr = (sumr < -32768) ? -32768 : (sumr > 32767) ? 32767 : sumr;
#endif // SOUNDTOUCH_INTEGER_SAMPLES
        dest[j] = (SAMPLETYPE)suml;
        dest[j + 1] = (SAMPLETYPE)sumr;
    }
    return numSamples - ilength;
}


// Usual C-version of the filter routine for mono sound
uint FIRFilter::evaluateFilterMono(SAMPLETYPE* dest, const SAMPLETYPE* src, uint numSamples) const
{
    int j, end;
#ifdef SOUNDTOUCH_FLOAT_SAMPLES
    // when using floating point samples, use a scaler instead of a divider
    // because division is much slower operation than multiplying.
    double dScaler = 1.0 / (double)resultDivider;
#endif

    // hint compiler autovectorization that loop length is divisible by 8
    int ilength = length & -8;

    assert(ilength != 0);

    end = numSamples - ilength;
#pragma omp parallel for
    for (j = 0; j < end; j++)
    {
        const SAMPLETYPE* pSrc = src + j;
        LONG_SAMPLETYPE sum;
        int i;

        sum = 0;
        for (i = 0; i < ilength; i++)
        {
            sum += pSrc[i] * filterCoeffs[i];
        }
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
        sum >>= resultDivFactor;
        // saturate to 16 bit integer limits
        sum = (sum < -32768) ? -32768 : (sum > 32767) ? 32767 : sum;
#endif // SOUNDTOUCH_INTEGER_SAMPLES
        dest[j] = (SAMPLETYPE)sum;
    }
    return end;
}


uint FIRFilter::evaluateFilterMulti(SAMPLETYPE* dest, const SAMPLETYPE* src, uint numSamples, uint numChannels)
{
    int j, end;

#ifdef SOUNDTOUCH_FLOAT_SAMPLES
    // when using floating point samples, use a scaler instead of a divider
    // because division is much slower operation than multiplying.
    double dScaler = 1.0 / (double)resultDivider;
#endif

    assert(length != 0);
    assert(src != NULL);
    assert(dest != NULL);
    assert(filterCoeffs != NULL);
    assert(numChannels < 16);

    // hint compiler autovectorization that loop length is divisible by 8
    int ilength = length & -8;

    end = numChannels * (numSamples - ilength);

#pragma omp parallel for
    for (j = 0; j < end; j += numChannels)
    {
        const SAMPLETYPE* ptr;
        LONG_SAMPLETYPE sums[16];
        uint c;
        int i;

        for (c = 0; c < numChannels; c++)
        {
            sums[c] = 0;
        }

        ptr = src + j;

        for (i = 0; i < ilength; i++)
        {
            SAMPLETYPE coef = filterCoeffs[i];
            for (c = 0; c < numChannels; c++)
            {
                sums[c] += ptr[0] * coef;
                ptr++;
            }
        }

        for (c = 0; c < numChannels; c++)
        {
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
            sums[c] >>= resultDivFactor;
#endif // SOUNDTOUCH_INTEGER_SAMPLES
            dest[j + c] = (SAMPLETYPE)sums[c];
        }
    }
    return numSamples - ilength;
}


// Set filter coeffiecients and length.
//
// Throws an exception if filter length isn't divisible by 8
void FIRFilter::setCoefficients(const SAMPLETYPE* coeffs, uint newLength, uint uResultDivFactor)
{
    assert(newLength > 0);
    if (newLength % 8) ST_THROW_RT_ERROR("FIR filter length not divisible by 8");

#ifdef SOUNDTOUCH_FLOAT_SAMPLES
    // scale coefficients already here if using floating samples
    double scale = 1.0 / resultDivider;
#else
    short scale = 1;
#endif

    lengthDiv8 = newLength / 8;
    length = lengthDiv8 * 8;
    assert(length == newLength);

    resultDivFactor = uResultDivFactor;
    resultDivider = (SAMPLETYPE)::pow(2.0, (int)resultDivFactor);

    delete[] filterCoeffs;
    filterCoeffs = new SAMPLETYPE[length];
    delete[] filterCoeffsStereo;
    filterCoeffsStereo = new SAMPLETYPE[length * 2];
    for (uint i = 0; i < length; i++)
    {
        filterCoeffs[i] = (SAMPLETYPE)(coeffs[i] * scale);
        // create also stereo set of filter coefficients: this allows compiler
        // to autovectorize filter evaluation much more efficiently
        filterCoeffsStereo[2 * i] = (SAMPLETYPE)(coeffs[i] * scale);
        filterCoeffsStereo[2 * i + 1] = (SAMPLETYPE)(coeffs[i] * scale);
    }
}


uint FIRFilter::getLength() const
{
    return length;
}


// Applies the filter to the given sequence of samples. 
//
// Note : The amount of outputted samples is by value of 'filter_length' 
// smaller than the amount of input samples.
uint FIRFilter::evaluate(SAMPLETYPE* dest, const SAMPLETYPE* src, uint numSamples, uint numChannels)
{
    assert(length > 0);
    assert(lengthDiv8 * 8 == length);

    if (numSamples < length) return 0;

#ifndef USE_MULTICH_ALWAYS
    if (numChannels == 1)
    {
        return evaluateFilterMono(dest, src, numSamples);
    }
    else if (numChannels == 2)
    {
        return evaluateFilterStereo(dest, src, numSamples);
    }
    else
#endif // USE_MULTICH_ALWAYS
    {
        assert(numChannels > 0);
        return evaluateFilterMulti(dest, src, numSamples, numChannels);
    }
}


// Operator 'new' is overloaded so that it automatically creates a suitable instance 
// depending on if we've a MMX-capable CPU available or not.
void* FIRFilter::operator new(size_t s)
{
    // Notice! don't use "new FIRFilter" directly, use "newInstance" to create a new instance instead!
    ST_THROW_RT_ERROR("Error in FIRFilter::new: Don't use 'new FIRFilter', use 'newInstance' member instead!");
    return newInstance();
}


FIRFilter* FIRFilter::newInstance()
{
    uint uExtensions;

    uExtensions = detectCPUextensions();

    // Check if MMX/SSE instruction set extensions supported by CPU

#ifdef SOUNDTOUCH_ALLOW_MMX
    // MMX routines available only with integer sample types
    if (uExtensions & SUPPORT_MMX)
    {
        return ::new FIRFilterMMX;
    }
    else
#endif // SOUNDTOUCH_ALLOW_MMX

#ifdef SOUNDTOUCH_ALLOW_SSE
        if (uExtensions & SUPPORT_SSE)
        {
            // SSE support
            return ::new FIRFilterSSE;
        }
        else
#endif // SOUNDTOUCH_ALLOW_SSE

        {
            // ISA optimizations not supported, use plain C version
            return ::new FIRFilter;
        }
}



#define _USE_MATH_DEFINES

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <cfloat>


// algorithm input sample block size
static const int INPUT_BLOCK_SIZE = 2048;

// decimated sample block size
static const int DECIMATED_BLOCK_SIZE = 256;

/// Target sample rate after decimation
static const int TARGET_SRATE = 1000;

/// XCorr update sequence size, update in about 200msec chunks
static const int XCORR_UPDATE_SEQUENCE = (int)(TARGET_SRATE / 5);

/// Moving average N size
static const int MOVING_AVERAGE_N = 15;

/// XCorr decay time constant, decay to half in 30 seconds
/// If it's desired to have the system adapt quicker to beat rate 
/// changes within a continuing music stream, then the 
/// 'xcorr_decay_time_constant' value can be reduced, yet that
/// can increase possibility of glitches in bpm detection.
static const double XCORR_DECAY_TIME_CONSTANT = 30.0;

/// Data overlap factor for beat detection algorithm
static const int OVERLAP_FACTOR = 4;

static const double TWOPI = (2 * M_PI);

////////////////////////////////////////////////////////////////////////////////

// Enable following define to create bpm analysis file:

//#define _CREATE_BPM_DEBUG_FILE

#define _SaveDebugData(name, a,b,c,d)
#define _SaveDebugBeatPos(name, b)

// Hamming window
void hamming(float* w, int N)
{
    for (int i = 0; i < N; i++)
    {
        w[i] = (float)(0.54 - 0.46 * cos(TWOPI * i / (N - 1)));
    }

}

////////////////////////////////////////////////////////////////////////////////
//
// IIR2_filter - 2nd order IIR filter

IIR2_filter::IIR2_filter(const double* lpf_coeffs)
{
    memcpy(coeffs, lpf_coeffs, 5 * sizeof(double));
    memset(prev, 0, sizeof(prev));
}


float IIR2_filter::update(float x)
{
    prev[0] = x;
    double y = x * coeffs[0];

    for (int i = 4; i >= 1; i--)
    {
        y += coeffs[i] * prev[i];
        prev[i] = prev[i - 1];
    }

    prev[3] = y;
    return (float)y;
}


// IIR low-pass filter coefficients, calculated with matlab/octave cheby2(2,40,0.05)
const double _LPF_coeffs[5] = { 0.00996655391939, -0.01944529148401, 0.00996655391939, 1.96867605796247, -0.96916387431724 };

////////////////////////////////////////////////////////////////////////////////

BPMDetect::BPMDetect(int numChannels, int aSampleRate) :
    beat_lpf(_LPF_coeffs)
{
    beats.reserve(250); // initial reservation to prevent frequent reallocation

    this->sampleRate = aSampleRate;
    this->channels = numChannels;

    decimateSum = 0;
    decimateCount = 0;

    // choose decimation factor so that result is approx. 1000 Hz
    decimateBy = sampleRate / TARGET_SRATE;
    if ((decimateBy <= 0) || (decimateBy * DECIMATED_BLOCK_SIZE < INPUT_BLOCK_SIZE))
    {
        ST_THROW_RT_ERROR("Too small samplerate");
    }

    // Calculate window length & starting item according to desired min & max bpms
    windowLen = (60 * sampleRate) / (decimateBy * MIN_BPM);
    windowStart = (60 * sampleRate) / (decimateBy * MAX_BPM_RANGE);

    assert(windowLen > windowStart);

    // allocate new working objects
    xcorr = new float[windowLen];
    memset(xcorr, 0, windowLen * sizeof(float));

    pos = 0;
    peakPos = 0;
    peakVal = 0;
    init_scaler = 1;
    beatcorr_ringbuffpos = 0;
    beatcorr_ringbuff = new float[windowLen];
    memset(beatcorr_ringbuff, 0, windowLen * sizeof(float));

    // allocate processing buffer
    buffer = new FIFOSampleBuffer();
    // we do processing in mono mode
    buffer->setChannels(1);
    buffer->clear();

    // calculate hamming windows
    hamw = new float[XCORR_UPDATE_SEQUENCE];
    hamming(hamw, XCORR_UPDATE_SEQUENCE);
    hamw2 = new float[XCORR_UPDATE_SEQUENCE / 2];
    hamming(hamw2, XCORR_UPDATE_SEQUENCE / 2);
}


BPMDetect::~BPMDetect()
{
    delete[] xcorr;
    delete[] beatcorr_ringbuff;
    delete[] hamw;
    delete[] hamw2;
    delete buffer;
}


/// convert to mono, low-pass filter & decimate to about 500 Hz. 
/// return number of outputted samples.
///
/// Decimation is used to remove the unnecessary frequencies and thus to reduce 
/// the amount of data needed to be processed as calculating autocorrelation 
/// function is a very-very heavy operation.
///
/// Anti-alias filtering is done simply by averaging the samples. This is really a 
/// poor-man's anti-alias filtering, but it's not so critical in this kind of application
/// (it'd also be difficult to design a high-quality filter with steep cut-off at very 
/// narrow band)
int BPMDetect::decimate(SAMPLETYPE* dest, const SAMPLETYPE* src, int numsamples)
{
    int count, outcount;
    LONG_SAMPLETYPE out;

    assert(channels > 0);
    assert(decimateBy > 0);
    outcount = 0;
    for (count = 0; count < numsamples; count++)
    {
        int j;

        // convert to mono and accumulate
        for (j = 0; j < channels; j++)
        {
            decimateSum += src[j];
        }
        src += j;

        decimateCount++;
        if (decimateCount >= decimateBy)
        {
            // Store every Nth sample only
            out = (LONG_SAMPLETYPE)(decimateSum / (decimateBy * channels));
            decimateSum = 0;
            decimateCount = 0;
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
            // check ranges for sure (shouldn't actually be necessary)
            if (out > 32767)
            {
                out = 32767;
            }
            else if (out < -32768)
            {
                out = -32768;
            }
#endif // SOUNDTOUCH_INTEGER_SAMPLES
            dest[outcount] = (SAMPLETYPE)out;
            outcount++;
        }
    }
    return outcount;
}


// Calculates autocorrelation function of the sample history buffer
void BPMDetect::updateXCorr(int process_samples)
{
    int offs;
    SAMPLETYPE* pBuffer;

    assert(buffer->numSamples() >= (uint)(process_samples + windowLen));
    assert(process_samples == XCORR_UPDATE_SEQUENCE);

    pBuffer = buffer->ptrBegin();

    // calculate decay factor for xcorr filtering
    float xcorr_decay = (float)pow(0.5, 1.0 / (XCORR_DECAY_TIME_CONSTANT * TARGET_SRATE / process_samples));

    // prescale pbuffer
    float tmp[XCORR_UPDATE_SEQUENCE];
    for (int i = 0; i < process_samples; i++)
    {
        tmp[i] = hamw[i] * hamw[i] * pBuffer[i];
    }

#pragma omp parallel for
    for (offs = windowStart; offs < windowLen; offs++)
    {
        float sum;
        int i;

        sum = 0;
        for (i = 0; i < process_samples; i++)
        {
            sum += tmp[i] * pBuffer[i + offs];  // scaling the sub-result shouldn't be necessary
        }
        xcorr[offs] *= xcorr_decay;   // decay 'xcorr' here with suitable time constant.

        xcorr[offs] += (float)fabs(sum);
    }
}


// Detect individual beat positions
void BPMDetect::updateBeatPos(int process_samples)
{
    SAMPLETYPE* pBuffer;

    assert(buffer->numSamples() >= (uint)(process_samples + windowLen));

    pBuffer = buffer->ptrBegin();
    assert(process_samples == XCORR_UPDATE_SEQUENCE / 2);

    //    static double thr = 0.0003;
    double posScale = (double)this->decimateBy / (double)this->sampleRate;
    int resetDur = (int)(0.12 / posScale + 0.5);

    // prescale pbuffer
    float tmp[XCORR_UPDATE_SEQUENCE / 2];
    for (int i = 0; i < process_samples; i++)
    {
        tmp[i] = hamw2[i] * hamw2[i] * pBuffer[i];
    }

#pragma omp parallel for
    for (int offs = windowStart; offs < windowLen; offs++)
    {
        float sum = 0;
        for (int i = 0; i < process_samples; i++)
        {
            sum += tmp[i] * pBuffer[offs + i];
        }
        beatcorr_ringbuff[(beatcorr_ringbuffpos + offs) % windowLen] += (float)((sum > 0) ? sum : 0); // accumulate only positive correlations
    }

    int skipstep = XCORR_UPDATE_SEQUENCE / OVERLAP_FACTOR;

    // compensate empty buffer at beginning by scaling coefficient
    float scale = (float)windowLen / (float)(skipstep * init_scaler);
    if (scale > 1.0f)
    {
        init_scaler++;
    }
    else
    {
        scale = 1.0f;
    }

    // detect beats
    for (int i = 0; i < skipstep; i++)
    {
        LONG_SAMPLETYPE max = 0;

        float sum = beatcorr_ringbuff[beatcorr_ringbuffpos];
        sum -= beat_lpf.update(sum);

        if (sum > peakVal)
        {
            // found new local largest value
            peakVal = sum;
            peakPos = pos;
        }
        if (pos > peakPos + resetDur)
        {
            // largest value not updated for 200msec => accept as beat
            peakPos += skipstep;
            if (peakVal > 0)
            {
                // add detected beat to end of "beats" vector
                BEAT temp = { (float)(peakPos * posScale), (float)(peakVal * scale) };
                beats.push_back(temp);
            }

            peakVal = 0;
            peakPos = pos;
        }

        beatcorr_ringbuff[beatcorr_ringbuffpos] = 0;
        pos++;
        beatcorr_ringbuffpos = (beatcorr_ringbuffpos + 1) % windowLen;
    }
}


void BPMDetect::inputSamples(const SAMPLETYPE* samples, int numSamples)
{
    SAMPLETYPE decimated[DECIMATED_BLOCK_SIZE];

    // iterate so that max INPUT_BLOCK_SAMPLES processed per iteration
    while (numSamples > 0)
    {
        int block;
        int decSamples;

        block = (numSamples > INPUT_BLOCK_SIZE) ? INPUT_BLOCK_SIZE : numSamples;

        // decimate. note that converts to mono at the same time
        decSamples = decimate(decimated, samples, block);
        samples += block * channels;
        numSamples -= block;

        buffer->putSamples(decimated, decSamples);
    }

    // when the buffer has enough samples for processing...
    int req = max(windowLen + XCORR_UPDATE_SEQUENCE, 2 * XCORR_UPDATE_SEQUENCE);
    while ((int)buffer->numSamples() >= req)
    {
        // ... update autocorrelations...
        updateXCorr(XCORR_UPDATE_SEQUENCE);
        // ...update beat position calculation...
        updateBeatPos(XCORR_UPDATE_SEQUENCE / 2);
        // ... and remove proceessed samples from the buffer
        int n = XCORR_UPDATE_SEQUENCE / OVERLAP_FACTOR;
        buffer->receiveSamples(n);
    }
}


void BPMDetect::removeBias()
{
    int i;

    // Remove linear bias: calculate linear regression coefficient
    // 1. calc mean of 'xcorr' and 'i'
    double mean_i = 0;
    double mean_x = 0;
    for (i = windowStart; i < windowLen; i++)
    {
        mean_x += xcorr[i];
    }
    mean_x /= (windowLen - windowStart);
    mean_i = 0.5 * (windowLen - 1 + windowStart);

    // 2. calculate linear regression coefficient
    double b = 0;
    double div = 0;
    for (i = windowStart; i < windowLen; i++)
    {
        double xt = xcorr[i] - mean_x;
        double xi = i - mean_i;
        b += xt * xi;
        div += xi * xi;
    }
    b /= div;

    // subtract linear regression and resolve min. value bias
    float minval = FLT_MAX;   // arbitrary large number
    for (i = windowStart; i < windowLen; i++)
    {
        xcorr[i] -= (float)(b * i);
        if (xcorr[i] < minval)
        {
            minval = xcorr[i];
        }
    }

    // subtract min.value
    for (i = windowStart; i < windowLen; i++)
    {
        xcorr[i] -= minval;
    }
}


// Calculate N-point moving average for "source" values
void MAFilter(float* dest, const float* source, int start, int end, int N)
{
    for (int i = start; i < end; i++)
    {
        int i1 = i - N / 2;
        int i2 = i + N / 2 + 1;
        if (i1 < start) i1 = start;
        if (i2 > end)   i2 = end;

        double sum = 0;
        for (int j = i1; j < i2; j++)
        {
            sum += source[j];
        }
        dest[i] = (float)(sum / (i2 - i1));
    }
}


float BPMDetect::getBpm()
{
    double peakPos;
    double coeff;
    PeakFinder peakFinder;

    // remove bias from xcorr data
    removeBias();

    coeff = 60.0 * ((double)sampleRate / (double)decimateBy);

    // save bpm debug data if debug data writing enabled
    _SaveDebugData("soundtouch-bpm-xcorr.txt", xcorr, windowStart, windowLen, coeff);

    // Smoothen by N-point moving-average
    float* data = new float[windowLen];
    memset(data, 0, sizeof(float) * windowLen);
    MAFilter(data, xcorr, windowStart, windowLen, MOVING_AVERAGE_N);

    // find peak position
    peakPos = peakFinder.detectPeak(data, windowStart, windowLen);

    // save bpm debug data if debug data writing enabled
    _SaveDebugData("soundtouch-bpm-smoothed.txt", data, windowStart, windowLen, coeff);

    delete[] data;

    assert(decimateBy != 0);
    if (peakPos < 1e-9) return 0.0; // detection failed.

    _SaveDebugBeatPos("soundtouch-detected-beats.txt", beats);

    // calculate BPM
    float bpm = (float)(coeff / peakPos);
    return (bpm >= MIN_BPM && bpm <= MAX_BPM_VALID) ? bpm : 0;
}


/// Get beat position arrays. Note: The array includes also really low beat detection values 
/// in absence of clear strong beats. Consumer may wish to filter low values away.
/// - "pos" receive array of beat positions
/// - "values" receive array of beat detection strengths
/// - max_num indicates max.size of "pos" and "values" array.  
///
/// You can query a suitable array sized by calling this with NULL in "pos" & "values".
///
/// \return number of beats in the arrays.
int BPMDetect::getBeats(float* pos, float* values, int max_num)
{
    int num = (int)beats.size();
    if ((!pos) || (!values)) return num;    // pos or values NULL, return just size

    for (int i = 0; (i < num) && (i < max_num); i++)
    {
        pos[i] = beats[i].pos;
        values[i] = beats[i].strength;
    }
    return num;
}

#define TWOPI    (2 * PI)

// define this to save AA filter coefficients to a file
// #define _DEBUG_SAVE_AAFILTER_COEFFICIENTS   1

#ifdef _DEBUG_SAVE_AAFILTER_COEFFICIENTS
#include <stdio.h>

static void _DEBUG_SAVE_AAFIR_COEFFS(SAMPLETYPE* coeffs, int len)
{
    FILE* fptr = fopen("aa_filter_coeffs.txt", "wt");
    if (fptr == NULL) return;

    for (int i = 0; i < len; i++)
    {
        double temp = coeffs[i];
        fprintf(fptr, "%lf\n", temp);
    }
    fclose(fptr);
}

#else
#define _DEBUG_SAVE_AAFIR_COEFFS(x, y)
#endif

/*****************************************************************************
 *
 * Implementation of the class 'AAFilter'
 *
 *****************************************************************************/

AAFilter::AAFilter(uint len)
{
    pFIR = FIRFilter::newInstance();
    cutoffFreq = 0.5;
    setLength(len);
}


AAFilter::~AAFilter()
{
    delete pFIR;
}


// Sets new anti-alias filter cut-off edge frequency, scaled to
// sampling frequency (nyquist frequency = 0.5).
// The filter will cut frequencies higher than the given frequency.
void AAFilter::setCutoffFreq(double newCutoffFreq)
{
    cutoffFreq = newCutoffFreq;
    calculateCoeffs();
}


// Sets number of FIR filter taps
void AAFilter::setLength(uint newLength)
{
    length = newLength;
    calculateCoeffs();
}


// Calculates coefficients for a low-pass FIR filter using Hamming window
void AAFilter::calculateCoeffs()
{
    uint i;
    double cntTemp, temp, tempCoeff, h, w;
    double wc;
    double scaleCoeff, sum;
    double* work;
    SAMPLETYPE* coeffs;

    assert(length >= 2);
    assert(length % 4 == 0);
    assert(cutoffFreq >= 0);
    assert(cutoffFreq <= 0.5);

    work = new double[length];
    coeffs = new SAMPLETYPE[length];

    wc = 2.0 * PI * cutoffFreq;
    tempCoeff = TWOPI / (double)length;

    sum = 0;
    for (i = 0; i < length; i++)
    {
        cntTemp = (double)i - (double)(length / 2);

        temp = cntTemp * wc;
        if (temp != 0)
        {
            h = sin(temp) / temp;                     // sinc function
        }
        else
        {
            h = 1.0;
        }
        w = 0.54 + 0.46 * cos(tempCoeff * cntTemp);       // hamming window

        temp = w * h;
        work[i] = temp;

        // calc net sum of coefficients 
        sum += temp;
    }

    // ensure the sum of coefficients is larger than zero
    assert(sum > 0);

    // ensure we've really designed a lowpass filter...
    assert(work[length / 2] > 0);
    assert(work[length / 2 + 1] > -1e-6);
    assert(work[length / 2 - 1] > -1e-6);

    // Calculate a scaling coefficient in such a way that the result can be
    // divided by 16384
    scaleCoeff = 16384.0f / sum;

    for (i = 0; i < length; i++)
    {
        temp = work[i] * scaleCoeff;
        // scale & round to nearest integer
        temp += (temp >= 0) ? 0.5 : -0.5;
        // ensure no overfloods
        assert(temp >= -32768 && temp <= 32767);
        coeffs[i] = (SAMPLETYPE)temp;
    }

    // Set coefficients. Use divide factor 14 => divide result by 2^14 = 16384
    pFIR->setCoefficients(coeffs, length, 14);

    _DEBUG_SAVE_AAFIR_COEFFS(coeffs, length);

    delete[] work;
    delete[] coeffs;
}


// Applies the filter to the given sequence of samples. 
// Note : The amount of outputted samples is by value of 'filter length' 
// smaller than the amount of input samples.
uint AAFilter::evaluate(SAMPLETYPE* dest, const SAMPLETYPE* src, uint numSamples, uint numChannels) const
{
    return pFIR->evaluate(dest, src, numSamples, numChannels);
}


/// Applies the filter to the given src & dest pipes, so that processed amount of
/// samples get removed from src, and produced amount added to dest 
/// Note : The amount of outputted samples is by value of 'filter length' 
/// smaller than the amount of input samples.
uint AAFilter::evaluate(FIFOSampleBuffer& dest, FIFOSampleBuffer& src) const
{
    SAMPLETYPE* pdest;
    const SAMPLETYPE* psrc;
    uint numSrcSamples;
    uint result;
    int numChannels = src.getChannels();

    assert(numChannels == dest.getChannels());

    numSrcSamples = src.numSamples();
    psrc = src.ptrBegin();
    pdest = dest.ptrEnd(numSrcSamples);
    result = pFIR->evaluate(pdest, psrc, numSrcSamples, numChannels);
    src.receiveSamples(result);
    dest.putSamples(result);

    return result;
}


uint AAFilter::getLength() const
{
    return pFIR->getLength();
}




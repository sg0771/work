

#ifndef SoundTouch_H
#define SoundTouch_H

#include <assert.h>
#include <stdlib.h>
#include <vector>

typedef unsigned int    uint;
typedef unsigned long   ulong;

// Patch for MinGW: on Win64 long is 32-bit
#ifdef _WIN64
typedef unsigned long long ulongptr;
#else
typedef ulong ulongptr;
#endif


// Helper macro for aligning pointer up to next 16-byte boundary
#define SOUNDTOUCH_ALIGN_POINTER_16(x)      ( ( (ulongptr)(x) + 15 ) & ~(ulongptr)15 )


#if (defined(__GNUC__) && !defined(ANDROID))
    // In GCC, include soundtouch_config.h made by config scritps.
    // Skip this in Android compilation that uses GCC but without configure scripts.
#include "soundtouch_config.h"
#endif


namespace soundtouch
{
    /// Max allowed number of channels
#define SOUNDTOUCH_MAX_CHANNELS     16

/// Activate these undef's to overrule the possible sampletype 
/// setting inherited from some other header file:
//#undef SOUNDTOUCH_INTEGER_SAMPLES
//#undef SOUNDTOUCH_FLOAT_SAMPLES

/// If following flag is defined, always uses multichannel processing 
/// routines also for mono and stero sound. This is for routine testing 
/// purposes; output should be same with either routines, yet disabling 
/// the dedicated mono/stereo processing routines will result in slower 
/// runtime performance so recommendation is to keep this off.
// #define USE_MULTICH_ALWAYS
#if (defined(_WIN32))
// For Android compilation: Force use of Integer samples in case that
// compilation uses soft-floating point emulation - soft-fp is way too slow
#undef  SOUNDTOUCH_FLOAT_SAMPLES
#define SOUNDTOUCH_INTEGER_SAMPLES      1
#endif

#if (defined(__SOFTFP__) && defined(ANDROID))
    // For Android compilation: Force use of Integer samples in case that
    // compilation uses soft-floating point emulation - soft-fp is way too slow
#undef  SOUNDTOUCH_FLOAT_SAMPLES
#define SOUNDTOUCH_INTEGER_SAMPLES      1
#endif

#if !(SOUNDTOUCH_INTEGER_SAMPLES || SOUNDTOUCH_FLOAT_SAMPLES)

    /// Choose either 32bit floating point or 16bit integer sampletype
    /// by choosing one of the following defines, unless this selection 
    /// has already been done in some other file.
    ////
    /// Notes:
    /// - In Windows environment, choose the sample format with the
    ///   following defines.
    /// - In GNU environment, the floating point samples are used by 
    ///   default, but integer samples can be chosen by giving the 
    ///   following switch to the configure script:
    ///       ./configure --enable-integer-samples
    ///   However, if you still prefer to select the sample format here 
    ///   also in GNU environment, then please #undef the INTEGER_SAMPLE
    ///   and FLOAT_SAMPLE defines first as in comments above.
    //#define SOUNDTOUCH_INTEGER_SAMPLES     1    //< 16bit integer samples
#define SOUNDTOUCH_FLOAT_SAMPLES       1    //< 32bit float samples

#endif

#if (_M_IX86 || __i386__ || __x86_64__ || _M_X64)
    /// Define this to allow X86-specific assembler/intrinsic optimizations. 
    /// Notice that library contains also usual C++ versions of each of these
    /// these routines, so if you're having difficulties getting the optimized 
    /// routines compiled for whatever reason, you may disable these optimizations 
    /// to make the library compile.

//#define SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS     1

/// In GNU environment, allow the user to override this setting by
/// giving the following switch to the configure script:
/// ./configure --disable-x86-optimizations
/// ./configure --enable-x86-optimizations=no
#ifdef SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS
#undef SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS
#endif
#else
    /// Always disable optimizations when not using a x86 systems.
#undef SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS

#endif

// If defined, allows the SIMD-optimized routines to skip unevenly aligned
// memory offsets that can cause performance penalty in some SIMD implementations.
// Causes slight compromise in sound quality.
// #define SOUNDTOUCH_ALLOW_NONEXACT_SIMD_OPTIMIZATION    1


#ifdef SOUNDTOUCH_INTEGER_SAMPLES
    // 16bit integer sample type
    typedef short SAMPLETYPE;
    // data type for sample accumulation: Use 32bit integer to prevent overflows
    typedef long  LONG_SAMPLETYPE;

#ifdef SOUNDTOUCH_FLOAT_SAMPLES
    // check that only one sample type is defined
#error "conflicting sample types defined"
#endif // SOUNDTOUCH_FLOAT_SAMPLES

#ifdef SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS
    // Allow MMX optimizations (not available in X64 mode)
#if (!_M_X64)
#define SOUNDTOUCH_ALLOW_MMX   1
#endif
#endif

#else

    // floating point samples
    typedef float  SAMPLETYPE;
    // data type for sample accumulation: Use float also here to enable
    // efficient autovectorization
    typedef float LONG_SAMPLETYPE;

#ifdef SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS
    // Allow SSE optimizations
#define SOUNDTOUCH_ALLOW_SSE       1
#endif

#endif  // SOUNDTOUCH_INTEGER_SAMPLES

#if ((SOUNDTOUCH_ALLOW_SSE) || (__SSE__) || (SOUNDTOUCH_USE_NEON))
#if SOUNDTOUCH_ALLOW_NONEXACT_SIMD_OPTIMIZATION
#define ST_SIMD_AVOID_UNALIGNED
#endif
#endif

};

// define ST_NO_EXCEPTION_HANDLING switch to disable throwing std exceptions:
// #define ST_NO_EXCEPTION_HANDLING    1
#ifdef ST_NO_EXCEPTION_HANDLING
    // Exceptions disabled. Throw asserts instead if enabled.
#include <assert.h>
#define ST_THROW_RT_ERROR(x)    {assert((const char *)x);}
#else
    // use c++ standard exceptions
#include <stdexcept>
#include <string>
#define ST_THROW_RT_ERROR(x)    {throw std::runtime_error(x);}
#endif



namespace soundtouch
{

    /// Abstract base class for FIFO (first-in-first-out) sample processing classes.
    class FIFOSamplePipe
    {
    protected:

        bool verifyNumberOfChannels(int nChannels) const
        {
            if ((nChannels > 0) && (nChannels <= SOUNDTOUCH_MAX_CHANNELS))
            {
                return true;
            }
            ST_THROW_RT_ERROR("Error: Illegal number of channels");
            return false;
        }

    public:
        // virtual default destructor
        virtual ~FIFOSamplePipe() {}


        /// Returns a pointer to the beginning of the output samples. 
        /// This function is provided for accessing the output samples directly. 
        /// Please be careful for not to corrupt the book-keeping!
        ///
        /// When using this function to output samples, also remember to 'remove' the
        /// output samples from the buffer by calling the 
        /// 'receiveSamples(numSamples)' function
        virtual SAMPLETYPE* ptrBegin() = 0;

        /// Adds 'numSamples' pcs of samples from the 'samples' memory position to
        /// the sample buffer.
        virtual void putSamples(const SAMPLETYPE* samples,  ///< Pointer to samples.
            uint numSamples             ///< Number of samples to insert.
        ) = 0;


        // Moves samples from the 'other' pipe instance to this instance.
        void moveSamples(FIFOSamplePipe& other  ///< Other pipe instance where from the receive the data.
        )
        {
            int oNumSamples = other.numSamples();

            putSamples(other.ptrBegin(), oNumSamples);
            other.receiveSamples(oNumSamples);
        };

        /// Output samples from beginning of the sample buffer. Copies requested samples to 
        /// output buffer and removes them from the sample buffer. If there are less than 
        /// 'numsample' samples in the buffer, returns all that available.
        ///
        /// \return Number of samples returned.
        virtual uint receiveSamples(SAMPLETYPE* output, ///< Buffer where to copy output samples.
            uint maxSamples                 ///< How many samples to receive at max.
        ) = 0;

        /// Adjusts book-keeping so that given number of samples are removed from beginning of the 
        /// sample buffer without copying them anywhere. 
        ///
        /// Used to reduce the number of samples in the buffer when accessing the sample buffer directly
        /// with 'ptrBegin' function.
        virtual uint receiveSamples(uint maxSamples   ///< Remove this many samples from the beginning of pipe.
        ) = 0;

        /// Returns number of samples currently available.
        virtual uint numSamples() const = 0;

        // Returns nonzero if there aren't any samples available for outputting.
        virtual int isEmpty() const = 0;

        /// Clears all the samples.
        virtual void clear() = 0;

        /// allow trimming (downwards) amount of samples in pipeline.
        /// Returns adjusted amount of samples
        virtual uint adjustAmountOfSamples(uint numSamples) = 0;

    };


    /// Base-class for sound processing routines working in FIFO principle. With this base 
    /// class it's easy to implement sound processing stages that can be chained together,
    /// so that samples that are fed into beginning of the pipe automatically go through 
    /// all the processing stages.
    ///
    /// When samples are input to this class, they're first processed and then put to 
    /// the FIFO pipe that's defined as output of this class. This output pipe can be
    /// either other processing stage or a FIFO sample buffer.
    class FIFOProcessor :public FIFOSamplePipe
    {
    protected:
        /// Internal pipe where processed samples are put.
        FIFOSamplePipe* output;

        /// Sets output pipe.
        void setOutPipe(FIFOSamplePipe* pOutput)
        {
            assert(output == NULL);
            assert(pOutput != NULL);
            output = pOutput;
        }

        /// Constructor. Doesn't define output pipe; it has to be set be 
        /// 'setOutPipe' function.
        FIFOProcessor()
        {
            output = NULL;
        }

        /// Constructor. Configures output pipe.
        FIFOProcessor(FIFOSamplePipe* pOutput   ///< Output pipe.
        )
        {
            output = pOutput;
        }

        /// Destructor.
        virtual ~FIFOProcessor()
        {
        }

        /// Returns a pointer to the beginning of the output samples. 
        /// This function is provided for accessing the output samples directly. 
        /// Please be careful for not to corrupt the book-keeping!
        ///
        /// When using this function to output samples, also remember to 'remove' the
        /// output samples from the buffer by calling the 
        /// 'receiveSamples(numSamples)' function
        virtual SAMPLETYPE* ptrBegin()
        {
            return output->ptrBegin();
        }

    public:

        /// Output samples from beginning of the sample buffer. Copies requested samples to 
        /// output buffer and removes them from the sample buffer. If there are less than 
        /// 'numsample' samples in the buffer, returns all that available.
        ///
        /// \return Number of samples returned.
        virtual uint receiveSamples(SAMPLETYPE* outBuffer, ///< Buffer where to copy output samples.
            uint maxSamples                    ///< How many samples to receive at max.
        )
        {
            return output->receiveSamples(outBuffer, maxSamples);
        }

        /// Adjusts book-keeping so that given number of samples are removed from beginning of the 
        /// sample buffer without copying them anywhere. 
        ///
        /// Used to reduce the number of samples in the buffer when accessing the sample buffer directly
        /// with 'ptrBegin' function.
        virtual uint receiveSamples(uint maxSamples   ///< Remove this many samples from the beginning of pipe.
        )
        {
            return output->receiveSamples(maxSamples);
        }

        /// Returns number of samples currently available.
        virtual uint numSamples() const
        {
            return output->numSamples();
        }

        /// Returns nonzero if there aren't any samples available for outputting.
        virtual int isEmpty() const
        {
            return output->isEmpty();
        }

        /// allow trimming (downwards) amount of samples in pipeline.
        /// Returns adjusted amount of samples
        virtual uint adjustAmountOfSamples(uint numSamples)
        {
            return output->adjustAmountOfSamples(numSamples);
        }
    };

}

namespace soundtouch
{

/// Soundtouch library version string
#define SOUNDTOUCH_VERSION          "2.2"

/// SoundTouch library version id
#define SOUNDTOUCH_VERSION_ID       (20200)

//
// Available setting IDs for the 'setSetting' & 'get_setting' functions:

/// Enable/disable anti-alias filter in pitch transposer (0 = disable)
#define SETTING_USE_AA_FILTER       0

/// Pitch transposer anti-alias filter length (8 .. 128 taps, default = 32)
#define SETTING_AA_FILTER_LENGTH    1

/// Enable/disable quick seeking algorithm in tempo changer routine
/// (enabling quick seeking lowers CPU utilization but causes a minor sound
///  quality compromising)
#define SETTING_USE_QUICKSEEK       2

/// Time-stretch algorithm single processing sequence length in milliseconds. This determines 
/// to how long sequences the original sound is chopped in the time-stretch algorithm. 
/// See "STTypes.h" or README for more information.
#define SETTING_SEQUENCE_MS         3

/// Time-stretch algorithm seeking window length in milliseconds for algorithm that finds the 
/// best possible overlapping location. This determines from how wide window the algorithm 
/// may look for an optimal joining location when mixing the sound sequences back together. 
/// See "STTypes.h" or README for more information.
#define SETTING_SEEKWINDOW_MS       4

/// Time-stretch algorithm overlap length in milliseconds. When the chopped sound sequences 
/// are mixed back together, to form a continuous sound stream, this parameter defines over 
/// how long period the two consecutive sequences are let to overlap each other. 
/// See "STTypes.h" or README for more information.
#define SETTING_OVERLAP_MS          5


/// Call "getSetting" with this ID to query processing sequence size in samples. 
/// This value gives approximate value of how many input samples you'll need to 
/// feed into SoundTouch after initial buffering to get out a new batch of
/// output samples. 
///
/// This value does not include initial buffering at beginning of a new processing 
/// stream, use SETTING_INITIAL_LATENCY to get the initial buffering size.
///
/// Notices: 
/// - This is read-only parameter, i.e. setSetting ignores this parameter
/// - This parameter value is not constant but change depending on 
///   tempo/pitch/rate/samplerate settings.
#define SETTING_NOMINAL_INPUT_SEQUENCE      6


/// Call "getSetting" with this ID to query nominal average processing output 
/// size in samples. This value tells approcimate value how many output samples 
/// SoundTouch outputs once it does DSP processing run for a batch of input samples.
///
/// Notices: 
/// - This is read-only parameter, i.e. setSetting ignores this parameter
/// - This parameter value is not constant but change depending on 
///   tempo/pitch/rate/samplerate settings.
#define SETTING_NOMINAL_OUTPUT_SEQUENCE     7


/// Call "getSetting" with this ID to query initial processing latency, i.e.
/// approx. how many samples you'll need to enter to SoundTouch pipeline before 
/// you can expect to get first batch of ready output samples out. 
///
/// After the first output batch, you can then expect to get approx. 
/// SETTING_NOMINAL_OUTPUT_SEQUENCE ready samples out for every
/// SETTING_NOMINAL_INPUT_SEQUENCE samples that you enter into SoundTouch.
///
/// Example:
///     processing with parameter -tempo=5
///     => initial latency = 5509 samples
///        input sequence  = 4167 samples
///        output sequence = 3969 samples
///
/// Accordingly, you can expect to feed in approx. 5509 samples at beginning of 
/// the stream, and then you'll get out the first 3969 samples. After that, for 
/// every approx. 4167 samples that you'll put in, you'll receive again approx. 
/// 3969 samples out.
///
/// This also means that average latency during stream processing is 
/// INITIAL_LATENCY-OUTPUT_SEQUENCE/2, in the above example case 5509-3969/2 
/// = 3524 samples
/// 
/// Notices: 
/// - This is read-only parameter, i.e. setSetting ignores this parameter
/// - This parameter value is not constant but change depending on 
///   tempo/pitch/rate/samplerate settings.
#define SETTING_INITIAL_LATENCY             8


class SoundTouch : public FIFOProcessor
{
private:
    /// Rate transposer class instance
    class RateTransposer *pRateTransposer;

    /// Time-stretch class instance
    class TDStretch *pTDStretch;

    /// Virtual pitch parameter. Effective rate & tempo are calculated from these parameters.
    double virtualRate;

    /// Virtual pitch parameter. Effective rate & tempo are calculated from these parameters.
    double virtualTempo;

    /// Virtual pitch parameter. Effective rate & tempo are calculated from these parameters.
    double virtualPitch;

    /// Flag: Has sample rate been set?
    bool  bSrateSet;

    /// Accumulator for how many samples in total will be expected as output vs. samples put in,
    /// considering current processing settings.
    double samplesExpectedOut;

    /// Accumulator for how many samples in total have been read out from the processing so far
    long   samplesOutput;

    /// Calculates effective rate & tempo valuescfrom 'virtualRate', 'virtualTempo' and 
    /// 'virtualPitch' parameters.
    void calcEffectiveRateAndTempo();

protected :
    /// Number of channels
    uint  channels;

    /// Effective 'rate' value calculated from 'virtualRate', 'virtualTempo' and 'virtualPitch'
    double rate;

    /// Effective 'tempo' value calculated from 'virtualRate', 'virtualTempo' and 'virtualPitch'
    double tempo;

public:
    SoundTouch();
    virtual ~SoundTouch();

    /// Get SoundTouch library version string
    static const char *getVersionString();

    /// Get SoundTouch library version Id
    static uint getVersionId();

    /// Sets new rate control value. Normal rate = 1.0, smaller values
    /// represent slower rate, larger faster rates.
    void setRate(double newRate);

    /// Sets new tempo control value. Normal tempo = 1.0, smaller values
    /// represent slower tempo, larger faster tempo.
    void setTempo(double newTempo);

    /// Sets new rate control value as a difference in percents compared
    /// to the original rate (-50 .. +100 %)
    void setRateChange(double newRate);

    /// Sets new tempo control value as a difference in percents compared
    /// to the original tempo (-50 .. +100 %)
    void setTempoChange(double newTempo);

    /// Sets new pitch control value. Original pitch = 1.0, smaller values
    /// represent lower pitches, larger values higher pitch.
    void setPitch(double newPitch);

    /// Sets pitch change in octaves compared to the original pitch  
    /// (-1.00 .. +1.00)
    void setPitchOctaves(double newPitch);

    /// Sets pitch change in semi-tones compared to the original pitch
    /// (-12 .. +12)
    void setPitchSemiTones(int newPitch);
    void setPitchSemiTones(double newPitch);

    /// Sets the number of channels, 1 = mono, 2 = stereo
    void setChannels(uint numChannels);

    /// Sets sample rate.
    void setSampleRate(uint srate);

    /// Get ratio between input and output audio durations, useful for calculating
    /// processed output duration: if you'll process a stream of N samples, then 
    /// you can expect to get out N * getInputOutputSampleRatio() samples.
    ///
    /// This ratio will give accurate target duration ratio for a full audio track, 
    /// given that the the whole track is processed with same processing parameters.
    /// 
    /// If this ratio is applied to calculate intermediate offsets inside a processing
    /// stream, then this ratio is approximate and can deviate +- some tens of milliseconds 
    /// from ideal offset, yet by end of the audio stream the duration ratio will become
    /// exact.
    ///
    /// Example: if processing with parameters "-tempo=15 -pitch=-3", the function
    /// will return value 0.8695652... Now, if processing an audio stream whose duration
    /// is exactly one million audio samples, then you can expect the processed 
    /// output duration  be 0.869565 * 1000000 = 869565 samples.
    double getInputOutputSampleRatio();

    /// Flushes the last samples from the processing pipeline to the output.
    /// Clears also the internal processing buffers.
    //
    /// Note: This function is meant for extracting the last samples of a sound
    /// stream. This function may introduce additional blank samples in the end
    /// of the sound stream, and thus it's not recommended to call this function
    /// in the middle of a sound stream.
    void flush();


    virtual void putSamples(const SAMPLETYPE *samples,uint numSamples );

    virtual uint receiveSamples(SAMPLETYPE *output,uint maxSamples);

    std::vector<SAMPLETYPE>m_temp;
    virtual void putSamples(const float* samples, uint numSamples) {
        for (int i = 0; i < numSamples;i++) {
            m_temp[i] = (SAMPLETYPE)(samples[i] * 32767.0f);
        }
        putSamples(m_temp.data(), numSamples);
    }

    virtual uint receiveSamples(float* output, uint maxSamples) {
        int numSamples = receiveSamples(m_temp.data(), maxSamples);
        if (numSamples > 0) {
            for (int i = 0; i < numSamples; i++) {
                output[i] = (float)(m_temp[i]) / 32767.0f;
            }
        }
        return numSamples;
    }

    virtual uint receiveSamples(uint maxSamples   ///< Remove this many samples from the beginning of pipe.
        );

    /// Clears all the samples in the object's output and internal processing
    /// buffers.
    virtual void clear();

    /// Changes a setting controlling the processing system behaviour. See the
    /// 'SETTING_...' defines for available setting ID's.
    /// 
    /// \return 'true' if the setting was successfully changed
    bool setSetting(int settingId,   ///< Setting ID number. see SETTING_... defines.
                    int value        ///< New setting value.
                    );

    /// Reads a setting controlling the processing system behaviour. See the
    /// 'SETTING_...' defines for available setting ID's.
    ///
    /// \return the setting value.
    int getSetting(int settingId    ///< Setting ID number, see SETTING_... defines.
                   ) const;

    /// Returns number of samples currently unprocessed.
    virtual uint numUnprocessedSamples() const;

    /// Return number of channels
    uint numChannels() const
    {
        return channels;
    }

    /// Other handy functions that are implemented in the ancestor classes (see
    /// classes 'FIFOProcessor' and 'FIFOSamplePipe')
    ///
    /// - receiveSamples() : Use this function to receive 'ready' processed samples from SoundTouch.
    /// - numSamples()     : Get number of 'ready' samples that can be received with 
    ///                      function 'receiveSamples()'
    /// - isEmpty()        : Returns nonzero if there aren't any 'ready' samples.
    /// - clear()          : Clears all samples from ready/processing buffers.
};

}




#define SUPPORT_MMX         0x0001
#define SUPPORT_3DNOW       0x0002
#define SUPPORT_ALTIVEC     0x0004
#define SUPPORT_SSE         0x0008
#define SUPPORT_SSE2        0x0010

/// Checks which instruction set extensions are supported by the CPU.
///
/// \return A bitmask of supported extensions, see SUPPORT_... defines.
uint detectCPUextensions(void);

/// Disables given set of instruction extensions. See SUPPORT_... defines.
void disableExtensions(uint wDisableMask);

namespace soundtouch
{

    /// Sample buffer working in FIFO (first-in-first-out) principle. The class takes
    /// care of storage size adjustment and data moving during input/output operations.
    ///
    /// Notice that in case of stereo audio, one sample is considered to consist of 
    /// both channel data.
    class FIFOSampleBuffer : public FIFOSamplePipe
    {
    private:
        /// Sample buffer.
        SAMPLETYPE* buffer;

        // Raw unaligned buffer memory. 'buffer' is made aligned by pointing it to first
        // 16-byte aligned location of this buffer
        SAMPLETYPE* bufferUnaligned;

        /// Sample buffer size in bytes
        uint sizeInBytes;

        /// How many samples are currently in buffer.
        uint samplesInBuffer;

        /// Channels, 1=mono, 2=stereo.
        uint channels;

        /// Current position pointer to the buffer. This pointer is increased when samples are 
        /// removed from the pipe so that it's necessary to actually rewind buffer (move data)
        /// only new data when is put to the pipe.
        uint bufferPos;

        /// Rewind the buffer by moving data from position pointed by 'bufferPos' to real 
        /// beginning of the buffer.
        void rewind();

        /// Ensures that the buffer has capacity for at least this many samples.
        void ensureCapacity(uint capacityRequirement);

        /// Returns current capacity.
        uint getCapacity() const;

    public:

        /// Constructor
        FIFOSampleBuffer(int numChannels = 2     ///< Number of channels, 1=mono, 2=stereo.
            ///< Default is stereo.
        );

        /// destructor
        ~FIFOSampleBuffer();

        /// Returns a pointer to the beginning of the output samples. 
        /// This function is provided for accessing the output samples directly. 
        /// Please be careful for not to corrupt the book-keeping!
        ///
        /// When using this function to output samples, also remember to 'remove' the
        /// output samples from the buffer by calling the 
        /// 'receiveSamples(numSamples)' function
        virtual SAMPLETYPE* ptrBegin();

        /// Returns a pointer to the end of the used part of the sample buffer (i.e. 
        /// where the new samples are to be inserted). This function may be used for 
        /// inserting new samples into the sample buffer directly. Please be careful
        /// not corrupt the book-keeping!
        ///
        /// When using this function as means for inserting new samples, also remember 
        /// to increase the sample count afterwards, by calling  the 
        /// 'putSamples(numSamples)' function.
        SAMPLETYPE* ptrEnd(
            uint slackCapacity   ///< How much free capacity (in samples) there _at least_ 
            ///< should be so that the caller can successfully insert the 
            ///< desired samples to the buffer. If necessary, the function 
            ///< grows the buffer size to comply with this requirement.
        );

        /// Adds 'numSamples' pcs of samples from the 'samples' memory position to
        /// the sample buffer.
        virtual void putSamples(const SAMPLETYPE* samples,  ///< Pointer to samples.
            uint numSamples                         ///< Number of samples to insert.
        );

        /// Adjusts the book-keeping to increase number of samples in the buffer without 
        /// copying any actual samples.
        ///
        /// This function is used to update the number of samples in the sample buffer
        /// when accessing the buffer directly with 'ptrEnd' function. Please be 
        /// careful though!
        virtual void putSamples(uint numSamples   ///< Number of samples been inserted.
        );

        /// Output samples from beginning of the sample buffer. Copies requested samples to 
        /// output buffer and removes them from the sample buffer. If there are less than 
        /// 'numsample' samples in the buffer, returns all that available.
        ///
        /// \return Number of samples returned.
        virtual uint receiveSamples(SAMPLETYPE* output, ///< Buffer where to copy output samples.
            uint maxSamples                 ///< How many samples to receive at max.
        );

        /// Adjusts book-keeping so that given number of samples are removed from beginning of the 
        /// sample buffer without copying them anywhere. 
        ///
        /// Used to reduce the number of samples in the buffer when accessing the sample buffer directly
        /// with 'ptrBegin' function.
        virtual uint receiveSamples(uint maxSamples   ///< Remove this many samples from the beginning of pipe.
        );

        /// Returns number of samples currently available.
        virtual uint numSamples() const;

        /// Sets number of channels, 1 = mono, 2 = stereo.
        void setChannels(int numChannels);

        /// Get number of channels
        int getChannels()
        {
            return channels;
        }

        /// Returns nonzero if there aren't any samples available for outputting.
        virtual int isEmpty() const;

        /// Clears all the samples.
        virtual void clear();

        /// allow trimming (downwards) amount of samples in pipeline.
        /// Returns adjusted amount of samples
        uint adjustAmountOfSamples(uint numSamples);

        /// Add silence to end of buffer
        void addSilent(uint nSamples);
    };

}


namespace soundtouch
{

    class AAFilter
    {
    protected:
        class FIRFilter* pFIR;

        /// Low-pass filter cut-off frequency, negative = invalid
        double cutoffFreq;

        /// num of filter taps
        uint length;

        /// Calculate the FIR coefficients realizing the given cutoff-frequency
        void calculateCoeffs();
    public:
        AAFilter(uint length);

        ~AAFilter();

        /// Sets new anti-alias filter cut-off edge frequency, scaled to sampling 
        /// frequency (nyquist frequency = 0.5). The filter will cut off the 
        /// frequencies than that.
        void setCutoffFreq(double newCutoffFreq);

        /// Sets number of FIR filter taps, i.e. ~filter complexity
        void setLength(uint newLength);

        uint getLength() const;

        /// Applies the filter to the given sequence of samples. 
        /// Note : The amount of outputted samples is by value of 'filter length' 
        /// smaller than the amount of input samples.
        uint evaluate(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            uint numSamples,
            uint numChannels) const;

        /// Applies the filter to the given src & dest pipes, so that processed amount of
        /// samples get removed from src, and produced amount added to dest 
        /// Note : The amount of outputted samples is by value of 'filter length' 
        /// smaller than the amount of input samples.
        uint evaluate(FIFOSampleBuffer& dest,
            FIFOSampleBuffer& src) const;

    };

}


#include <stddef.h>

namespace soundtouch
{

    class FIRFilter
    {
    protected:
        // Number of FIR filter taps
        uint length;
        // Number of FIR filter taps divided by 8
        uint lengthDiv8;

        // Result divider factor in 2^k format
        uint resultDivFactor;

        // Result divider value.
        SAMPLETYPE resultDivider;

        // Memory for filter coefficients
        SAMPLETYPE* filterCoeffs;
        SAMPLETYPE* filterCoeffsStereo;

        virtual uint evaluateFilterStereo(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            uint numSamples) const;
        virtual uint evaluateFilterMono(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            uint numSamples) const;
        virtual uint evaluateFilterMulti(SAMPLETYPE* dest, const SAMPLETYPE* src, uint numSamples, uint numChannels);

    public:
        FIRFilter();
        virtual ~FIRFilter();

        /// Operator 'new' is overloaded so that it automatically creates a suitable instance 
        /// depending on if we've a MMX-capable CPU available or not.
        static void* operator new(size_t s);

        static FIRFilter* newInstance();

        /// Applies the filter to the given sequence of samples. 
        /// Note : The amount of outputted samples is by value of 'filter_length' 
        /// smaller than the amount of input samples.
        ///
        /// \return Number of samples copied to 'dest'.
        uint evaluate(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            uint numSamples,
            uint numChannels);

        uint getLength() const;

        virtual void setCoefficients(const SAMPLETYPE* coeffs,
            uint newLength,
            uint uResultDivFactor);
    };


    // Optional subclasses that implement CPU-specific optimizations:

#ifdef SOUNDTOUCH_ALLOW_MMX

/// Class that implements MMX optimized functions exclusive for 16bit integer samples type.
    class FIRFilterMMX : public FIRFilter
    {
    protected:
        short* filterCoeffsUnalign;
        short* filterCoeffsAlign;

        virtual uint evaluateFilterStereo(short* dest, const short* src, uint numSamples) const;
    public:
        FIRFilterMMX();
        ~FIRFilterMMX();

        virtual void setCoefficients(const short* coeffs, uint newLength, uint uResultDivFactor);
    };

#endif // SOUNDTOUCH_ALLOW_MMX


#ifdef SOUNDTOUCH_ALLOW_SSE
    /// Class that implements SSE optimized functions exclusive for floating point samples type.
    class FIRFilterSSE : public FIRFilter
    {
    protected:
        float* filterCoeffsUnalign;
        float* filterCoeffsAlign;

        virtual uint evaluateFilterStereo(float* dest, const float* src, uint numSamples) const;
    public:
        FIRFilterSSE();
        ~FIRFilterSSE();

        virtual void setCoefficients(const float* coeffs, uint newLength, uint uResultDivFactor);
    };

#endif // SOUNDTOUCH_ALLOW_SSE

}



namespace soundtouch
{

    /// Minimum allowed BPM rate. Used to restrict accepted result above a reasonable limit.
#define MIN_BPM 45

/// Maximum allowed BPM rate range. Used for calculating algorithm parametrs
#define MAX_BPM_RANGE 200

/// Maximum allowed BPM rate range. Used to restrict accepted result below a reasonable limit.
#define MAX_BPM_VALID 190

////////////////////////////////////////////////////////////////////////////////

    typedef struct
    {
        float pos;
        float strength;
    } BEAT;


    class IIR2_filter
    {
        double coeffs[5];
        double prev[5];

    public:
        IIR2_filter(const double* lpf_coeffs);
        float update(float x);
    };


    /// Class for calculating BPM rate for audio data.
    class BPMDetect
    {
    protected:
        /// Auto-correlation accumulator bins.
        float* xcorr;

        /// Sample average counter.
        int decimateCount;

        /// Sample average accumulator for FIFO-like decimation.
        soundtouch::LONG_SAMPLETYPE decimateSum;

        /// Decimate sound by this coefficient to reach approx. 500 Hz.
        int decimateBy;

        /// Auto-correlation window length
        int windowLen;

        /// Number of channels (1 = mono, 2 = stereo)
        int channels;

        /// sample rate
        int sampleRate;

        /// Beginning of auto-correlation window: Autocorrelation isn't being updated for
        /// the first these many correlation bins.
        int windowStart;

        /// window functions for data preconditioning
        float* hamw;
        float* hamw2;

        // beat detection variables
        int pos;
        int peakPos;
        int beatcorr_ringbuffpos;
        int init_scaler;
        float peakVal;
        float* beatcorr_ringbuff;

        /// FIFO-buffer for decimated processing samples.
        soundtouch::FIFOSampleBuffer* buffer;

        /// Collection of detected beat positions
        //BeatCollection beats;
        std::vector<BEAT> beats;

        // 2nd order low-pass-filter
        IIR2_filter beat_lpf;

        /// Updates auto-correlation function for given number of decimated samples that 
        /// are read from the internal 'buffer' pipe (samples aren't removed from the pipe 
        /// though).
        void updateXCorr(int process_samples      /// How many samples are processed.
        );

        /// Decimates samples to approx. 500 Hz.
        ///
        /// \return Number of output samples.
        int decimate(soundtouch::SAMPLETYPE* dest,      ///< Destination buffer
            const soundtouch::SAMPLETYPE* src, ///< Source sample buffer
            int numsamples                     ///< Number of source samples.
        );

        /// Calculates amplitude envelope for the buffer of samples.
        /// Result is output to 'samples'.
        void calcEnvelope(soundtouch::SAMPLETYPE* samples,  ///< Pointer to input/output data buffer
            int numsamples                    ///< Number of samples in buffer
        );

        /// remove constant bias from xcorr data
        void removeBias();

        // Detect individual beat positions
        void updateBeatPos(int process_samples);


    public:
        /// Constructor.
        BPMDetect(int numChannels,  ///< Number of channels in sample data.
            int sampleRate    ///< Sample rate in Hz.
        );

        /// Destructor.
        virtual ~BPMDetect();

        /// Inputs a block of samples for analyzing: Envelopes the samples and then
        /// updates the autocorrelation estimation. When whole song data has been input
        /// in smaller blocks using this function, read the resulting bpm with 'getBpm' 
        /// function. 
        /// 
        /// Notice that data in 'samples' array can be disrupted in processing.
        void inputSamples(const soundtouch::SAMPLETYPE* samples,    ///< Pointer to input/working data buffer
            int numSamples                            ///< Number of samples in buffer
        );

        /// Analyzes the results and returns the BPM rate. Use this function to read result
        /// after whole song data has been input to the class by consecutive calls of
        /// 'inputSamples' function.
        ///
        /// \return Beats-per-minute rate, or zero if detection failed.
        float getBpm();

        /// Get beat position arrays. Note: The array includes also really low beat detection values 
        /// in absence of clear strong beats. Consumer may wish to filter low values away.
        /// - "pos" receive array of beat positions
        /// - "values" receive array of beat detection strengths
        /// - max_num indicates max.size of "pos" and "values" array.  
        ///
        /// You can query a suitable array sized by calling this with NULL in "pos" & "values".
        ///
        /// \return number of beats in the arrays.
        int getBeats(float* pos, float* strength, int max_num);
    };
}

namespace soundtouch
{

    class PeakFinder
    {
    protected:
        /// Min, max allowed peak positions within the data vector
        int minPos, maxPos;

        /// Calculates the mass center between given vector items.
        double calcMassCenter(const float* data, ///< Data vector.
            int firstPos,      ///< Index of first vector item belonging to the peak.
            int lastPos        ///< Index of last vector item belonging to the peak.
        ) const;

        /// Finds the data vector index where the monotoniously decreasing signal crosses the
        /// given level.
        int   findCrossingLevel(const float* data,  ///< Data vector.
            float level,        ///< Goal crossing level.
            int peakpos,        ///< Peak position index within the data vector.
            int direction       /// Direction where to proceed from the peak: 1 = right, -1 = left.
        ) const;

        // Finds real 'top' of a peak hump from neighnourhood of the given 'peakpos'.
        int findTop(const float* data, int peakpos) const;


        /// Finds the 'ground' level, i.e. smallest level between two neighbouring peaks, to right- 
        /// or left-hand side of the given peak position.
        int   findGround(const float* data,     /// Data vector.
            int peakpos,           /// Peak position index within the data vector.
            int direction          /// Direction where to proceed from the peak: 1 = right, -1 = left.
        ) const;

        /// get exact center of peak near given position by calculating local mass of center
        double getPeakCenter(const float* data, int peakpos) const;

    public:
        /// Constructor. 
        PeakFinder();

        /// Detect exact peak position of the data vector by finding the largest peak 'hump'
        /// and calculating the mass-center location of the peak hump.
        ///
        /// \return The location of the largest base harmonic peak hump.
        double detectPeak(const float* data, /// Data vector to be analyzed. The data vector has
            /// to be at least 'maxPos' items long.
            int minPos,        ///< Min allowed peak location within the vector data.
            int maxPos         ///< Max allowed peak location within the vector data.
        );
    };

}


namespace soundtouch
{

    /// Abstract base class for transposer implementations (linear, advanced vs integer, float etc)
    class TransposerBase
    {
    public:
        enum ALGORITHM {
            LINEAR = 0,
            CUBIC,
            SHANNON
        };

    protected:
        virtual int transposeMono(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples) = 0;
        virtual int transposeStereo(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples) = 0;
        virtual int transposeMulti(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples) = 0;

        static ALGORITHM algorithm;

    public:
        double rate;
        int numChannels;

        TransposerBase();
        virtual ~TransposerBase();

        virtual int transpose(FIFOSampleBuffer& dest, FIFOSampleBuffer& src);
        virtual void setRate(double newRate);
        virtual void setChannels(int channels);
        virtual int getLatency() const = 0;

        virtual void resetRegisters() = 0;

        // static factory function
        static TransposerBase* newInstance();

        // static function to set interpolation algorithm
        static void setAlgorithm(ALGORITHM a);
    };


    /// A common linear samplerate transposer class.
    ///
    class RateTransposer : public FIFOProcessor
    {
    protected:
        /// Anti-alias filter object
        AAFilter* pAAFilter;
        TransposerBase* pTransposer;

        /// Buffer for collecting samples to feed the anti-alias filter between
        /// two batches
        FIFOSampleBuffer inputBuffer;

        /// Buffer for keeping samples between transposing & anti-alias filter
        FIFOSampleBuffer midBuffer;

        /// Output sample buffer
        FIFOSampleBuffer outputBuffer;

        bool bUseAAFilter;


        /// Transposes sample rate by applying anti-alias filter to prevent folding. 
        /// Returns amount of samples returned in the "dest" buffer.
        /// The maximum amount of samples that can be returned at a time is set by
        /// the 'set_returnBuffer_size' function.
        void processSamples(const SAMPLETYPE* src,
            uint numSamples);

    public:
        RateTransposer();
        virtual ~RateTransposer();

        /// Returns the output buffer object
        FIFOSamplePipe* getOutput() { return &outputBuffer; };

        /// Return anti-alias filter object
        AAFilter* getAAFilter();

        /// Enables/disables the anti-alias filter. Zero to disable, nonzero to enable
        void enableAAFilter(bool newMode);

        /// Returns nonzero if anti-alias filter is enabled.
        bool isAAFilterEnabled() const;

        /// Sets new target rate. Normal rate = 1.0, smaller values represent slower 
        /// rate, larger faster rates.
        virtual void setRate(double newRate);

        /// Sets the number of channels, 1 = mono, 2 = stereo
        void setChannels(int channels);

        /// Adds 'numSamples' pcs of samples from the 'samples' memory position into
        /// the input of the object.
        void putSamples(const SAMPLETYPE* samples, uint numSamples);

        /// Clears all the samples in the object
        void clear();

        /// Returns nonzero if there aren't any samples available for outputting.
        int isEmpty() const;

        /// Return approximate initial input-output latency
        int getLatency() const;
    };

}


namespace soundtouch
{

    /// Default values for sound processing parameters:
    /// Notice that the default parameters are tuned for contemporary popular music 
    /// processing. For speech processing applications these parameters suit better:
    ///     #define DEFAULT_SEQUENCE_MS     40
    ///     #define DEFAULT_SEEKWINDOW_MS   15
    ///     #define DEFAULT_OVERLAP_MS      8
    ///

    /// Default length of a single processing sequence, in milliseconds. This determines to how 
    /// long sequences the original sound is chopped in the time-stretch algorithm.
    ///
    /// The larger this value is, the lesser sequences are used in processing. In principle
    /// a bigger value sounds better when slowing down tempo, but worse when increasing tempo
    /// and vice versa.
    ///
    /// Increasing this value reduces computational burden & vice versa.
    //#define DEFAULT_SEQUENCE_MS         40
#define DEFAULT_SEQUENCE_MS         USE_AUTO_SEQUENCE_LEN

/// Giving this value for the sequence length sets automatic parameter value
/// according to tempo setting (recommended)
#define USE_AUTO_SEQUENCE_LEN       0

/// Seeking window default length in milliseconds for algorithm that finds the best possible 
/// overlapping location. This determines from how wide window the algorithm may look for an 
/// optimal joining location when mixing the sound sequences back together. 
///
/// The bigger this window setting is, the higher the possibility to find a better mixing
/// position will become, but at the same time large values may cause a "drifting" artifact
/// because consequent sequences will be taken at more uneven intervals.
///
/// If there's a disturbing artifact that sounds as if a constant frequency was drifting 
/// around, try reducing this setting.
///
/// Increasing this value increases computational burden & vice versa.
//#define DEFAULT_SEEKWINDOW_MS       15
#define DEFAULT_SEEKWINDOW_MS       USE_AUTO_SEEKWINDOW_LEN

/// Giving this value for the seek window length sets automatic parameter value
/// according to tempo setting (recommended)
#define USE_AUTO_SEEKWINDOW_LEN     0

/// Overlap length in milliseconds. When the chopped sound sequences are mixed back together, 
/// to form a continuous sound stream, this parameter defines over how long period the two 
/// consecutive sequences are let to overlap each other. 
///
/// This shouldn't be that critical parameter. If you reduce the DEFAULT_SEQUENCE_MS setting 
/// by a large amount, you might wish to try a smaller value on this.
///
/// Increasing this value increases computational burden & vice versa.
#define DEFAULT_OVERLAP_MS      8


/// Class that does the time-stretch (tempo change) effect for the processed
/// sound.
    class TDStretch : public FIFOProcessor
    {
    protected:
        int channels;
        int sampleReq;

        int overlapLength;
        int seekLength;
        int seekWindowLength;
        int overlapDividerBitsNorm;
        int overlapDividerBitsPure;
        int slopingDivider;
        int sampleRate;
        int sequenceMs;
        int seekWindowMs;
        int overlapMs;

        unsigned long maxnorm;
        float maxnormf;

        double tempo;
        double nominalSkip;
        double skipFract;

        bool bQuickSeek;
        bool bAutoSeqSetting;
        bool bAutoSeekSetting;
        bool isBeginning;

        SAMPLETYPE* pMidBuffer;
        SAMPLETYPE* pMidBufferUnaligned;

        FIFOSampleBuffer outputBuffer;
        FIFOSampleBuffer inputBuffer;

        void acceptNewOverlapLength(int newOverlapLength);

        virtual void clearCrossCorrState();
        void calculateOverlapLength(int overlapMs);

        virtual double calcCrossCorr(const SAMPLETYPE* mixingPos, const SAMPLETYPE* compare, double& norm);
        virtual double calcCrossCorrAccumulate(const SAMPLETYPE* mixingPos, const SAMPLETYPE* compare, double& norm);

        virtual int seekBestOverlapPositionFull(const SAMPLETYPE* refPos);
        virtual int seekBestOverlapPositionQuick(const SAMPLETYPE* refPos);
        virtual int seekBestOverlapPosition(const SAMPLETYPE* refPos);

        virtual void overlapStereo(SAMPLETYPE* output, const SAMPLETYPE* input) const;
        virtual void overlapMono(SAMPLETYPE* output, const SAMPLETYPE* input) const;
        virtual void overlapMulti(SAMPLETYPE* output, const SAMPLETYPE* input) const;

        void clearMidBuffer();
        void overlap(SAMPLETYPE* output, const SAMPLETYPE* input, uint ovlPos) const;

        void calcSeqParameters();
        void adaptNormalizer();

        /// Changes the tempo of the given sound samples.
        /// Returns amount of samples returned in the "output" buffer.
        /// The maximum amount of samples that can be returned at a time is set by
        /// the 'set_returnBuffer_size' function.
        void processSamples();

    public:
        TDStretch();
        virtual ~TDStretch();

        /// Operator 'new' is overloaded so that it automatically creates a suitable instance 
        /// depending on if we've a MMX/SSE/etc-capable CPU available or not.
        static void* operator new(size_t s);

        /// Use this function instead of "new" operator to create a new instance of this class. 
        /// This function automatically chooses a correct feature set depending on if the CPU
        /// supports MMX/SSE/etc extensions.
        static TDStretch* newInstance();

        /// Returns the output buffer object
        FIFOSamplePipe* getOutput() { return &outputBuffer; };

        /// Returns the input buffer object
        FIFOSamplePipe* getInput() { return &inputBuffer; };

        /// Sets new target tempo. Normal tempo = 'SCALE', smaller values represent slower 
        /// tempo, larger faster tempo.
        void setTempo(double newTempo);

        /// Returns nonzero if there aren't any samples available for outputting.
        virtual void clear();

        /// Clears the input buffer
        void clearInput();

        /// Sets the number of channels, 1 = mono, 2 = stereo
        void setChannels(int numChannels);

        /// Enables/disables the quick position seeking algorithm. Zero to disable, 
        /// nonzero to enable
        void enableQuickSeek(bool enable);

        /// Returns nonzero if the quick seeking algorithm is enabled.
        bool isQuickSeekEnabled() const;

        /// Sets routine control parameters. These control are certain time constants
        /// defining how the sound is stretched to the desired duration.
        //
        /// 'sampleRate' = sample rate of the sound
        /// 'sequenceMS' = one processing sequence length in milliseconds
        /// 'seekwindowMS' = seeking window length for scanning the best overlapping 
        ///      position
        /// 'overlapMS' = overlapping length
        void setParameters(int sampleRate,          ///< Samplerate of sound being processed (Hz)
            int sequenceMS = -1,     ///< Single processing sequence length (ms)
            int seekwindowMS = -1,   ///< Offset seeking window length (ms)
            int overlapMS = -1       ///< Sequence overlapping length (ms)
        );

        /// Get routine control parameters, see setParameters() function.
        /// Any of the parameters to this function can be NULL, in such case corresponding parameter
        /// value isn't returned.
        void getParameters(int* pSampleRate, int* pSequenceMs, int* pSeekWindowMs, int* pOverlapMs) const;

        /// Adds 'numsamples' pcs of samples from the 'samples' memory position into
        /// the input of the object.
        virtual void putSamples(
            const SAMPLETYPE* samples,  ///< Input sample data
            uint numSamples                         ///< Number of samples in 'samples' so that one sample
            ///< contains both channels if stereo
        );

        /// return nominal input sample requirement for triggering a processing batch
        int getInputSampleReq() const
        {
            return (int)(nominalSkip + 0.5);
        }

        /// return nominal output sample amount when running a processing batch
        int getOutputBatchSize() const
        {
            return seekWindowLength - overlapLength;
        }

        /// return approximate initial input-output latency
        int getLatency() const
        {
            return sampleReq;
        }
    };


    // Implementation-specific class declarations:

#ifdef SOUNDTOUCH_ALLOW_MMX
    /// Class that implements MMX optimized routines for 16bit integer samples type.
    class TDStretchMMX : public TDStretch
    {
    protected:
        double calcCrossCorr(const short* mixingPos, const short* compare, double& norm);
        double calcCrossCorrAccumulate(const short* mixingPos, const short* compare, double& norm);
        virtual void overlapStereo(short* output, const short* input) const;
        virtual void clearCrossCorrState();
    };
#endif /// SOUNDTOUCH_ALLOW_MMX


#ifdef SOUNDTOUCH_ALLOW_SSE
    /// Class that implements SSE optimized routines for floating point samples type.
    class TDStretchSSE : public TDStretch
    {
    protected:
        double calcCrossCorr(const float* mixingPos, const float* compare, double& norm);
        double calcCrossCorrAccumulate(const float* mixingPos, const float* compare, double& norm);
    };

#endif /// SOUNDTOUCH_ALLOW_SSE

}


namespace soundtouch
{

    class InterpolateCubic : public TransposerBase
    {
    protected:
        virtual int transposeMono(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples);
        virtual int transposeStereo(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples);
        virtual int transposeMulti(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples);

        double fract;

    public:
        InterpolateCubic();

        virtual void resetRegisters();

        int getLatency() const
        {
            return 1;
        }
    };

}


namespace soundtouch
{

    /// Linear transposer class that uses integer arithmetic
    class InterpolateLinearInteger : public TransposerBase
    {
    protected:
        int iFract;
        int iRate;

        virtual int transposeMono(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples);
        virtual int transposeStereo(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples);
        virtual int transposeMulti(SAMPLETYPE* dest, const SAMPLETYPE* src, int& srcSamples);
    public:
        InterpolateLinearInteger();

        /// Sets new target rate. Normal rate = 1.0, smaller values represent slower 
        /// rate, larger faster rates.
        virtual void setRate(double newRate);

        virtual void resetRegisters();

        int getLatency() const
        {
            return 0;
        }
    };


    /// Linear transposer class that uses floating point arithmetic
    class InterpolateLinearFloat : public TransposerBase
    {
    protected:
        double fract;

        virtual int transposeMono(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples);
        virtual int transposeStereo(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples);
        virtual int transposeMulti(SAMPLETYPE* dest, const SAMPLETYPE* src, int& srcSamples);

    public:
        InterpolateLinearFloat();

        virtual void resetRegisters();

        int getLatency() const
        {
            return 0;
        }
    };

}

namespace soundtouch
{

    class InterpolateShannon : public TransposerBase
    {
    protected:
        int transposeMono(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples);
        int transposeStereo(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples);
        int transposeMulti(SAMPLETYPE* dest,
            const SAMPLETYPE* src,
            int& srcSamples);

        double fract;

    public:
        InterpolateShannon();

        void resetRegisters();

        int getLatency() const
        {
            return 3;
        }
    };

}

#endif

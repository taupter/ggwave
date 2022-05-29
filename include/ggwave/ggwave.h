#ifndef GGWAVE_H
#define GGWAVE_H

#ifdef GGWAVE_SHARED
#    ifdef _WIN32
#        ifdef GGWAVE_BUILD
#            define GGWAVE_API __declspec(dllexport)
#        else
#            define GGWAVE_API __declspec(dllimport)
#        endif
#    else
#        define GGWAVE_API __attribute__ ((visibility ("default")))
#    endif
#else
#    define GGWAVE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

    //
    // C interface
    //

    // Data format of the audio samples
    typedef enum {
        GGWAVE_SAMPLE_FORMAT_UNDEFINED,
        GGWAVE_SAMPLE_FORMAT_U8,
        GGWAVE_SAMPLE_FORMAT_I8,
        GGWAVE_SAMPLE_FORMAT_U16,
        GGWAVE_SAMPLE_FORMAT_I16,
        GGWAVE_SAMPLE_FORMAT_F32,
    } ggwave_SampleFormat;

    // TxProtocol ids
    typedef enum {
        GGWAVE_TX_PROTOCOL_AUDIBLE_NORMAL,
        GGWAVE_TX_PROTOCOL_AUDIBLE_FAST,
        GGWAVE_TX_PROTOCOL_AUDIBLE_FASTEST,
        GGWAVE_TX_PROTOCOL_ULTRASOUND_NORMAL,
        GGWAVE_TX_PROTOCOL_ULTRASOUND_FAST,
        GGWAVE_TX_PROTOCOL_ULTRASOUND_FASTEST,
        GGWAVE_TX_PROTOCOL_DT_NORMAL,
        GGWAVE_TX_PROTOCOL_DT_FAST,
        GGWAVE_TX_PROTOCOL_DT_FASTEST,

        GGWAVE_TX_PROTOCOL_CUSTOM_0,
        GGWAVE_TX_PROTOCOL_CUSTOM_1,
        GGWAVE_TX_PROTOCOL_CUSTOM_2,
        GGWAVE_TX_PROTOCOL_CUSTOM_3,
        GGWAVE_TX_PROTOCOL_CUSTOM_4,
        GGWAVE_TX_PROTOCOL_CUSTOM_5,
        GGWAVE_TX_PROTOCOL_CUSTOM_6,
        GGWAVE_TX_PROTOCOL_CUSTOM_7,
        GGWAVE_TX_PROTOCOL_CUSTOM_8,
        GGWAVE_TX_PROTOCOL_CUSTOM_9,
    } ggwave_TxProtocolId;

    // Operating modes of ggwave
    typedef enum {
        GGWAVE_OPERATING_MODE_RX            = 1 << 1,
        GGWAVE_OPERATING_MODE_TX            = 1 << 2,
        GGWAVE_OPERATING_MODE_RX_AND_TX     = (GGWAVE_OPERATING_MODE_RX |
                                               GGWAVE_OPERATING_MODE_TX),
        GGWAVE_OPERATING_MODE_TX_ONLY_TONES = 1 << 3,
    } ggwave_OperatingMode;

    // GGWave instance parameters
    //
    //   If payloadLength <= 0, then GGWave will transmit with variable payload length
    //   depending on the provided payload. Sound markers are used to identify the
    //   start and end of the transmission.
    //
    //   If payloadLength > 0, then the transmitted payload will be of the specified
    //   fixed length. In this case, no sound markers are emitted and a slightly
    //   different decoding scheme is applied. This is useful in cases where the
    //   length of the payload is known in advance.
    //
    //   The sample rates are values typically between 1000 and 96000.
    //   Default value: GGWave::kDefaultSampleRate
    //
    //   The captured audio is resampled to the specified sampleRate if sampleRatInp
    //   is different from sampleRate. Same applies to the transmitted audio.
    //
    //   The samplesPerFrame is the number of samples on which ggwave performs FFT.
    //   This affects the number of bins in the Fourier spectrum.
    //   Default value: GGWave::kDefaultSamplesPerFrame
    //
    //   The operatingMode controls which functions of the ggwave instance are enabled.
    //   Use this parameter to reduce the memory footprint of the ggwave instance. For
    //   example, if only Rx is enabled, then the memory buffers needed for the Tx will
    //   not be allocated.
    //
    typedef struct {
        int payloadLength;                      // payload length
        float sampleRateInp;                    // capture sample rate
        float sampleRateOut;                    // playback sample rate
        float sampleRate;                       // the operating sample rate
        int samplesPerFrame;                    // number of samples per audio frame
        float soundMarkerThreshold;             // sound marker detection threshold
        ggwave_SampleFormat sampleFormatInp;    // format of the captured audio samples
        ggwave_SampleFormat sampleFormatOut;    // format of the playback audio samples
        ggwave_OperatingMode operatingMode;     // operating mode
    } ggwave_Parameters;

    // GGWave instances are identified with an integer and are stored
    // in a private map container. Using void * caused some issues with
    // the python module and unfortunately had to do it this way
    typedef int ggwave_Instance;

    // Change file stream for internal ggwave logging. NULL - disable logging
    //
    //   Intentionally passing it as void * instead of FILE * to avoid including a header
    //
    //     // log to standard error
    //     ggwave_setLogFile(stderr);
    //
    //     // log to standard output
    //     ggwave_setLogFile(stdout);
    //
    //     // disable logging
    //     ggwave_setLogFile(NULL);
    //
    //  Note: not thread-safe. Do not call while any GGWave instances are running
    //
    GGWAVE_API void ggwave_setLogFile(void * fptr);

    // Helper method to get default instance parameters
    GGWAVE_API ggwave_Parameters ggwave_getDefaultParameters(void);

    // Create a new GGWave instance with the specified parameters
    //
    //   The newly created instance is added to the internal map container.
    //   This function returns an id that can be used to identify this instance.
    //   Make sure to deallocate the instance at the end by calling ggwave_free()
    //
    GGWAVE_API ggwave_Instance ggwave_init(const ggwave_Parameters parameters);

    // Free a GGWave instance
    GGWAVE_API void ggwave_free(ggwave_Instance instance);

    // Encode data into audio waveform
    //
    //   instance       - the GGWave instance to use
    //   dataBuffer     - the data to encode
    //   dataSize       - number of bytes in the input dataBuffer
    //   txProtocolId   - the protocol to use for encoding
    //   volume         - the volume of the generated waveform [0, 100]
    //                    usually 25 is OK and you should not go over 50
    //   outputBuffer   - the generated audio waveform. must be big enough to fit the generated data
    //   query          - if != 0, do not perform encoding.
    //                    if == 1, return waveform size in bytes
    //                    if != 1, return waveform size in samples
    //
    //   returns the number of generated bytes or samples (see query)
    //
    //   returns -1 if there was an error
    //
    //   This function can be used to encode some binary data (payload) into an audio waveform.
    //
    //     payload -> waveform
    //
    //   When calling it, make sure that the outputBuffer is big enough to store the
    //   generated waveform. This means that its size must be at least:
    //
    //     nSamples*sizeOfSample_bytes
    //
    //   Where nSamples is the number of audio samples in the waveform and sizeOfSample_bytes
    //   is the size of a single sample in bytes based on the sampleFormatOut parameter
    //   specified during the initialization of the GGWave instance.
    //
    //   If query != 0, then this function does not perform the actual encoding and just
    //   outputs the expected size of the waveform that would be generated if you call it
    //   with query == 0. This mechanism can be used to ask ggwave how much memory to
    //   allocate for the outputBuffer. For example:
    //
    //     // this is the data to encode
    //     const char * payload = "test";
    //
    //     // query the number of bytes in the waveform
    //     int n = ggwave_encode(instance, payload, 4, GGWAVE_TX_PROTOCOL_AUDIBLE_FAST, 25, NULL, 1);
    //
    //     // allocate the output buffer
    //     char waveform[n];
    //
    //     // generate the waveform
    //     ggwave_encode(instance, payload, 4, GGWAVE_TX_PROTOCOL_AUDIBLE_FAST, 25, waveform, 0);
    //
    //   The dataBuffer can be any binary data that you would like to transmit (i.e. the payload).
    //   Usually, this is some text, but it can be any sequence of bytes.
    //
    //   todo:
    //      - change the type of dataBuffer to const void *
    //      - change the type of outputBuffer to void *
    //      - rename dataBuffer to payloadBuffer
    //      - rename dataSize to payloadSize
    //      - rename outputBuffer to waveformBuffer
    //
    GGWAVE_API int ggwave_encode(
            ggwave_Instance instance,
            const char * dataBuffer,
            int dataSize,
            ggwave_TxProtocolId txProtocolId,
            int volume,
            char * outputBuffer,
            int query);

    // Decode an audio waveform into data
    //
    //   instance       - the GGWave instance to use
    //   dataBuffer     - the audio waveform
    //   dataSize       - number of bytes in the input dataBuffer
    //   outputBuffer   - stores the decoded data on success
    //                    the maximum size of the output is GGWave::kMaxDataSize
    //
    //   returns the number of decoded bytes
    //
    //   Use this function to continuously provide audio samples to a GGWave instance.
    //   On each call, GGWave will analyze the provided data and if it detects a payload,
    //   it will return a non-zero result.
    //
    //     waveform -> payload
    //
    //   If the return value is -1 then there was an error during the decoding process.
    //   Usually can occur if there is a lot of background noise in the audio.
    //
    //   If the return value is greater than 0, then there are that number of bytes decoded.
    //
    //   IMPORTANT:
    //   Notice that the decoded data written to the outputBuffer is NOT null terminated.
    //
    //   Example:
    //
    //     char payload[256];
    //
    //     while (true) {
    //         ... capture samplesPerFrame audio samples into waveform ...
    //
    //         int ret = ggwave_decode(instance, waveform, samplesPerFrame*sizeOfSample_bytes, payload);
    //         if (ret > 0) {
    //             printf("Received payload: '%s'\n", payload);
    //         }
    //     }
    //
    //   todo:
    //      - change the type of dataBuffer to const void *
    //      - change the type of outputBuffer to void *
    //      - rename dataBuffer to waveformBuffer
    //      - rename dataSize to waveformSize
    //      - rename outputBuffer to payloadBuffer
    //
    GGWAVE_API int ggwave_decode(
            ggwave_Instance instance,
            const char * dataBuffer,
            int dataSize,
            char * outputBuffer);

    // Memory-safe overload of ggwave_decode
    //
    //   outputSize     - optionally specify the size of the output buffer
    //
    //   If the return value is -2 then the provided outputBuffer was not big enough to
    //   store the decoded data.
    //
    //   See ggwave_decode for more information
    //
    GGWAVE_API int ggwave_ndecode(
            ggwave_Instance instance,
            const char * dataBuffer,
            int dataSize,
            char * outputBuffer,
            int outputSize);

    // Toggle Rx protocols on and off
    //
    //   instance       - the GGWave instance to use
    //   rxProtocolId   - Id of the Rx protocol to modify
    //   state          - 0 - disable, 1 - enable
    //
    //   If an Rx protocol is enabled, the GGWave instance will attempt to decode received
    //   data using this protocol. By default, all protocols are enabled.
    //   Use this function to restrict the number of Rx protocols used in the decoding
    //   process. This helps to reduce the number of false positives and improves the transmission
    //   accuracy, especially when the Tx/Rx protocol is known in advance.
    //
    GGWAVE_API void ggwave_toggleRxProtocol(
            ggwave_Instance instance,
            ggwave_TxProtocolId rxProtocolId,
            int state);

#ifdef __cplusplus
}

//
// C++ interface
//

#include <cstdint>
#include <functional>
#include <vector>
#include <map>
#include <memory>

class GGWave {
public:
    static constexpr auto kSampleRateMin               = 1000.0f;
    static constexpr auto kSampleRateMax               = 96000.0f;
    static constexpr auto kDefaultSampleRate           = 48000.0f;
    static constexpr auto kDefaultSamplesPerFrame      = 1024;
    static constexpr auto kDefaultVolume               = 10;
    static constexpr auto kDefaultSoundMarkerThreshold = 3.0f;
    static constexpr auto kDefaultMarkerFrames         = 16;
    static constexpr auto kDefaultEncodedDataOffset    = 3;
    static constexpr auto kMaxSamplesPerFrame          = 1024;
    static constexpr auto kMaxDataSize                 = 256;
    static constexpr auto kMaxLengthVariable           = 140;
    static constexpr auto kMaxLengthFixed              = 16;
    static constexpr auto kMaxSpectrumHistory          = 4;
    static constexpr auto kMaxRecordedFrames           = 2048;

    using Parameters    = ggwave_Parameters;
    using SampleFormat  = ggwave_SampleFormat;
    using TxProtocolId  = ggwave_TxProtocolId;
    using RxProtocolId  = ggwave_TxProtocolId;
    using OperatingMode = ggwave_OperatingMode;

    struct TxProtocol {
        const char * name;  // string identifier of the protocol

        int freqStart;      // FFT bin index of the lowest frequency
        int framesPerTx;    // number of frames to transmit a single chunk of data
        int bytesPerTx;     // number of bytes in a chunk of data

        int nDataBitsPerTx() const { return 8*bytesPerTx; }

        bool operator==(const TxProtocol & other) const {
            return freqStart == other.freqStart &&
                   framesPerTx == other.framesPerTx &&
                   bytesPerTx == other.bytesPerTx;
        }

        bool operator!=(const TxProtocol & other) const {
            return !(*this == other);
        }
    };

    using RxProtocol = TxProtocol;

    using TxProtocols = std::map<TxProtocolId, TxProtocol>;
    using RxProtocols = std::map<RxProtocolId, RxProtocol>;

    static const TxProtocols & getTxProtocols() {
        static const TxProtocols kTxProtocols {
            { GGWAVE_TX_PROTOCOL_AUDIBLE_NORMAL,        { "Normal",       40,  9, 3, } },
            { GGWAVE_TX_PROTOCOL_AUDIBLE_FAST,          { "Fast",         40,  6, 3, } },
            { GGWAVE_TX_PROTOCOL_AUDIBLE_FASTEST,       { "Fastest",      40,  3, 3, } },
            { GGWAVE_TX_PROTOCOL_ULTRASOUND_NORMAL,     { "[U] Normal",   320, 9, 3, } },
            { GGWAVE_TX_PROTOCOL_ULTRASOUND_FAST,       { "[U] Fast",     320, 6, 3, } },
            { GGWAVE_TX_PROTOCOL_ULTRASOUND_FASTEST,    { "[U] Fastest",  320, 3, 3, } },
            { GGWAVE_TX_PROTOCOL_DT_NORMAL,             { "[DT] Normal",  24,  9, 1, } },
            { GGWAVE_TX_PROTOCOL_DT_FAST,               { "[DT] Fast",    24,  6, 1, } },
            { GGWAVE_TX_PROTOCOL_DT_FASTEST,            { "[DT] Fastest", 24,  3, 1, } },
        };

        return kTxProtocols;
    }

    struct ToneData {
        double freq_hz;
        double duration_ms;
    };

    using Tones         = std::vector<ToneData>;
    using WaveformTones = std::vector<Tones>;

    using AmplitudeData    = std::vector<float>;
    using AmplitudeDataI16 = std::vector<int16_t>;
    using SpectrumData     = std::vector<float>;
    using RecordedData     = std::vector<float>;
    using TxRxData         = std::vector<uint8_t>;

    using CBWaveformOut = std::function<void(const void * data, uint32_t nBytes)>;
    using CBWaveformInp = std::function<uint32_t(void * data, uint32_t nMaxBytes)>;

    GGWave(const Parameters & parameters);
    ~GGWave();

    // set file stream for the internal ggwave logging
    //
    //  By default, ggwave prints internal log messages to stderr.
    //  To disable logging all together, call this method with nullptr.
    //
    //  Note: not thread-safe. Do not call while any GGWave instances are running
    //
    static void setLogFile(FILE * fptr);

    static const Parameters & getDefaultParameters();

    // set Tx data to encode
    //
    //  This prepares the GGWave instance for transmission.
    //  To perform the actual encoding, the encode() method must be called
    //
    //  returns false upon invalid parameters or failure to initialize
    //
    bool init(const char * text, const int volume = kDefaultVolume);
    bool init(const char * text, const TxProtocol & txProtocol, const int volume = kDefaultVolume);
    bool init(int dataSize, const char * dataBuffer, const int volume = kDefaultVolume);
    bool init(int dataSize, const char * dataBuffer, const TxProtocol & txProtocol, const int volume = kDefaultVolume);

    // expected waveform size of the encoded Tx data in bytes
    //
    //   When the output sampling rate is not equal to operating sample rate the result of this method is overestimation of
    //   the actual number of bytes that would be produced
    //
    uint32_t encodeSize_bytes() const;

    // expected waveform size of the encoded Tx data in samples
    //
    //   When the output sampling rate is not equal to operating sample rate the result of this method is overestimation of
    //   the actual number of samples that would be produced
    //
    uint32_t encodeSize_samples() const;

    // encode Tx data into an audio waveform
    //
    //   The generated waveform is returned by calling the cbWaveformOut callback.
    //
    //   returns false if the encoding fails
    //
    bool encode(const CBWaveformOut & cbWaveformOut);

    // decode an audio waveform
    //
    //   This methods calls cbWaveformInp multiple times (at least once) until it returns 0.
    //   Use the Rx methods to check if any data was decoded successfully.
    //
    void decode(const CBWaveformInp & cbWaveformInp);

    // instance state
    const bool & hasTxData() const;

    const int & getSamplesPerFrame()    const;
    const int & getSampleSizeBytesInp() const;
    const int & getSampleSizeBytesOut() const;

    const float & getSampleRateInp() const;
    const float & getSampleRateOut() const;
    const SampleFormat & getSampleFormatInp() const;
    const SampleFormat & getSampleFormatOut() const;

    // Tx
    static TxProtocolId getDefaultTxProtocolId()     { return GGWAVE_TX_PROTOCOL_AUDIBLE_FAST; }
    static const TxProtocol & getDefaultTxProtocol() { return getTxProtocols().at(getDefaultTxProtocolId()); }
    static const TxProtocol & getTxProtocol(int id)  { return getTxProtocols().at(TxProtocolId(id)); }
    static const TxProtocol & getTxProtocol(TxProtocolId id) { return getTxProtocols().at(id); }

    // get a list of the tones generated for the last waveform
    //
    //   Call this method after calling encode() to get a list of the tones participating in the generated waveform
    //
    const WaveformTones & getWaveformTones() const;

    bool takeTxAmplitudeI16(AmplitudeDataI16 & dst);

    // Rx
    const bool & isReceiving() const;
    const bool & isAnalyzing() const;

    const int & getFramesToRecord()      const;
    const int & getFramesLeftToRecord()  const;
    const int & getFramesToAnalyze()     const;
    const int & getFramesLeftToAnalyze() const;

    bool stopReceiving();
    void setRxProtocols(const RxProtocols & rxProtocols);
    const RxProtocols & getRxProtocols() const;

    int lastRxDataLength() const;

    const TxRxData & getRxData()           const;
    const RxProtocol & getRxProtocol()     const;
    const RxProtocolId & getRxProtocolId() const;

    int takeRxData(TxRxData & dst);
    bool takeRxSpectrum(SpectrumData & dst);
    bool takeRxAmplitude(AmplitudeData & dst);

    // compute FFT of real values
    //
    //   src - input real-valued data, size is N
    //   dst - output complex-valued data, size is 2*N
    //
    //   d is scaling factor
    //   N must be <= kMaxSamplesPerFrame
    //
    static bool computeFFTR(const float * src, float * dst, int N, float d);

    // resample audio waveforms from one sample rate to another using
    // sinc interpolation
    class Resampler {
    public:
        // this controls the number of neighboring samples
        // which are used to interpolate the new samples. The
        // processing time is linearly related to this width
        static const int kWidth = 64;

        Resampler();

        void reset();

        int nSamplesTotal() const { return m_state.nSamplesTotal; }

        int resample(
                float factor,
                int nSamples,
                const float * samplesInp,
                float * samplesOut);

    private:
        float getData(int j) const;
        void newData(float data);
        void makeSinc();
        double sinc(double x) const;

        static const int kDelaySize = 140;

        // this defines how finely the sinc function is sampled for storage in the table
        static const int kSamplesPerZeroCrossing = 32;

        std::vector<float> m_sincTable;
        std::vector<float> m_delayBuffer;
        std::vector<float> m_edgeSamples;
        std::vector<float> m_samplesInp;

        struct State {
            int nSamplesTotal = 0;
            int timeInt = 0;
            int timeLast = 0;
            double timeNow = 0.0;
        };

        State m_state;
    };

private:
    void decode_fixed();
    void decode_variable();

    int maxFramesPerTx() const;
    int minBytesPerTx() const;
    int maxBytesPerTx() const;

    double bitFreq(const TxProtocol & p, int bit) const {
        return m_hzPerSample*p.freqStart + m_freqDelta_hz*bit;
    }

    const float m_sampleRateInp;
    const float m_sampleRateOut;
    const float m_sampleRate;
    const int m_samplesPerFrame;
    const float m_isamplesPerFrame;
    const int m_sampleSizeBytesInp;
    const int m_sampleSizeBytesOut;
    const SampleFormat m_sampleFormatInp;
    const SampleFormat m_sampleFormatOut;

    const float m_hzPerSample;
    const float m_ihzPerSample;

    const int m_freqDelta_bin;
    const float m_freqDelta_hz;

    const int m_nBitsInMarker;
    const int m_nMarkerFrames;
    const int m_encodedDataOffset;

    const float m_soundMarkerThreshold;

    const bool m_isFixedPayloadLength;
    const int m_payloadLength;

    const bool m_isRxEnabled;
    const bool m_isTxEnabled;
    const bool m_needResampling;
    const bool m_txOnlyTones;

    // common
    TxRxData m_dataEncoded;
    TxRxData m_workRSLength; // Reed-Solomon work buffers
    TxRxData m_workRSData;

    // Impl
    struct Rx;
    std::unique_ptr<Rx> m_rx;

    struct Tx;
    std::unique_ptr<Tx> m_tx;

    std::unique_ptr<Resampler> m_resampler;
};

#endif

#endif

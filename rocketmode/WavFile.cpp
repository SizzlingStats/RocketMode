
#include "WavFile.h"

#include "sourcesdk/public/filesystem.h"

#include "base/math.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int ReadFileToMemory(const char* file, char** outData, IBaseFileSystem* fileSystem)
{
    FileHandle_t fp = fileSystem->Open(file, "rb");
    if (fp == FILESYSTEM_INVALID_HANDLE)
    {
        return 0;
    }

    const unsigned int size = fileSystem->Size(fp);

    char* data = static_cast<char*>(malloc(size));
    const int read = fileSystem->Read(data, size, fp);
    assert(read == size);

    *outData = data;

    fileSystem->Close(fp);
    return size;
}

class ByteReader
{
public:
    ByteReader(const char* data, int32_t length) :
        mReadCursor(data),
        mDataEnd(data + length)
    {
    }

    template<typename T>
    T ReadValue()
    {
        const char* afterRead = mReadCursor + sizeof(T);
        if (afterRead <= mDataEnd)
        {
            const T v = *reinterpret_cast<const T*>(mReadCursor);
            mReadCursor = afterRead;
            return v;
        }
        return T();
    }

    int32_t ReadBytes(void* data, int32_t maxLength)
    {
        const int32_t bytesLeft = mDataEnd - mReadCursor;
        const int32_t length = Math::Min(bytesLeft, maxLength);
        memcpy(data, mReadCursor, length);
        mReadCursor += length;
        return length;
    }

    void SkipBytes(int32_t count)
    {
        mReadCursor += count;
    }

    bool EndOrOverflow() const
    {
        return mReadCursor >= mDataEnd;
    }

    bool Overflowed() const
    {
        return mReadCursor > mDataEnd;
    }

private:
    const char* mReadCursor;
    const char* mDataEnd;
};

struct RiffChunk
{
    uint32_t riff;
    uint32_t size;
    uint32_t wave;
};

struct SubChunk
{
    uint32_t id;
    uint32_t size;
};

struct FmtChunk
{
    uint16_t format;
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t avgBytesPerSec;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
};

WavFile::~WavFile()
{
    free(mSampleData);
}

bool WavFile::Load(const char* file, IBaseFileSystem* fileSystem)
{
    static constexpr uint32_t sRIFF = 0x46464952;  // 'FFIR'
    static constexpr uint32_t sWAVE = 0x45564157;  // 'EVAW'
    static constexpr uint32_t sFmt = 0x20746d66;   // ' tmf'
    static constexpr uint32_t sData = 0x61746164;  // 'atad'

    char* data;
    const int32_t length = ReadFileToMemory(file, &data, fileSystem);

    ByteReader reader(data, length);

    RiffChunk riffChunk;
    reader.ReadBytes(&riffChunk, sizeof(riffChunk));
    assert(riffChunk.riff == sRIFF);
    assert((riffChunk.size + 8) == length);
    assert(riffChunk.wave == sWAVE);

    FmtChunk fmt;

    do
    {
        SubChunk subChunk;
        reader.ReadBytes(&subChunk, sizeof(subChunk));
        if (subChunk.id == sFmt)
        {
            reader.ReadBytes(&fmt, sizeof(fmt));

            assert(fmt.format == WAVE_FORMAT_PCM || fmt.format == WAVE_FORMAT_IEEE_FLOAT);
            assert(fmt.format != WAVE_FORMAT_PCM || fmt.bitsPerSample == 16);
            assert(fmt.format != WAVE_FORMAT_IEEE_FLOAT || fmt.bitsPerSample == 32);

            mFormat = static_cast<SampleFormat>(fmt.format);
            mNumChannels = fmt.channels;
            mSampleRate = fmt.sampleRate;
        }
        else if (subChunk.id == sData)
        {
            assert(!mSampleData);
            char* sampleData = static_cast<char*>(malloc(subChunk.size));
            const int32_t bytesRead = reader.ReadBytes(sampleData, subChunk.size);
            assert(bytesRead == subChunk.size);

            mSampleData = sampleData;
            mNumSamples = subChunk.size / (fmt.bitsPerSample / 8) / fmt.channels;

            break; // don't care about the rest
        }
        else
        {
            reader.SkipBytes(subChunk.size);
        }
    } while (!reader.EndOrOverflow());

    return !reader.Overflowed();
}

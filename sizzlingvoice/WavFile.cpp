
#include "WavFile.h"

#include "base/math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>

static int GetFileSize(int fd)
{
    struct stat buf;
    const int ret = fstat(fd, &buf);
    return ret == 0 ? buf.st_size : -1;
}

static int GetFileSize(FILE* f)
{
    return GetFileSize(fileno(f));
}

static int ReadFileToMemory(const char* file, char** outData)
{
    FILE* fp = fopen(file, "rb");
    if (!fp)
    {
        return 0;
    }

    const int size = GetFileSize(fp);
    if (size > 0)
    {
        char* data = static_cast<char*>(malloc(size));
        const int read = fread(data, 1, size, fp);
        assert(read == size);

        *outData = data;
    }

    fclose(fp);
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

bool WavFile::Load(const char* file)
{
    char* data;
    const int32_t length = ReadFileToMemory(file, &data);

    ByteReader reader(data, length);

    RiffChunk riffChunk;
    reader.ReadBytes(&riffChunk, sizeof(riffChunk));
    assert(riffChunk.riff == 'FFIR');
    assert((riffChunk.size + 8) == length);
    assert(riffChunk.wave == 'EVAW');

    FmtChunk fmt;

    do
    {
        SubChunk subChunk;
        reader.ReadBytes(&subChunk, sizeof(subChunk));
        if (subChunk.id == ' tmf')
        {
            reader.ReadBytes(&fmt, sizeof(fmt));

            assert(fmt.format == WAVE_FORMAT_PCM || fmt.format == WAVE_FORMAT_IEEE_FLOAT);
            assert(fmt.format != WAVE_FORMAT_PCM || fmt.bitsPerSample == 16);
            assert(fmt.format != WAVE_FORMAT_IEEE_FLOAT || fmt.bitsPerSample == 32);

            mFormat = static_cast<SampleFormat>(fmt.format);
            mNumChannels = fmt.channels;
            mSampleRate = fmt.sampleRate;
        }
        else if (subChunk.id == 'atad')
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
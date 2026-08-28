#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

struct StereoWav
{
    int sampleRate = 0;
    std::vector<float> left;
    std::vector<float> right;
};

namespace wav_detail
{
inline bool readExact (std::ifstream& in, void* dst, std::streamsize n)
{
    in.read (static_cast<char*> (dst), n);
    return in.good() && in.gcount() == n;
}

inline uint32_t u32le (const unsigned char* p)
{
    return static_cast<uint32_t> (p[0]) | (static_cast<uint32_t> (p[1]) << 8)
         | (static_cast<uint32_t> (p[2]) << 16) | (static_cast<uint32_t> (p[3]) << 24);
}

inline uint16_t u16le (const unsigned char* p)
{
    return static_cast<uint16_t> (p[0] | (p[1] << 8));
}

inline void putU32 (std::vector<unsigned char>& b, uint32_t v)
{
    b.push_back (static_cast<unsigned char> (v));
    b.push_back (static_cast<unsigned char> (v >> 8));
    b.push_back (static_cast<unsigned char> (v >> 16));
    b.push_back (static_cast<unsigned char> (v >> 24));
}

inline void putU16 (std::vector<unsigned char>& b, uint16_t v)
{
    b.push_back (static_cast<unsigned char> (v));
    b.push_back (static_cast<unsigned char> (v >> 8));
}
} // namespace wav_detail

inline bool writeWav32fStereo (const std::string& path, const StereoWav& wav)
{
    if (wav.sampleRate <= 0 || wav.left.size() != wav.right.size() || wav.left.empty())
        return false;

    const uint32_t nFrames = static_cast<uint32_t> (wav.left.size());
    const uint16_t channels = 2;
    const uint16_t bits = 32;
    const uint32_t dataBytes = nFrames * channels * (bits / 8);
    const uint32_t byteRate = static_cast<uint32_t> (wav.sampleRate) * channels * (bits / 8);

    std::vector<unsigned char> bytes;
    bytes.reserve (44 + dataBytes);
    bytes.insert (bytes.end(), { 'R', 'I', 'F', 'F' });
    wav_detail::putU32 (bytes, 36 + dataBytes);
    bytes.insert (bytes.end(), { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
    wav_detail::putU32 (bytes, 16);
    wav_detail::putU16 (bytes, 3); // IEEE float
    wav_detail::putU16 (bytes, channels);
    wav_detail::putU32 (bytes, static_cast<uint32_t> (wav.sampleRate));
    wav_detail::putU32 (bytes, byteRate);
    wav_detail::putU16 (bytes, static_cast<uint16_t> (channels * (bits / 8)));
    wav_detail::putU16 (bytes, bits);
    bytes.insert (bytes.end(), { 'd', 'a', 't', 'a' });
    wav_detail::putU32 (bytes, dataBytes);

    for (uint32_t i = 0; i < nFrames; ++i)
    {
        float l = wav.left[i];
        float r = wav.right[i];
        unsigned char tmp[4];
        std::memcpy (tmp, &l, 4);
        bytes.insert (bytes.end(), tmp, tmp + 4);
        std::memcpy (tmp, &r, 4);
        bytes.insert (bytes.end(), tmp, tmp + 4);
    }

    std::ofstream out (path, std::ios::binary);
    if (! out)
        return false;
    out.write (reinterpret_cast<const char*> (bytes.data()), static_cast<std::streamsize> (bytes.size()));
    return static_cast<bool> (out);
}

inline bool readWavStereo (const std::string& path, StereoWav& wav)
{
    std::ifstream in (path, std::ios::binary);
    if (! in)
        return false;

    unsigned char riff[12];
    if (! wav_detail::readExact (in, riff, 12))
        return false;
    if (std::memcmp (riff, "RIFF", 4) != 0 || std::memcmp (riff + 8, "WAVE", 4) != 0)
        return false;

    int sampleRate = 0;
    int channels = 0;
    int bits = 0;
    int audioFormat = 0;
    std::vector<unsigned char> data;

    while (in)
    {
        unsigned char hdr[8];
        if (! wav_detail::readExact (in, hdr, 8))
            break;
        const uint32_t size = wav_detail::u32le (hdr + 4);
        std::vector<unsigned char> chunk (size);
        if (size > 0 && ! wav_detail::readExact (in, chunk.data(), static_cast<std::streamsize> (size)))
            return false;
        if (size & 1)
            in.ignore (1);

        if (std::memcmp (hdr, "fmt ", 4) == 0)
        {
            if (size < 16)
                return false;
            audioFormat = wav_detail::u16le (chunk.data());
            channels = wav_detail::u16le (chunk.data() + 2);
            sampleRate = static_cast<int> (wav_detail::u32le (chunk.data() + 4));
            bits = wav_detail::u16le (chunk.data() + 14);
        }
        else if (std::memcmp (hdr, "data", 4) == 0)
        {
            data = std::move (chunk);
        }
    }

    if (channels != 2 || sampleRate <= 0 || data.empty())
        return false;

    wav.sampleRate = sampleRate;
    wav.left.clear();
    wav.right.clear();

    if (audioFormat == 3 && bits == 32)
    {
        if (data.size() % 8 != 0)
            return false;
        const size_t n = data.size() / 8;
        wav.left.resize (n);
        wav.right.resize (n);
        for (size_t i = 0; i < n; ++i)
        {
            std::memcpy (&wav.left[i], data.data() + i * 8, 4);
            std::memcpy (&wav.right[i], data.data() + i * 8 + 4, 4);
        }
        return true;
    }

    if (audioFormat == 1 && bits == 16)
    {
        if (data.size() % 4 != 0)
            return false;
        const size_t n = data.size() / 4;
        wav.left.resize (n);
        wav.right.resize (n);
        for (size_t i = 0; i < n; ++i)
        {
            const int16_t ls = static_cast<int16_t> (wav_detail::u16le (data.data() + i * 4));
            const int16_t rs = static_cast<int16_t> (wav_detail::u16le (data.data() + i * 4 + 2));
            wav.left[i] = static_cast<float> (ls) / 32768.0f;
            wav.right[i] = static_cast<float> (rs) / 32768.0f;
        }
        return true;
    }

    return false;
}

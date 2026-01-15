#ifndef _MACARON_BASE64_H_
#define _MACARON_BASE64_H_

#include <cstdint>
#include <string>
#include <vector>
#include <cstddef>

namespace macaron {

class Base64 {
private:
    static constexpr char sEncodingTable[64] = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M',
        'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
        'a','b','c','d','e','f','g','h','i','j','k','l','m',
        'n','o','p','q','r','s','t','u','v','w','x','y','z',
        '0','1','2','3','4','5','6','7','8','9','+','/'
    };

    // Core byte-based encoder
    static std::string EncodeBytes(const unsigned char* data, size_t in_len) {
        size_t out_len = 4 * ((in_len + 2) / 3);
        std::string out(out_len, '\0');

        size_t i = 0, j = 0;

        while (i + 2 < in_len) {
            out[j++] = sEncodingTable[(data[i] >> 2) & 0x3F];
            out[j++] = sEncodingTable[((data[i] & 0x03) << 4) |
                                      ((data[i + 1] & 0xF0) >> 4)];
            out[j++] = sEncodingTable[((data[i + 1] & 0x0F) << 2) |
                                      ((data[i + 2] & 0xC0) >> 6)];
            out[j++] = sEncodingTable[data[i + 2] & 0x3F];
            i += 3;
        }

        if (i < in_len) {
            out[j++] = sEncodingTable[(data[i] >> 2) & 0x3F];

            if (i + 1 < in_len) {
                out[j++] = sEncodingTable[((data[i] & 0x03) << 4) |
                                          ((data[i + 1] & 0xF0) >> 4)];
                out[j++] = sEncodingTable[(data[i + 1] & 0x0F) << 2];
                out[j++] = '=';
            } else {
                out[j++] = sEncodingTable[(data[i] & 0x03) << 4];
                out[j++] = '=';
                out[j++] = '=';
            }
        }

        return out;
    }

public:
    // Existing API (string)
    static std::string Encode(const std::string& data) {
        return EncodeBytes(
            reinterpret_cast<const unsigned char*>(data.data()),
            data.size());
    }

    // NEW API: vector<unsigned char>
    static std::string Encode(const std::vector<unsigned char>& data) {
        return EncodeBytes(data.data(), data.size());
    }

    // Decode unchanged (already binary-safe)
    static std::string Decode(const std::string& input, std::string& out) {
        static constexpr unsigned char kDecodingTable[256] = {
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
            52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,
            64,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,
            64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64,
            /* rest initialized to 64 */
        };

        size_t in_len = input.size();
        if (in_len % 4 != 0)
            return "Input data size is not a multiple of 4";

        size_t out_len = in_len / 4 * 3;
        if (input[in_len - 1] == '=') out_len--;
        if (input[in_len - 2] == '=') out_len--;

        out.resize(out_len);

        for (size_t i = 0, j = 0; i < in_len;) {
            uint32_t a = input[i] == '=' ? 0 : kDecodingTable[(unsigned char)input[i]]; i++;
            uint32_t b = input[i] == '=' ? 0 : kDecodingTable[(unsigned char)input[i]]; i++;
            uint32_t c = input[i] == '=' ? 0 : kDecodingTable[(unsigned char)input[i]]; i++;
            uint32_t d = input[i] == '=' ? 0 : kDecodingTable[(unsigned char)input[i]]; i++;

            uint32_t triple = (a << 18) | (b << 12) | (c << 6) | d;

            if (j < out_len) out[j++] = (triple >> 16) & 0xFF;
            if (j < out_len) out[j++] = (triple >> 8) & 0xFF;
            if (j < out_len) out[j++] = triple & 0xFF;
        }

        return "";
    }

    static std::vector<std::byte> DecodeToBytes(const std::string& input) {
        static constexpr unsigned char kDecodingTable[256] = {
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
            52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,
            64,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,
            64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64,
            /* rest implicitly 64 */
        };

        if (input.size() % 4 != 0)
            throw std::runtime_error("Invalid Base64 length");

        size_t out_len = input.size() / 4 * 3;
        if (!input.empty() && input.back() == '=') out_len--;
        if (input.size() > 1 && input[input.size() - 2] == '=') out_len--;

        std::vector<std::byte> out(out_len);

        for (size_t i = 0, j = 0; i < input.size();) {
            uint32_t a = input[i] == '=' ? 0 : kDecodingTable[(unsigned char)input[i]]; i++;
            uint32_t b = input[i] == '=' ? 0 : kDecodingTable[(unsigned char)input[i]]; i++;
            uint32_t c = input[i] == '=' ? 0 : kDecodingTable[(unsigned char)input[i]]; i++;
            uint32_t d = input[i] == '=' ? 0 : kDecodingTable[(unsigned char)input[i]]; i++;

            uint32_t triple = (a << 18) | (b << 12) | (c << 6) | d;

            if (j < out_len) out[j++] = std::byte((triple >> 16) & 0xFF);
            if (j < out_len) out[j++] = std::byte((triple >> 8) & 0xFF);
            if (j < out_len) out[j++] = std::byte(triple & 0xFF);
        }

        return out;
    }
};

} // namespace macaron

#endif /* _MACARON_BASE64_H_ */

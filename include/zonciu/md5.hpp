#ifndef ZONCIU_MD5_HPP
#define ZONCIU_MD5_HPP
#include <string>
#include <cstdint>
#include <array>
namespace zonciu
{
    class Md5
    {
    public:
        Md5(const unsigned char* data, size_t length) {
            Make((const unsigned char*)data, length);
        }
        Md5(const std::string& data) {
            Make((const unsigned char*)data.c_str(), data.length());
        }

        std::array<uint32_t, 4> raw() { return raw_; }
        std::string GetString() {
            static const unsigned char dict[] = {
                '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
            };
            int hexNum;
            char res[33]{ 0 };
            int c = 0;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    hexNum = (raw_[i] >> j * 8) % 256;
                    res[c + 1] = dict[hexNum % 16];
                    res[c] = dict[(hexNum / 16) % 16];
                    c += 2;
                }
            }
            return std::string(res);
        }
    private:
        std::array<uint32_t, 4> raw_;
        void Make(const unsigned char*data, size_t length) {
            static const uint32_t s[64] = {
                7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
                5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20,
                4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
                6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21
            };
            static const uint32_t k[64] = {
                0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
                0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
                0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
                0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
                0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
                0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
                0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
                0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
                0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
                0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
            };
            raw_[0] = 0x67452301;
            raw_[1] = 0xefcdab89;
            raw_[2] = 0x98badcfe;
            raw_[3] = 0x10325476;
            size_t chunk_count = ((length + 8) >> 6) + 1;
            uint32_t *message = new uint32_t[chunk_count << 4]{ 0 };
            for (size_t i = 0; i < length; i++) {
                message[i >> 2] |= (data[i]) << ((i & 3) << 3);
            }
            message[length >> 2] |= 0x80 << ((length & 3) << 3);
            message[(chunk_count << 4) - 2] = length << 3;

            for (size_t i = 0; i < chunk_count; i++) {
                uint32_t a = raw_[0];
                uint32_t b = raw_[1];
                uint32_t c = raw_[2];
                uint32_t d = raw_[3];
                uint32_t f, g;
                for (uint32_t j = 0; j < 64; j++) {
                    if (j < 16) {
                        f = (b & c) | (~b & d);//F
                        g = j;
                    } else if (j < 32) {
                        f = ((b & d) | (c & ~d));//G
                        g = (5 * j + 1) & 15;
                    } else if (j < 48) {
                        f = ((b) ^ (c) ^ (d));//H
                        g = (3 * j + 5) & 15;
                    } else {
                        f = ((c) ^ ((b) | (~d)));//I
                        g = (7 * j) & 15;
                    }
                    auto tmpd = d;
                    d = c;
                    c = b;
                    b = b + LeftRotate(a + f + k[j] + message[(i << 4) + g], s[j]);
                    a = tmpd;
                }
                raw_[0] += a;
                raw_[1] += b;
                raw_[2] += c;
                raw_[3] += d;
            }
            delete message;
        }

        uint32_t LeftRotate(uint32_t x, uint32_t n) {
            return ((x << n) | (x >> (32 - n)));
        }
    };
}

#endif // ZONCIU_MD5_HPP

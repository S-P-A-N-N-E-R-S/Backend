#include <auth/auth_utils.hpp>

#include <limits>
#include <random>
#include "argon2.h"

namespace server {

namespace auth_utils {

    namespace {
        constexpr const size_t HASH_LENGTH = 32;
        constexpr const size_t SALT_LENGTH = 16;
        constexpr const uint32_t PASSES = 2;
        constexpr const uint32_t MEM_USE = (1 << 16);  // 64 MB
        constexpr const uint32_t THREADS = 1;
    }  // namespace

    bool check_password(const std::string &password, const binary_data &salt,
                        const binary_data &correct_hash)
    {
        binary_data cur_hash(HASH_LENGTH, std::byte{0});

        if (const int result =
                argon2id_hash_raw(PASSES, MEM_USE, THREADS, password.c_str(), password.length(),
                                  salt.data(), salt.length(), cur_hash.data(), cur_hash.length());
            result != ARGON2_OK)
        {
            // TODO: What to do if result != ARGON2_OK? At least log this somewhere.
            return false;
        }

        return (cur_hash == correct_hash);
    }

    bool hash_password(const std::string &pw, binary_data &hashed_pw, binary_data &salt)
    {
        hashed_pw.resize(HASH_LENGTH);
        salt.resize(SALT_LENGTH);

        // Generate random salt
        std::random_device rd;
        std::uniform_int_distribution<uint16_t> dist{0, std::numeric_limits<uint16_t>::max()};
        *reinterpret_cast<uint16_t *>(salt.data()) = dist(rd);

        if (const int result =
                argon2id_hash_raw(PASSES, MEM_USE, THREADS, pw.c_str(), pw.length(), salt.data(),
                                  salt.length(), hashed_pw.data(), hashed_pw.length());
            result != ARGON2_OK)
        {
            // TODO: What to do if result != ARGON2_OK? At least log this somewhere.
            return false;
        }
        else
        {
            return true;
        }
    }

}  // namespace auth_utils

}  // namespace server

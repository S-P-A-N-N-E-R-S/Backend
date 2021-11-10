#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "argon2.h"

#define HASHLEN 32
#define SALTLEN 16
#define PWD "password"

using binary_data = std::basic_string<std::byte>;

int main(void)
{
    uint8_t hash1[HASHLEN];
    uint8_t hash2[HASHLEN];

    uint8_t salt[SALTLEN];
    memset(salt, 0x00, SALTLEN);

    uint8_t *pwd = (uint8_t *)strdup(PWD);
    uint32_t pwdlen = strlen((char *)pwd);

    uint32_t t_cost = 2;          // 2-pass computation
    uint32_t m_cost = (1 << 16);  // 64 mebibytes memory usage
    uint32_t parallelism = 1;     // number of threads and lanes

    binary_data salt2{SALTLEN, std::byte{0}};
    binary_data hashed_pwd{HASHLEN, std::byte{0}};

    // high-level API
    argon2i_hash_raw(t_cost, m_cost, parallelism, pwd, pwdlen, salt2.data(), salt2.length(),
                     hashed_pwd.data(), HASHLEN);

    // low-level API
    argon2_context context = {hash2,   /* output array, at least HASHLEN in size */
                              HASHLEN, /* digest length */
                              pwd,     /* password array */
                              pwdlen,  /* password length */
                              salt,    /* salt array */
                              SALTLEN, /* salt length */
                              NULL, 0, /* optional secret data */
                              NULL, 0, /* optional associated data */
                              t_cost, m_cost, parallelism, parallelism,
                              ARGON2_VERSION_13, /* algorithm version */
                              NULL, NULL, /* custom memory allocation / deallocation functions */
                              /* by default only internal memory is cleared (pwd is not wiped) */
                              ARGON2_DEFAULT_FLAGS};

    int rc = argon2i_ctx(&context);
    if (ARGON2_OK != rc)
    {
        printf("Error: %s\n", argon2_error_message(rc));
        exit(1);
    }
    free(pwd);

    for (int i = 0; i < HASHLEN; ++i)
        printf("%02x", hashed_pwd.data()[i]);
    printf("\n");
    if (memcmp(reinterpret_cast<uint8_t *>(hashed_pwd.data()), hash2, HASHLEN))
    {
        for (int i = 0; i < HASHLEN; ++i)
        {
            printf("%02x", hash2[i]);
        }
        printf("\nfail\n");
    }
    else
        printf("ok\n");
    return 0;
}

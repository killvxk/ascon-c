#include <string.h>
#include "api.h"
#include "endian.h"
#include "permutations.h"

#define RATE (64 / 8)
#define PA_ROUNDS 12

int crypto_hash(unsigned char* out, const unsigned char* in,
                unsigned long long inlen) {
  state s;
  u64 outlen;

  // initialization
  to_bit_interleaving_big_immediate(s.x0, 0x16d44c813b277eb5);
  to_bit_interleaving_big_immediate(s.x1, 0x2024ae622504512b);
  to_bit_interleaving_big_immediate(s.x2, 0x1822df8d76a7a366);
  to_bit_interleaving_big_immediate(s.x3, 0x0c6553817a0aad5a);
  to_bit_interleaving_big_immediate(s.x4, 0xb6939453320e3e4f);

  u32_2 t0;
  u64 tmp0;

  // absorb plaintext
  while (inlen >= RATE) {
    tmp0 = (*(u64*)in);
    t0 = to_bit_interleaving_big(tmp0);
    s.x0.e ^= t0.e;
    s.x0.o ^= t0.o;
    P(&s, PA_ROUNDS);
    inlen -= RATE;
    in += RATE;
  }

  u8 bytes[16];
  memcpy(bytes, in, inlen);
  memset(bytes + inlen, 0, 16 - inlen);
  bytes[inlen] ^= 0x80;

  u64* tmp = (u64*)bytes;

  t0 = to_bit_interleaving_big(tmp[0]);
  s.x0.e ^= t0.e;
  s.x0.o ^= t0.o;

  P(&s, PA_ROUNDS);

  // squeeze hash
  outlen = CRYPTO_BYTES;
  while (outlen >= RATE) {
    tmp0 = from_bit_interleaving_big(s.x0);
    *(u64*)out = tmp0;
    P(&s, PA_ROUNDS);
    outlen -= RATE;
    out += RATE;
  }

  return 0;
}


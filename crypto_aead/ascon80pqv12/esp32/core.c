#include "core.h"

#include <string.h>

void ascon_duplex(state* s, unsigned char* out, const unsigned char* in,
                  unsigned long len, u8 mode) {
  u32_2 tmp;

  while (len >= RATE) {
    tmp.l = ((u32*)in)[0];
    tmp.h = ((u32*)in)[1];
    tmp = ascon_rev8_half(tmp);
    s->x0.h ^= tmp.h;
    s->x0.l ^= tmp.l;

    if (mode != ASCON_AD) {
      u32_2 tmp0 = ascon_rev8_half(s->x0);
      ((u32*)out)[0] = tmp0.l;
      ((u32*)out)[1] = tmp0.h;
    }
    if (mode == ASCON_DEC) {
      s->x0 = tmp;
    }

    P(s, PB_START_ROUND);

    in += RATE;
    out += RATE;
    len -= RATE;
  }

  u8* bytes = (u8*)&tmp;
  memset(bytes, 0, sizeof tmp);
  memcpy(bytes, in, len);
  bytes[len] ^= 0x80;

  tmp = ascon_rev8_half(tmp);
  s->x0.h ^= tmp.h;
  s->x0.l ^= tmp.l;

  if (mode != ASCON_AD) {
    tmp = ascon_rev8_half(s->x0);
    memcpy(out, bytes, len);
  }
  if (mode == ASCON_DEC) {
    memcpy(bytes, in, len);
    tmp = ascon_rev8_half(tmp);
    s->x0 = tmp;
  }
}

void ascon_core(state* s, unsigned char* out, const unsigned char* in,
                unsigned long long tlen, const unsigned char* ad,
                unsigned long long adlen, const unsigned char* npub,
                const unsigned char* k, u8 mode) {
  u32_4 tmp;
  u32_2 K0, K1, K2, N0, N1;

  // load key
  K0.l = U32BIG(((u32*)k)[0]);
  tmp.words[0].l = ((u32*)k)[1];
  tmp.words[0].h = ((u32*)k)[2];
  tmp.words[1].l = ((u32*)k)[3];
  tmp.words[1].h = ((u32*)k)[4];
  tmp = ascon_rev8(tmp);
  K1 = tmp.words[0];
  K2 = tmp.words[1];

  // load nonce
  tmp.words[0].l = ((u32*)npub)[0];
  tmp.words[0].h = ((u32*)npub)[1];
  tmp.words[1].l = ((u32*)npub)[2];
  tmp.words[1].h = ((u32*)npub)[3];
  tmp = ascon_rev8(tmp);
  N0 = tmp.words[0];
  N1 = tmp.words[1];

  // initialization
  s->x0.h = IV;
  s->x0.l = K0.l;
  s->x1.h = K1.h;
  s->x1.l = K1.l;
  s->x2.h = K2.h;
  s->x2.l = K2.l;
  s->x3.h = N0.h;
  s->x3.l = N0.l;
  s->x4.h = N1.h;
  s->x4.l = N1.l;
  P(s, PA_START_ROUND);
  s->x2.l ^= K0.l;
  s->x3.h ^= K1.h;
  s->x3.l ^= K1.l;
  s->x4.h ^= K2.h;
  s->x4.l ^= K2.l;

  // process associated data
  if (adlen) {
    ascon_duplex(s, (void*)0, ad, adlen, ASCON_AD);
    P(s, PB_START_ROUND);
  }
  s->x4.l ^= 1;

  // process plaintext/ciphertext
  ascon_duplex(s, out, in, tlen, mode);

  // finalization
  s->x1.h ^= K0.l;
  s->x1.l ^= K1.h;
  s->x2.h ^= K1.l;
  s->x2.l ^= K2.h;
  s->x3.h ^= K2.l;
  P(s, PA_START_ROUND);
  s->x3.h ^= K1.h;
  s->x3.l ^= K1.l;
  s->x4.h ^= K2.h;
  s->x4.l ^= K2.l;
}

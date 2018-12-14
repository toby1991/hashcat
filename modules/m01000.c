/**
 * Author......: See docs/credits.txt
 * License.....: MIT
 */

#include "common.h"
#include "types.h"
#include "bitops.h"
#include "convert.h"
#include "interface.h"
#include "inc_hash_constants.h"
#include "modules.h"

static const char *HASH_NAME    = "NTLM";
static const u32   SALT_TYPE    = SALT_TYPE_NONE;
static const u32   ATTACK_EXEC  = ATTACK_EXEC_INSIDE_KERNEL;
static const u32   OPTS_TYPE    = OPTS_TYPE_PT_GENERATE_LE
                                | OPTS_TYPE_PT_ADD80
                                | OPTS_TYPE_PT_ADDBITS14
                                | OPTS_TYPE_PT_UTF16LE;
static const u32   DGST_SIZE    = DGST_SIZE_4_4;
static const u64   ESALT_SIZE   = 0;
static const u32   OPTI_TYPE    = OPTI_TYPE_ZERO_BYTE
                                | OPTI_TYPE_PRECOMPUTE_INIT
                                | OPTI_TYPE_PRECOMPUTE_MERKLE
                                | OPTI_TYPE_MEET_IN_MIDDLE
                                | OPTI_TYPE_EARLY_SKIP
                                | OPTI_TYPE_NOT_ITERATED
                                | OPTI_TYPE_NOT_SALTED
                                | OPTI_TYPE_RAW_HASH;
static const u32   DGST_POS0    = 0;
static const u32   DGST_POS1    = 3;
static const u32   DGST_POS2    = 2;
static const u32   DGST_POS3    = 1;
static const char *ST_HASH      = "b4b9b02e6f09a9bd760f388b67351e2b";
static const char *ST_PASS      = "hashcat";

u32         module_attack_exec () { return ATTACK_EXEC; }
u32         module_dgst_pos0   () { return DGST_POS0;   }
u32         module_dgst_pos1   () { return DGST_POS1;   }
u32         module_dgst_pos2   () { return DGST_POS2;   }
u32         module_dgst_pos3   () { return DGST_POS3;   }
u32         module_dgst_size   () { return DGST_SIZE;   }
u64         module_esalt_size  () { return ESALT_SIZE;  }
const char *module_hash_name   () { return HASH_NAME;   }
u32         module_opti_type   () { return OPTI_TYPE;   }
u64         module_opts_type   () { return OPTS_TYPE;   }
u32         module_salt_type   () { return SALT_TYPE;   }
const char *module_st_hash     () { return ST_HASH;     }
const char *module_st_pass     () { return ST_PASS;     }

u32 module_salt_min (MAYBE_UNUSED const hashconfig_t *hashconfig, MAYBE_UNUSED const user_options_t *user_options, MAYBE_UNUSED const user_options_extra_t *user_options_extra)
{
  const bool optimized_kernel = (hashconfig->opti_type & OPTI_TYPE_OPTIMIZED_KERNEL);

  return hashconfig_salt_min (hashconfig, user_options, user_options_extra, optimized_kernel);
}

u32 module_salt_max (MAYBE_UNUSED const hashconfig_t *hashconfig, MAYBE_UNUSED const user_options_t *user_options, MAYBE_UNUSED const user_options_extra_t *user_options_extra)
{
  const bool optimized_kernel = (hashconfig->opti_type & OPTI_TYPE_OPTIMIZED_KERNEL);

  return hashconfig_salt_max (hashconfig, user_options, user_options_extra, optimized_kernel);
}

u32 module_pw_min (MAYBE_UNUSED const hashconfig_t *hashconfig, MAYBE_UNUSED const user_options_t *user_options, MAYBE_UNUSED const user_options_extra_t *user_options_extra)
{
  const bool optimized_kernel = (hashconfig->opti_type & OPTI_TYPE_OPTIMIZED_KERNEL);

  return hashconfig_pw_min (hashconfig, user_options, user_options_extra, optimized_kernel);
}

u32 module_pw_max (MAYBE_UNUSED const hashconfig_t *hashconfig, MAYBE_UNUSED const user_options_t *user_options, MAYBE_UNUSED const user_options_extra_t *user_options_extra)
{
  const bool optimized_kernel = (hashconfig->opti_type & OPTI_TYPE_OPTIMIZED_KERNEL);

  return hashconfig_pw_max (hashconfig, user_options, user_options_extra, optimized_kernel);
}

int module_hash_decode (MAYBE_UNUSED const hashconfig_t *hashconfig, MAYBE_UNUSED void *digest_buf, MAYBE_UNUSED salt_t *salt, MAYBE_UNUSED void *esalt_buf, const char *line_buf, const int line_len)
{
  u32 *digest = (u32 *) digest_buf;

  token_t token;

  token.token_cnt  = 1;

  token.len_min[0] = 32;
  token.len_max[0] = 32;
  token.attr[0]    = TOKEN_ATTR_VERIFY_LENGTH
                   | TOKEN_ATTR_VERIFY_HEX;

  const int rc_tokenizer = input_tokenizer ((const u8 *) line_buf, line_len, &token);

  if (rc_tokenizer != PARSER_OK) return (rc_tokenizer);

  const u8 *hash_pos = token.buf[0];

  digest[0] = hex_to_u32 (hash_pos +  0);
  digest[1] = hex_to_u32 (hash_pos +  8);
  digest[2] = hex_to_u32 (hash_pos + 16);
  digest[3] = hex_to_u32 (hash_pos + 24);

  if (hashconfig->opti_type & OPTI_TYPE_PRECOMPUTE_MERKLE)
  {
    digest[0] -= MD4M_A;
    digest[1] -= MD4M_B;
    digest[2] -= MD4M_C;
    digest[3] -= MD4M_D;
  }

  return (PARSER_OK);
}

int module_hash_encode (MAYBE_UNUSED const hashconfig_t *hashconfig, MAYBE_UNUSED const void *digest_buf, MAYBE_UNUSED const salt_t *salt, MAYBE_UNUSED const void *esalt_buf, char *line_buf, const int line_size)
{
  const u32 *digest = (const u32 *) digest_buf;

  // we can not change anything in the original buffer, otherwise destroying sorting
  // therefore create some local buffer

  u32 tmp[4];

  tmp[0] = digest[0];
  tmp[1] = digest[1];
  tmp[2] = digest[2];
  tmp[3] = digest[3];

  if (hashconfig->opti_type & OPTI_TYPE_PRECOMPUTE_MERKLE)
  {
    tmp[0] += MD4M_A;
    tmp[1] += MD4M_B;
    tmp[2] += MD4M_C;
    tmp[3] += MD4M_D;
  }

  tmp[0] = byte_swap_32 (tmp[0]);
  tmp[1] = byte_swap_32 (tmp[1]);
  tmp[2] = byte_swap_32 (tmp[2]);
  tmp[3] = byte_swap_32 (tmp[3]);

  const int out_len = snprintf (line_buf, line_size, "%08x%08x%08x%08x",
    tmp[0],
    tmp[1],
    tmp[2],
    tmp[3]);

  return out_len;
}

void module_register (module_ctx_t *module_ctx)
{
  module_ctx->module_attack_exec = module_attack_exec;
  module_ctx->module_dgst_pos0   = module_dgst_pos0;
  module_ctx->module_dgst_pos1   = module_dgst_pos1;
  module_ctx->module_dgst_pos2   = module_dgst_pos2;
  module_ctx->module_dgst_pos3   = module_dgst_pos3;
  module_ctx->module_dgst_size   = module_dgst_size;
  module_ctx->module_esalt_size  = module_esalt_size;
  module_ctx->module_hash_decode = module_hash_decode;
  module_ctx->module_hash_encode = module_hash_encode;
  module_ctx->module_hash_name   = module_hash_name;
  module_ctx->module_opti_type   = module_opti_type;
  module_ctx->module_opts_type   = module_opts_type;
  module_ctx->module_pw_max      = module_pw_max;
  module_ctx->module_pw_min      = module_pw_min;
  module_ctx->module_salt_max    = module_salt_max;
  module_ctx->module_salt_min    = module_salt_min;
  module_ctx->module_salt_type   = module_salt_type;
  module_ctx->module_st_hash     = module_st_hash;
  module_ctx->module_st_pass     = module_st_pass;
}
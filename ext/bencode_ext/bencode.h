#ifndef __BENCODE_H__
#define __BENCODE_H__

#include "ruby.h"

static VALUE BEncode;
static VALUE DecodeError;
static VALUE EncodeError;
static VALUE readId;

static long parse_num(char**, long*);
static VALUE decode(VALUE, VALUE);
static VALUE encode(VALUE);
static int hash_traverse(VALUE, VALUE, VALUE);
static VALUE str_bdecode(VALUE);
static VALUE mod_encode(VALUE, VALUE);
static VALUE _decode_file(VALUE);
static VALUE decode_file(VALUE, VALUE);
static VALUE get_max_depth(VALUE);
static VALUE set_max_depth(VALUE, VALUE);
static long max_depth;
void Init_bencode_ext();

#endif

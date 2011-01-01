/*
 * Document-module: BEncode
 *
 * BEncode module provides functionality for encoding/decoding Ruby objects
 * in BitTorrent loosly structured data format - <i>bencode</i>. To decode data you sould use one of the following:
 *   'string'.bdecode
 *   BEncode.decode('string')
 * Where string contains bencoded data (i.e. some torrent file)
 * To encode your objects into bencode format:
 *   object.bencode
 *   # or
 *   BEncode.encode(object)
 * bencode format has only following datatypes:
 *   Integer
 *   String
 *   List
 *   Dictionary (Hash)
 * No other types allowed. Only symbols are converted to string for convenience
 * (<b>but they're decoded as strings</b>).
 *
 * <b>BEncode is included into Object on load.</b>
 */

#include "bencode.h"

#define NEXT_CHAR ++*str; --*len;
#define END_CHECK_SKIP(t) {if(!*len)\
                            rb_raise(DecodeError, "Unpexpected " #t " end!");\
                           if(**str != 'e')\
                            rb_raise(DecodeError, "Mailformed " #t " at %d byte: %c", rlen - *len, **str);\
                           NEXT_CHAR;}

static long parse_num(char** str, long* len){
  long t = 1, ret = 0;

  if(**str == '-'){
    t = -1;
    NEXT_CHAR;
  }

  while(*len && **str >= '0' && **str <= '9'){
    ret = ret * 10 + (**str - '0');
    NEXT_CHAR;
  }

  return ret * t;
}

static VALUE _decode(char** str, long* len, long rlen){
  if(!*len)
    return Qnil;

  switch(**str){
    case 'l':{
      VALUE ret = rb_ary_new();
      NEXT_CHAR;

      while(**str != 'e' && *len)
        rb_ary_push(ret, _decode(str, len, rlen));

      END_CHECK_SKIP(list);
      return ret;
    }
    case 'd':{
      VALUE ret = rb_hash_new();
      NEXT_CHAR;

      while(**str != 'e' && *len){
        long t = *len;
        VALUE k = _decode(str, len, rlen);

        if(TYPE(k) != T_STRING)
          rb_raise(DecodeError, "Dictionary key is not a string at %d!", rlen - t);

        rb_hash_aset(ret, k, _decode(str, len, rlen));
      }

      END_CHECK_SKIP(dictionary);
      return ret;
    }
    case 'i':{
      long t;
      NEXT_CHAR;
      t = parse_num(str, len);
      END_CHECK_SKIP(integer);
      return LONG2FIX(t);
    }
    case '0'...'9':{
      VALUE ret;
      long slen = parse_num(str, len);

      if(slen < 0 || (*len && **str != ':'))
        rb_raise(DecodeError, "Invalid string length specification at %d: %c", rlen - *len, **str);

      if(!*len || *len < slen + 1)
        rb_raise(DecodeError, "Unexpected string end!");

      ret = rb_str_new(++*str, slen);
      *str += slen;
      *len -= slen + 1;
      return ret;
    }
    default:
      rb_raise(DecodeError, "Unknown element type at %d byte: %c", rlen - *len, **str);
      return Qnil;
  }
}

/*
 * Document-method: BEncode.decode
 * call-seq:
 *     BEncode.decode(string)
 *
 * Returns data structure from parsed _string_.
 * String must be valid bencoded data, or
 * BEncode::DecodeError will be raised with description
 * of error.
 *
 * Examples:
 *
 *    'i1e' => 1
 *    'i-1e' => -1
 *    '6:string' => 'string'
 */

static VALUE decode(VALUE self, VALUE str){
  long rlen, len;
  char* ptr;
  VALUE ret;

  if(!rb_obj_is_kind_of(str, rb_cString))
    rb_raise(rb_eTypeError, "String expected");

  rlen = len = RSTRING_LEN(str);
  ptr = RSTRING_PTR(str);
  ret = _decode(&ptr, &len, len);

  if(len)
    rb_raise(DecodeError, "String has garbage on the end (starts at %d).", rlen - len);

  return ret;
}

static VALUE _decode_file(VALUE fp){
  return decode(BEncode, rb_funcall(fp, readId, 0));
}

/*
 * Document-method: BEncode.decode_file
 * call-seq:
 *    BEncode.decode_file(file)
 *
 * Load content of _file_ and decodes it.
 * _file_ may be either IO instance or
 * String path to file.
 *
 * Examples:
 *
 *   BEncode.decode_file('/path/to/file.torrent')
 *
 *   open('/path/to/file.torrent', 'rb') do |f|
 *     BEncode.decode_file(f)
 *   end
 */

static VALUE decode_file(VALUE self, VALUE path){
  if(rb_obj_is_kind_of(path, rb_cIO)){
    return _decode_file(path);
  }else{
    VALUE fp = rb_file_open_str(path, "rb");
    return rb_ensure(_decode_file, fp, rb_io_close, fp);
  }
}

/*
 * Document-method: BEncode#bencode
 * call-seq:
 *    object.bencode
 *
 * Returns a string representing _object_ in
 * bencoded format. _object_ must be one of:
 *   Integer
 *   String
 *   Symbol (will be converter to String)
 *   Array
 *   Hash
 * If _object_ does not belong to these types
 * or their derivates BEncode::EncodeError exception will
 * be raised.
 *
 * Because BEncode is included into Object
 * This method is avilable for all objects.
 *
 * Examples:
 *
 *   1.bencode => 'i1e'
 *   -1.bencode => 'i-1e'
 *   'string'.bencode => '6:string'
 */

static VALUE encode(VALUE self){
  if(TYPE(self) == T_SYMBOL){
    return encode(rb_id2str(SYM2ID(self)));
  }if(rb_obj_is_kind_of(self, rb_cString)){
    long len = RSTRING_LEN(self);
    return rb_sprintf("%d:%.*s", len, len, RSTRING_PTR(self));
  }else if(rb_obj_is_kind_of(self, rb_cInteger)){
    return rb_sprintf("i%de", NUM2LONG(self));
  }else if(rb_obj_is_kind_of(self, rb_cHash)){
    VALUE ret = rb_str_new2("d");
    rb_hash_foreach(self, hash_traverse, ret);
    rb_str_cat2(ret, "e");
    return ret;
  }else if(rb_obj_is_kind_of(self, rb_cArray)){
    long i, c;
    VALUE *ptr, ret = rb_str_new2("l");

    for(i = 0, c = RARRAY_LEN(self), ptr = RARRAY_PTR(self); i < c; ++i)
      rb_str_concat(ret, encode(ptr[i]));

    rb_str_cat2(ret, "e");
    return ret;
  }else
    rb_raise(EncodeError, "Don't know how to encode %s!", rb_class2name(CLASS_OF(self)));
}

static int hash_traverse(VALUE key, VALUE val, VALUE str){
  if(!rb_obj_is_kind_of(key, rb_cString) && TYPE(key) != T_SYMBOL)
    rb_raise(EncodeError, "Keys must be strings, not %s!", rb_class2name(CLASS_OF(key)));

  rb_str_concat(str, encode(key));
  rb_str_concat(str, encode(val));
  return ST_CONTINUE;
}

/*
 * Document-method: String#bdecode
 * call-seq:
 *    string.bdecode
 *
 * Shortcut to BEncode.encode(_string_)
 */

static VALUE str_bdecode(VALUE self){
  return decode(BEncode, self);
}

/*
 * Document-method: encode
 * call-seq:
 *    BEncode.encode(object)
 *
 * Shortcut to _object_.bencode
 */

static VALUE mod_encode(VALUE self, VALUE x){
  return encode(x);
}

void Init_bencode_ext(){
  readId = rb_intern("read");
  BEncode = rb_define_module("BEncode");
  /*
   * Document-class: BEncode::DecodeError
   * Exception for indicating decoding errors.
   */
  DecodeError = rb_define_class_under(BEncode, "DecodeError", rb_eRuntimeError);

  /*
   * Document-class: BEncode::EncodeError
   * Exception for indicating encoding errors.
   */
  EncodeError = rb_define_class_under(BEncode, "EncodeError", rb_eRuntimeError);

  rb_define_singleton_method(BEncode, "decode", decode, 1);
  rb_define_singleton_method(BEncode, "encode", mod_encode, 1);
  rb_define_singleton_method(BEncode, "decode_file", decode_file, 1);

  rb_define_method(BEncode, "bencode", encode, 0);
  rb_define_method(rb_cString, "bdecode", str_bdecode, 0);

  rb_include_module(rb_cObject, BEncode);
}

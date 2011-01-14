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

static long parse_num(char** str, long* len){
  long t = 1, ret = 0;

  if(**str == '-'){
    t = -1;
    ++*str;
    --*len;
  }

  while(*len && **str >= '0' && **str <= '9'){
    ret = ret * 10 + (**str - '0');
    ++*str;
    --*len;
  }

  return ret * t;
}

#define NEXT_CHAR ++str; --len;
#define ELEMENT_SCALAR 0
#define ELEMENT_STRUCT 1
#define ELEMENT_END 2

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
 *    BEncode.decode('i1e') => 1
 *    BEncode.decode('i-1e') => -1
 *    BEncode.decode('6:string') => 'string'
 */

static VALUE decode(VALUE self, VALUE encoded){
  long  len, rlen;
  char* str;
  VALUE ret, container_stack, current_container, key, crt;

  if(!rb_obj_is_kind_of(encoded, rb_cString))
    rb_raise(rb_eTypeError, "String expected");

  len = rlen = RSTRING_LEN(encoded);
  if(!len)
    return Qnil;

  str = RSTRING_PTR(encoded);
  container_stack = rb_ary_new();
  current_container = ret = key = Qnil;

  while(len){
    int state = ELEMENT_SCALAR;
    switch(*str){
      case 'l':
      case 'd':
        crt = *str == 'l' ? rb_ary_new() : rb_hash_new();
        NEXT_CHAR;
        if(NIL_P(current_container)){
          if(max_depth == 0)
            rb_raise(DecodeError, "Structure is too deep!");
          ret = current_container = crt;
          continue;
        }
        state = ELEMENT_STRUCT;
        break;
      case 'i':
        NEXT_CHAR;
        crt = LONG2FIX(parse_num(&str, &len));

        if(!len)
          rb_raise(DecodeError, "Unpexpected integer end!");
        if(*str != 'e')
          rb_raise(DecodeError, "Mailformed integer at %d byte: %c", rlen - len, *str);

        NEXT_CHAR;
        break;
      case '0'...'9':{
        long slen = parse_num(&str, &len);

        if(slen < 0 || (len && *str != ':'))
          rb_raise(DecodeError, "Invalid string length specification at %d: %c", rlen - len, *str);

        if(!len || len < slen + 1)
          rb_raise(DecodeError, "Unexpected string end!");

        crt = rb_str_new(++str, slen);
        str += slen;
        len -= slen + 1;
        break;
      }
      case 'e':
        state = ELEMENT_END;
        NEXT_CHAR;
        break;
      default:
        rb_raise(DecodeError, "Unknown element type at %d: %c!", rlen - len, *str);
    }

    if(state == ELEMENT_END){
      if(NIL_P(current_container))
        rb_raise(DecodeError, "Unexpected container end at %d!", rlen - len);
      current_container = rb_ary_pop(container_stack);
    }else if(NIL_P(current_container)){
      if(NIL_P(ret))
        ret = crt;
      break;
    }else{
      if(TYPE(current_container) == T_ARRAY){
        rb_ary_push(current_container, crt);
      }else if(NIL_P(key)){
        if(TYPE(crt) != T_STRING)
          rb_raise(DecodeError, "Dictionary key must be a string (at %d)!", rlen - len);
        key = crt;
      }else{
        rb_hash_aset(current_container, key, crt);
        key = Qnil;
      }

      if(state == ELEMENT_STRUCT){
        rb_ary_push(container_stack, current_container);
        if(max_depth > -1 && max_depth < RARRAY_LEN(container_stack) + 1)
          rb_raise(DecodeError, "Structure is too deep!");
        current_container = crt;
      }
    }
  }

  if(len)
    rb_raise(DecodeError, "String has garbage on the end (starts at %d: %s).", rlen - len);

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

/*
 * Document-method: max_depth
 * call-seq:
 *    BEncode.max_depth
 *
 * Get maximum depth of parsed structure.
 */

static VALUE get_max_depth(VALUE self){
  return LONG2FIX(max_depth);
}

/*
 * Document-method: max_depth=
 * call-seq:
 *    BEncode.max_depth = _integer_
 *
 * Sets maximum depth of parsed structure.
 * Expects integer greater or equal to 0.
 * By default this value is 5000.
 * Assigning nil will disable depth check.
 */

static VALUE set_max_depth(VALUE self, VALUE depth){
  long t;

  if(NIL_P(depth)){
    max_depth = -1;
    return depth;
  }

  if(!rb_obj_is_kind_of(depth, rb_cInteger))
    rb_raise(rb_eArgError, "Integer expected!");

  t = NUM2LONG(depth);
  if(t < 0)
    rb_raise(rb_eArgError, "Depth must be between 0 and 5000000");

  max_depth = t;
  return depth;
}

void Init_bencode_ext(){
  readId = rb_intern("read");
  max_depth = 5000;
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
  rb_define_singleton_method(BEncode, "max_depth", get_max_depth, 0);
  rb_define_singleton_method(BEncode, "max_depth=", set_max_depth, 1);

  rb_define_method(BEncode, "bencode", encode, 0);
  rb_define_method(rb_cString, "bdecode", str_bdecode, 0);

  rb_include_module(rb_cObject, BEncode);
}

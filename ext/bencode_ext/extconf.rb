#!/usr/bin/ruby -w

require 'mkmf'
$CFLAGS='-g'
$LDFLAGS='-g'
create_makefile('bencode_ext')

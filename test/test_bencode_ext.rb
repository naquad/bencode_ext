require 'helper'

class TestBencodeExt < Test::Unit::TestCase
  def test_encoding
    assert_equal('i1e', 1.bencode)
    assert_equal('i-1e', -1.bencode)
    assert_equal('6:symbol', :symbol.bencode)
    assert_equal('6:string', 'string'.bencode)
    assert_equal('li1ei2ee', [1, 2].bencode)
    assert_equal('d3:keyi10ee', {:key => 10}.bencode)
    assert_equal('ld1:ki1eed1:ki2eed1:kd1:v3:123eee', [{:k => 1}, {:k => 2}, {:k => {:v => '123'}}].bencode)

    assert_raises(BEncode::EncodeError) { STDERR.bencode }
  end

  def test_decoding
    assert_equal(1, 'i1e'.bdecode)
    assert_equal(-1, 'i-1e'.bdecode)
    assert_equal('string', '6:string'.bdecode)
    assert_equal([1, 2], 'li1ei2ee'.bdecode)
    assert_equal({'key' => 10}, 'd3:keyi10ee'.bdecode)
    assert_equal([{'k' => 1}, {'k' => 2}, {'k' => {'v' => '123'}}], 'ld1:ki1eed1:ki2eed1:kd1:v3:123eee'.bdecode)

    assert_raises(BEncode::DecodeError) {'33:unpexpected_end'.bdecode }
    assert_raises(BEncode::DecodeError) { 'i1x'.bdecode }
    assert_nil(''.bdecode)
  end
end

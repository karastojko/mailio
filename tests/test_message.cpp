#include <gtest/gtest.h>
#include <string>
#include <mailio/message.hpp>

using namespace mailio;
using namespace std;

class tmessage : public message {
	public:
		string_t parse_address_name(const string& address_name) const {
			return message::parse_address_name(address_name);
    }
};

/*Orinal code passes it*/
TEST(SimpleTest, BasicAssertions) {
	string testStr = "John Smith";
	string_t ret;

  tmessage msg;
	msg.line_policy(codec::line_len_policy_t::NONE, codec::line_len_policy_t::NONE);
  try {
		ret = msg.parse_address_name(testStr);		
  } catch (codec_error &err) {
    ret.buffer = err.what();	
  }
  // Expect two strings not to be equal.
  EXPECT_EQ(ret, "John Smith");
}

/*Orinal code passes it*/
TEST(NonRepeatTest, BasicAssertions) {
	string testStr = "=?UTF-8?B?Vm9sdGEg0YfQtdGA0LXQtyDQpNC+0YDRg9C8INCT0L7QstC+0YDQuNC8INC/?=";
	string_t ret;

  tmessage msg;
	msg.line_policy(codec::line_len_policy_t::NONE, codec::line_len_policy_t::NONE);
  try {
		ret = msg.parse_address_name(testStr);		
  } catch (codec_error &err) {
    ret.buffer = err.what();	
  }
  // Expect two strings not to be equal.
  EXPECT_EQ(ret, "Volta через Форум Говорим п");
}

/*Orinal code does not pass it*/
TEST(RepeatTest, BasicAssertions) {
	string testStr = "=?UTF-8?B?Vm9sdGEg0YfQtdGA0LXQtyDQpNC+0YDRg9C8INCT0L7QstC+0YDQuNC8INC/?= =?UTF-8?B?0YDQviDQkNC80LXRgNC40LrRgw==?=";
	string_t ret;

  tmessage msg;
	msg.line_policy(codec::line_len_policy_t::NONE, codec::line_len_policy_t::NONE);
  try {
		ret = msg.parse_address_name(testStr);		
  } catch (codec_error &err) {
    ret.buffer = err.what();	
  }
  // Expect two strings not to be equal.
  EXPECT_EQ(ret, "Volta через Форум Говорим про Америку");
}


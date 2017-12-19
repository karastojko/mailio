BOOST_DIR = /usr
PROJECT = mailio
INC_DIR = include
SRC_DIR = src
BUILD_DIR = build
LIB_DIR = lib
TEST_DIR = test
TEST_BUILD_DIR = $(BUILD_DIR)/$(TEST_DIR)
AR = ar
CXX = g++
CXX_FLAGS = -g -std=c++11 -Wall -Wno-unused-local-typedefs -Wno-unused-variable
LD_FLAGS = -lpthread -lboost_system -lboost_regex -lboost_date_time -lboost_random -lssl -lcrypto
INCS_DIR = -I$(INC_DIR) -I$(BOOST_DIR)/include
LIBS_DIR = -L$(BOOST_DIR)/lib
SOURCES = codec.cpp base64.cpp quoted_printable.cpp bit7.cpp bit8.cpp binary.cpp q_codec.cpp mailboxes.cpp mime.cpp message.cpp dialog.cpp smtp.cpp pop3.cpp imap.cpp
TEST_SOURCES = message.cpp smtp.cpp pop3.cpp imap.cpp
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
TEST_BINS = $(basename $(TEST_SOURCES))
TARGET = $(LIB_DIR)/lib$(PROJECT).so
DOC = doc
DOXYGEN = doxygen
DOXYGEN_CONF = doxygen.conf
EXAMPLES_DIR = examples
EXAMPLES_BUILD_DIR = $(BUILD_DIR)/$(EXAMPLES_DIR)
EXAMPLES_SOURCES = smtps_simple_msg.cpp smtp_utf8_qp_msg.cpp smtps_attachment.cpp \
    pop3s_fetch_one.cpp pop3_remove_msg.cpp pop3s_attachment.cpp \
    imaps_stat.cpp imap_remove_msg.cpp
EXAMPLES_BINS = $(basename $(EXAMPLES_SOURCES))

$(info *** Building mailio ***)

all: dirs $(TARGET)

dirs:
	test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)
	test -d $(LIB_DIR) || mkdir -p $(LIB_DIR)
	test -d $(TEST_BUILD_DIR) || mkdir -p $(TEST_BUILD_DIR)

$(TARGET): $(addprefix $(BUILD_DIR)/,$(OBJECTS))
	$(CXX) $^ -shared -fPIC $(LD_FLAGS) $(LIBS_DIR) $(LD_LIBS) -o $@
	$(AR) rcs $(LIB_DIR)/lib$(PROJECT).a $(BUILD_DIR)/*.o

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -c -fPIC $(CXX_FLAGS) $(INCS_DIR) -o $@ $<

tests: $(TEST_BINS)

$(TEST_BINS): $(addprefix $(TEST_BUILD_DIR)/,$(TEST_OBJECTS))
	$(CXX) $(CXX_FLAGS) $(INCS_DIR) $(LIBS_DIR) -L$(LIB_DIR) $(LD_FLAGS) -lboost_unit_test_framework -l$(PROJECT) $(TEST_DIR)/$@.cpp -o $(TEST_BUILD_DIR)/$@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(LIB_DIR)

doc:
	test -d $(DOC) || mkdir -p $(DOC)
	$(DOXYGEN) $(DOXYGEN_CONF)

examples: $(EXAMPLES_BINS)
	test -d $(EXAMPLES_DIR) || mkdir -p $(EXAMPLES_DIR)

$(EXAMPLES_BINS):
	test -d $(EXAMPLES_BUILD_DIR) || mkdir -p $(EXAMPLES_BUILD_DIR)
	$(CXX) $(CXX_FLAGS) $(INCS_DIR) $(LIBS_DIR) -L$(LIB_DIR) $(LD_FLAGS) -l$(PROJECT) $(EXAMPLES_DIR)/$@.cpp -o $(EXAMPLES_BUILD_DIR)/$@
	cp $(EXAMPLES_DIR)/*.png $(EXAMPLES_BUILD_DIR)/

.PHONY: dirs $(TARGET) clean tests doc examples

DEBUG=0
DBUS=0

APP := ./bin/pulse_test


SRC_DIR := ./src
SRCS := \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/libpcm_aac.cpp \
	$(SRC_DIR)/Raspberry_Pi_Record.cpp

OBJS := $(SRCS:.cpp=.o)

CPPFLAGS += \
	-std=c++11 \
	-I"/usr/local/include" \
	-I"../../include" \
	-I"include" \

LDFLAGS += \
	-L"$(CUDA_PATH)/targets/aarch64-linux/lib" \
	-lpthread \
	-L/usr/local/lib\
	-L/usr/lib/aarch64-linux-gnu/\
	-lm\
	-lz\
	-Llib\
	-I/usr/lib/aarch64-linux-gnu/\
	-lfaac\
	-lasound\
	-lpulse-simple\
	-lpulse \

#COMMON :=

CPP:=g++

all: $(APP)

$(CLASS_DIR)/%.o: $(CLASS_DIR)/%.cpp
	$(MAKE) -C  $(CLASS_DIR)


%.o: %.cpp
	@echo "Compiling: $<"
	$(CPP) $(CPPFLAGS) -c $< -o $@

$(APP): $(OBJS)
	@echo "Linking: $@"
	$(CPP) -o $@ $(OBJS) $(CPPFLAGS) $(LDFLAGS)

#	$(CPP) -g  -o  $@ -O0 $(OBJS) $(CPPFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	rm -f $(SRC_DIR)/*.o
	rm -f  $(APP)
	rm -f tags

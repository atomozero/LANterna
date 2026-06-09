# Build del core portabile + CLI di test (g++/Clang, POSIX).
# La UI nativa Haiku (BeAPI) avra' un proprio build separato piu' avanti.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Isrc
BIN      := lanterna-cli
LDLIBS   :=

# Su Haiku servono header BSD (getifaddrs) e libnetwork (socket, DNS).
ifeq ($(shell uname -s),Haiku)
  CXXFLAGS += -D_DEFAULT_SOURCE -I/boot/system/develop/headers/bsd
  LDLIBS   += -lbsd -lnetwork
endif

SRCS := \
  src/net/Subnet.cpp \
  src/net/PortProbe.cpp \
  src/net/Resolver.cpp \
  src/net/ArpCache.cpp \
  src/net/WakeOnLan.cpp \
  src/model/DeviceStore.cpp \
  src/enrich/ReverseDnsEnricher.cpp \
  src/enrich/ArpMacEnricher.cpp \
  src/enrich/OuiDatabase.cpp \
  src/enrich/OuiVendorEnricher.cpp \
  src/enrich/TypeInferenceEnricher.cpp \
  src/enrich/MdnsEnricher.cpp \
  src/enrich/NetBiosEnricher.cpp \
  src/enrich/SnmpEnricher.cpp \
  src/enrich/SsdpEnricher.cpp \
  src/scan/Scanner.cpp \
  src/cli/main.cpp

OBJS := $(SRCS:.cpp=.o)

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(BIN)

.PHONY: all clean

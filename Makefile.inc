CXXFLAGS += -Wall

BUILD_DIR_WITHOUT_MODE = build

ifndef cfg
    cfg=debug
endif

ifeq ($(cfg), debug)
    CXXFLAGS += -O0 -D_DEBUG -g
    BUILD_DIR = $(BUILD_DIR_WITHOUT_MODE)/debug
else
ifeq ($(cfg), release)
    CXXFLAGS += -O3 -DNDEBUG
    BUILD_DIR = $(BUILD_DIR_WITHOUT_MODE)/release
else
    $(error Error! Can not build with configuration $(cfg))
endif
endif

depends = $(sources:%.cpp=$(BUILD_DIR)/%.d)
objs = $(sources:%.cpp=$(BUILD_DIR)/%.o)

-include $(depends)

$(project): $(objs)
	$(CXX) -g -o $(BUILD_DIR)/$@ $^ $(CXXFLAGS)

$(BUILD_DIR)/%.d: $(sources_dir)/%.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) -MM -MT '$(patsubst %.d,%.o,$@) $@' -MF $@ $(CXXFLAGS) $<

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.d

$(BUILD_DIR)/%.o: $(sources_dir)/%.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

clean:
	rm -rf $(BUILD_DIR)

cleanall:
	rm -rf $(BUILD_DIR_WITHOUT_MODE)

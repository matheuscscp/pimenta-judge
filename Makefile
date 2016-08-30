# ==============================================================================
# BEGIN template
# ==============================================================================

# parameters
EXE     = pjudge
LIBS    = -pthread

CXX     = g++ -g -std=c++0x
SRCS    = $(shell find src -name '*.cpp')
HEADERS = $(shell find src -name '*.hpp')
OBJS    = $(addprefix obj/,$(notdir $(SRCS:%.cpp=%.o)))

$(EXE): $(OBJS)
	$(CXX) $(LIBS) $(OBJS) -o $@

obj/%.o: src/%.cpp $(HEADERS)
	mkdir -p obj
	$(CXX) -c $< -o $@

.PHONY: clean

clean:
	rm -rf $(EXE) obj

# ==============================================================================
# END template
# ==============================================================================

.PHONY: install

install:
	cp pjudge /usr/local/bin
	rm -rf /usr/local/share/pjudge
	mkdir /usr/local/share/pjudge
	cp -rf bundle/* /usr/local/share/pjudge

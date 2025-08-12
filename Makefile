NAME := webserv

CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98  -g

PARN_DIR := .
SRC_DIR := $(PARN_DIR)/src
INCLUDE_DIR := $(PARN_DIR)/include
BUILD_DIR := $(PARN_DIR)/build

SRC := webserv.cpp server.cpp utils.cpp parser.cpp httprequest.cpp helpers.cpp url.cpp response.cpp errors.cpp special_response.cpp logger.cpp ClientPool.cpp response_utils.cpp ConfigParser.cpp cgi.cpp

INCLUDE := errors.hpp helpers.hpp parser.hpp webserv.hpp ClientPool.hpp ConfigParser.hpp libs.hpp

INCLUDE := $(addprefix $(INCLUDE_DIR)/,$(INCLUDE))

OBJ := $(SRC:%.cpp=$(BUILD_DIR)/%.o)

all: build $(NAME)

$(NAME): $(OBJ)
	@$(CXX) $(CXXFLAGS) $(OBJ) -o $@
	@echo "\033[1;34m$(NAME) \033[0;34mhas been compiled"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(INCLUDE)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	@echo "\033[0;34mCompiling \033[1;34m$(NAME)"
	@[ -d "$(BUILD_DIR)" ] || mkdir "$(BUILD_DIR)"

clean:
	@rm -f $(OBJ)

fclean: clean
	@rm -rf $(NAME) $(BUILD_DIR)

re: fclean all

.PHONY: all clean fclean re

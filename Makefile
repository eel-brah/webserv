NAME := webserv

CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -g

SRCS := webserv.cpp server.cpp utils.cpp parser.cpp httprequest.cpp helpers.cpp url.cpp response.cpp errors.cpp special_response.cpp logger.cpp ClientPool.cpp response_utils.cpp
OBJS := $(SRCS:.cpp=.o)

INCLUDE := errors.hpp helpers.hpp parser.hpp webserv.hpp ClientPool.hpp

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

%.o: %.cpp $(INCLUDE)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

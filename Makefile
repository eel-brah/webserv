NAME = webserv

CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98
OFLAGS := -g

SRCS := webserv.cpp server.cpp utils.cpp parser.cpp httprequest.cpp helpers.cpp url.cpp response.cpp errors.cpp special_response.cpp logger.cpp ClientPool.cpp response_utils.cpp
OBJS := $(SRCS:.cpp=.o)

INCLUDE := errors.hpp helpers.hpp parser.hpp webserv.hpp ClientPool.hpp


all: $(NAME)

$(NAME): $(OBJS) $(INCLUDE)
	$(CXX) $(OBJS) -o $@


%.o: %.cpp $(INCLUDE)
	$(CXX) $(OFLAGS) -c $< -o $@


clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all


.PHONEY: all clean fclean re

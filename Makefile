NAME = webserv

CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98

SRCS := webserv.cpp server.cpp utils.cpp parser.cpp httprequest.cpp helpers.cpp url.cpp response.cpp errors.cpp
OBJS := $(SRCS:.cpp=.o)


all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $@


%.o: %.cpp
	$(CXX) $(OFLAGS) -c $< -o $@


clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all


.PHONEY: all clean fclean re

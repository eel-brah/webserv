NAME = webserv

CC = c++

OFLAGS = -Wall -Wextra -Werror -std=c++98 -g

OBJS = server.o parser.o httprequest.o helpers.o url.o


all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) -o $@


%.o: %.cpp
	$(CC) $(OFLAGS) -c $< -o $@


clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all


.PHONEY: all clean fclean re


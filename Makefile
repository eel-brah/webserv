NAME = webserv

CC = c++

OFLAGS = -Wall -Wextra -Werror -std=c++98

OBJS = server.o


all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) -o $@


%.o: %.cpp megaphone.hpp
	$(CC) $(OFLAGS) -c $< -o $@


clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all


.PHONEY: all clean fclean re


# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: muel-bak <muel-bak@student.1337.ma>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/06/08 17:17:38 by muel-bak          #+#    #+#              #
#    Updated: 2025/06/14 12:02:41 by muel-bak         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

C := c++
FLAGS := -std=c++98 -Wall -Wextra -Werror
SRC := main.cpp  ConfigParser.cpp
HEADER_FILE :=  ConfigParser.hpp
OBJS := $(SRC:%.cpp=%.o)
NAME :=  nginx

all: $(NAME)

$(NAME): $(OBJS) $(HEADER_FILE)
	@$(C) $(FLAGS) $(OBJS) -o $(NAME)
	@echo "$(NAME) has been compiled"

%.o: %.cpp $(HEADER_FILE)
	@$(C) $(FLAGS) -c $< -o $@

clean:
	@rm -f $(OBJS)
	@echo "The object files have been removed"

fclean: clean
	@rm -f $(NAME)
	@echo "The object files and the program have been removed"

re: fclean all
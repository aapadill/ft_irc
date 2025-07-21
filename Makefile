# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/06/24 11:10:24 by nmeintje          #+#    #+#              #
#    Updated: 2025/06/24 11:10:26 by nmeintje         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME := ircserv

CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17 -Iincludes

SOURCE_DIR := sources
OBJ_DIR := objects
INCLUDE := include

SRC := main.cpp Parser.cpp Server.cpp User.cpp Channel.cpp Logger.cpp Client.cpp Command.cpp
OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o))

RM := rm -rf
MKDIR := mkdir -p

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp #$(INCLUDE)
	$(MKDIR) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re

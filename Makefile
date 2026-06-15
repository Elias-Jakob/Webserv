SRC = src/http/ClientConnection.cpp \
		src/http/HttpRequest.cpp \
		src/http/HttpResponse.cpp \
		src/methods/AMethod.cpp \
		src/methods/Get.cpp \
		src/methods/Post.cpp \
		src/parsers/ABodyParser.cpp \
		src/parsers/FormParser.cpp \
		src/parsers/MultipartParser.cpp \
		src/server/Server.cpp \
		src/server/Socket.cpp \
		src/main.cpp

OBJDIR = obj
OBJ = $(SRC:%.cpp=$(OBJDIR)/%.o)
DEPS = $(OBJ:.o=:.d)

CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -std=c++98 -g
NAME = webserv

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJ) $(DEPS) $(OBJDIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re

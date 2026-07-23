SRC = src/http/ClientConnection.cpp \
		src/http/HttpRequest.cpp \
		src/http/HttpStatus.cpp \
		src/http/MethodExecuter.cpp \
		src/http/ResponseBuilder.cpp \
		src/methods/AMethod.cpp \
		src/methods/Delete.cpp \
		src/methods/Get.cpp \
		src/methods/Post.cpp \
		src/methods/Head.cpp \
		src/parsers/ABodyParser.cpp \
		src/parsers/FormParser.cpp \
		src/parsers/MultipartParser.cpp \
		src/config/ConfigFileParser.cpp \
		src/server/Server.cpp \
		src/server/ServerEventLoop.cpp \
		src/server/CGIHandler.cpp \
		src/server/ReceiveHandler.cpp \
		src/server/SendHandler.cpp \
		src/cgi/CGIProcessLauncher.cpp \
		src/cgi/CGIError.cpp \
		src/cgi/ChildProcess.cpp \
		src/utils/Epoll.cpp \
		src/utils/cgi.cpp \
		src/utils/utils.cpp \
		src/main.cpp

OBJDIR = obj
INCDIR = ./inc
OBJ = $(SRC:%.cpp=$(OBJDIR)/%.o)
DEPS = $(OBJ:.o=.d)

CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -std=c++98 -g
INCFLAGS = -I$(INCDIR)
NAME = webserv

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJ) $(DEPS) $(OBJDIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

run:
	./webserv confFiles/webserv.conf

runre: re run

.PHONY: all clean fclean re run

NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

ifeq ($(DEBUG_RECV),1)
	CXXFLAGS += -DDEBUG_RECV
endif
INCLUDES = -I includes
DEPFLAGS = -MMD -MP

SRC_DIR = srcs
OBJ_DIR = obj

SRCS = main.cpp \
       channel/Channel.cpp \
       channel/ChannelManager.cpp \
       channel/InviteList.cpp \
       channel/MemberList.cpp \
       channel/ModeState.cpp \
       channel/OperatorList.cpp \
       client/Client.cpp \
       client/ClientManager.cpp \
       client/ReceiveBuffer.cpp \
       client/SendBuffer.cpp \
       event/Event.cpp \
       parser/Parser.cpp \
       server/Listener.cpp \
       server/Server.cpp \
       server/ClientIO.cpp \
       server/Poll.cpp \
       server/Signal.cpp \
       serverMessage/Message.cpp \
       serverCommand/Command.cpp \
       serverCommand/serverClientCommand/ClientCommand.cpp \
       serverCommand/serverChannelCommand/CommandHelper.cpp \
       serverCommand/serverChannelCommand/command/Join.cpp \
       serverCommand/serverChannelCommand/command/Part.cpp \
       serverCommand/serverChannelCommand/command/Privmsg.cpp \
       serverCommand/serverChannelCommand/command/Names.cpp \
       serverCommand/serverChannelCommand/command/Who.cpp \
       serverCommand/serverChannelCommand/command/Topic.cpp \
       serverCommand/serverChannelCommand/command/Invite.cpp \
       serverCommand/serverChannelCommand/command/Kick.cpp \
       serverCommand/serverChannelCommand/mode/ModeChange.cpp \
       serverCommand/serverChannelCommand/mode/ModeChecker.cpp \
       serverCommand/serverChannelCommand/mode/ModeParser.cpp \
       serverCommand/serverChannelCommand/mode/ModeApplier.cpp \
       serverCommand/serverChannelCommand/mode/Mode.cpp \

OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re

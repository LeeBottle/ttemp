# IRC 서버 모듈 가이드

이 문서는 `final_debug_recv_toggle` 코드 기준으로 모듈 경계, 실행 흐름, 명령 처리 방식, irssi/nc 테스트 방법을 정리한 문서다.

## 현재 코드 기준 핵심 변경점

| 항목 | 현재 처리 |
|---|---|
| receive debug log | 기본 빌드에서는 출력하지 않음 |
| debug log 켜기 | `make DEBUG_RECV=1` |
| hex log | 제거됨 |
| `CAP LS` | `:ircserv CAP * LS :` 응답 |
| `CAP END` | 조용히 무시 |
| client `PING` | `PONG` 응답 |
| `PONG` | 현재 서버가 먼저 PING을 보내지 않으므로 no-op |
| server-driven heartbeat | 구현하지 않음 |
| `QUIT` | `ERROR :Closing Link` 전송 후 연결 종료 |
| user `MODE nick` | `221 nick +i` 응답 |
| user `MODE nick +i` | 중복 `221` 없이 조용히 무시 |
| channel default mode | `+t` |
| `MODE #channel b` | ban list는 구현하지 않고 `368 End of Channel Ban List`만 응답 |
| `NAMES` | `353`, `366` 응답. raw에는 channel name 포함 |

## 근거 자료

| 주제 | 링크 |
|---|---|
| IRC 메시지 형식, `CR-LF` 종료 규칙 | [RFC 1459 - 2.3 Messages](https://www.rfc-editor.org/rfc/rfc1459) |
| IRC 명령 목록과 의미 | [RFC 1459 - 4 Message details](https://www.rfc-editor.org/rfc/rfc1459) |
| `socket()` | [Linux man page - socket(2)](https://man7.org/linux/man-pages/man2/socket.2.html) |
| `poll()` | [Linux man page - poll(2)](https://man7.org/linux/man-pages/man2/poll.2.html) |
| `recv()` | [Linux man page - recv(2)](https://man7.org/linux/man-pages/man2/recv.2.html) |
| `send()` | [Linux man page - send(2)](https://man7.org/linux/man-pages/man2/send.2.html) |

## 전체 디렉토리 구조

```text
.
├── Makefile
├── includes
│   ├── channel
│   ├── client
│   ├── event
│   ├── parser
│   ├── server
│   ├── serverCommand
│   │   ├── serverChannelCommand
│   │   │   ├── command
│   │   │   └── mode
│   │   └── serverClientCommand
│   └── serverMessage
└── srcs
    ├── channel
    ├── client
    ├── event
    ├── parser
    ├── server
    ├── serverCommand
    │   ├── serverChannelCommand
    │   │   ├── command
    │   │   └── mode
    │   └── serverClientCommand
    └── serverMessage
```

## 전체 모듈 그림

```text
main
 -> server
    -> Server
    -> Listener
    -> Poll
    -> ClientIO
    -> Signal
 -> event
    -> Event
 -> client
    -> Client
    -> ClientManager
    -> ReceiveBuffer
    -> SendBuffer
 -> parser
    -> Parser
 -> serverMessage
    -> Message
 -> serverCommand
    -> Command
    -> serverClientCommand
       -> ClientCommand
    -> serverChannelCommand
       -> CommandHelper
       -> command
          -> Join
          -> Part
          -> Privmsg
          -> Names
          -> Who
          -> Topic
          -> Invite
          -> Kick
       -> mode
          -> Mode
          -> ModeChecker
          -> ModeParser
          -> ModeApplier
          -> ModeChange
 -> channel
    -> ChannelManager
    -> Channel
    -> MemberList
    -> OperatorList
    -> InviteList
    -> ModeState
```

## 모듈 책임 요약

| 모듈 | 책임 |
|---|---|
| `main` | 실행 인자 검사, port/password 검증, `Server` 시작 |
| `server` | listen socket 준비, client accept, client recv/send, poll 결과 분기, terminal 입력 처리 |
| `event` | `poll()` 호출과 system error 출력 |
| `client` | client 한 명의 상태와 receive/send buffer 저장 |
| `parser` | 한 줄 IRC message를 command name, params, type으로 변환 |
| `serverMessage` | receive buffer에서 line을 꺼내 parser와 command 실행으로 연결 |
| `serverCommand` | 실제 IRC 명령 실행 |
| `channel` | channel 상태, member/operator/invite/mode/topic 관리 |

## 전체 실행 흐름

```text
main(argc, argv)
 -> Server::run()
    -> Signal::setup()
    -> Listener::setup()
       -> socket()
       -> setsockopt(SO_REUSEADDR)
       -> fcntl(O_NONBLOCK)
       -> bind()
       -> listen()
    -> loop
       -> Poll::build()
       -> Event::wait()
       -> Server::handlePoll()
```

client message 처리 흐름은 아래처럼 이어진다.

```text
TCP bytes
 -> ClientIO::receive()
 -> Client::ReceiveBuffer
 -> Message::process()
 -> ReceiveBuffer::pop()
 -> Message::handle()
 -> Parser::parse()
 -> Command::execute()
 -> ClientCommand / channel command / Mode
 -> Client::SendBuffer
 -> ClientIO::send()
```

중요한 점은 `recv()`는 byte만 읽고, command 실행은 하지 않는다는 것이다. `recv()`로 받은 bytes는 먼저 `ReceiveBuffer`에 쌓이고, `Message::process()`가 `\r\n` 단위로 line을 꺼내 command 흐름으로 넘긴다.

---

# `main` 모듈

## 파일

| 파일 | 역할 |
|---|---|
| `srcs/main.cpp` | 실행 인자 검사, port/password 검증, `Server` 생성 |

## 책임

`main`은 IRC 명령이나 socket loop를 직접 처리하지 않는다.

```text
argc 검사
 -> port 문자열을 int로 변환
 -> port 범위 검사: 1 ~ 65535
 -> password 검사
 -> Server server(port, password)
 -> server.run()
```

password는 빈 문자열이면 안 되고, 너무 길어도 안 된다. `\0`, `\r`, `\n`도 거른다.

---

# `server` 모듈

## 파일

| 파일 | 역할 |
|---|---|
| `Server.hpp/cpp` | 서버 전체 흐름 조립, poll 결과 분기 |
| `Listener.hpp/cpp` | listen socket 생성, option 설정, bind/listen, accept |
| `Poll.hpp/cpp` | `pollfd` 목록 구성 |
| `ClientIO.hpp/cpp` | client socket `recv()` / `send()` / 제거 |
| `Signal.hpp/cpp` | `SIGINT`, `SIGQUIT` 종료 요청 처리 |

## `Server`

`Server`는 네트워크 흐름의 중심이다. 하지만 실제 IRC command 로직은 `Command` 쪽으로 넘긴다.

| 멤버 | 의미 |
|---|---|
| `_password` | 서버 password |
| `_clients` | 연결된 client 목록 |
| `_channels` | 생성된 channel 목록 |
| `_listener` | listen socket 담당 |
| `_poll` | poll 대상 fd 목록 생성 |
| `_clientIO` | client fd의 receive/send 담당 |
| `_message` | receive buffer line을 parser/command 흐름으로 넘김 |
| `_event` | `poll()` 호출 담당 |
| `_signal` | 종료 signal 상태 |

`Server::run()`은 아래 순서로 움직인다.

```text
Signal::setup()
 -> Listener::setup()
 -> while (!shouldStop)
    -> Poll::build()
    -> Event::wait()
    -> handlePoll()
```

`Server::handlePoll()`는 fd 종류에 따라 나눈다.

| fd | 처리 |
|---|---|
| `STDIN_FILENO` | `DIE` 입력 확인 |
| listen socket fd | 새 client accept |
| client fd | `POLLIN`, `POLLOUT`, error 처리 |

`Server::handleClient()` 흐름은 아래와 같다.

```text
POLLERR / POLLHUP / POLLNVAL
 -> client 제거

POLLIN
 -> ClientIO::receive()
 -> Message::process()

POLLOUT
 -> ClientIO::send()
```

`Message::process()`가 `false`를 반환하면, 현재 명령은 연결 종료를 뜻한다. 대표적으로 `QUIT`이 여기에 해당한다. 이 경우 서버는 남은 send buffer를 한 번 보내고 client를 제거한다.

## `Listener`

`Listener::setup()` 흐름:

```text
socket(AF_INET, SOCK_STREAM, 0)
 -> setsockopt(SO_REUSEADDR)
 -> fcntl(O_NONBLOCK)
 -> bind(INADDR_ANY, port)
 -> listen(SOMAXCONN)
```

`acceptClient()`는 새 client fd를 받고, 그 fd도 non-blocking으로 바꾼다.

| 상황 | 처리 |
|---|---|
| `accept()` 성공 | client fd 반환 |
| `EINTR` | 정상 재시도 가능 상태 |
| `EAGAIN/EWOULDBLOCK` | 더 받을 client 없음. `clientFd = -1` |
| 그 외 error | system error 출력 후 실패 |

## `Poll`

`Poll::build()`는 매 loop마다 poll 대상 목록을 다시 만든다.

| 대상 | event |
|---|---|
| terminal stdin | `POLLIN`, 단 stdin이 tty일 때만 |
| listen socket fd | `POLLIN` |
| client fd | 기본 `POLLIN` |
| send buffer가 있는 client fd | `POLLIN | POLLOUT` |

send buffer에 데이터가 생기면 다음 loop에서 해당 fd가 `POLLOUT` 대상에 포함된다.

## `ClientIO`

`ClientIO::receive()`는 kernel socket receive buffer에서 bytes를 읽어서 `ReceiveBuffer`에 붙인다.

```text
kernel socket receive buffer
 -> recv()
 -> char buffer[512]
 -> Client::ReceiveBuffer::append()
```

`recv()` 결과별 처리:

| 결과 | 의미 | 처리 |
|---|---|---|
| `> 0` | bytes 읽음 | receive buffer에 저장하고 계속 읽기 |
| `0` | 상대가 연결 종료 | client 제거 |
| `-1`, `EINTR` | signal 때문에 끊김 | 다시 시도 |
| `-1`, `EAGAIN/EWOULDBLOCK` | 지금 더 읽을 데이터 없음 | 정상 종료 |
| 그 외 | socket error | client 제거 |

`ClientIO::send()`는 `SendBuffer`에 쌓인 bytes를 kernel socket send buffer로 넘긴다.

```text
Client::SendBuffer
 -> send()
 -> kernel socket send buffer
 -> TCP로 client에게 전송
```

partial send가 가능하므로 실제로 보낸 byte 수만큼만 send buffer에서 제거한다.

## DEBUG_RECV

기본 빌드는 receive log를 출력하지 않는다.

```sh
make fclean && make
```

partial packet을 확인할 때만 debug build를 사용한다.

```sh
make fclean && make DEBUG_RECV=1
```

이때 출력되는 로그 예시:

```text
[recv raw] fd=4 bytes=2 text=[PA]
[recv raw] fd=4 bytes=7 text=[SS pass\r\n]
received from fd 4: PASS pass
```

현재 debug log는 text만 보여준다. `\r`, `\n`은 escape해서 보여주고, hex 출력은 제거했다.

---

# `client` 모듈

## 파일

| 파일 | 역할 |
|---|---|
| `Client.hpp/cpp` | client fd, 등록 상태, nickname/user/realname, receive/send buffer 소유 |
| `ClientManager.hpp/cpp` | client 생성, 삭제, fd/nickname 검색 |
| `ReceiveBuffer.hpp/cpp` | `recv()`로 받은 raw bytes 저장, `\r\n` 기준 line 추출 |
| `SendBuffer.hpp/cpp` | 나중에 `send()`할 bytes 저장 |

## `Client`

`Client`는 연결된 client 한 명의 상태 저장소다.

| 멤버 | 의미 |
|---|---|
| `_fd` | client socket fd |
| `_hasPassword` | `PASS` 성공 여부 |
| `_registered` | `PASS + NICK + USER` 완료 여부 |
| `_nickname` | IRC nickname |
| `_username` | IRC username |
| `_realname` | IRC realname |
| `_receive` | 받은 raw bytes |
| `_send` | 보낼 bytes |

등록 완료 조건:

```text
PASS 성공
NICK 설정
USER 설정
 -> _registered = true
```

`Client::prefix()`는 message prefix를 만든다.

| 상태 | prefix |
|---|---|
| nickname 없음 | `*` |
| username 없음 | `nick!nick@localhost` |
| username 있음 | `nick!username@localhost` |

## `ReceiveBuffer`

`ReceiveBuffer`는 TCP stream을 IRC message line으로 자르는 역할만 한다.

| 함수 | 역할 |
|---|---|
| `append()` | `recv()`로 받은 bytes를 뒤에 붙임 |
| `pop()` | `\r\n` 기준으로 line 하나 꺼냄 |
| `remove()` | 앞쪽 bytes 제거 |
| `data()` | 현재 raw buffer 확인 |

IRC message는 `CR-LF`, 즉 `\r\n`으로 끝난다.

```text
raw buffer: "PASS pa"
 -> pop() 실패

raw buffer: "PASS pass\r\nNICK alice\r\n"
 -> pop() = "PASS pass"
 -> pop() = "NICK alice"
```

## `SendBuffer`

| 함수 | 역할 |
|---|---|
| `append()` | 보낼 message 저장 |
| `hasData()` | 보낼 데이터가 있는지 확인 |
| `data()` | `send()`에 넘길 pointer 반환 |
| `size()` | 보낼 byte 수 반환 |
| `remove()` | 이미 보낸 bytes 제거 |

---

# `parser` 모듈

## 파일

| 파일 | 역할 |
|---|---|
| `Parser.hpp/cpp` | line을 command name, params, type으로 파싱 |

`Parser`는 receive buffer에서 line을 꺼내지 않는다. line 추출은 `ReceiveBuffer::pop()`이 한다.

`Parser`가 만드는 값:

| 멤버 | 의미 |
|---|---|
| `_name` | 대문자로 정규화된 command 이름 |
| `_params` | command parameter 목록 |
| `_type` | `Parser::Type` enum |

지원 type:

```text
UNKNOWN, CAP, PING, PONG, QUIT,
PASS, NICK, USER,
JOIN, PART, PRIVMSG, NAMES, WHO, TOPIC,
INVITE, KICK, MODE
```

파싱 예시:

```text
입력:
PRIVMSG bob :hello bob

결과:
name   = PRIVMSG
type   = Parser::PRIVMSG
params = ["bob", "hello bob"]
```

trailing parameter는 `:` 뒤쪽 전체를 하나의 parameter로 묶는다.

```text
TOPIC #test :hello world
 -> params = ["#test", "hello world"]
```

command name은 대문자로 정규화한다.

| 입력 | 내부 name |
|---|---|
| `join` | `JOIN` |
| `Join` | `JOIN` |
| `JOIN` | `JOIN` |

---

# `serverMessage` 모듈

## 파일

| 파일 | 역할 |
|---|---|
| `Message.hpp/cpp` | receive buffer line을 parser와 command 실행 흐름으로 연결 |

`Message`는 실제 IRC 명령을 실행하지 않는다.

```text
ReceiveBuffer
 -> pop(line)
 -> Parser::parse(line)
 -> Command::execute(client, parser)
```

| 작업 | 처리 위치 |
|---|---|
| `\r\n` 기준 line 추출 | `Message::process()` + `ReceiveBuffer::pop()` |
| 빈 line 무시 | `Parser::parse()` 결과로 무시 |
| command type 분기 | `Command::execute()` |
| 실제 command 실행 | `ClientCommand`, channel command, `Mode` |

---

# `serverCommand` 모듈

## 파일

| 파일 | 역할 |
|---|---|
| `Command.hpp/cpp` | `Parser::Type`을 보고 실제 command 객체에 위임 |
| `serverClientCommand/ClientCommand.hpp/cpp` | 서버나 nickname 대상 command 처리 |
| `serverChannelCommand/CommandHelper.hpp/cpp` | channel command 공통 reply/broadcast helper |
| `serverChannelCommand/command/*.hpp/cpp` | `JOIN`, `PART`, `PRIVMSG`, `NAMES`, `WHO`, `TOPIC`, `INVITE`, `KICK` 처리 |
| `serverChannelCommand/mode/*.hpp/cpp` | `MODE` 처리 |

## `Command`

`Command::execute()`는 `Parser::Type`을 보고 맞는 객체로 바로 넘긴다.

```text
Parser::PASS    -> ClientCommand::pass()
Parser::NICK    -> ClientCommand::nick()
Parser::USER    -> ClientCommand::user()
Parser::CAP     -> ClientCommand::cap()
Parser::PING    -> ClientCommand::ping()
Parser::PONG    -> ClientCommand::pong()
Parser::QUIT    -> ClientCommand::quit()
Parser::JOIN    -> Join::handle()
Parser::PART    -> Part::handle()
Parser::NAMES   -> Names::handle()
Parser::WHO     -> Who::handle()
Parser::TOPIC   -> Topic::handle()
Parser::INVITE  -> Invite::handle()
Parser::KICK    -> Kick::handle()
Parser::MODE    -> Mode::handle()
```

`PRIVMSG`만 target을 보고 한 번 더 나눈다.

| 입력 | 위임 |
|---|---|
| `PRIVMSG bob :hello` | `ClientCommand::privmsg()` |
| `PRIVMSG #test :hello` | `Privmsg::handle()` |

## `ClientCommand`

`ClientCommand`는 서버 자체 또는 특정 nickname을 대상으로 하는 command를 처리한다.

| 명령 | 함수 | 역할 |
|---|---|---|
| `PASS` | `pass()` | password 검증 |
| `NICK` | `nick()` | nickname 설정, 중복 검사 |
| `USER` | `user()` | username, realname 설정 |
| `CAP` | `cap()` | irssi capability 요청 최소 처리 |
| `PING` | `ping()` | client PING에 PONG 응답 |
| `PONG` | `pong()` | 현재는 no-op |
| `QUIT` | `quit()` | `ERROR :Closing Link` 후 연결 종료 요청 |
| `PRIVMSG nick` | `privmsg()` | nickname 대상 개인 메시지 전송 |
| unknown | `unknown()` | `421 Unknown command` |

등록 완료 후 한 번만 아래 응답을 보낸다.

```text
:ircserv 001 <nick> :Welcome to ircserv
:ircserv 221 <nick> +i
```

### `CAP`

irssi는 접속 초기에 `CAP LS`, `CAP END`를 보낼 수 있다.

| 입력 | 응답 |
|---|---|
| `CAP LS` | `:ircserv CAP * LS :` |
| `CAP END` | 응답 없음 |
| 그 외 CAP | 응답 없음 |

목적은 reference client가 `CAP Unknown command`를 띄우지 않게 하는 것이다. capability 자체는 제공하지 않는다.

### `PING` / `PONG`

현재 서버는 client가 보낸 `PING`에만 응답한다.

```text
client -> server: PING ircserv
server -> client: PONG ircserv :ircserv
```

현재 서버는 주기적으로 client에게 `PING`을 보내지 않는다. 그래서 `PONG` timeout도 검사하지 않는다.

```text
구현함:
  client PING -> server PONG

구현하지 않음:
  server periodic PING
  client PONG timeout check
```

이 정도면 현재 과제 테스트와 irssi 호환에는 충분하다.

## channel command

`MODE`를 제외한 channel command는 명령 하나당 클래스 하나로 나뉜다.

| 명령 | 클래스 | 역할 |
|---|---|---|
| `JOIN` | `Join` | channel 참가 |
| `PART` | `Part` | channel 나가기 |
| `PRIVMSG #channel` | `Privmsg` | channel member들에게 message 전송 |
| `NAMES` | `Names` | channel member 목록 응답 |
| `WHO` | `Who` | channel member 상세 목록 응답 |
| `TOPIC` | `Topic` | topic 조회 또는 설정 |
| `INVITE` | `Invite` | target client를 invite list에 추가 |
| `KICK` | `Kick` | target client를 channel에서 제거 |

## `CommandHelper`

channel command들이 공유하는 static helper다.

| helper | 역할 |
|---|---|
| `validChannel()` | `#`로 시작하고 공백/`,`/CR/LF/NUL이 없는 channel 이름 검사 |
| `reply()` | client send buffer에 응답 추가 |
| `namesReply()` | `353`, `366` names 응답 추가 |
| `topicReply()` | `331` 또는 `332` topic 응답 추가 |
| `toAll()` | channel 전체 member에게 message 추가 |
| `toOthers()` | sender를 제외한 member에게 message 추가 |
| `target()` | nickname이 없으면 `*`, 있으면 nickname 반환 |
| `memberNames()` | operator는 `@nick`, 일반 member는 `nick`으로 목록 생성 |

---

# channel command 상세

## `JOIN`

```text
등록 여부 검사
 -> parameter 검사
 -> channel 이름 검사
 -> ChannelManager::findOrCreate()
 -> invite/key/limit 검사
 -> member 추가
 -> 첫 member면 operator 추가
 -> invite list에서 제거
 -> JOIN message 전파
 -> topic 있으면 topic reply
 -> names reply
```

실패 응답:

| 상황 | 응답 |
|---|---|
| 미등록 | `451` |
| parameter 부족 | `461 JOIN` |
| 잘못된 channel 이름 | `403` |
| invite-only인데 invite 없음 | `473` |
| key 불일치 | `475` |
| limit 초과 | `471` |

## `PART`

```text
등록 여부 검사
 -> parameter 검사
 -> channel 존재 검사
 -> member 여부 검사
 -> PART message 전파
 -> member/operator/invite에서 제거
 -> channel이 비었으면 삭제
```

`PART #test :bye`처럼 메시지가 있으면 그 메시지를 사용하고, 없으면 `Leaving`을 기본값으로 쓴다.

## `PRIVMSG #channel`

channel message는 sender를 제외한 channel member들에게 전송한다.

| 상황 | 응답 |
|---|---|
| 미등록 | `451` |
| recipient 없음 | `411` |
| text 없음 | `412` |
| channel 없음 | `403` |
| sender가 channel member가 아님 | `404 Cannot send to channel` |
| 정상 | `:<prefix> PRIVMSG #channel :text`를 다른 member에게 전송 |

현재 sender에게는 channel message를 echo하지 않는다.

## `NAMES`

`NAMES #test`는 아래 형식으로 응답한다.

```text
:ircserv 353 <nick> = #test :@alice bob kevin
:ircserv 366 <nick> #test :End of /NAMES list
```

irssi UI에서는 channel 이름이 축약되어 보일 수 있다. raw 응답 확인은 아래처럼 한다.

```text
/rawlog open /tmp/irssi-alice.log
/names #test
/rawlog close
```

```sh
grep " 353 " /tmp/irssi-alice.log
grep " 366 " /tmp/irssi-alice.log
```

rawlog에 `#test`가 들어 있으면 서버 응답은 정상이다.

## `WHO`

`WHO #test`는 member마다 `352`를 보내고, 마지막에 `315`를 보낸다.

```text
:ircserv 352 <requester> #test <user> localhost ircserv <nick> H :0 <realname>
:ircserv 315 <requester> #test :End of WHO list
```

channel이 없어도 현재 구현은 `315 End of WHO list`로 끝낸다.

## `TOPIC`

| 입력 | 동작 |
|---|---|
| `TOPIC #test` | topic 조회 |
| `TOPIC #test :hello` | topic 설정 |
| `TOPIC #test :` | 빈 문자열 topic으로 설정, 사실상 topic 삭제처럼 동작 |

응답:

| 상황 | 응답 |
|---|---|
| topic 없음 | `331 No topic is set` |
| topic 있음 | `332 <topic>` |
| topic 변경 성공 | channel 전체에 `TOPIC` 전파 |

channel mode가 `+t`이면 operator만 topic을 바꿀 수 있다. 현재 channel default mode가 `+t`이므로 첫 member, 즉 operator가 아닌 일반 member는 기본적으로 topic 변경 권한이 없다.

## `INVITE`

```text
등록 여부 검사
 -> parameter 검사
 -> target nick 검사
 -> channel 존재 검사
 -> sender가 channel member인지 검사
 -> sender가 operator인지 검사
 -> target이 이미 channel member인지 검사
 -> invite list에 추가
 -> sender에게 341 응답
 -> target에게 INVITE message 전송
```

## `KICK`

```text
등록 여부 검사
 -> parameter 검사
 -> channel 존재 검사
 -> sender member/operator 검사
 -> target member 검사
 -> KICK message를 channel 전체에 전파
 -> target을 member/operator/invite에서 제거
 -> channel이 비었으면 삭제
```

---

# `MODE` 모듈

## 파일

| 파일 | 역할 |
|---|---|
| `Mode.hpp/cpp` | `MODE` 전체 흐름 조립 |
| `ModeChecker.hpp/cpp` | 등록 여부, 대상, channel 존재, 권한 검사 |
| `ModeParser.hpp/cpp` | mode 문자열과 parameter 해석 |
| `ModeApplier.hpp/cpp` | 수집된 변경을 channel 상태에 적용 |
| `ModeChange.hpp/cpp` | mode 변경 자료 구조 |

## 지원 mode

| mode | 의미 | 구현 |
|---|---|---|
| `+i`, `-i` | invite-only on/off | 구현 |
| `+t`, `-t` | topic restricted on/off | 구현 |
| `+k`, `-k` | channel key 설정/해제 | 구현 |
| `+l`, `-l` | user limit 설정/해제 | 구현 |
| `+o`, `-o` | operator 부여/해제 | 구현 |
| `b` | ban list query | ban 기능 없이 `368`만 응답 |

## `ModeState` 기본값

| 상태 | 기본값 |
|---|---|
| topic | empty |
| invite-only | false |
| topic restricted | true |
| key | 없음 |
| limit | 없음 |

따라서 새 channel의 기본 mode string은 `+t`다.

## `MODE #channel`

mode 조회:

```text
MODE #test
```

응답 예시:

```text
:ircserv 324 alice #test +t
```

key/limit이 있으면 mode parameter도 뒤에 붙는다.

```text
:ircserv 324 alice #test +tkl pass 3
```

## `MODE #channel +o bob`

```text
등록 여부 검사
 -> channel 존재 검사
 -> sender가 channel member인지 검사
 -> sender가 operator인지 검사
 -> bob이 channel member인지 검사
 -> operator 추가
 -> MODE message를 channel 전체에 전파
```

전파 예시:

```text
:alice!alice@localhost MODE #test +o bob
```

## `MODE #channel b`

irssi는 channel sync 과정에서 ban list를 조회할 수 있다.

```text
MODE #test b
```

현재 서버는 ban 기능을 구현하지 않는다. 대신 irssi가 에러를 띄우지 않도록 아래 응답만 보낸다.

```text
:ircserv 368 alice #test :End of Channel Ban List
```

이 처리는 operator 권한 검사보다 먼저 수행된다. 그래서 일반 member가 irssi 기본 설정으로 join해도 `You're not channel operator`가 뜨지 않는다.

## user `MODE`

irssi는 등록 직후 user mode를 조회하거나 설정하려 할 수 있다.

| 입력 | 처리 |
|---|---|
| `MODE alice` | `221 alice +i` 응답 |
| `MODE alice +i` | 조용히 무시 |
| `MODE otherNick ...` | `502 Cant change mode for other users` |

---

# `channel` 모듈

## 파일

| 파일 | 역할 |
|---|---|
| `Channel.hpp/cpp` | channel 하나의 중심 객체 |
| `ChannelManager.hpp/cpp` | channel 목록 생성, 삭제, 검색 |
| `MemberList.hpp/cpp` | member 목록 관리 |
| `OperatorList.hpp/cpp` | operator 목록 관리 |
| `InviteList.hpp/cpp` | invite 목록 관리 |
| `ModeState.hpp/cpp` | topic, invite-only, topic restriction, key, limit 상태 |

## `Channel`

| 멤버 | 의미 |
|---|---|
| `_name` | channel 이름 |
| `_members` | 참가 client 목록 |
| `_operators` | operator client 목록 |
| `_invites` | invite된 client 목록 |
| `_modes` | topic/mode 상태 |

## `ChannelManager`

| 함수 | 역할 |
|---|---|
| `findOrCreate()` | channel이 있으면 반환, 없으면 생성 |
| `find()` | 이름으로 channel 찾기 |
| `removeEmpty()` | member가 없으면 channel 삭제 |
| `removeClientFromAll()` | client 종료 시 모든 channel에서 제거 |
| `clear()` | 전체 channel 삭제 |

client가 연결 종료되면 모든 channel에서 제거된다.

```text
client close
 -> ClientIO::remove()
 -> ChannelManager::removeClientFromAll()
 -> ClientManager::removeByFd()
```

---

# 모듈 경계 요약

| 작업 | 담당 |
|---|---|
| port/password 검증 | `main` |
| socket 생성, bind, listen | `Listener` |
| `pollfd` 목록 만들기 | `Poll` |
| `poll()` 대기 | `Event` |
| 새 client accept | `Server` + `Listener` |
| client bytes 읽기 | `ClientIO` |
| receive buffer 저장과 `pop()` 제공 | `ReceiveBuffer` |
| receive buffer에서 line 반복 처리 | `Message` |
| line을 type/params로 파싱 | `Parser` |
| command type으로 handler 선택 | `Command` |
| 서버/nickname 대상 command 실행 | `ClientCommand` |
| channel 대상 command 실행 | `Join`, `Part`, `Privmsg`, `Names`, `Who`, `Topic`, `Invite`, `Kick` |
| channel mode 실행 | `Mode`, `ModeChecker`, `ModeParser`, `ModeApplier` |
| client 상태 저장 | `Client` |
| channel 상태 저장 | `Channel` |

---

# 테스트 가이드

## 빌드

기본 빌드:

```sh
make fclean && make
```

debug receive log 빌드:

```sh
make fclean && make DEBUG_RECV=1
```

현재 코드 기준으로 `make fclean && make`는 통과했다.

## 서버 실행

```sh
./ircserv 6667 pass
```

예상 출력:

```text
server is listening on port 6667
```

## 서버 종료

server terminal에 입력:

```text
DIE
```

또는 `Ctrl-C`.

예상 출력:

```text
server shutting down
```

## `nc` 등록 테스트

`nc`는 `-C` 옵션을 쓰면 line 끝에 `\r\n`을 붙인다.

```sh
nc -C localhost 6667
```

입력:

```text
PASS pass
NICK alice
USER alice 0 * :Alice
```

예상 응답:

```text
:ircserv 001 alice :Welcome to ircserv
:ircserv 221 alice +i
```

## partial packet 테스트

server는 debug build로 실행한다.

```sh
make fclean && make DEBUG_RECV=1
./ircserv 6667 pass
```

client:

```sh
nc localhost 6667
```

조각 입력 예시:

```text
PA
```

이 시점에는 `\r\n`이 없으므로 command가 실행되지 않아야 한다.

이어서:

```text
SS pass
NICK alice
USER alice 0 * :Alice
```

`\r\n`이 들어온 뒤에야 `PASS pass`가 하나의 line으로 처리되어야 한다.

## irssi 테스트 설정

alice:

```sh
irssi --home=/tmp/irssi-alice -n alice
```

kevin:

```sh
irssi --home=/tmp/irssi-kevin -n kevin
```

주의:

```sh
irssi --home = /tmp/irssi-kevin -n kevin
```

처럼 `=` 앞뒤에 공백을 넣으면 안 된다. 이렇게 실행하면 현재 디렉토리에 `=` 디렉토리가 생기고, 그 안에 config가 만들어질 수 있다.

irssi 내부에서 서버 연결:

```text
/connect localhost 6667 pass
```

빠른 채팅 테스트를 위해 irssi command queue 설정을 조정할 수 있다.

```text
/set cmds_max_at_once 100
/set cmd_queue_speed 0msec
```

설정을 저장하려면:

```text
/save
```

단, `--home`이 다르면 config도 따로 저장된다.

| 실행 방식 | config 위치 |
|---|---|
| `irssi` | `~/.irssi/config` |
| `irssi --home=/tmp/irssi-alice` | `/tmp/irssi-alice/config` |
| `irssi --home=/tmp/irssi-kevin` | `/tmp/irssi-kevin/config` |

## irssi 기본 접속 기대값

접속 직후 불필요한 에러가 없어야 한다.

뜨면 안 되는 예시:

```text
CAP Unknown command
PING Unknown command
alice: No such channel
You're not channel operator
```

정상적으로 보일 수 있는 예시:

```text
Capabilities supported:
Welcome to ircserv
Your user mode is [+i]
```

## channel 기본 테스트

alice:

```text
/join #test
```

bob 또는 kevin:

```text
JOIN #test
```

확인할 것:

```text
첫 member인 alice는 operator가 된다.
JOIN 후 353/366 NAMES 응답이 온다.
channel default mode는 +t다.
irssi가 MODE #test b를 보내도 368로 끝나고 482가 뜨지 않는다.
```

## nc spam 테스트

서버 broadcast가 느린지 확인하려면 nc로 여러 PRIVMSG를 한 번에 보낸다.

```sh
{
  printf 'PASS pass\r\n'
  printf 'NICK spam\r\n'
  printf 'USER spam 0 * :Spam\r\n'
  printf 'JOIN #test\r\n'
  sleep 1
  for i in $(seq 1 20); do
    printf "PRIVMSG #test :msg$i\r\n"
  done
  sleep 1
} | nc localhost 6667
```

상대 irssi에 `msg1`부터 `msg20`이 빠르게 뜨면 서버 broadcast 경로는 정상이다.

---

# 현재 구현 범위와 의도적으로 안 한 것

## 구현한 것

```text
PASS / NICK / USER
CAP LS 최소 응답
PING 응답
PONG no-op
QUIT
JOIN / PART
PRIVMSG nick / PRIVMSG #channel
NAMES / WHO
TOPIC
INVITE / KICK
MODE i/t/k/o/l
MODE #channel b의 368 최소 응답
```

## 구현하지 않은 것

```text
full capability negotiation
server-driven heartbeat
PONG timeout 검사
ban list 실제 저장/적용
multi-channel JOIN/PART comma parsing
full IRC user mode system
DCC, NOTICE, AWAY 등 mandatory 밖 command
```

`CAP`, `PING`, `PONG`, `QUIT`, `NAMES`, `WHO`, `MODE b`는 과제 mandatory 핵심 명령이라기보다는 irssi reference client를 에러 없이 쓰기 위한 호환 처리에 가깝다. 특히 `MODE b`는 ban 기능 구현이 아니라 irssi의 channel sync 요청에 `368`로 정상 종료를 알려주는 최소 처리다.

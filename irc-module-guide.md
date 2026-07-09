# IRC 서버 모듈 가이드

이 문서는 현재 final code 기준으로 `ft_irc` 서버의 모듈 경계, 실행 흐름, 명령 처리 방식, irssi/nc 테스트 방법을 정리한 문서다.

제출용 README가 아니라, 평가 준비와 코드 설명을 위한 내부 정리 문서다.

이번 버전에서는 `EAGAIN/EWOULDBLOCK`, `ReceiveBuffer::pop(line)`의 정확한 주체, 현재 코드의 server numeric reply를 추가로 정리했다.


## 목차

- [근거 자료](#근거-자료)
  - [IRC protocol references](#irc-protocol-references)
  - [Linux / POSIX system call references](#linux--posix-system-call-references)
- [전체 디렉토리 구조](#전체-디렉토리-구조)
- [전체 모듈 구조](#전체-모듈-구조)
- [모듈 책임 요약](#모듈-책임-요약)
- [전체 실행 흐름](#전체-실행-흐름)
- [`main` 모듈](#main-모듈)
- [`server` 모듈](#server-모듈)
  - [`Server`](#server)
  - [`Listener`](#listener)
  - [`Poll`](#poll)
  - [`ClientIO`](#clientio)
  - [`Signal`](#signal)
- [`event` 모듈](#event-모듈)
- [`client` 모듈](#client-모듈)
  - [`Client`](#client)
  - [`Client::prefix()`](#clientprefix)
  - [`ClientManager`](#clientmanager)
  - [`ReceiveBuffer`](#receivebuffer)
  - [`SendBuffer`](#sendbuffer)
- [`parser` 모듈](#parser-모듈)
- [`serverMessage` 모듈](#servermessage-모듈)
- [`serverCommand` 모듈](#servercommand-모듈)
  - [`Command`](#command)
  - [`PRIVMSG` 분기](#privmsg-분기)
  - [`ClientCommand`](#clientcommand)
  - [channel command](#channel-command)
  - [`CommandHelper`](#commandhelper)
- [channel command 상세](#channel-command-상세)
  - [`JOIN`](#join)
  - [`PART`](#part)
  - [`PRIVMSG #channel`](#privmsg-channel)
  - [`NAMES`](#names)
  - [`WHO`](#who)
  - [`TOPIC`](#topic)
  - [`INVITE`](#invite)
  - [`KICK`](#kick)
- [`MODE` 모듈](#mode-모듈)
  - [전체 처리 흐름](#전체-처리-흐름)
  - [지원 mode](#지원-mode)
  - [`ModeChecker`](#modechecker)
  - [`ModeParser`](#modeparser)
  - [`ModeChange`](#modechange)
  - [`ModeApplier`](#modeapplier)
  - [`ModeState` 기본값](#modestate-기본값)
- [`channel` 모듈](#channel-모듈)
  - [`Channel`](#channel)
  - [`ChannelManager`](#channelmanager)
  - [`MemberList`](#memberlist)
  - [`OperatorList`](#operatorlist)
  - [`InviteList`](#invitelist)
  - [`ModeState`](#modestate)
- [서버 응답 코드 정리](#서버-응답-코드-정리)
  - [등록 / client command 응답](#등록--client-command-응답)
  - [PRIVMSG 응답](#privmsg-응답)
  - [channel command 응답](#channel-command-응답)
  - [TOPIC / WHO 응답](#topic--who-응답)
  - [MODE 응답](#mode-응답)
  - [내부 bool 반환값](#내부-bool-반환값)
- [모듈 경계 요약](#모듈-경계-요약)
- [테스트 가이드](#테스트-가이드)
- [현재 구현 범위와 의도적으로 안 한 것](#현재-구현-범위와-의도적으로-안-한-것)

---

## 근거 자료

이 프로젝트는 IRC server를 C++98로 구현하는 과제다.
프로토콜 동작은 IRC RFC를 기준으로 확인했고, 네트워크 I/O와 system call 동작은 Linux man page를 기준으로 확인했다.

### IRC protocol references

| 주제 | 자료 | 용도 |
|---|---|---|
| IRC 원본 프로토콜 | [RFC 1459 - Internet Relay Chat Protocol](https://www.rfc-editor.org/rfc/rfc1459.html) | IRC message 형식, command, numeric reply의 기본 기준 |
| IRC architecture | [RFC 2810 - Internet Relay Chat: Architecture](https://www.rfc-editor.org/rfc/rfc2810.html) | IRC client/server/channel 구조 이해용 |
| IRC channel management | [RFC 2811 - Internet Relay Chat: Channel Management](https://www.rfc-editor.org/rfc/rfc2811.html) | channel, channel mode, operator, invite-only 개념 확인 |
| IRC client protocol | [RFC 2812 - Internet Relay Chat: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812.html) | client-server command 형식, PASS/NICK/USER/JOIN/PRIVMSG/MODE 등 확인 |
| IRC server protocol | [RFC 2813 - Internet Relay Chat: Server Protocol](https://www.rfc-editor.org/rfc/rfc2813.html) | server-to-server 참고용. ft_irc mandatory 구현 대상은 아님 |
| IRCv3 capability negotiation | [IRCv3 - Capability Negotiation](https://ircv3.net/specs/extensions/capability-negotiation.html) | irssi가 접속 초기에 보내는 `CAP LS`, `CAP END` 이해용 |

### Linux / POSIX system call references

| 분류 | 함수 | 자료 |
|---|---|---|
| socket 생성 | `socket()` | [Linux man page - socket(2)](https://man7.org/linux/man-pages/man2/socket.2.html) |
| socket option | `setsockopt()` | [Linux man page - setsockopt(2)](https://man7.org/linux/man-pages/man2/setsockopt.2.html) |
| bind/listen/accept | `bind()`, `listen()`, `accept()` | [bind(2)](https://man7.org/linux/man-pages/man2/bind.2.html), [listen(2)](https://man7.org/linux/man-pages/man2/listen.2.html), [accept(2)](https://man7.org/linux/man-pages/man2/accept.2.html) |
| client connect | `connect()` | [Linux man page - connect(2)](https://man7.org/linux/man-pages/man2/connect.2.html) |
| socket address 확인 | `getsockname()` | [Linux man page - getsockname(2)](https://man7.org/linux/man-pages/man2/getsockname.2.html) |
| 주소 확인 | `getaddrinfo()`, `freeaddrinfo()` | [Linux man page - getaddrinfo(3)](https://man7.org/linux/man-pages/man3/getaddrinfo.3.html) |
| 주소 확인, legacy | `gethostbyname()` | [Linux man page - gethostbyname(3)](https://man7.org/linux/man-pages/man3/gethostbyname.3.html) |
| protocol DB | `getprotobyname()` | [Linux man page - getprotoent(3)](https://man7.org/linux/man-pages/man3/getprotoent.3.html) |
| byte order 변환 | `htons()`, `htonl()`, `ntohs()`, `ntohl()` | [Linux man page - byteorder(3)](https://man7.org/linux/man-pages/man3/htons.3.html) |
| IP 주소 변환 | `inet_addr()`, `inet_ntoa()`, `inet_ntop()` | [inet(3)](https://man7.org/linux/man-pages/man3/inet.3.html), [inet_ntop(3)](https://man7.org/linux/man-pages/man3/inet_ntop.3.html) |
| data receive | `recv()` | [Linux man page - recv(2)](https://man7.org/linux/man-pages/man2/recv.2.html) |
| data send | `send()` | [Linux man page - send(2)](https://man7.org/linux/man-pages/man2/send.2.html) |
| event multiplexing | `poll()` | [Linux man page - poll(2)](https://man7.org/linux/man-pages/man2/poll.2.html) |
| non-blocking fd 설정 | `fcntl()` | [Linux man page - fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html) |
| signal 처리 | `signal()`, `sigaction()` | [signal(2)](https://man7.org/linux/man-pages/man2/signal.2.html), [sigaction(2)](https://man7.org/linux/man-pages/man2/sigaction.2.html) |
| signal set 조작 | `sigemptyset()`, `sigfillset()`, `sigaddset()`, `sigdelset()`, `sigismember()` | [Linux man page - sigsetops(3)](https://man7.org/linux/man-pages/man3/sigsetops.3.html) |
| fd 닫기 | `close()` | [Linux man page - close(2)](https://man7.org/linux/man-pages/man2/close.2.html) |
| file offset / file info | `lseek()`, `fstat()` | [lseek(2)](https://man7.org/linux/man-pages/man2/lseek.2.html), [fstat(2)](https://man7.org/linux/man-pages/man2/fstat.2.html) |

---

## 전체 디렉토리 구조

```text
/
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

---

## 전체 모듈 구조

현재 서버는 8개 모듈로 나뉜다.

```text
[main]
  └── main.cpp
      └── 실행 인자 검사, Server 생성, Server::run() 호출

[server]
  ├── Server
  │   └── 서버 전체 흐름 조립
  ├── Listener
  │   └── listen socket 생성, bind, listen, accept
  ├── Poll
  │   └── pollfd 목록 생성
  ├── ClientIO
  │   └── client recv/send/remove 처리
  └── Signal
      └── SIGINT, SIGQUIT 종료 처리

[event]
  └── Event
      └── poll() 호출과 system error 처리

[client]
  ├── Client
  │   └── client 한 명의 fd, nickname, username, 등록 상태 저장
  ├── ClientManager
  │   └── client 생성, 검색, 삭제
  ├── ReceiveBuffer
  │   └── recv()로 받은 bytes 저장, pop(line) 기능 제공
  └── SendBuffer
      └── send()로 보낼 bytes 저장

[parser]
  └── Parser
      └── IRC line을 command name, params, type으로 변환

[serverMessage]
  └── Message
      └── ReceiveBuffer::pop(line)을 호출해 Parser와 Command로 연결

[serverCommand]
  ├── Command
  │   └── Parser::Type에 따라 실제 command handler 선택
  ├── serverClientCommand
  │   └── ClientCommand
  │       └── PASS, NICK, USER, CAP, PING, PONG, QUIT,
  │           PRIVMSG nick, unknown command 처리
  └── serverChannelCommand
      ├── CommandHelper
      │   └── channel command 공통 reply/broadcast helper 함수
      ├── command
      │   ├── Join
      │   ├── Part
      │   ├── Privmsg
      │   ├── Names
      │   ├── Who
      │   ├── Topic
      │   ├── Invite
      │   └── Kick
      └── mode
          ├── Mode
          ├── ModeChecker
          ├── ModeParser
          ├── ModeApplier
          └── ModeChange

[channel]
  ├── ChannelManager
  │   └── channel 생성, 검색, 삭제
  ├── Channel
  │   └── channel 하나의 중심 객체
  ├── MemberList
  │   └── channel member 목록 관리
  ├── OperatorList
  │   └── channel operator 목록 관리
  ├── InviteList
  │   └── invite된 client 목록 관리
  └── ModeState
      └── topic, invite-only, topic restriction, key, limit 상태 관리
```

---

## 모듈 책임 요약

| 모듈 | 핵심 책임 | 직접 하지 않는 일 |
|---|---|---|
| `main` | 실행 인자 검증, `Server` 시작 | socket loop, command 처리 |
| `server` | listen socket, poll, accept, recv/send 조립 | IRC command 의미 처리 |
| `event` | `poll()` 호출과 system error 처리 | poll 대상 구성, client 처리 |
| `client` | client 상태와 receive/send buffer 저장 | 실제 `recv()`/`send()` 호출 |
| `parser` | IRC line을 command type과 params로 변환 | command 실행, 권한 검사 |
| `serverMessage` | `ReceiveBuffer::pop(line)`을 반복 호출해 command 실행으로 연결 | raw bytes 저장, parsing 세부 규칙, command 세부 로직 |
| `serverCommand` | IRC command 실행 | raw socket I/O, poll 처리 |
| `channel` | channel 상태 저장과 변경 | numeric reply 생성, socket I/O |

---

## 전체 실행 흐름

서버 시작 흐름은 아래와 같다.

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
 -> TCP reply
```

중요한 점은 `recv()`는 byte만 읽고, command 실행은 하지 않는다는 것이다.
`recv()`로 받은 bytes는 먼저 `ReceiveBuffer`에 쌓이고, `Message::process()`가 `\r\n` 단위로 line을 꺼내 command 흐름으로 넘긴다.

---

# `main` 모듈

`main` 모듈은 프로그램의 시작점이다.

이 모듈은 IRC protocol이나 socket event loop를 직접 처리하지 않는다.
역할은 실행 인자를 검증하고, 검증된 값으로 `Server` 객체를 만든 뒤 `Server::run()`을 호출하는 것이다.

쉽게 말하면 아래 세 가지를 담당한다.

```text
1. 실행 인자 개수 확인
2. port / password 값 검증
3. Server 생성 후 실행 시작
```

## 파일

| 파일 | 역할 |
|---|---|
| `srcs/main.cpp` | 실행 인자 검사, port/password 검증, `Server` 생성 |

## 처리 흐름

```text
main(argc, argv)
 -> argc 검사
 -> port 문자열 검사
 -> port를 int로 변환
 -> port 범위 검사: 1 ~ 65535
 -> password 검사
 -> Server server(port, password)
 -> server.run()
```

## `port` 검사

서버는 아래 형식으로 실행된다.

```sh
./ircserv <port> <password>
```

`port`는 listen socket이 사용할 TCP port 번호다.

현재 서버는 port에 대해 아래 내용을 검사한다.

| 검사 | 이유 |
|---|---|
| 빈 문자열 금지 | port 값이 없는 경우 방지 |
| 숫자만 허용 | `abc`, `12a3` 같은 값 방지 |
| `1 ~ 65535` 범위 검사 | TCP/UDP port 범위 밖 값 방지 |
| `0` 금지 | 일반 listen port로 쓰지 않음 |

## `password` 검사

`password`는 client가 `PASS` 명령으로 보내야 하는 서버 접속 비밀번호다.

현재 서버는 password에 대해 아래 내용을 검사한다.

| 검사 | 이유 |
|---|---|
| 빈 문자열 금지 | 아무 password 없이 접속되는 상황 방지 |
| 504자 초과 금지 | IRC message 길이 제한을 고려한 비정상 입력 방지 |
| `\0` 금지 | 문자열 중간 종료 문제 방지 |
| `\r`, `\n` 금지 | IRC line 구조 깨짐 방지 |

## 이 모듈이 하지 않는 일

`main`은 아래 일을 하지 않는다.

```text
socket 생성
client accept
recv / send
IRC message parsing
IRC command 실행
channel 상태 관리
```

이 작업들은 전부 `Server` 이후의 모듈로 넘어간다.

---

# `server` 모듈

`server` 모듈은 서버 실행 흐름의 중심이다.

이 모듈은 socket을 열고, client 연결을 받고, `poll()` 결과에 따라 `recv()` 또는 `send()`를 호출한다.
하지만 IRC command의 실제 의미를 직접 처리하지는 않는다.

쉽게 말하면 아래 네 가지를 담당한다.

```text
1. listen socket 준비
2. poll 대상 fd 목록 관리
3. client accept / recv / send 처리
4. 받은 bytes를 Message 모듈로 넘김
```

## 파일

| 파일 | 역할 |
|---|---|
| `Server.hpp/cpp` | 서버 전체 흐름 조립, poll 결과 분기 |
| `Listener.hpp/cpp` | listen socket 생성, option 설정, bind/listen, accept |
| `Poll.hpp/cpp` | `pollfd` 목록 구성 |
| `ClientIO.hpp/cpp` | client socket `recv()` / `send()` / 제거 |
| `Signal.hpp/cpp` | `SIGINT`, `SIGQUIT` 종료 요청 처리 |

## `Server`

`Server`는 여러 모듈을 조립하는 중심 객체다.

| 멤버 | 의미 |
|---|---|
| `_password` | 서버 접속 password |
| `_clients` | 연결된 client 목록 |
| `_channels` | 생성된 channel 목록 |
| `_listener` | listen socket 담당 |
| `_poll` | poll 대상 fd 목록 생성 |
| `_clientIO` | client fd의 receive/send/remove 담당 |
| `_message` | receive buffer를 parser/command 흐름으로 넘김 |
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
| `STDIN_FILENO` | terminal에서 `DIE` 입력 확인 |
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

`Message::process()`가 `false`를 반환하면 현재 명령은 연결 종료를 뜻한다.
대표적으로 `QUIT`이 여기에 해당한다.
이 경우 서버는 남은 send buffer를 한 번 보내고 client를 제거한다.

## `Listener`

`Listener`는 listen socket을 준비한다.

처리 흐름:

```text
socket(AF_INET, SOCK_STREAM, 0)
 -> setsockopt(SO_REUSEADDR)
 -> fcntl(O_NONBLOCK)
 -> bind(INADDR_ANY, port)
 -> listen(SOMAXCONN)
```

각 단계의 의미는 아래와 같다.

| 단계 | 의미 |
|---|---|
| `socket()` | TCP 통신용 fd 생성 |
| `setsockopt()` | 서버 재시작 시 같은 port 재사용 가능하게 설정 |
| `fcntl(O_NONBLOCK)` | fd를 non-blocking 모드로 변경 |
| `bind()` | socket을 특정 port에 묶음 |
| `listen()` | client 연결 요청을 받을 준비 상태로 변경 |

`acceptClient()`는 새 client 연결을 받는다.

```text
listen fd에 POLLIN 발생
 -> accept()
 -> 새 client fd 생성
 -> client fd도 non-blocking 설정
 -> ClientManager에 Client 추가
```

`accept()` 결과별 처리:

| 상황 | 처리 |
|---|---|
| 성공 | 새 client fd 반환 |
| `EINTR` | signal 때문에 중단. loop에서 다시 시도 가능 |
| `EAGAIN/EWOULDBLOCK` | 더 받을 client 없음. `clientFd = -1` |
| 그 외 error | system error 출력 후 실패 |

## `Poll`

`Poll`은 `poll()`에 넘길 `pollfd` 목록을 만든다.

현재 서버는 매 loop마다 poll 대상 목록을 다시 만든다.

| 대상 | event |
|---|---|
| terminal stdin | `POLLIN`, 단 stdin이 tty일 때만 |
| listen socket fd | `POLLIN` |
| client fd | 기본 `POLLIN` |
| send buffer가 있는 client fd | `POLLIN | POLLOUT` |

중요한 점은 `send buffer`에 보낼 데이터가 있을 때만 `POLLOUT`을 켠다는 것이다.

```text
보낼 데이터 없음
 -> POLLIN만 감시

보낼 데이터 있음
 -> POLLIN | POLLOUT 감시
```

이렇게 해야 불필요하게 계속 write 가능 상태를 검사하지 않는다.

## `ClientIO`

`ClientIO`는 client socket의 실제 입출력과 제거를 담당한다.

### receive 흐름

```text
client socket fd
 -> recv()
 -> char buffer[512]
 -> Client::ReceiveBuffer::append()
```

`recv()`는 IRC command를 처리하지 않는다.
그냥 kernel socket receive buffer에 있는 bytes를 user space buffer로 복사할 뿐이다.

`recv()` 결과별 처리:

| 결과 | 의미 | 처리 |
|---|---|---|
| `> 0` | bytes 읽음 | receive buffer에 저장하고 계속 읽기 |
| `0` | 상대가 연결 종료 | client 제거 |
| `-1`, `EINTR` | signal 때문에 중단 | 다시 시도 |
| `-1`, `EAGAIN/EWOULDBLOCK` | non-blocking fd에서 지금 당장 더 읽을 데이터가 없음 | 정상 종료 |
| 그 외 | socket error | client 제거 |

### `EAGAIN` / `EWOULDBLOCK`

`EAGAIN`과 `EWOULDBLOCK`은 non-blocking socket에서 자주 나오는 `errno` 값이다.

blocking socket이라면 읽을 데이터가 없을 때 `recv()`가 멈춰서 기다릴 수 있다.
하지만 ft_irc 과제에서는 모든 I/O가 non-blocking이어야 하므로, socket fd를 `O_NONBLOCK`으로 설정한다.

non-blocking fd에서는 읽을 데이터가 없을 때 기다리지 않고 바로 실패처럼 돌아온다.
이때 `recv()`는 `-1`을 반환하고, `errno`가 `EAGAIN` 또는 `EWOULDBLOCK`으로 설정된다.

이 값은 치명적인 error가 아니다.
현재 시점에 kernel socket receive buffer가 비었다는 뜻이다.

```text
recv() > 0
 -> bytes를 읽음
 -> ReceiveBuffer에 append
 -> 더 읽을 수 있는지 다시 recv()

recv() == -1 && errno == EAGAIN/EWOULDBLOCK
 -> 지금은 더 읽을 bytes 없음
 -> 정상적으로 receive loop 종료
 -> Message::process()로 넘어가서 지금까지 쌓인 line 처리
```

`send()`에서도 같은 의미로 사용된다.
non-blocking fd에서 kernel socket send buffer가 지금 당장 더 받을 수 없으면 `send()`가 `-1`을 반환하고 `errno`가 `EAGAIN` 또는 `EWOULDBLOCK`이 될 수 있다.
이 경우 현재 서버는 연결을 끊지 않고, 다음 `POLLOUT` event 때 남은 `SendBuffer`를 다시 전송한다.

정리하면 아래와 같다.

| 상황 | 의미 | 현재 코드 처리 |
|---|---|---|
| `recv()`에서 `EAGAIN/EWOULDBLOCK` | 지금 더 읽을 데이터 없음 | 정상 종료, 연결 유지 |
| `send()`에서 `EAGAIN/EWOULDBLOCK` | 지금 더 보낼 수 없음 | 전송 중단, 남은 데이터 유지 |
| 일반 socket error | 실제 오류 | client 제거 |

### send 흐름

```text
Client::SendBuffer
 -> send()
 -> kernel socket send buffer
 -> TCP로 client에게 전송
```

`send()`는 한 번에 모든 데이터를 보내지 못할 수 있다.
그래서 실제로 보낸 byte 수만큼만 `SendBuffer`에서 제거한다.

```text
send buffer에 100 bytes 있음
send()가 40 bytes만 보냄
 -> 앞 40 bytes 제거
 -> 나머지 60 bytes는 다음 POLLOUT 때 다시 전송
```

### remove 흐름

client를 제거할 때는 단순히 fd만 닫으면 안 된다.
channel 안에 남아 있는 member/operator/invite 상태도 같이 정리해야 한다.

```text
client 제거
 -> ChannelManager::removeClientFromAll(client)
 -> ClientManager::removeByFd(fd)
 -> Client destructor에서 fd close
```

## `Signal`

`Signal`은 서버 종료 요청을 처리한다.
현재 서버는 `SIGINT`, `SIGQUIT` 같은 signal을 받아 종료 상태를 기록한다.
그 뒤 `Server::run()` loop가 종료 조건을 확인하고 안전하게 빠져나온다.

또 terminal이 tty인 경우 server terminal에서 아래 입력으로도 종료할 수 있다.

```text
DIE
```

## 이 모듈이 하지 않는 일

`server` 모듈은 아래 일을 직접 하지 않는다.

```text
IRC line parsing
PASS/NICK/USER 처리
JOIN/PART 처리
MODE 처리
channel member/operator 관리
```

서버 모듈은 네트워크 입출력과 큰 흐름만 담당한다.
IRC command의 실제 의미는 `serverCommand`와 `channel` 모듈에서 처리한다.

---

# `event` 모듈

`event` 모듈은 `poll()` 호출만 담당한다.

이 모듈은 poll 대상 fd 목록을 만들지 않는다.
그 일은 `Poll` 모듈이 한다.
`Event`는 이미 만들어진 `pollfd` vector를 받아서 kernel에 대기 요청을 넣는다.

## 파일

| 파일 | 역할 |
|---|---|
| `Event.hpp/cpp` | `poll()` 호출과 system error 출력 |

## 처리 흐름

```text
Poll::build()
 -> pollfd vector 생성
 -> Event::wait(pollFds)
    -> poll(&pollFds[0], pollFds.size(), -1)
 -> Server::handlePoll()
```

`poll()`의 timeout은 `-1`이다.
즉, fd event가 생길 때까지 기다린다.

## `poll()` 결과 처리

| 결과 | 의미 | 처리 |
|---|---|---|
| `> 0` | event가 발생한 fd가 있음 | true 반환 |
| `-1`, `EINTR` | signal 때문에 중단 | true 반환 후 loop 계속 |
| `-1`, 그 외 error | 실제 system error | error 출력 후 false 반환 |

## 이 모듈이 하지 않는 일

`event` 모듈은 아래 일을 하지 않는다.

```text
pollfd 목록 구성
client accept
recv/send
IRC command 실행
```

이 모듈은 `poll()` 대기만 담당한다.

---

# `client` 모듈

`client` 모듈은 연결된 client들의 상태를 관리한다.

여기서 client는 단순히 socket fd만 뜻하지 않는다.
IRC 서버 입장에서는 client마다 아래 정보가 필요하다.

```text
fd
password 인증 여부
registered 여부
nickname
username
realname
receive buffer
send buffer
```

즉, `client` 모듈은 “연결된 사용자 한 명의 상태 저장소”와 “그 목록 관리자”를 담당한다.

## 파일

| 파일 | 역할 |
|---|---|
| `Client.hpp/cpp` | client fd, 등록 상태, nickname/user/realname, receive/send buffer 소유 |
| `ClientManager.hpp/cpp` | client 생성, 삭제, fd/nickname 검색 |
| `ReceiveBuffer.hpp/cpp` | `recv()`로 받은 raw bytes 저장, `\r\n` 기준 line 추출 |
| `SendBuffer.hpp/cpp` | 나중에 `send()`할 bytes 저장 |

## `Client`

`Client`는 연결된 client 한 명의 상태를 저장한다.

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

`Client`가 직접 socket을 읽거나 쓰지는 않는다.
`recv()`와 `send()`는 `ClientIO`가 담당하고, `Client`는 그 결과를 저장하는 역할을 한다.

## `Client::prefix()`

IRC message는 보통 누가 보냈는지 prefix를 붙인다.

```text
:nick!username@host COMMAND ...
```

예:

```text
:alice!alice@localhost PRIVMSG #test :hello
```

현재 서버는 host를 `localhost`로 고정해서 prefix를 만든다.

| 상태 | prefix |
|---|---|
| nickname 없음 | `*` |
| username 없음 | `nick!nick@localhost` |
| username 있음 | `nick!username@localhost` |

이 prefix는 `PRIVMSG`, `JOIN`, `PART`, `TOPIC`, `KICK`, `MODE` 전파 메시지에서 사용된다.

## `ClientManager`

`ClientManager`는 여러 client를 관리한다.

주요 역할:

```text
새 client 추가
fd로 client 찾기
nickname으로 client 찾기
nickname 중복 검사
client 삭제
전체 client 정리
```

예를 들어 개인 메시지는 nickname으로 target client를 찾아야 한다.

```text
PRIVMSG bob :hello
 -> ClientManager::findByNickname("bob")
 -> bob의 SendBuffer에 message 추가
```

client 연결이 끊길 때는 fd 기준으로 삭제한다.

```text
client fd error
 -> ClientManager::findByFd(fd)
 -> channel에서 제거
 -> ClientManager에서 제거
 -> fd close
```

## `ReceiveBuffer`

TCP는 message 단위가 아니라 byte stream이다.

즉, client가 이렇게 보냈다고 해서:

```text
PASS pass\r\n
```

server가 항상 한 번의 `recv()`로 정확히 이만큼 받는 것은 아니다.
이렇게 쪼개져 들어올 수 있다.

```text
recv #1: "PA"
recv #2: "SS pass\r\nNI"
recv #3: "CK alice\r\n"
```

그래서 `ReceiveBuffer`가 필요하다.

`ReceiveBuffer`의 역할은 아래와 같다.

```text
recv()로 받은 bytes를 계속 저장
 -> \r\n이 나올 때까지 기다림
 -> 완성된 line 하나를 꺼냄
 -> 나머지 bytes는 buffer에 남김
```

예시:

```text
raw buffer: "PASS pa"
 -> 아직 \r\n 없음
 -> pop() 실패
 -> command 실행 안 함

raw buffer: "PASS pass\r\nNICK alice\r\n"
 -> pop() = "PASS pass"
 -> pop() = "NICK alice"
```

중요한 점은 `ReceiveBuffer`는 parsing을 하지 않는다는 것이다.
여기서는 오직 `\r\n` 기준으로 line만 자른다.

## `SendBuffer`

`SendBuffer`는 client에게 보낼 데이터를 저장한다.

command 처리 중에는 바로 `send()`를 호출하지 않는다.
대신 보낼 message를 `SendBuffer`에 쌓아둔다.

```text
Command 처리
 -> Client::SendBuffer::append(reply)
 -> 다음 poll loop에서 POLLOUT 감지
 -> ClientIO::send()
```

이 구조가 필요한 이유는 non-blocking socket에서는 `send()`가 항상 전부 성공하지 않기 때문이다.

```text
send buffer: 100 bytes
send() success: 40 bytes
 -> 40 bytes 제거
 -> 60 bytes 유지
 -> 다음 POLLOUT 때 이어서 전송
```

## 이 모듈이 하지 않는 일

`client` 모듈은 아래 일을 하지 않는다.

```text
socket accept
recv/send system call 호출
IRC command parsing
JOIN/MODE 같은 명령 실행
channel 상태 변경
```

`client` 모듈은 상태 저장과 buffer 관리에 집중한다.

---

# `parser` 모듈

`parser` 모듈은 IRC message 한 줄을 command 실행에 필요한 형태로 바꾼다.

중요한 점은 `Parser`가 TCP bytes를 직접 다루지 않는다는 것이다.
`ReceiveBuffer`가 이미 `\r\n` 기준으로 line 하나를 꺼낸 뒤, 그 line을 `Parser`가 해석한다.

```text
raw TCP bytes
 -> ReceiveBuffer에서 line 추출
 -> Parser가 line을 command / params / type으로 변환
```

## 파일

| 파일 | 역할 |
|---|---|
| `Parser.hpp/cpp` | IRC line을 command name, params, type으로 파싱 |

## Parser가 받는 입력

Parser의 입력은 이미 완성된 IRC line이다.

예:

```text
PRIVMSG bob :hello bob
```

여기에는 끝의 `\r\n`은 포함되지 않는다.
`ReceiveBuffer::pop()` 단계에서 line 단위로 잘렸기 때문이다.

## Parser가 만드는 값

| 값 | 의미 |
|---|---|
| `_name` | 대문자로 정규화된 command 이름 |
| `_params` | command parameter 목록 |
| `_type` | 내부 command type enum |

예:

```text
입력:
PRIVMSG bob :hello bob

결과:
name   = PRIVMSG
type   = Parser::PRIVMSG
params = ["bob", "hello bob"]
```

## command name 정규화

IRC command는 client가 소문자나 섞인 대소문자로 보낼 수 있다.

```text
join
Join
JOIN
```

현재 Parser는 command name을 대문자로 정규화한다.

| 입력 | 내부 name |
|---|---|
| `join` | `JOIN` |
| `Join` | `JOIN` |
| `JOIN` | `JOIN` |

그래서 command 실행 단계에서는 대소문자를 따로 신경 쓰지 않아도 된다.

## parameter parsing

IRC message는 보통 공백으로 parameter를 나눈다.

```text
JOIN #test
 -> params = ["#test"]
```

하지만 `:`로 시작하는 trailing parameter는 다르게 처리한다.

```text
PRIVMSG bob :hello bob
 -> params = ["bob", "hello bob"]
```

여기서 `hello bob`은 공백이 있지만 하나의 parameter다.

다른 예:

```text
TOPIC #test :hello world
 -> params = ["#test", "hello world"]
```

즉, `:` 뒤쪽은 message 끝까지 하나로 묶는다.

## 지원 command type

현재 Parser는 아래 command를 구분한다.

```text
UNKNOWN, CAP, PING, PONG, QUIT,
PASS, NICK, USER,
JOIN, PART, PRIVMSG, NAMES, WHO, TOPIC,
INVITE, KICK, MODE
```

알 수 없는 command는 `UNKNOWN`으로 분류된다.

```text
HELLO
 -> type = Parser::UNKNOWN
```

그 뒤 `Command::execute()`에서 `unknown()`으로 처리된다.

## 이 모듈이 하지 않는 일

`parser` 모듈은 아래 일을 하지 않는다.

```text
recv() 호출
\r\n 기준 line 추출
client 등록 상태 검사
channel 존재 여부 검사
실제 command 실행
numeric reply 생성
```

Parser는 오직 “문자열 한 줄을 구조화된 command 정보로 바꾸는 역할”만 한다.

---

# `serverMessage` 모듈

`serverMessage` 모듈은 receive buffer와 command 실행 사이를 연결한다.

중요한 점은 `ReceiveBuffer`가 스스로 message를 처리하는 객체가 아니라는 것이다.
`ReceiveBuffer`는 raw bytes를 저장하고, `pop(line)`이라는 함수를 제공한다.
실제로 `pop(line)`을 반복 호출해서 line을 꺼내고 다음 단계로 넘기는 주체는 `Message::process()`다.

```text
ClientIO::receive()
 -> recv()로 bytes 읽기
 -> Client::ReceiveBuffer::append()

Message::process()
 -> Client::ReceiveBuffer::pop(line)을 반복 호출
 -> line이 있으면 Parser / Command 흐름으로 전달
```

---

## 파일

| 파일 | 역할 |
|---|---|
| `Message.hpp/cpp` | receive buffer에 저장된 bytes에서 완성된 IRC line을 꺼내 parser와 command 실행 흐름으로 연결 |

---

## 왜 Message 모듈이 필요한가?

TCP는 message 단위가 아니라 byte stream이다.
즉, client가 command를 한 줄씩 보냈다고 해서 server가 한 번의 `recv()`로 command 하나씩 받는 것이 아니다.

예를 들어 client가 아래처럼 보냈어도:

```text
PASS pass\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n
```

server는 이렇게 쪼개서 받을 수 있다.

```text
recv #1: "PASS pa"
recv #2: "ss\r\nNICK alice\r\nUSER"
recv #3: " alice 0 * :Alice\r\n"
```

또 반대로, 한 번의 `recv()` 결과 안에 여러 command가 한꺼번에 들어올 수도 있다.

그래서 흐름을 둘로 나눈다.

```text
ClientIO / ReceiveBuffer
 -> bytes 저장 담당

Message / Parser / Command
 -> 완성된 IRC line 처리 담당
```

---

## 정확한 처리 흐름

```text
ClientIO::receive(clientFd)
 -> recv()로 bytes를 읽음
 -> Client::ReceiveBuffer::append()

Server::handleClient()
 -> Message::process(client)

Message::process(client)
 -> while client.receiveBuffer().pop(line)
    -> Message::handle(client, line)
       -> Parser::parse(line)
       -> Command::execute(client, parser)
```

`ReceiveBuffer::pop(line)`의 역할은 아래와 같다.

```text
내부 buffer에서 "\r\n" 검색
 -> 없으면 false 반환
 -> 있으면 "\r\n" 앞까지 line으로 복사
 -> 꺼낸 line과 "\r\n"을 내부 buffer에서 제거
 -> true 반환
```

즉, `pop(line)`의 구현은 `ReceiveBuffer` 안에 있지만, 이 함수를 호출해서 처리 흐름을 진행하는 객체는 `Message::process()`다.

---

## 여러 command가 한 번에 들어온 경우

한 번의 `ClientIO::receive()` 호출 안에서는 `recv()`가 여러 번 실행될 수 있다.
`recv()`는 읽을 수 있는 bytes가 있으면 계속 읽고, 더 읽을 데이터가 없어서 `EAGAIN/EWOULDBLOCK`이 나오면 멈춘다.

그 결과 `ReceiveBuffer` 안에 여러 IRC line이 쌓일 수 있다.
`Message::process()`는 `pop(line)`을 while loop로 반복 호출해서 완성된 line을 모두 처리한다.

예:

```text
PASS pass\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n
```

위 데이터가 buffer에 들어 있으면 `pop(line)`은 순서대로 아래 세 line을 반환한다.

```text
PASS pass
NICK alice
USER alice 0 * :Alice
```

처리 흐름은 아래처럼 세 번 반복된다.

```text
1번째 line: PASS pass
 -> Parser::parse()
 -> Command::execute()
 -> ClientCommand::pass()

2번째 line: NICK alice
 -> Parser::parse()
 -> Command::execute()
 -> ClientCommand::nick()

3번째 line: USER alice 0 * :Alice
 -> Parser::parse()
 -> Command::execute()
 -> ClientCommand::user()
```

---

## line 길이와 buffer 크기

현재 `ClientIO::receive()`의 임시 read buffer는 아래처럼 512 bytes다.

```text
char buffer[512]
```

하지만 이것은 한 번의 `recv()` system call에서 user space로 복사할 수 있는 임시 크기일 뿐이다.
`ReceiveBuffer`에 누적되는 전체 크기와 `pop(line)`이 한 번에 반환하는 line 길이에는 별도 제한을 두지 않았다.

정리하면 아래와 같다.

| 구분 | 현재 코드 기준 |
|---|---|
| 한 번의 `recv()` 임시 buffer | 512 bytes |
| `ReceiveBuffer` 전체 누적 크기 | 별도 제한 없음 |
| `pop(line)`이 반환하는 line 길이 | `\r\n` 전까지 전부, 별도 제한 없음 |
| IRC RFC의 일반 message 길이 제한 | CR-LF 포함 512 bytes 기준이지만 현재 코드에서 별도 검사하지 않음 |

따라서 `\r\n`이 오지 않으면 bytes는 `ReceiveBuffer`에 계속 누적된다.
일반적인 ft_irc 평가와 irssi/nc 테스트에서는 문제가 되지 않지만, 엄밀한 IRC message length 제한까지 검사하는 구현은 아니다.

---

## command 실행 결과

`Command::execute()`는 `bool` 값을 반환한다.

| 반환값 | 의미 |
|---|---|
| `true` | client 연결 유지 |
| `false` | client 연결 종료 필요 |

대표적으로 `QUIT`이 `false`를 반환한다.

```text
QUIT
 -> ClientCommand::quit()
 -> ERROR :Closing Link를 SendBuffer에 저장
 -> false 반환
 -> Message::process() false 반환
 -> Server가 남은 SendBuffer를 한 번 전송
 -> client 제거
```

parameter 오류, 권한 오류, unknown command 같은 경우에는 numeric reply를 보내고 보통 `true`를 반환한다.
즉, 에러 응답을 보냈다고 해서 연결을 바로 끊지는 않는다.

---

## 빈 line 처리

빈 line이나 parsing할 수 없는 line은 command 실행으로 이어지지 않는다.

```text
\r\n
```

이런 입력이 들어오면 `Parser::parse()`가 실패하고, `Message::handle()`은 연결을 유지한 채 true를 반환한다.

---

## 이 모듈이 하지 않는 일

`serverMessage` 모듈은 아래 일을 하지 않는다.

```text
socket recv
socket send
raw bytes 저장 자체
command 의미 판단
channel 상태 변경
client 목록 관리
```

`Message`는 받은 bytes를 직접 저장하는 객체가 아니다.
`ReceiveBuffer`에 이미 저장된 data를 `pop(line)`으로 꺼내서 Parser와 Command 흐름으로 넘기는 연결 계층이다.

---

# `serverCommand` 모듈

`serverCommand` 모듈은 Parser가 만든 command type을 보고 실제 IRC command를 실행한다.

이 모듈은 크게 세 부분으로 나뉜다.

```text
1. Command
   Parser::Type을 보고 어떤 handler로 보낼지 결정

2. ClientCommand
   server 자체 또는 nickname 대상 command 처리

3. serverChannelCommand
   channel 대상 command와 channel mode 처리
```

## 파일

| 파일 | 역할 |
|---|---|
| `Command.hpp/cpp` | `Parser::Type`을 보고 실제 command 객체에 위임 |
| `serverClientCommand/ClientCommand.hpp/cpp` | 서버나 nickname 대상 command 처리 |
| `serverChannelCommand/CommandHelper.hpp/cpp` | channel command 공통 reply/broadcast helper 함수 |
| `serverChannelCommand/command/*.hpp/cpp` | `JOIN`, `PART`, `PRIVMSG`, `NAMES`, `WHO`, `TOPIC`, `INVITE`, `KICK` 처리 |
| `serverChannelCommand/mode/*.hpp/cpp` | `MODE` 처리 |

## `Command`

`Command`는 command 실행의 입구다.

`Parser`는 line을 아래처럼 바꾼다.

```text
line
 -> command type
 -> params
```

그 다음 `Command::execute()`가 type을 보고 맞는 handler를 선택한다.

```text
Parser::CAP     -> ClientCommand::cap()
Parser::PING    -> ClientCommand::ping()
Parser::PONG    -> ClientCommand::pong()
Parser::QUIT    -> ClientCommand::quit()
Parser::PASS    -> ClientCommand::pass()
Parser::NICK    -> ClientCommand::nick()
Parser::USER    -> ClientCommand::user()

Parser::JOIN    -> Join::handle()
Parser::PART    -> Part::handle()
Parser::NAMES   -> Names::handle()
Parser::WHO     -> Who::handle()
Parser::TOPIC   -> Topic::handle()
Parser::INVITE  -> Invite::handle()
Parser::KICK    -> Kick::handle()

Parser::MODE    -> Mode::handle()
Parser::UNKNOWN -> ClientCommand::unknown()
```

`Command`는 명령의 세부 로직을 직접 실행하지 않는다.
그냥 “어느 handler가 처리해야 하는지”를 결정한다.

## `PRIVMSG` 분기

`PRIVMSG`는 target이 nickname일 수도 있고 channel일 수도 있다.

```text
PRIVMSG bob :hello
PRIVMSG #test :hello
```

그래서 `PRIVMSG`만 target을 보고 한 번 더 나눈다.

| 입력 | 위임 |
|---|---|
| `PRIVMSG bob :hello` | `ClientCommand::privmsg()` |
| `PRIVMSG #test :hello` | `Privmsg::handle()` |

기준은 target이 `#`으로 시작하는지 여부다.

## command 실행 결과

각 command handler는 성공/실패 여부와 별개로 client 연결을 계속 유지할지 결정한다.

| 반환값 | 의미 |
|---|---|
| `true` | 연결 유지 |
| `false` | 연결 종료 요청 |

대표적으로 `QUIT`은 `false`를 반환한다.
반면에 parameter 오류, 권한 오류, unknown command는 보통 에러 응답을 보내고 연결은 유지한다.

## 이 모듈이 하지 않는 일

`serverCommand` 모듈은 아래 일을 하지 않는다.

```text
recv/send system call 직접 호출
TCP packet 조립
poll event 감시
raw bytes를 line으로 자르기
```

이 모듈은 이미 Parser가 만든 command 정보를 받아서 IRC 동작을 실행하는 계층이다.

---

## `ClientCommand`

`ClientCommand`는 channel이 아니라 server 자체 또는 특정 nickname을 대상으로 하는 command를 처리한다.

쉽게 말하면 아래 두 종류를 담당한다.

```text
1. client registration 관련 명령
   PASS / NICK / USER / CAP

2. server 또는 nickname 대상 명령
   PING / PONG / QUIT / PRIVMSG nick / unknown command
```

channel을 대상으로 하는 명령은 여기서 직접 처리하지 않는다.

```text
PRIVMSG bob :hello
 -> ClientCommand::privmsg()

PRIVMSG #test :hello
 -> channel command Privmsg::handle()
```

### 처리 command 요약

| 명령 | 함수 | 역할 |
|---|---|---|
| `PASS` | `pass()` | 서버 접속 password 검증 |
| `NICK` | `nick()` | nickname 설정, 중복 검사 |
| `USER` | `user()` | username, realname 설정 |
| `CAP` | `cap()` | irssi capability 요청 최소 처리 |
| `PING` | `ping()` | client PING에 PONG 응답 |
| `PONG` | `pong()` | 현재 서버가 PING timeout을 검사하지 않으므로 no-op |
| `QUIT` | `quit()` | `ERROR :Closing Link` 전송 후 연결 종료 요청 |
| `PRIVMSG nick` | `privmsg()` | nickname 대상 개인 메시지 전송 |
| unknown | `unknown()` | `421 Unknown command` 응답 |

### Registration 흐름

IRC에서는 TCP 연결이 성공했다고 바로 IRC 사용자로 등록되는 것이 아니다.

현재 서버는 아래 세 조건이 모두 준비되면 client를 registered 상태로 바꾼다.

```text
PASS 성공
NICK 설정
USER 설정
 -> registered = true
```

등록 완료 순간에는 아래 응답을 한 번만 보낸다.

```text
:ircserv 001 <nick> :Welcome to ircserv
:ircserv 221 <nick> +i
```

`001`은 welcome 응답이고, `221`은 현재 user mode가 `+i`라는 응답이다.
현재 서버는 full user mode system을 구현한 것이 아니라, irssi 호환을 위해 최소한의 `+i` 응답만 제공한다.

### `PASS`

`PASS`는 서버 접속 password를 확인하는 명령이다.

서버는 실행될 때 password를 인자로 받는다.

```sh
./ircserv 6667 pass
```

그러면 client는 등록 과정에서 같은 password를 보내야 한다.

```text
PASS pass
```

현재 처리 흐름:

```text
이미 registered 상태인지 검사
 -> parameter가 있는지 검사
 -> password가 서버 password와 같은지 검사
 -> 성공하면 client.hasPassword = true
 -> PASS / NICK / USER가 모두 준비되었으면 등록 완료 응답 전송
```

응답:

| 상황 | 응답 |
|---|---|
| 이미 등록된 client가 다시 `PASS` | `462 You may not reregister` |
| parameter 없음 | `461 PASS :Not enough parameters` |
| password 불일치 | `464 Password incorrect` |
| 성공 | 바로 응답은 없고, 등록 조건이 완성되면 `001`, `221` 전송 |

주의할 점은 password가 틀렸다고 즉시 연결을 끊지는 않는다는 것이다.
현재 구현은 `464`만 보내고 연결은 유지한다.

### `NICK`

`NICK`은 IRC 안에서 client를 식별하는 nickname을 설정하는 명령이다.

```text
NICK alice
```

nickname은 여러 곳에서 사용된다.

```text
PRIVMSG bob :hello
```

위 명령에서 서버는 `bob`이라는 nickname을 가진 client를 찾는다.
또 message prefix에도 nickname이 들어간다.

```text
:alice!alice@localhost PRIVMSG bob :hello
```

현재 처리 흐름:

```text
parameter가 있는지 검사
 -> 같은 nickname을 쓰는 다른 client가 있는지 검사
 -> client.nickname 갱신
 -> PASS / NICK / USER가 모두 준비되었으면 등록 완료 응답 전송
```

응답:

| 상황 | 응답 |
|---|---|
| nickname parameter 없음 | `431 No nickname given` |
| 이미 다른 client가 사용 중인 nickname | `433 <nick> :Nickname is already in use` |
| 성공 | 바로 응답은 없고, 등록 조건이 완성되면 `001`, `221` 전송 |

현재 코드 기준으로는 등록 후에도 `NICK newNick` 자체는 받을 수 있다.
다만 실제 IRC 서버처럼 channel member들에게 nickname 변경 메시지를 전파하는 full nickname-change 기능까지 구현한 것은 아니다.

### `USER`

`USER`는 username과 realname 정보를 설정하는 명령이다.

```text
USER alice 0 * :Alice
```

현재 서버는 여기서 username과 realname을 저장한다.

```text
username = alice
realname = Alice
```

이 정보는 prefix나 WHO 응답에서 사용된다.

```text
nick!username@host
```

예:

```text
alice!alice@localhost
```

현재 처리 흐름:

```text
이미 registered 상태인지 검사
 -> parameter 개수 검사
 -> username / realname 저장
 -> PASS / NICK / USER가 모두 준비되었으면 등록 완료 응답 전송
```

응답:

| 상황 | 응답 |
|---|---|
| 이미 등록된 client가 다시 `USER` | `462 You may not reregister` |
| parameter 부족 | `461 USER :Not enough parameters` |
| 성공 | 바로 응답은 없고, 등록 조건이 완성되면 `001`, `221` 전송 |

### `CAP`

`CAP`은 IRCv3 capability negotiation 명령이다.

여기서 capability는 서버가 지원하는 추가 기능을 뜻한다.
예를 들면 `sasl`, `multi-prefix`, `away-notify` 같은 기능이다.

irssi는 접속 초기에 서버가 어떤 추가 기능을 지원하는지 확인하기 위해 `CAP LS`를 보낼 수 있다.

현재 서버는 IRCv3 capability를 실제로 제공하지 않는다.
따라서 `CAP LS`에는 빈 capability 목록을 응답하고, `CAP END`는 조용히 무시한다.

| 입력 | 처리 |
|---|---|
| `CAP LS` | `:ircserv CAP * LS :` 응답 |
| `CAP END` | 응답 없이 무시 |
| 그 외 `CAP` | 응답 없이 무시 |

이 처리는 mandatory command 구현이라기보다, irssi reference client가 접속 초기에 `CAP Unknown command`를 띄우지 않도록 하기 위한 최소 호환 처리다.

### `PING` / `PONG`

현재 서버는 client가 보낸 `PING`에만 응답한다.

```text
client -> server: PING ircserv
server -> client: PONG ircserv :ircserv
```

현재 처리 흐름:

```text
PING parameter 검사
 -> parameter가 없으면 409 응답
 -> parameter가 있으면 PONG 응답
```

응답:

| 상황 | 응답 |
|---|---|
| `PING` parameter 없음 | `409 No origin specified` |
| `PING token` | `PONG ircserv :token` |

현재 서버는 주기적으로 client에게 `PING`을 보내지 않는다.
그래서 client가 보내는 `PONG`도 따로 검사하지 않는다.

```text
구현함:
  client PING -> server PONG

구현하지 않음:
  server periodic PING
  client PONG timeout check
```

따라서 `PONG`은 현재 no-op이다.

### `QUIT`

`QUIT`은 client가 연결 종료를 요청하는 명령이다.

```text
QUIT :bye
```

현재 서버는 quit message 내용은 따로 사용하지 않고, 아래 응답을 send buffer에 넣은 뒤 연결 종료를 요청한다.

```text
:ircserv ERROR :Closing Link
```

처리 흐름:

```text
QUIT 수신
 -> ERROR :Closing Link 응답 저장
 -> false 반환
 -> Server가 남은 send buffer를 한 번 전송
 -> client 제거
```

즉, `QUIT`은 command 처리 함수가 `false`를 반환하는 대표적인 경우다.
이 `false`는 “이 client 연결을 닫아야 한다”는 의미로 사용된다.

### `PRIVMSG nick`

`PRIVMSG nick`은 특정 nickname을 대상으로 개인 메시지를 보내는 명령이다.

```text
PRIVMSG bob :hello
```

이 명령은 channel message가 아니다.
target이 `#`으로 시작하지 않을 때 `ClientCommand::privmsg()`가 처리한다.

처리 흐름:

```text
client가 registered 상태인지 검사
 -> recipient parameter 검사
 -> text parameter 검사
 -> target nickname 검색
 -> target client의 send buffer에 PRIVMSG 저장
```

응답:

| 상황 | 응답 |
|---|---|
| 미등록 client | `451 You have not registered` |
| recipient 없음 | `411 No recipient given (PRIVMSG)` |
| text 없음 | `412 No text to send` |
| target nickname 없음 | `401 <target> :No such nick/channel` |
| 성공 | target client에게 `:<prefix> PRIVMSG <target> :<text>` 전송 |

예시:

```text
alice -> server:
PRIVMSG bob :hello

server -> bob:
:alice!alice@localhost PRIVMSG bob :hello
```

현재 sender에게는 개인 메시지를 echo하지 않는다.
받는 client의 send buffer에만 메시지를 넣는다.

### unknown command

Parser가 알 수 없는 command를 만나면 `unknown()`으로 온다.

예:

```text
HELLO
```

응답:

```text
:ircserv 421 <nick> HELLO :Unknown command
```

nickname이 아직 없으면 target은 `*`를 사용한다.

```text
:ircserv 421 * HELLO :Unknown command
```

---

## channel command

`serverChannelCommand/command`는 channel을 대상으로 하는 IRC command를 처리한다.

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

channel command들의 공통 특징은 아래와 같다.

```text
등록 여부 검사
 -> parameter 검사
 -> channel 존재 여부 검사
 -> member/operator 권한 검사
 -> channel 상태 변경
 -> 필요한 numeric reply 또는 broadcast message 생성
```

---

## `CommandHelper`

`CommandHelper`는 class가 아니라 channel command들이 공유하는 helper 함수 모음이다.

| helper | 역할 |
|---|---|
| `commandValidChannel()` | `#`로 시작하고 공백/`,`/CR/LF/NUL이 없는 channel 이름 검사 |
| `commandReply()` | client send buffer에 응답 추가 |
| `commandNamesReply()` | `353`, `366` names 응답 추가 |
| `commandTopicReply()` | `331` 또는 `332` topic 응답 추가 |
| `commandToAll()` | channel 전체 member에게 message 추가 |
| `commandToOthers()` | sender를 제외한 member에게 message 추가 |
| `commandTarget()` | nickname이 없으면 `*`, 있으면 nickname 반환 |
| `commandMemberNames()` | operator는 `@nick`, 일반 member는 `nick`으로 목록 생성 |

중요한 점은 helper 함수들이 직접 `send()`를 호출하지 않는다는 것이다.
모든 응답은 client의 `SendBuffer`에 쌓이고, 실제 전송은 `ClientIO::send()`가 한다.

---

# channel command 상세

## `JOIN`

`JOIN`은 client를 channel에 참가시키는 명령이다.

```text
JOIN #test
JOIN #test key
```

처리 흐름:

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

성공 시 channel 전체에 아래 메시지를 보낸다.

```text
:<prefix> JOIN #channel
```

그 뒤 topic과 names 응답을 보낸다.

```text
:ircserv 353 <nick> = #test :@alice bob
:ircserv 366 <nick> #test :End of /NAMES list
```

실패 응답:

| 상황 | 응답 |
|---|---|
| 미등록 | `451 You have not registered` |
| parameter 부족 | `461 JOIN :Not enough parameters` |
| 잘못된 channel 이름 | `403 <channel> :No such channel` |
| invite-only인데 invite 없음 | `473 <channel> :Cannot join channel (+i)` |
| key 불일치 | `475 <channel> :Cannot join channel (+k)` |
| limit 초과 | `471 <channel> :Cannot join channel (+l)` |

현재 구현은 multi-channel JOIN comma parsing은 하지 않는다.

---

## `PART`

`PART`는 client를 channel에서 나가게 하는 명령이다.

```text
PART #test
PART #test :bye
```

처리 흐름:

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

성공 시 channel 전체에 아래 메시지를 보낸다.

```text
:<prefix> PART #channel :message
```

실패 응답:

| 상황 | 응답 |
|---|---|
| 미등록 | `451 You have not registered` |
| parameter 부족 | `461 PART :Not enough parameters` |
| channel 없음 | `403 <channel> :No such channel` |
| sender가 channel member가 아님 | `442 <channel> :You're not on that channel` |

현재 구현은 multi-channel PART comma parsing은 하지 않는다.

---

## `PRIVMSG #channel`

`PRIVMSG #channel`은 channel member들에게 메시지를 보내는 명령이다.

```text
PRIVMSG #test :hello
```

channel message는 sender를 제외한 channel member들에게 전송한다.

처리 흐름:

```text
등록 여부 검사
 -> recipient parameter 검사
 -> text parameter 검사
 -> channel 존재 검사
 -> sender가 channel member인지 검사
 -> sender를 제외한 channel member에게 message 저장
```

응답:

| 상황 | 응답 |
|---|---|
| 미등록 | `451 You have not registered` |
| recipient 없음 | `411 No recipient given (PRIVMSG)` |
| text 없음 | `412 No text to send` |
| channel 없음 | `403 <channel> :No such channel` |
| sender가 channel member가 아님 | `404 <channel> :Cannot send to channel` |
| 정상 | `:<prefix> PRIVMSG #channel :text`를 다른 member에게 전송 |

현재 sender에게는 channel message를 echo하지 않는다.

---

## `NAMES`

`NAMES`는 channel member 목록을 보여주는 명령이다.

```text
NAMES #test
```

처리 흐름:

```text
등록 여부 검사
 -> parameter 검사
 -> channel 이름 검사
 -> channel 존재 검사
 -> 353 names reply
 -> 366 end reply
```

응답 형식:

```text
:ircserv 353 <nick> = #test :@alice bob kevin
:ircserv 366 <nick> #test :End of /NAMES list
```

`@alice`는 alice가 channel operator라는 뜻이다.

irssi UI에서는 channel 이름이 축약되어 보일 수 있다.
raw 응답 확인은 아래처럼 한다.

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

---

## `WHO`

`WHO`는 channel member의 상세 정보를 보여주는 명령이다.

```text
WHO #test
```

처리 흐름:

```text
등록 여부 검사
 -> parameter 검사
 -> channel 검색
 -> channel이 있으면 member마다 352 응답
 -> 마지막에 315 end reply
```

응답 형식:

```text
:ircserv 352 <requester> #test <user> localhost ircserv <nick> H :0 <realname>
:ircserv 315 <requester> #test :End of WHO list
```

channel이 없어도 현재 구현은 `315 End of WHO list`로 끝낸다.
이 처리는 irssi에서 WHO 요청이 불필요한 에러를 만들지 않게 하는 호환 처리에 가깝다.

---

## `TOPIC`

`TOPIC`은 channel topic을 조회하거나 설정하는 명령이다.

| 입력 | 동작 |
|---|---|
| `TOPIC #test` | topic 조회 |
| `TOPIC #test :hello` | topic 설정 |
| `TOPIC #test :` | 빈 문자열 topic으로 설정. 사실상 topic 삭제처럼 동작 |

처리 흐름:

```text
등록 여부 검사
 -> parameter 검사
 -> channel 존재 검사
 -> 조회면 331/332 응답
 -> 설정이면 member 여부 검사
 -> +t이면 operator 여부 검사
 -> topic 저장
 -> channel 전체에 TOPIC message 전파
```

응답:

| 상황 | 응답 |
|---|---|
| 미등록 | `451 You have not registered` |
| parameter 부족 | `461 TOPIC :Not enough parameters` |
| channel 없음 | `403 <channel> :No such channel` |
| topic 없음 | `331 <channel> :No topic is set` |
| topic 있음 | `332 <channel> :<topic>` |
| sender가 channel member가 아님 | `442 <channel> :You're not on that channel` |
| 권한 없음 | `482 <channel> :You're not channel operator` |
| topic 변경 성공 | channel 전체에 `:<prefix> TOPIC #channel :topic` 전파 |

channel mode가 `+t`이면 operator만 topic을 바꿀 수 있다.
현재 channel default mode가 `+t`이므로 첫 member, 즉 operator가 아닌 일반 member는 기본적으로 topic 변경 권한이 없다.

---

## `INVITE`

`INVITE`는 target client를 channel invite list에 추가하는 명령이다.

```text
INVITE kevin #test
```

처리 흐름:

```text
등록 여부 검사
 -> parameter 검사
 -> target nickname 검사
 -> channel 존재 검사
 -> sender가 channel member인지 검사
 -> sender가 operator인지 검사
 -> target이 이미 channel member인지 검사
 -> invite list에 추가
 -> sender에게 341 응답
 -> target에게 INVITE message 전송
```

응답:

| 상황 | 응답 |
|---|---|
| 미등록 | `451 You have not registered` |
| parameter 부족 | `461 INVITE :Not enough parameters` |
| target nick 없음 | `401 <nick> :No such nick/channel` |
| channel 없음 | `403 <channel> :No such channel` |
| sender가 channel member가 아님 | `442 <channel> :You're not on that channel` |
| sender가 operator가 아님 | `482 <channel> :You're not channel operator` |
| target이 이미 channel member | `443 <nick> <channel> :is already on channel` |
| 성공 | sender에게 `341`, target에게 `INVITE` message |

성공 예시:

```text
:ircserv 341 alice kevin #test
:alice!alice@localhost INVITE kevin :#test
```

---

## `KICK`

`KICK`은 target client를 channel에서 제거하는 명령이다.

```text
KICK #test bob :reason
```

처리 흐름:

```text
등록 여부 검사
 -> parameter 검사
 -> channel 존재 검사
 -> sender member 여부 검사
 -> sender operator 여부 검사
 -> target member 여부 검사
 -> KICK message를 channel 전체에 전파
 -> target을 member/operator/invite에서 제거
 -> channel이 비었으면 삭제
```

응답:

| 상황 | 응답 |
|---|---|
| 미등록 | `451 You have not registered` |
| parameter 부족 | `461 KICK :Not enough parameters` |
| channel 없음 | `403 <channel> :No such channel` |
| sender가 channel member가 아님 | `442 <channel> :You're not on that channel` |
| sender가 operator가 아님 | `482 <channel> :You're not channel operator` |
| target이 channel member가 아님 | `441 <nick> <channel> :They aren't on that channel` |
| 성공 | channel 전체에 `KICK` message 전파 |

comment가 없으면 sender nickname을 기본 comment로 사용한다.

```text
:alice!alice@localhost KICK #test bob :alice
```

---

# `MODE` 모듈

`MODE` 모듈은 channel mode 변경과 일부 user mode 호환 처리를 담당한다.

ft_irc mandatory에서 중요한 부분은 channel mode다.

```text
MODE #channel +i / -i
MODE #channel +t / -t
MODE #channel +k key / -k
MODE #channel +o nick / -o nick
MODE #channel +l limit / -l
```

추가로 irssi 호환을 위해 아래 요청도 최소 처리한다.

```text
MODE #channel b
MODE <myNick>
MODE <myNick> +i
```

## 파일

| 파일 | 역할 |
|---|---|
| `Mode.hpp/cpp` | `MODE` 전체 흐름 조립 |
| `ModeChecker.hpp/cpp` | 등록 여부, 대상, channel 존재, 권한 검사 |
| `ModeParser.hpp/cpp` | mode 문자열과 parameter 해석 |
| `ModeApplier.hpp/cpp` | 수집된 변경을 channel 상태에 적용 |
| `ModeChange.hpp/cpp` | mode 변경 자료 구조 |

## 전체 처리 흐름

```text
Mode::handle()
 -> ModeChecker::prepare()
    -> 등록 여부 검사
    -> user MODE 처리
    -> channel 존재 검사
    -> ban list query 처리
    -> mode 조회 처리
    -> member/operator 권한 검사
 -> ModeParser::collect()
    -> mode string 해석
    -> 필요한 parameter 수집
    -> ModeChange에 변경 목록 저장
 -> ModeApplier::apply()
    -> Channel / ModeState / OperatorList 변경
 -> Mode::broadcast()
    -> 실제 변경이 있으면 channel 전체에 MODE message 전파
```

## 지원 mode

| mode | 의미 | 구현 |
|---|---|---|
| `+i`, `-i` | invite-only on/off | 구현 |
| `+t`, `-t` | topic restricted on/off | 구현 |
| `+k`, `-k` | channel key 설정/해제 | 구현 |
| `+l`, `-l` | user limit 설정/해제 | 구현 |
| `+o`, `-o` | operator 부여/해제 | 구현 |
| `b` | ban list query | ban 기능 없이 `368`만 응답 |

## `ModeChecker`

`ModeChecker`는 MODE 명령이 실제 변경 단계로 가도 되는지 검사한다.

처리 순서:

```text
미등록 검사
 -> parameter 검사
 -> user MODE인지 검사
 -> channel 존재 검사
 -> MODE #channel b 검사
 -> MODE #channel 조회인지 검사
 -> sender가 channel member인지 검사
 -> sender가 operator인지 검사
```

`prepare()`가 `true`를 반환하면 Mode 처리는 끝난다.
즉, 이미 응답을 보냈거나 더 진행하면 안 된다는 뜻이다.

`prepare()`가 `false`를 반환하면 mode 변경을 계속 진행한다.

### channel mode 조회

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

### ban list query

irssi는 channel sync 과정에서 ban list를 조회할 수 있다.

```text
MODE #test b
```

현재 서버는 ban 기능을 구현하지 않는다.
대신 irssi가 에러를 띄우지 않도록 아래 응답만 보낸다.

```text
:ircserv 368 alice #test :End of Channel Ban List
```

이 처리는 operator 권한 검사보다 먼저 수행된다.
그래서 일반 member가 irssi 기본 설정으로 join해도 `You're not channel operator`가 뜨지 않는다.

### user MODE

irssi는 등록 직후 user mode를 조회하거나 설정하려 할 수 있다.

| 입력 | 처리 |
|---|---|
| `MODE <myNick>` | 현재 client 자신의 user mode 조회. `221 <myNick> +i` 응답 |
| `MODE <myNick> +i` | irssi 호환용 user mode 설정 요청. 조용히 무시 |
| `MODE <otherNick> ...` | `502 Cant change mode for other users` |

여기서 `<myNick>`은 현재 연결된 client 자신의 실제 nickname이다.
예를 들어 현재 nick이 `kevin`이면 `MODE kevin`이 자기 mode 조회이고, `MODE alice`는 다른 유저 mode 조회로 처리된다.

현재 서버는 full user mode system을 구현하지 않는다.
`+i`는 irssi 호환을 위한 최소 응답이다.

## `ModeParser`

`ModeParser`는 mode string을 왼쪽부터 읽으면서 실제 변경할 작업을 `ModeChange`에 모은다.

예:

```text
MODE #test +kl pass 3
```

해석 결과:

```text
+k pass
+l 3
```

mode string은 `+` 또는 `-` sign과 mode 문자로 구성된다.

```text
+i-t+o bob
```

이런 입력도 sign을 바꿔가며 처리한다.

### parameter가 필요한 mode

| mode | parameter 필요 여부 |
|---|---|
| `+i`, `-i` | 필요 없음 |
| `+t`, `-t` | 필요 없음 |
| `+k` | key 필요 |
| `-k` | parameter 사용하지 않음 |
| `+l` | limit 숫자 필요 |
| `-l` | parameter 사용하지 않음 |
| `+o`, `-o` | target nickname 필요 |

### unknown mode

지원하지 않는 mode 문자가 들어오면 `472`를 보낸다.

```text
MODE #test +z
 -> :ircserv 472 alice z :is unknown mode char to me
```

## `ModeChange`

`ModeChange`는 mode 변경을 임시로 모아두는 자료 구조다.

주요 정보:

| 필드 | 의미 |
|---|---|
| `changes` | 실제 broadcast할 mode 문자열. 예: `+kl` |
| `params` | broadcast에 붙일 parameter 문자열. 예: ` pass 3` |
| `paramIndex` | mode parameter를 읽을 현재 index |
| `sign` | 현재 sign. `+` 또는 `-` |
| `currentSign` | broadcast 문자열에서 마지막으로 출력한 sign |
| `operations` | 실제 적용할 mode 작업 목록 |

중요한 점은 mode parsing 중에 바로 channel 상태를 바꾸지 않는다는 것이다.
먼저 `ModeChange`에 모으고, parsing이 성공하면 `ModeApplier`가 한 번에 적용한다.

## `ModeApplier`

`ModeApplier`는 `ModeChange.operations`에 모인 작업을 실제 channel 상태에 적용한다.

| operation | 적용 위치 |
|---|---|
| `+i`, `-i` | `ModeState::setInviteOnly()` |
| `+t`, `-t` | `ModeState::setTopicRestricted()` |
| `+k` | `ModeState::setKey()` |
| `-k` | `ModeState::clearKey()` |
| `+l` | `ModeState::setLimit()` |
| `-l` | `ModeState::clearLimit()` |
| `+o` | `OperatorList::add()` |
| `-o` | `OperatorList::remove()` |

## `ModeState` 기본값

| 상태 | 기본값 |
|---|---|
| topic | empty |
| invite-only | false |
| topic restricted | true |
| key | 없음 |
| limit | 없음 |

따라서 새 channel의 기본 mode string은 `+t`다.

## 전파 방식

mode 변경이 실제로 있으면 channel 전체에 아래 형식으로 전파한다.

```text
:<prefix> MODE #channel <changes><params>
```

예:

```text
:alice!alice@localhost MODE #test +o bob
:alice!alice@localhost MODE #test +kl pass 3
```

이미 켜져 있는 mode를 다시 켜거나, 없는 mode를 다시 끄는 경우에는 실제 변경이 없으므로 broadcast하지 않는다.

---

# `channel` 모듈

`channel` 모듈은 IRC channel의 상태를 관리한다.

channel은 단순한 이름 문자열이 아니다.
각 channel은 아래 상태를 가진다.

```text
channel name
member 목록
operator 목록
invite 목록
topic
mode 상태
```

`JOIN`, `PART`, `PRIVMSG #channel`, `TOPIC`, `INVITE`, `KICK`, `MODE` 같은 명령은 모두 channel 상태를 읽거나 바꾼다.

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

`Channel`은 channel 하나의 중심 객체다.

| 멤버 | 의미 |
|---|---|
| `_name` | channel 이름 |
| `_members` | 참가 client 목록 |
| `_operators` | operator client 목록 |
| `_invites` | invite된 client 목록 |
| `_modes` | topic/mode 상태 |

예를 들어 `#test` channel이 있으면, 그 안에는 아래 정보가 들어 있다.

```text
name: #test
members: alice, bob
operators: alice
invites: kevin
modes: +t, key 없음, limit 없음
topic: empty
```

## `ChannelManager`

`ChannelManager`는 여러 channel을 관리한다.

주요 역할:

```text
channel 찾기
channel 없으면 생성
비어 있는 channel 삭제
client 종료 시 모든 channel에서 client 제거
전체 channel 정리
```

### `findOrCreate()`

`JOIN #test`가 들어왔을 때 channel이 없으면 새로 만든다.

```text
JOIN #test
 -> ChannelManager::findOrCreate("#test")
 -> 없으면 새 Channel 생성
 -> 있으면 기존 Channel 반환
```

첫 번째 member가 들어오면 그 client가 operator가 된다.

```text
#test가 비어 있음
 -> alice JOIN
 -> alice를 member에 추가
 -> alice를 operator에 추가
```

### `removeEmpty()`

channel에 member가 하나도 없으면 channel을 삭제한다.

```text
PART #test
 -> member 제거
 -> channel이 empty인지 확인
 -> empty면 ChannelManager에서 삭제
```

### `removeClientFromAll()`

client가 연결 종료되면 그 client가 들어가 있던 모든 channel에서 제거해야 한다.

```text
client close
 -> ChannelManager::removeClientFromAll(client)
 -> 모든 channel에서 member/operator/invite 제거
 -> 비어진 channel 삭제
```

이 처리를 하지 않으면 끊어진 client 포인터가 channel 안에 남을 수 있다.

## `MemberList`

`MemberList`는 channel에 들어와 있는 client 목록을 관리한다.

사용되는 곳:

```text
JOIN 성공 시 member 추가
PART/KICK/QUIT 시 member 제거
PRIVMSG #channel 전송 대상 확인
NAMES 응답 생성
WHO 응답 생성
```

channel message는 member에게만 보낼 수 있다.

```text
PRIVMSG #test :hello
 -> sender가 #test member인지 검사
 -> member가 아니면 404 Cannot send to channel
```

## `OperatorList`

`OperatorList`는 channel operator 목록을 관리한다.

operator는 channel 관리 권한을 가진 client다.

사용되는 곳:

```text
TOPIC 변경 권한 검사
INVITE 권한 검사
KICK 권한 검사
MODE 변경 권한 검사
NAMES에서 @nick 표시
```

예:

```text
@alice bob kevin
```

여기서 `@alice`는 alice가 channel operator라는 뜻이다.

첫 번째로 channel에 들어온 client는 operator가 된다.

```text
JOIN #test
 -> channel이 비어 있음
 -> 첫 member를 operator로 추가
```

## `InviteList`

`InviteList`는 invite-only channel에 들어올 수 있는 client를 저장한다.

사용되는 흐름:

```text
MODE #test +i
 -> invite-only on

INVITE kevin #test
 -> kevin을 invite list에 추가

kevin JOIN #test
 -> invite list에 있는지 검사
 -> 있으면 JOIN 허용
 -> JOIN 후 invite list에서 제거
```

invite list는 영구 권한이 아니다.
현재 구현에서는 초대받은 client가 join하면 invite list에서 제거된다.

## `ModeState`

`ModeState`는 channel mode와 topic 상태를 저장한다.

| 상태 | 의미 | 기본값 |
|---|---|---|
| topic | channel topic | empty |
| invite-only | 초대받은 client만 join 가능 | false |
| topic restricted | operator만 topic 변경 가능 | true |
| key | channel password | 없음 |
| limit | 최대 member 수 | 없음 |

따라서 새 channel의 기본 mode는 `+t`다.

```text
MODE #test
 -> :ircserv 324 alice #test +t
```

`modeString()`은 현재 켜진 mode를 문자열로 만든다.

```text
+t
+it
+tkl
```

`modeParameters()`는 key와 limit처럼 mode string 뒤에 붙어야 하는 parameter를 만든다.

```text
MODE #test
 -> +tkl pass 3
```

## channel 상태가 바뀌는 대표 흐름

### JOIN

```text
JOIN #test
 -> ChannelManager::findOrCreate()
 -> invite/key/limit 검사
 -> MemberList에 client 추가
 -> 첫 member면 OperatorList에도 추가
```

### PART / KICK / QUIT

```text
client가 channel에서 나감
 -> MemberList에서 제거
 -> OperatorList에서 제거
 -> InviteList에서도 제거
 -> channel이 비었으면 삭제
```

### MODE

```text
MODE #test +k pass
 -> ModeApplier가 ModeState의 key 설정

MODE #test +l 3
 -> ModeApplier가 ModeState의 limit 설정

MODE #test +o bob
 -> OperatorList에 bob 추가
```

## 이 모듈이 하지 않는 일

`channel` 모듈은 아래 일을 하지 않는다.

```text
IRC line parsing
numeric reply 문자열 생성
recv/send 호출
poll event 처리
```

channel 모듈은 channel의 실제 상태를 저장하고 변경하는 역할에 집중한다.
응답 생성과 command 흐름 제어는 `serverCommand` 쪽에서 담당한다.

---

# 서버 응답 코드 정리

이 섹션은 현재 코드에서 실제로 사용하는 server reply와 numeric reply를 정리한 것이다.
평가 중에는 “이 에러가 왜 나왔는지”를 설명해야 할 수 있으므로, 코드에 들어간 응답은 최소한 한 번씩 확인해두는 것이 좋다.

---

## 등록 / client command 응답

| 코드 / 응답 | 사용 command | 발생 상황 | 현재 응답 의미 |
|---|---|---|---|
| `001` | `PASS` / `NICK` / `USER` | PASS 성공, NICK 설정, USER 설정이 모두 끝나서 등록 완료 | Welcome to ircserv |
| `221` | 등록 완료, `MODE <myNick>` | 현재 user mode 조회 결과 | `+i` 응답 |
| `409` | `PING` | PING parameter 없음 | No origin specified |
| `421` | unknown command | Parser가 모르는 command | Unknown command |
| `431` | `NICK` | nickname parameter 없음 | No nickname given |
| `433` | `NICK` | 이미 다른 client가 같은 nickname 사용 중 | Nickname is already in use |
| `461` | `PASS`, `USER` 등 | 필수 parameter 부족 | Not enough parameters |
| `462` | `PASS`, `USER` | 이미 registered 상태에서 다시 등록하려 함 | You may not reregister |
| `464` | `PASS` | password 불일치 | Password incorrect |
| `CAP * LS` | `CAP LS` | irssi capability 목록 요청 | 지원 capability 없음, 빈 목록 응답 |
| `PONG` | `PING token` | client PING에 대한 응답 | `PONG ircserv :token` |
| `ERROR` | `QUIT` | client가 연결 종료 요청 | Closing Link 후 연결 종료 |

---

## PRIVMSG 응답

`PRIVMSG`는 target이 nickname이면 `ClientCommand::privmsg()`가 처리하고, target이 channel이면 channel command `Privmsg::handle()`이 처리한다.

| 코드 | 사용 command | 발생 상황 | 현재 응답 의미 |
|---|---|---|---|
| `451` | `PRIVMSG nick`, `PRIVMSG #channel` | 미등록 client가 메시지를 보내려 함 | You have not registered |
| `411` | `PRIVMSG` | recipient parameter 없음 | No recipient given |
| `412` | `PRIVMSG` | text parameter 없음 | No text to send |
| `401` | `PRIVMSG nick` | target nickname을 찾지 못함 | No such nick/channel |
| `403` | `PRIVMSG #channel` | target channel 없음 | No such channel |
| `404` | `PRIVMSG #channel` | sender가 channel member가 아님 | Cannot send to channel |

정상 개인 메시지:

```text
:sender!user@localhost PRIVMSG targetNick :text
```

정상 channel 메시지:

```text
:sender!user@localhost PRIVMSG #channel :text
```

현재 sender에게는 echo하지 않고, target client 또는 sender를 제외한 channel member에게만 보낸다.

---

## channel command 응답

| 코드 | 사용 command | 발생 상황 | 현재 응답 의미 |
|---|---|---|---|
| `341` | `INVITE` | invite 성공 | inviter에게 invite 성공 알림 |
| `353` | `JOIN`, `NAMES` | channel member 목록 응답 | NAMES list |
| `366` | `JOIN`, `NAMES` | NAMES 응답 종료 | End of /NAMES list |
| `401` | `INVITE` | 초대할 target nick 없음 | No such nick/channel |
| `403` | `JOIN`, `PART`, `NAMES`, `PRIVMSG #channel`, `TOPIC`, `INVITE`, `KICK`, `MODE` | channel 이름이 잘못됐거나 channel이 없음 | No such channel |
| `441` | `KICK`, `MODE +o/-o` | target client가 해당 channel member가 아님 | They aren't on that channel |
| `442` | `PART`, `TOPIC`, `INVITE`, `KICK`, `MODE` | sender가 해당 channel member가 아님 | You're not on that channel |
| `443` | `INVITE` | target client가 이미 channel member임 | is already on channel |
| `451` | 대부분의 channel command | 미등록 client가 channel command 사용 | You have not registered |
| `461` | `JOIN`, `PART`, `NAMES`, `WHO`, `TOPIC`, `INVITE`, `KICK`, `MODE` | 필수 parameter 부족 | Not enough parameters |
| `471` | `JOIN` | channel limit 초과 | Cannot join channel (+l) |
| `473` | `JOIN` | invite-only channel인데 invite 없음 | Cannot join channel (+i) |
| `475` | `JOIN` | channel key 불일치 | Cannot join channel (+k) |
| `482` | `TOPIC`, `INVITE`, `KICK`, `MODE` | operator 권한 없음 | You're not channel operator |

---

## TOPIC / WHO 응답

| 코드 | 사용 command | 발생 상황 | 현재 응답 의미 |
|---|---|---|---|
| `331` | `TOPIC #channel` | topic 없음 | No topic is set |
| `332` | `TOPIC #channel` | topic 있음 | 현재 topic 반환 |
| `352` | `WHO #channel` | channel member 한 명마다 전송 | WHO reply |
| `315` | `WHO #channel` | WHO 응답 종료 | End of WHO list |

`WHO #channel`은 member마다 `352`를 보내고 마지막에 `315`를 보낸다.
channel이 없어도 현재 구현은 `315 End of WHO list`로 끝낸다.

---

## MODE 응답

| 코드 | 사용 command | 발생 상황 | 현재 응답 의미 |
|---|---|---|---|
| `221` | `MODE <myNick>` | 자신의 user mode 조회 | `+i` 응답 |
| `324` | `MODE #channel` | channel mode 조회 | 현재 channel mode 반환 |
| `368` | `MODE #channel b` | ban list 조회 | ban 기능 없이 End of Channel Ban List만 응답 |
| `441` | `MODE #channel +o/-o nick` | target이 channel member가 아님 | They aren't on that channel |
| `442` | `MODE #channel ...` | sender가 channel member가 아님 | You're not on that channel |
| `451` | `MODE` | 미등록 client | You have not registered |
| `461` | `MODE` | parameter 부족, `+k`, `+l`, `+o`에 필요한 parameter 부족 | Not enough parameters |
| `472` | `MODE #channel +x` | 지원하지 않는 mode 문자 | is unknown mode char to me |
| `482` | `MODE #channel ...` | sender가 channel operator가 아님 | You're not channel operator |
| `502` | `MODE <otherNick> ...` | 다른 user의 user mode를 보거나 바꾸려 함 | Cant change mode for other users |

주의할 점은 `+i`가 두 종류라는 것이다.

```text
MODE #test +i
 -> channel mode +i
 -> invite-only channel

MODE <myNick> +i
 -> user mode +i
 -> 현재 서버는 irssi 호환용으로 조용히 무시
```

---

## 내부 bool 반환값

numeric reply와 별개로, command handler들은 `bool`을 반환한다.
이 값은 command 성공/실패가 아니라 client 연결을 유지할지 여부를 나타낸다.

| 반환값 | 의미 | 대표 상황 |
|---|---|---|
| `true` | 연결 유지 | 정상 command, parameter error, 권한 error, unknown command |
| `false` | 연결 종료 필요 | `QUIT`, socket error, peer close |

예를 들어 `461`, `482`, `421` 같은 에러 응답을 보내도 연결은 유지된다.
반대로 `QUIT`은 `ERROR :Closing Link`를 보낸 뒤 `false`를 반환해서 서버가 client를 제거하게 만든다.

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
| client bytes 전송 | `ClientIO` |
| client 제거 | `ClientIO` + `ClientManager` + `ChannelManager` |
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
ctrl + D
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
mkdir -p /tmp/irssi-alice
irssi --home=/tmp/irssi-alice
```

kevin:

```sh
mkdir -p /tmp/irssi-kevin
irssi --home=/tmp/irssi-kevin
```

주의:

```sh
irssi --home = /tmp/irssi-kevin
```

처럼 `=` 앞뒤에 공백을 넣으면 안 된다.
이렇게 실행하면 현재 디렉토리에 `=` 디렉토리가 생기고, 그 안에 config가 만들어질 수 있다.

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

kevin:

```text
/join #test
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
MODE <myNick>의 221 최소 응답
```

## 구현하지 않은 것

```text
full capability negotiation
server-driven heartbeat
PONG timeout 검사
ban list 실제 저장/적용
multi-channel JOIN/PART comma parsing
full IRC user mode system
nickname 변경 broadcast
DCC, NOTICE, AWAY 등 mandatory 밖 command
server-to-server communication
```

`CAP`, `PING`, `PONG`, `QUIT`, `NAMES`, `WHO`, `MODE b`, `MODE <myNick>`는 과제 mandatory 핵심 명령이라기보다는 irssi reference client를 에러 없이 쓰기 위한 호환 처리에 가깝다.
특히 `MODE b`는 ban 기능 구현이 아니라 irssi의 channel sync 요청에 `368`로 정상 종료를 알려주는 최소 처리다.

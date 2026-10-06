# RemoteOps Communication Protocol

## TCP Connection

- Agent listens on TCP port: 9358
- Controller connects using TCP
- Text commands and responses are newline terminated
- PUT and GET transfer exact raw file bytes

## Authentication

```text
AUTH OPS-3146

Successful response:
OK AUTHENTICATED SID:6413

Invalid authentication:
ERR 001 AUTH_FAILED SID:6413

SYSINFO
Request:
SYSINFO

Response:
OK SYSINFO <cpu_load> <mem_used_mb> <uptime_sec> SID:6413

LISTPROC
Request:
LISTPROC

Response:
OK PROCS <process_information> SID:6413

EXEC
Supported commands:
EXEC DATE
EXEC UPTIME
EXEC DISKFREE
EXEC HOSTNAME
EXEC WHOAMI

Response:
OK EXEC_RESULT <output> SID:6413

Unsupported commands are rejected.
PUT
Request:
PUT <filename> <filesize>

The exact number of file bytes immediately follows the command.
Successful response:
OK FILE_RECEIVED <filename> SID:6413

Files are stored under:
./agentfiles/IT23583146/

GET
Request:
GET <filename>

Response header:
OK FILE_SEND <filename> <filesize> SID:6413

The exact number of file bytes follows the response header.
The downloaded file was verified byte-for-byte using cmp.
UDP Monitoring
Controller UDP port:
9001

Start:
MONITOR START 9001

Response:
OK MONITOR_STARTED SID:6413

Monitoring datagram:
SYSINFO <cpu_load> <mem_used_mb> <uptime_sec> SID:6413

Stop:
MONITOR STOP

Response:
OK MONITOR_STOPPED SID:6413

QUIT
Request:
QUIT

Response:
OK BYE SID:6413

The Agent then closes the TCP connection cleanly.
Session Identification
Every TCP response and UDP monitoring datagram includes:
SID:6413
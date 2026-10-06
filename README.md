# RemoteOps - IE3090 Network Programming

## Student Details

- Registration Number: IT23583146
- Agent Port: 9358
- Session ID: 6413
- Authentication Token: OPS-3146
- Agent Source: agent_146.c
- Controller Source: controller_146.c
- Makefile: Makefile_146
- Log File: remoteops_IT23583146.log
- Storage Path: ./agentfiles/IT23583146/

## Project Overview

RemoteOps is a TCP/IP based remote system monitoring and management tool consisting of an Agent and Controller.

The Agent operates as the server on the managed machine. The Controller connects to the Agent and performs authenticated system monitoring and management operations.

TCP is used for command and file-transfer operations, while UDP is used for periodic system monitoring.

## Architecture

```text
                    TCP :9358
Controller ----------------------------> Agent
           AUTH / SYSINFO / LISTPROC
           EXEC / PUT / GET
           MONITOR / QUIT

Controller <---------------------------- Agent
                    TCP responses

                    UDP :9001
Controller <---------------------------- Agent
             Periodic SYSINFO

The Agent uses a thread-per-client concurrency model. Each accepted Controller connection is handled by a separate POSIX thread.

Implemented Commands
Command	Status
AUTH	Implemented
SYSINFO	Implemented
LISTPROC	Implemented
EXEC DATE	Implemented
EXEC UPTIME	Implemented
EXEC DISKFREE	Implemented
EXEC HOSTNAME	Implemented
EXEC WHOAMI	Implemented
PUT	Implemented
GET	Implemented
MONITOR START	Implemented
MONITOR STOP	Implemented
QUIT	Implemented


EXEC Security
Remote command execution is restricted to the required whitelist:
- DATE
- UPTIME
- DISKFREE
- HOSTNAME
- WHOAMI
Commands outside this whitelist are rejected.

File Transfer
Uploaded files are stored under:
./agentfiles/IT23583146/
PUT transfers exactly the specified number of raw bytes.
GET returns the file size followed by exactly the specified number of raw bytes.
The GET implementation was verified using cmp to confirm that the downloaded file was byte-for-byte identical to the original.

UDP Monitoring
The Controller binds UDP port 9001.
After MONITOR START 9001, the Agent periodically sends:
SYSINFO <cpu_load> <mem_used_mb> <uptime_sec> SID:6413
The Controller receives and displays the monitoring datagrams.

Logging
The Agent writes timestamped activity to:
remoteops_IT23583146.log
The log records connections, commands, file transfers, monitoring operations and disconnections.

Testing
The following tests were completed successfully:
- Authentication
- SYSINFO
- LISTPROC
- EXEC DATE
- PUT
- GET
- Byte-for-byte GET verification
- UDP monitoring
- MONITOR STOP
- QUIT
- Invalid authentication
- Five simultaneous Controller connections
- Agent listening on port 9358
- Timestamped logging

Build
Compile the Agent:
gcc -Wall -Wextra -pthread -o agent_146 agent_146.c

Compile the Controller:
gcc -Wall -Wextra -pthread -o controller_146 controller_146.c

Or use the personalised Makefile:
make -f Makefile_146

Running
Start the Agent:
./agent_146

Start the Controller in another terminal:
./controller_146

Personalised Storage
Example uploaded file:
./agentfiles/IT23583146/upload.txt

Concurrency
The Agent supports multiple simultaneous Controller connections using POSIX threads. Five simultaneous Controller connections were tested successfully.
Error Handling
Invalid authentication produces:
ERR 001 AUTH_FAILED SID:6413

The Agent also handles unknown/disallowed commands using the required error-response format.
Evidence
Testing evidence includes:
- Agent listening on TCP port 9358
- Successful authentication
- SYSINFO
- LISTPROC
- EXEC
- PUT
- GET
- UDP monitoring
- Graceful QUIT
- Authentication failure
- Five simultaneous Controller connections
- Personalised storage directory
- Timestamped log file
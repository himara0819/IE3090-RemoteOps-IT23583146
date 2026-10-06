# RemoteOps Design Diary

## Project
IE3090 Network Programming - RemoteOps

## Personalisation
- Registration Number: IT23583146
- Port: 9358
- SID: 6413
- Authentication Token: OPS-3146

## Initial Design

RemoteOps was designed using a client-server architecture consisting of a TCP Agent and a Controller.

The Agent acts as the server and listens on TCP port 9358. The Controller connects to the Agent and sends authenticated management commands.

## Concurrency Decision

A thread-per-client model was selected for the Agent.

When a Controller connects, the Agent creates a POSIX thread to handle that Controller. This allows multiple Controllers to communicate with the same Agent concurrently.

Five simultaneous Controller connections were tested successfully.

## Authentication

Authentication was implemented using the personalised token OPS-3146.

The Agent requires successful authentication before processing normal commands.

Invalid authentication was also tested and produced the required AUTH_FAILED response.

## TCP Commands

The implementation supports:

- AUTH
- SYSINFO
- LISTPROC
- EXEC
- PUT
- GET
- MONITOR START
- MONITOR STOP
- QUIT

## File Transfer

PUT and GET were implemented using the specified file size so that the exact number of file bytes can be transferred.

The uploaded file is stored under:

./agentfiles/IT23583146/

GET was verified using the Linux `cmp` command to confirm byte-for-byte equality between the original and downloaded files.

## UDP Monitoring

UDP was implemented as a secondary monitoring channel.

The Controller listens on UDP port 9001. After MONITOR START, the Agent periodically sends system information to the Controller.

The monitoring interval used in the implementation is approximately two seconds.

MONITOR STOP terminates the monitoring thread cleanly.

## Logging

Timestamped logging was added to:

remoteops_IT23583146.log

The log records connections, commands, file transfers, monitoring operations and disconnections.

## Problems Encountered

During development, several issues were identified and corrected.

The Controller initially had command-response synchronization problems around file transfer operations. The command flow was corrected so that each command waits for its appropriate response.

The UDP monitoring implementation initially caused a double-free problem when monitoring stopped. Ownership of the monitoring structure was corrected so that it is freed by the appropriate handler.

The Controller initially sent QUIT immediately after MONITOR START. The sequence was corrected to:

MONITOR START
MONITOR STOP
QUIT

## Testing

The implementation was tested for:

- Authentication
- SYSINFO
- LISTPROC
- EXEC
- PUT
- GET
- UDP monitoring
- MONITOR STOP
- QUIT
- Invalid authentication
- Five simultaneous Controllers
- Logging
- Personalised port and storage path

All mandatory functional tests completed successfully.
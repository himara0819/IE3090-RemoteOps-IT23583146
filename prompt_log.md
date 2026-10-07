# IE3090 RemoteOps - AI Prompt Log

## Student

- Registration Number: IT23583146
- Project: RemoteOps
- Module: IE3090 Network Programming

## AI Tool Used

ChatGPT

## Purpose of AI Assistance

AI assistance was used during the Part 1 implementation for debugging, code review, protocol troubleshooting, testing guidance, documentation support and identifying implementation issues.

All generated suggestions were reviewed, implemented where appropriate, compiled and tested before being retained in the project.

---

## Interaction 1 — TCP Server and Client

### Task

Develop and review the initial TCP Agent/Controller structure using the BSD sockets API.

### How the output was used

The socket structure and command-processing approach were reviewed and adapted to the project requirements.

### Validation

The Agent and Controller were compiled using GCC and tested locally.

---

## Interaction 2 — Concurrent Connections

### Task

Implement multiple simultaneous Controller connections using POSIX threads.

### How the output was used

The Agent was modified to create a separate thread for each accepted Controller connection.

### Validation

Five simultaneous Controller connections were tested successfully.

---

## Interaction 3 — File Transfer Debugging

### Task

Debug PUT and GET behaviour and ensure exact file-size transfers.

### How the output was used

The TCP file-transfer logic was reviewed and corrected so that the exact number of file bytes is transferred.

### Validation

The downloaded file was compared with the original using:

```bash
cmp upload.txt downloaded_upload.txt
```
No output was produced, confirming byte-for-byte equality.
Interaction 4 — UDP Monitoring
Task
Implement and troubleshoot UDP monitoring using MONITOR START and MONITOR STOP.
How the output was used
The UDP monitoring thread and Controller UDP receiver were reviewed and corrected.
Validation
The Controller successfully received repeated SYSINFO UDP datagrams.
Interaction 5 — Memory Management
Task
Investigate a double-free error occurring when UDP monitoring stopped.
How the output was used
Ownership of the monitoring structure was reviewed. Duplicate free() operations in the monitoring thread were removed so that the appropriate handler owns the allocated structure.
Validation
MONITOR STOP followed by QUIT completed without a crash.
Interaction 6 — Protocol Synchronisation
Task
Investigate command/response ordering problems between the Controller and Agent.
How the output was used
The command sequence was reviewed so that each Controller request waits for its corresponding Agent response.
Validation
AUTH, SYSINFO, LISTPROC, EXEC, PUT, GET, MONITOR START, MONITOR STOP and QUIT were tested successfully.
Interaction 7 — Logging
Task
Add timestamped logging required by the assignment.
How the output was used
A logging function was added to record connection, command, transfer, monitoring and disconnection events.
Validation
remoteops_IT23583146.log was generated and inspected successfully.
Interaction 8 — Documentation and Testing
Task
Review the project documentation, testing evidence and submission structure.
How the output was used
README, design diary, protocol documentation and testing documentation were prepared.
Validation
The repository was checked using Git status and commit history.
Critical Evaluation
AI suggestions were not accepted blindly. The implementation was compiled and executed after changes, and several suggestions required correction during testing.
In particular, the UDP monitoring implementation exposed a double-free issue and the Controller command sequence required debugging. These problems were identified through actual execution and corrected before finalising the implementation.
The final implementation was tested against the required functional features, including authentication, system information, process listing, restricted command execution, file upload/download, UDP monitoring, graceful disconnection, logging and concurrent Controller connections.

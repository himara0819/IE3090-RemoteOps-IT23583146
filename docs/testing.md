# RemoteOps Testing Record

## Test Environment

- OS: Linux
- Language: C
- Compiler: GCC
- Agent Port: 9358
- Controller UDP Port: 9001
- Session ID: 6413

## Functional Tests

| Test | Expected Result | Result |
|---|---|---|
| Valid AUTH | Authentication succeeds | PASS |
| Invalid AUTH | `ERR 001 AUTH_FAILED` | PASS |
| SYSINFO | CPU, memory and uptime returned | PASS |
| LISTPROC | Process list returned | PASS |
| EXEC DATE | Date returned | PASS |
| PUT | File uploaded to personalised storage | PASS |
| GET | File downloaded successfully | PASS |
| File comparison | Original and downloaded files identical | PASS |
| MONITOR START | UDP monitoring starts | PASS |
| UDP monitoring | Periodic SYSINFO datagrams received | PASS |
| MONITOR STOP | Monitoring stops | PASS |
| QUIT | Connection closes cleanly | PASS |
| Logging | Timestamped log generated | PASS |
| Port verification | Agent listens on port 9358 | PASS |
| Concurrent connections | Five Controllers served | PASS |

## GET Verification

The downloaded file was verified using:

```bash
cmp upload.txt downloaded_upload.txt

The command produced no output, confirming byte-for-byte equality.
UDP Verification
The Controller successfully received periodic datagrams such as:
UDP Monitor: SYSINFO 0.68 1850 13368 SID:6413

Monitoring was then stopped successfully using MONITOR STOP.
Authentication Error Test
An incorrect authentication token produced:
ERR 001 AUTH_FAILED SID:6413

Concurrency Test
Five simultaneous TCP Controller connections were created.
All five successfully authenticated and returned:
OK AUTHENTICATED SID:6413

All five subsequently returned:
OK BYE SID:6413

Logging Verification
The following personalised log file was verified:
remoteops_IT23583146.log

The log contained timestamps for connections, commands, file transfers, monitoring operations and disconnections.
Personalisation Verification
The Agent was verified listening on:
0.0.0.0:9358

The personalised storage directory was verified:
./agentfiles/IT23583146/

and contained:
upload.txt
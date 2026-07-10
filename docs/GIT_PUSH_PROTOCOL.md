# Git Push Protocol (Smart HTTP)

This document explains the Git Smart HTTP Protocol for push operations, specifically the `git-receive-pack` service. It focuses on response parsing, sideband multiplexing, and common pitfalls.

## Table of Contents
1. [Push Protocol Overview](#push-protocol-overview)
2. [Request Format](#request-format)
3. [Response Format](#response-format)
4. [Example Response](#example-response)
5. [Common Issues](#common-issues)

---

## Push Protocol Overview

Git push over Smart HTTP uses a two-phase protocol:

1. **Reference Discovery**: `GET /info/refs?service=git-receive-pack`
   - Server advertises current refs and capabilities
   - Client learns what refs exist and what the server supports

2. **Push Request**: `POST /git-receive-pack`
   - Client sends ref update commands and packfile
   - Server unpacks objects and updates refs
   - Server responds with status report

### Key Capabilities

- **report-status**: Server sends detailed feedback about unpack and ref updates
- **report-status-v2**: Extended version with additional metadata (options)
- **side-band-64k**: Multiplexes packfile data, progress, and errors into separate channels
- **delete-refs**: Allows deleting references
- **ofs-delta**: Enables offset-based delta encoding in packfiles
- **atomic**: All ref updates succeed or fail together

---

## Request Format

### Reference Discovery Response

```
HTTP/1.1 200 OK
Content-Type: application/x-git-receive-pack-advertisement

001f# service=git-receive-pack\n
0000
00b27d1665144a3a975c05f1f43902ddaf084e784dbe refs/heads/main\0report-status report-status-v2 delete-refs side-band-64k ofs-delta atomic\n
003f74730d410fcb6603ace96f1dc55ea6196122532d refs/heads/feature\n
0000
```

**Format Breakdown:**
- `001f# service=git-receive-pack\n` - Service announcement (length 001f = 31 bytes)
- `0000` - Flush packet (marks end of service line)
- `00b27d16...` - First ref with capabilities (length 00b2 = 178 bytes)
  - Old SHA-1 (40 hex chars)
  - Space
  - Ref name
  - `\0` (NUL byte) separates ref from capabilities
  - Capability list (space-separated)
  - `\n` (newline)
- `003f7473...` - Subsequent refs without capabilities
- `0000` - Flush packet (end of ref list)

### Push Request Format

```
POST /git-receive-pack HTTP/1.1
Content-Type: application/x-git-receive-pack-request

00677d1665144a3a975c05f1f43902ddaf084e784dbe 74730d410fcb6603ace96f1dc55ea6196122532d refs/heads/main\0report-status side-band-64k\n
0000
[PACKFILE DATA]
```

**Command Structure:**
```
command-list = PKT-LINE(command NUL capability-list)
               *PKT-LINE(command)
               flush-pkt

command = old-id SP new-id SP refname

where:
  old-id = 40-hex SHA-1 (or 40 zeros for create)
  new-id = 40-hex SHA-1 (or 40 zeros for delete)
  refname = "refs/heads/main", etc.
```

**First Command:**
- Includes `\0` separator followed by space-separated capabilities
- Only the first command includes capabilities

**Subsequent Commands:**
- Same format but WITHOUT the `\0` and capability list

**Flush Packet:**
- `0000` marks end of command list

**Packfile:**
- Immediately follows the flush packet
- Contains Git objects in pack format
- If sideband is negotiated, response will be multiplexed

---

## Response Format

### Pkt-line Format

Every piece of data in Git's protocol uses the **pkt-line** format:

```
[4-byte hex length][data]
```

- **Length**: 4-byte hexadecimal string representing total packet size (including the 4 length bytes)
- **Minimum**: `0004` (special cases: `0000` = flush, `0001` = delim, `0002` = response-end)
- **Maximum**: Depends on context
  - `1000` bytes (FFF) for side-band
  - `65520` bytes (FFF0) for side-band-64k
- **Data**: Arbitrary bytes, often text ending in `\n`

**Special Packets:**
- `0000` - **Flush packet**: End of message/section
- `0001` - **Delimiter packet**: Separates sections (protocol v2)
- `0002` - **Response-end packet**: End of response (protocol v2)

**Examples:**
```
000eunpack ok\n
  └─ 000e = 14 bytes total (4 length + 10 data)

0018ok refs/heads/main\n
  └─ 0018 = 24 bytes total (4 length + 20 data)

002ang refs/heads/test non-fast-forward\n
  └─ 002a = 42 bytes total (4 length + 38 data)
```

### Sideband Multiplexing

When `side-band-64k` is negotiated, all server output is **multiplexed** into three channels:

```
[4-byte pkt-line length][1-byte channel][data]
```

**Channels:**
- **Channel 1**: Protocol data (packfile data during fetch, report-status during push)
- **Channel 2**: Progress messages (informational, typically displayed to stderr)
- **Channel 3**: Error messages (fatal errors before aborting)

**Size Limits:**
- `side-band`: Up to 999 bytes data + 1 byte channel = 1000 bytes total
- `side-band-64k`: Up to 65,519 bytes data + 1 byte channel = 65,520 bytes total

**Modern Behavior:**
- Modern clients always prefer `side-band-64k`
- These capabilities are mutually exclusive
- Client MUST NOT request both

**Example Sideband Packet:**
```
00120001unpack ok\n
  └─ 0012 = 18 bytes total
     └─ 01 = channel 1 (protocol data)
        └─ unpack ok\n = 11 bytes payload
```

### Report-Status Format

After receiving the packfile, if `report-status` capability was negotiated, the server sends a status report.

#### Without Sideband

```
report-status = unpack-status
                1*(command-status)
                flush-pkt

unpack-status = PKT-LINE("unpack" SP unpack-result)
unpack-result = "ok" / error-msg

command-status = command-ok / command-fail
command-ok = PKT-LINE("ok" SP refname)
command-fail = PKT-LINE("ng" SP refname SP error-msg)
```

**Example:**
```
000eunpack ok\n
0018ok refs/heads/main\n
002ang refs/heads/test non-fast-forward\n
0000
```

#### With Sideband-64k

When sideband is active, **each pkt-line is wrapped in a sideband packet on channel 1**:

```
[pkt-line-length][0x01][pkt-line-data]
```

**Key Insight:**
- The report-status data is sent on **channel 1** (protocol data)
- Each pkt-line from the report-status grammar is prefixed with channel byte `0x01`
- The sideband packet length includes: 4 bytes (pkt-line length prefix) + 1 byte (channel) + N bytes (actual pkt-line)

**Critical Parsing Rule:**
```
When parsing with sideband:
1. Read 4-byte pkt-line length
2. Check for special packets (0000, 0001, 0002)
3. If not special, read 1 byte for channel
4. Read remaining (length - 5) bytes as data
5. If channel == 1, parse as report-status pkt-line
```

### Report-Status-v2 (Extended)

The v2 format includes optional metadata for each successful ref update:

```
command-ok-v2 = command-ok
                *option-line

option-line = option-refname / option-old-oid / option-new-oid / option-forced-update

option-refname = PKT-LINE("option" SP "refname" SP refname)
option-old-oid = PKT-LINE("option" SP "old-oid" SP obj-id)
option-new-oid = PKT-LINE("option" SP "new-oid" SP obj-id)
option-forced-update = PKT-LINE("option" SP "forced-update")
```

**Use Case:**
- Allows `proc-receive` hook to create pseudo-refs that map to multiple actual refs
- Each option line provides details about what actually happened server-side

**Example:**
```
0018ok refs/for/main\n
001foption refname refs/pull/123/head\n
0033option old-oid 0000000000000000000000000000000000000000\n
0030option new-oid 74730d410fcb6603ace96f1dc55ea6196122532d\n
0000
```

### Progress Messages (Channel 2)

During pack transfer and unpacking, servers send progress messages on channel 2:

```
[pkt-line-length][0x02][progress text]
```

**Example:**
```
0024\x02Receiving objects:  50% (25/50)\r
0024\x02Receiving objects: 100% (50/50)\n
```

**Handling:**
- Typically displayed to user's stderr
- Often contain `\r` for overwriting previous line
- Client can suppress with `quiet` capability

### Error Messages (Channel 3)

Fatal errors that cause immediate termination use channel 3:

```
[pkt-line-length][0x03][error text]
```

**Example:**
```
002a\x03fatal: disk quota exceeded\n
```

**Behavior:**
- Sent immediately before connection abort
- Client should display prominently and halt operation

---

## Example Response

### Successful Push (with sideband-64k and report-status)

**Scenario:** Push one commit to `refs/heads/main`, server accepts it.

**Raw Bytes (Hex Representation):**

```
HTTP/1.1 200 OK
Content-Type: application/x-git-receive-pack-result

[Progress messages during unpack]
0024\x02Unpacking objects:  50% (1/2)\r
0024\x02Unpacking objects: 100% (2/2)\n
0018\x02Unpacking objects: done\n

[Report-status on channel 1]
0012\x01000eunpack ok\n
001c\x010018ok refs/heads/main\n
0009\x010000

[Note: The last packet is sideband-wrapped flush]
```

**Parsed Breakdown:**

| Raw Packet | Length | Channel | Pkt-line | Meaning |
|------------|--------|---------|----------|---------|
| `0024\x02Unpacking objects:  50% (1/2)\r` | 36 | 2 | N/A | Progress |
| `0024\x02Unpacking objects: 100% (2/2)\n` | 36 | 2 | N/A | Progress |
| `0018\x02Unpacking objects: done\n` | 24 | 2 | N/A | Progress |
| `0012\x01000eunpack ok\n` | 18 | 1 | `000eunpack ok\n` | Status |
| `001c\x010018ok refs/heads/main\n` | 28 | 1 | `0018ok refs/heads/main\n` | Ref OK |
| `0009\x010000` | 9 | 1 | `0000` | Flush |

**Algorithm:**

```python
while True:
    # Read 4-byte length
    length_hex = read(4)
    length = int(length_hex, 16)

    # Check for flush
    if length == 0:
        break

    # Read channel byte
    channel = read(1)[0]

    # Read data
    data = read(length - 5)  # Subtract 4 (length) + 1 (channel)

    if channel == 1:
        # Protocol data - parse as pkt-line
        parse_report_status_pktline(data)
    elif channel == 2:
        # Progress - display to stderr
        print_progress(data)
    elif channel == 3:
        # Error - abort
        die(data)
```

### Failed Push (non-fast-forward)

**Scenario:** Trying to push to `main` without pulling latest changes.

**Parsed Response:**

```
[Progress messages]
0024\x02Unpacking objects: 100% (2/2)\n
0018\x02Unpacking objects: done\n

[Report-status]
0012\x01000eunpack ok\n
0030\x01002cng refs/heads/main non-fast-forward\n
0009\x010000
```

**Interpretation:**
- Packfile unpacked successfully (`unpack ok`)
- Ref update **rejected** (`ng refs/heads/main non-fast-forward`)
- Error message: "non-fast-forward" indicates need to pull first

### Multiple Ref Updates

**Scenario:** Push to `main` (success) and `test` (fail), with report-status-v2.

**Parsed Response:**

```
0012\x01000eunpack ok\n
001c\x010018ok refs/heads/main\n
001b\x010017option forced-update\n
0030\x01002cng refs/heads/test pre-receive hook declined\n
0009\x010000
```

**Interpretation:**
- Packfile OK
- `main` updated successfully (forced update)
- `test` rejected by server-side hook

---

## Common Issues

### 1. Invalid Channel Error (Channel 101)

**Symptom:**
```
fatal: protocol error: bad band #101
```

**Root Cause:**
- ASCII code 101 = 'e'
- Server sends `0013error: disk quota exceeded` **without** sideband encoding
- Client expects sideband, reads 'e' as channel byte

**Why It Happens:**
- Server encounters critical error (disk quota, filesystem issue)
- Server switches to non-sideband error format mid-stream
- Protocol violation: once sideband is negotiated, **all** output must be multiplexed

**Fix:**
- Server-side: Increase disk quota / fix storage issues
- Client-side: Detect non-sideband errors gracefully
  - Check if first byte is valid channel (1, 2, 3)
  - If not, treat entire packet as error message

**Detection Code:**
```c
uint8_t channel = buffer[4];  // First byte after pkt-line length
if (channel != 1 && channel != 2 && channel != 3) {
    // Not a valid channel - might be protocol error
    // Check if packet starts with "ERR " or "error:"
    if (memcmp(buffer + 4, "ERR ", 4) == 0 ||
        memcmp(buffer + 4, "error:", 6) == 0) {
        // Treat as server error, not sideband packet
        handle_server_error(buffer + 4, length - 4);
        return;
    }
}
```

### 2. Confusing Pkt-line with Sideband Length

**Symptom:**
- Parsing fails after reading unpack status
- Off-by-one errors in packet boundaries

**Root Cause:**
- When sideband is active, there are **two length prefixes**:
  1. **Sideband packet length**: Total bytes including channel byte
  2. **Pkt-line length**: Embedded in the data on channel 1

**Example:**
```
0012\x01000eunpack ok\n
^^^^    ^^^^
│       └─ Pkt-line length (14 bytes including itself)
└─ Sideband packet length (18 bytes)
```

**Correct Parsing:**
```c
// Step 1: Read sideband packet
uint16_t sb_len = parse_pktline_length(buffer);
uint8_t channel = buffer[4];
const char* data = buffer + 5;
size_t data_len = sb_len - 5;

if (channel == 1) {
    // Step 2: Parse embedded pkt-line
    uint16_t pkt_len = parse_pktline_length(data);
    const char* content = data + 4;
    size_t content_len = pkt_len - 4;

    // Now process report-status line
    if (strncmp(content, "unpack ", 7) == 0) {
        // Handle unpack status
    }
}
```

### 3. Not Handling Flush Packet in Sideband

**Symptom:**
- Parser hangs or crashes at end of report-status
- Misinterpreting flush as data

**Root Cause:**
- Flush packet `0000` is **also sent through sideband**: `0009\x010000`
- Must check for flush within channel 1 data

**Fix:**
```c
// Read sideband packet
uint16_t len = parse_pktline_length(buffer);

// Check for sideband flush (connection end)
if (len == 0) {
    return DONE;
}

uint8_t channel = buffer[4];
if (channel == 1) {
    // Check for embedded flush (end of report-status)
    if (memcmp(buffer + 5, "0000", 4) == 0) {
        return REPORT_STATUS_DONE;
    }

    // Parse normal pkt-line
    parse_report_status_line(buffer + 5);
}
```

### 4. Ignoring Progress Messages

**Symptom:**
- Parser attempts to interpret progress as protocol data
- Errors like "unexpected 'R' (Receiving objects...)"

**Root Cause:**
- Not filtering out channel 2 (progress) messages
- Trying to parse all sideband data as report-status

**Fix:**
```c
switch (channel) {
    case 1:
        // Protocol data - parse report-status
        parse_report_status(data, data_len);
        break;
    case 2:
        // Progress - display or ignore
        fprintf(stderr, "remote: %.*s", (int)data_len, data);
        break;
    case 3:
        // Error - abort
        die("remote error: %.*s", (int)data_len, data);
        break;
    default:
        die("protocol error: invalid channel %d", channel);
}
```

### 5. HTTP 200 vs 500 Confusion

**Symptom:**
- HTTP 200 but push failed
- HTTP 500 with actual server error

**Key Points:**
- **HTTP 200** does NOT mean push succeeded
  - It means HTTP request was processed
  - Actual push result is in report-status
- **HTTP 500** indicates server error before pack processing
  - Might not follow sideband protocol
  - Check `Content-Type` header

**Handling:**
```c
if (http_status == 500) {
    // Server error - response might not be git protocol
    if (content_type != "application/x-git-receive-pack-result") {
        // Probably HTML error page
        fprintf(stderr, "Server returned HTTP 500\n");
        return ERROR;
    }
}

if (http_status == 200) {
    // Parse report-status to determine actual result
    parse_report_status(response_body);

    // Check unpack status
    if (unpack_status != "ok") {
        fprintf(stderr, "Unpack failed: %s\n", unpack_status);
        return ERROR;
    }

    // Check ref updates
    for (ref in refs) {
        if (!ref.ok) {
            fprintf(stderr, "Failed to update %s: %s\n",
                    ref.name, ref.error);
            return ERROR;
        }
    }
}
```

### 6. GitHub-Specific Behavior

**Differences from Standard Git:**

1. **Longer Capability Lists:**
   - GitHub advertises many custom capabilities
   - Example: `agent=github/spokes-receive-pack-*`

2. **Pre-receive Hook Messages:**
   - GitHub may send channel 2 messages from pre-receive hooks
   - Can include CI/CD status, warnings, etc.

3. **Content-Type Headers:**
   - Always check: `application/x-git-receive-pack-result`
   - Errors may return `text/html` with HTTP 500/503

4. **Repository Size Limits:**
   - GitHub enforces 100MB file limit, 5GB repo limit
   - Rejection sent as `ng` message in report-status

5. **Force Push Detection:**
   - GitHub detects force pushes and includes in audit log
   - Client can use `report-status-v2` with `forced-update` option

**Example GitHub Error:**
```
002cng refs/heads/main GH001: Large files detected
```

### 7. Incomplete Packfile Upload

**Symptom:**
- Server responds with `unpack [error]`
- Errors like "short read", "truncated pack", "corrupt pack"

**Root Cause:**
- Network interruption during packfile transfer
- Client didn't send all packfile bytes

**Prevention:**
```c
// Calculate packfile size
size_t packfile_size = calculate_pack_size(objects);

// Set Content-Length header
snprintf(headers, sizeof(headers),
         "Content-Length: %zu\r\n"
         "Content-Type: application/x-git-receive-pack-request\r\n",
         command_size + packfile_size);

// Send with retry logic
if (!send_all(socket, packfile, packfile_size)) {
    return ERROR_NETWORK;
}
```

### 8. Line Ending Confusion

**Symptom:**
- Parser doesn't match `"unpack ok\n"`
- Extra characters in parsed strings

**Root Cause:**
- Report-status uses `\n` (LF) line endings
- Windows clients might expect `\r\n` (CRLF)

**Fix:**
- Always expect `\n` (single byte 0x0A)
- Strip trailing whitespace when parsing
- Don't use text-mode file I/O for protocol parsing

```c
// Safe comparison
bool is_unpack_ok(const char* line, size_t len) {
    // Handle both "unpack ok\n" and "unpack ok"
    return (len == 10 && memcmp(line, "unpack ok\n", 10) == 0) ||
           (len == 9 && memcmp(line, "unpack ok", 9) == 0);
}
```

### 9. Timeout During Large Push

**Symptom:**
- Push hangs during packfile upload
- No progress messages, eventually times out

**Causes:**
- Network congestion
- Server overwhelmed
- Firewall/proxy dropping connection

**Mitigation:**
```c
// Set reasonable timeout
curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);  // 5 minutes

// Enable progress function
curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);

// Increase buffer size for large packs
git_config_set_int("http.postBuffer", 524288000);  // 500MB
```

### 10. Authentication Failures

**Symptom:**
- HTTP 401 or 403 before reaching git-receive-pack

**Common Causes:**
1. **Token expired**: Regenerate access token
2. **Wrong credentials**: Check username/password
3. **Insufficient permissions**: Need push access to repository
4. **Repository archived**: Cannot push to archived repos

**Debugging:**
```bash
# Enable verbose curl output
GIT_CURL_VERBOSE=1 git push origin main

# Check credential storage
git credential fill <<EOF
protocol=https
host=github.com
path=user/repo.git
EOF
```

---

## Summary

### Key Takeaways

1. **Pkt-line is fundamental**: Every piece of data uses 4-byte hex length prefix
2. **Sideband adds a layer**: When active, pkt-lines are wrapped with channel byte
3. **Channel 1 is protocol**: Report-status comes on channel 1 as pkt-lines
4. **Parse incrementally**: Read length, check channel, handle accordingly
5. **HTTP 200 != success**: Check report-status for actual push result
6. **Handle edge cases**: Invalid channels, non-sideband errors, flush packets

### Parsing Pseudocode

```python
def parse_push_response(stream):
    while True:
        # Read pkt-line length
        length = read_pkt_line_length(stream)

        if length == 0:
            break  # Flush packet - end of response

        # Read channel byte
        channel = stream.read(1)[0]

        # Read data
        data = stream.read(length - 5)

        if channel == 1:
            # Protocol data - embedded pkt-line
            inner_length = int(data[0:4], 16)

            if inner_length == 0:
                break  # End of report-status

            inner_data = data[4:inner_length]

            if inner_data.startswith(b"unpack "):
                parse_unpack_status(inner_data)
            elif inner_data.startswith(b"ok "):
                parse_ref_success(inner_data)
            elif inner_data.startswith(b"ng "):
                parse_ref_failure(inner_data)
            elif inner_data.startswith(b"option "):
                parse_option(inner_data)

        elif channel == 2:
            # Progress message
            print_to_stderr(data)

        elif channel == 3:
            # Fatal error
            die(data.decode('utf-8'))

        else:
            # Protocol violation - might be non-sideband error
            if data.startswith(b"ERR ") or data.startswith(b"error:"):
                die(data.decode('utf-8'))
            else:
                die(f"Invalid channel: {channel}")
```

---

## References

- [Git pack-protocol Documentation](https://git-scm.com/docs/pack-protocol)
- [Git gitprotocol-pack Documentation](https://git-scm.com/docs/gitprotocol-pack)
- [Git protocol-capabilities Documentation](https://git-scm.com/docs/protocol-capabilities)
- [Git HTTP Protocol Documentation](https://git-scm.com/docs/http-protocol)
- [Git Transfer Protocols (Book)](https://git-scm.com/book/en/v2/Git-Internals-Transfer-Protocols)
- [Git receive-pack Documentation](https://git-scm.com/docs/git-receive-pack)
- [Git source: sideband.c](https://github.com/git/git/blob/master/sideband.c)
- [Git source: receive-pack.c](https://github.com/git/git/blob/master/builtin/receive-pack.c)
- [Public Inbox: Invalid Channel 101 Discussion](https://public-inbox.org/git/CAENte7j9De5Bqu2jDcmXQAxZheSGo+EntzsYUaen0N7cnuiCDQ@mail.gmail.com/)
- [Git Rev News: Sideband Protocol Evolution](https://git.github.io/rev_news/2015/06/03/edition-4/)

---

**Document Version:** 1.0
**Last Updated:** 2025-12-13
**Author:** Research compiled for GitSmarter project

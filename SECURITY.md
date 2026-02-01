# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |

## Reporting a Vulnerability

We take security vulnerabilities seriously. If you discover a security issue, please report it responsibly.

### How to Report

1. **Create a private security advisory** on GitHub at: https://github.com/ByteAshen/CDP-for-CPP/security/advisories/new
2. Include the following information:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if any)

### What to Expect

- **Acknowledgment**: We will acknowledge receipt within 48 hours
- **Initial Assessment**: We will provide an initial assessment within 7 days
- **Resolution Timeline**: We aim to resolve critical issues within 30 days
- **Credit**: We will credit reporters in the release notes (unless you prefer anonymity)

## Security Considerations

### Chrome DevTools Protocol

CDP for C++ connects to Chrome's debugging port. Be aware of these security implications:

1. **Local Network Exposure**: When Chrome runs with `--remote-debugging-port`, it exposes a debugging endpoint. By default, this binds to `127.0.0.1` (localhost only). Never bind to `0.0.0.0` or expose the debugging port to untrusted networks.

2. **User Data Directory**: When using custom user data directories, the library modifies Chrome's `Secure Preferences` file to load extensions. Only use this feature with temporary profiles or directories you fully control.

3. **Extension Loading**: The extension loading feature grants extensions full permissions automatically. Only load trusted extensions from known sources.

### Best Practices

1. **Use Temporary Profiles**: Always use `useTempProfile = true` in production to isolate sessions
2. **Validate Input**: Sanitize any user-provided data before passing to CDP commands
3. **Network Isolation**: Run browser automation in isolated network environments when possible
4. **Credential Handling**: Never hardcode credentials. Use environment variables or secure vaults
5. **Timeout Configuration**: Set appropriate timeouts to prevent resource exhaustion

### Known Limitations

1. **No TLS for WebSocket**: The library currently uses unencrypted WebSocket connections (`ws://`) for CDP communication. This is acceptable for localhost connections but should not be used across networks.

2. **Process Permissions**: Browser processes launched by the library run with the same permissions as the parent process. Use appropriate OS-level sandboxing when needed.

## Dependency Security

CDP for C++ has **zero external dependencies** (only Windows system libraries). This minimizes supply chain attack surface. The library implements:

- Custom JSON parser (no external JSON library)
- Custom WebSocket client (RFC 6455 compliant)
- Custom Base64 and SHA1 implementations

All cryptographic operations (SHA1 for WebSocket handshake) are implemented internally and are not used for security-critical purposes.


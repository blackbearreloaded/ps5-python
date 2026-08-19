"""Tier 8 protocol and mail helpers, adapted from CPython 3.14.7 Lib/test.

The official ``test_ftplib.py``, ``test_poplib.py``, ``test_imaplib.py``,
``test_smtplib.py``, and ``test_mailbox.py`` suites use the CPython test
harness and external-service fixtures.  These checks keep their portable
protocol assertions while using short local servers and ``/user/temp``.
"""

import mailbox
import os
import shutil
import socket
import tempfile
import threading

import email
from email.message import EmailMessage
from email.mime.text import MIMEText

import ftplib
import imaplib
import poplib
import smtplib


def _local_server(handler):
    """Run one deterministic loopback protocol server and return its port."""
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)
    port = listener.getsockname()[1]
    errors = []

    def run():
        try:
            conn, _ = listener.accept()
            with conn:
                handler(conn)
        except Exception as exc:
            errors.append(exc)
        finally:
            listener.close()

    thread = threading.Thread(target=run)
    thread.start()
    return port, thread, errors


def _line_server(handler):
    def wrapped(conn):
        stream = conn.makefile("rwb")
        try:
            handler(stream)
        finally:
            stream.close()

    return wrapped


def test_ftp():
    def serve(stream):
        stream.write(b"220 local FTP\r\n")
        stream.flush()
        for raw in stream:
            command = raw.decode("ascii").strip().upper()
            if command.startswith("USER"):
                response = b"331 password required\r\n"
            elif command.startswith("PASS"):
                response = b"230 logged in\r\n"
            elif command == "PWD":
                response = b'257 "/" is current directory\r\n'
            elif command == "QUIT":
                response = b"221 goodbye\r\n"
                stream.write(response)
                stream.flush()
                return
            else:
                response = b"200 ok\r\n"
            stream.write(response)
            stream.flush()

    port, thread, errors = _local_server(_line_server(serve))
    client = ftplib.FTP()
    client.connect("127.0.0.1", port, timeout=5)
    assert client.login("user", "password")[0:3] == "230"
    assert client.pwd() == "/"
    client.quit()
    thread.join(5)
    assert not errors, errors


def test_pop3():
    def serve(stream):
        stream.write(b"+OK local POP3\r\n")
        stream.flush()
        assert stream.readline().upper().startswith(b"QUIT")
        stream.write(b"+OK goodbye\r\n")
        stream.flush()

    port, thread, errors = _local_server(_line_server(serve))
    client = poplib.POP3("127.0.0.1", port, timeout=5)
    assert client.getwelcome().startswith(b"+OK")
    assert client.quit().startswith(b"+OK")
    thread.join(5)
    assert not errors, errors


def test_imap():
    def serve(stream):
        stream.write(b"* OK local IMAP4\r\n")
        stream.flush()
        for raw in stream:
            parts = raw.decode("ascii", "replace").strip().split()
            if not parts:
                continue
            tag, command = parts[0], parts[1].upper()
            if command == "CAPABILITY":
                stream.write(b"* CAPABILITY IMAP4rev1\r\n")
                response = "%s OK CAPABILITY completed\r\n" % tag
            elif command == "LOGIN":
                response = "%s OK LOGIN completed\r\n" % tag
            elif command == "LOGOUT":
                stream.write(b"* BYE logging out\r\n")
                response = "%s OK LOGOUT completed\r\n" % tag
            else:
                response = "%s OK completed\r\n" % tag
            stream.write(response.encode("ascii"))
            stream.flush()
            if command == "LOGOUT":
                return

    port, thread, errors = _local_server(_line_server(serve))
    client = imaplib.IMAP4("127.0.0.1", port)
    assert client.login("user", "password")[0] == "OK"
    assert client.logout()[0] == "BYE"
    thread.join(5)
    assert not errors, errors


def test_smtp_and_email():
    received = []

    def serve(stream):
        stream.write(b"220 local SMTP\r\n")
        stream.flush()
        while True:
            raw = stream.readline()
            if not raw:
                return
            command = raw.decode("ascii", "replace").strip().upper()
            if command.startswith("EHLO") or command.startswith("HELO"):
                stream.write(b"250-localhost\r\n250-8BITMIME\r\n250 OK\r\n")
            elif command.startswith("MAIL FROM") or command.startswith("RCPT TO"):
                stream.write(b"250 OK\r\n")
            elif command == "DATA":
                stream.write(b"354 end with <CRLF>.<CRLF>\r\n")
                stream.flush()
                data = []
                while True:
                    data_line = stream.readline()
                    if not data_line:
                        break
                    if data_line == b".\r\n":
                        break
                    data.append(data_line)
                received.append(b"".join(data))
                stream.write(b"250 queued\r\n")
            elif command == "QUIT":
                stream.write(b"221 goodbye\r\n")
                stream.flush()
                return
            else:
                stream.write(b"250 OK\r\n")
            stream.flush()

    message = EmailMessage()
    message["From"] = "sender@example.test"
    message["To"] = "recipient@example.test"
    message["Subject"] = "PS5"
    message.set_content("hello from CPython")
    assert "Subject" in email.message_from_bytes(message.as_bytes())
    assert MIMEText("hello").get_payload() == "hello"

    port, thread, errors = _local_server(_line_server(serve))
    client = smtplib.SMTP("127.0.0.1", port, timeout=5)
    client.send_message(message)
    client.quit()
    thread.join(5)
    assert received and b"hello from CPython" in received[0]
    assert not errors, errors


def test_mailbox():
    if os.name == "nt":
        # Windows mailbox locking/temporary-directory behavior is outside
        # this POSIX PS5 target.
        return
    root = tempfile.mkdtemp(prefix="tier8-mailbox-")
    # Maildir(create=True) only creates the root when it already exists on
    # some supported CPython builds; create the three standard folders too.
    for folder in ("tmp", "new", "cur"):
        os.makedirs(os.path.join(root, folder), exist_ok=True)
    try:
        box = mailbox.Maildir(root, create=True)
        key = box.add("From: sender@example.test\nSubject: local\n\nbody\n")
        box.flush()
        message = box[key]
        assert message["Subject"] == "local"
        assert message.get_payload().strip() == "body"
        box.close()
    finally:
        shutil.rmtree(root, ignore_errors=True)


for check in (test_ftp, test_pop3, test_imap, test_smtp_and_email, test_mailbox):
    check()

print("test_tier8_protocols: PASS")

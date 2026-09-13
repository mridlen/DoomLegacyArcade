#!/usr/bin/env python3
"""[Arcade] A hostile or broken Cabinet Link peer, for tools/linktest.sh.

Plays the parts a real cabinet never would, so the engine's refusals can be
tested: a client that sends junk, a client that sends an oversized frame, a
client whose passcode proof is not bound to the TLS session, and a fake master
that answers with a bad proof.  Prints one RESULT line.

    linktest_peer.py garbage   HOST PORT
    linktest_peer.py bigframe  HOST PORT CERTDIR
    linktest_peer.py unbound   HOST PORT CERTDIR PASSCODE MASTER_CRT
    linktest_peer.py fakemaster PORT CERTDIR SECONDS

CERTDIR gets a throwaway key and certificate (made with the openssl CLI).
"""
import hashlib, hmac, os, socket, ssl, struct, subprocess, sys, time

MSG_AUTH = 1
PROTO_VERSION = 2   # must match LK_PROTO_VERSION in d_link.c


def make_cert(d):
    key, crt = os.path.join(d, "peer.key"), os.path.join(d, "peer.crt")
    if not os.path.exists(crt):
        os.makedirs(d, exist_ok=True)
        subprocess.run(["openssl", "req", "-x509", "-newkey", "ec",
                        "-pkeyopt", "ec_paramgen_curve:P-256", "-nodes",
                        "-subj", "/CN=linktest", "-days", "2",
                        "-keyout", key, "-out", crt],
                       check=True, capture_output=True)
    return key, crt


def pubkey_fp(crt):
    """SHA-256 of the DER SubjectPublicKeyInfo: the engine's cabinet id."""
    pub = subprocess.run(["openssl", "x509", "-in", crt, "-pubkey", "-noout"],
                         check=True, capture_output=True).stdout
    der = subprocess.run(["openssl", "pkey", "-pubin", "-outform", "DER"],
                         input=pub, check=True, capture_output=True).stdout
    return hashlib.sha256(der).digest()


def frame(t, payload=b""):
    return struct.pack("<IB", len(payload), t) + payload


def tls_client(host, port, certdir):
    key, crt = make_cert(certdir)
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE
    ctx.minimum_version = ssl.TLSVersion.TLSv1_3
    ctx.load_cert_chain(crt, key)
    raw = socket.create_connection((host, port), timeout=10)
    return ctx.wrap_socket(raw), crt


def closed_by_peer(s, wait=12):
    """True when the other end closes the connection within `wait` seconds."""
    s.settimeout(wait)
    try:
        return s.recv(1) == b""
    except (ssl.SSLError, ConnectionError, OSError) as e:
        return not isinstance(e, socket.timeout)


def main():
    mode = sys.argv[1]
    if mode == "garbage":
        host, port = sys.argv[2], int(sys.argv[3])
        s = socket.create_connection((host, port), timeout=10)
        s.sendall(os.urandom(4096))
        # The master may answer with a TLS alert before closing, so read to
        # end-of-file: closed means EOF (or a reset) inside the window.
        s.settimeout(12)
        gone = False
        try:
            while True:
                if s.recv(4096) == b"":
                    gone = True
                    break
        except socket.timeout:
            gone = False
        except OSError:
            gone = True
        print("RESULT garbage", "closed" if gone else "open")

    elif mode == "bigframe":
        host, port, certdir = sys.argv[2], int(sys.argv[3]), sys.argv[4]
        s, _ = tls_client(host, port, certdir)
        # An AUTH frame claiming to be a megabyte long, before authenticating.
        s.sendall(struct.pack("<IB", 1 << 20, MSG_AUTH))
        print("RESULT bigframe", "closed" if closed_by_peer(s, 4) else "open")

    elif mode == "unbound":
        host, port, certdir, passcode, master_crt = sys.argv[2:7]
        s, crt = tls_client(host, int(port), certdir)
        mine, master = pubkey_fp(crt), pubkey_fp(master_crt)
        lo, hi = sorted([mine, master])
        key = hashlib.pbkdf2_hmac("sha256", passcode.encode(),
                                  b"dla-link-v1-salt" + lo + hi, 60000, 32)
        # Everything right -- passcode, identities, roles -- except the TLS
        # exporter, which a proof lifted from another session would carry.
        other_session = bytes(32)
        proof = hmac.new(key, other_session + b"member" + mine + master,
                         hashlib.sha256).digest()
        s.sendall(frame(MSG_AUTH, bytes([PROTO_VERSION]) + proof))
        print("RESULT unbound", "closed" if closed_by_peer(s) else "open")

    elif mode == "udpjunk":
        # Game channel: packets from a machine that holds no key.  Half are
        # random, half are shaped like the engine's own plaintext packets (a
        # checksum-sized header, a packet type, zero padding) -- what a stranger
        # speaking the stock netcode would send.
        host, port, count = sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        for i in range(count):
            if i % 2:
                pkt = os.urandom(40 + i)
            else:
                pkt = struct.pack("<IBB", 0, 0, 12) + bytes(64)
            s.sendto(pkt, (host, port))
            time.sleep(0.02)
        print("RESULT udpjunk sent", count)

    elif mode == "fakemaster":
        port, certdir, secs = int(sys.argv[2]), sys.argv[3], float(sys.argv[4])
        key, crt = make_cert(certdir)
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.minimum_version = ssl.TLSVersion.TLSv1_3
        ctx.load_cert_chain(crt, key)
        # Do not ask for the member's certificate: with no CA loaded Python
        # would reject the self-signed one and abort the handshake.
        ctx.verify_mode = ssl.CERT_NONE
        ls = socket.socket()
        ls.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        ls.bind(("127.0.0.1", port))
        ls.listen(4)
        ls.settimeout(secs)
        got_auth = False
        deadline = time.time() + secs
        try:
            while time.time() < deadline and not got_auth:
                c, _ = ls.accept()
                try:
                    t = ctx.wrap_socket(c, server_side=True)
                    t.settimeout(4)
                    hdr = t.recv(5)
                    if len(hdr) == 5 and hdr[4] == MSG_AUTH:
                        got_auth = True
                        t.recv(struct.unpack("<I", hdr[:4])[0])
                        # A proof the real master would never send.
                        t.sendall(frame(MSG_AUTH, bytes([PROTO_VERSION]) + os.urandom(32)))
                        time.sleep(2)
                    t.close()
                except (ssl.SSLError, OSError):
                    pass
        except socket.timeout:
            pass
        print("RESULT fakemaster", "got_member_proof" if got_auth else "no_member_proof")


if __name__ == "__main__":
    main()

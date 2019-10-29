this challenge is protected. hitme.tasteless.eu:10001 protected challenges will require a proof-of-work like this:

sha1(abc123, input) prefix = 00000... you need to respond with a single line suffix to abc123, so that sha1(abc123[input]) has a 00000 prefix example: sha(abc12344739190).hexdigest = 000000872D5625DEE5FD0EA44B230D7A98C1B2CA

you can use go run pow.go abc123 00000 or python pow.py abc123 00000 to generate your own. the pow binary is go, compiled for linux/amd64.

---

this challenge was just to set you up for using the proof-of-work challenges. no actual code was harmed during the ctf.

// clarification 2019-10-27T10:19:46Z: the h.internal flag is set to false for requests from the outside/internet // ---

lol, taking pictures is fun, but dumpster diving is more fun!

we found something on a usb stick with a post-it saying "git.corp". can you do something with this?

hopefully you already got access in the beyondcorp 1 challenge.

author: ccm

# Exploit

There were two flags, and two challenges. The first one consisted of some information in form of pictures, essentually hinting for a network inlcuding:
- auth-proxy.corp, accessable from both outside (hitme.tasteless.eu:10501) and inside
- auth.corp, accessable from inside
    - /login is allowed to be access unauthenticated
    - /register is not accessable from the outside
- fileserver.corp, accessable from the inside
as well as a picture taken from a monitor, including the `ServeHTTP` function from the proxy.

## Beyondcorp1

The main vuln was that that the proxy would, after authentication, create a "raw" tcp stream to the backend.
That allows us to send a request to auth.corp/login, which is explicitly allowed for unauthenticated traffic.
As the TCP connection is not further processed after the initial connection we can issue a `Connection: keep-alive` header, and reuse the same TCP connection.
This allows us to issue a second request to `/register?user=ccm` and get a valid JWT token to login against the auth-proxy.
Afterwards we can request the flag from `file.corp/flag` (just requesting file.corp will show a directory listing, so no need to guess anything).

See `exploit1`

## Beyondcorp2

Now we are given an SSH private key (unprotected), as well as a hint to a hostname `git.corp`.
This time, as the SSH key hints to, we need to make a raw TCP connection to git.corp to talk SSH.

The auth-proxy allows HTTP CONNECT requests, but only from internal endpoints. We can use the same trick from Beyondcorp1: with a valid token we do a GET request to `auth-proxy.corp`, which results in an error ("internal interface"), and for the second request we do a HTTP CONNECT to git.corp:22. This allows us to speak raw TCP to git.corp port 22.
When we now do an SSH git@git.corp we get an info that there is a `flag` repository.
Now putting everything in a ssh-config file and doing `git clone git@git.corp:flag` we get a repository, which contains the flag.

See `exploit2`

ssh-config
```
Host git.corp
	UserKnownHostsFile /dev/null
	StrictHostKeyChecking no
	IdentityFile ../git/shared-key
	ProxyCommand go run exploit.go
```

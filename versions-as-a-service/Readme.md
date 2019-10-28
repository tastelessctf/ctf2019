versions as a service! 2019 has called and it's finally time to ditch github, gitlab & co!

we are proud to introduce you to versions-as-a-service, our enterprise version control service. finally get rid of any decentralized stuff and put it to the only place it belongs to, the cloud.

try yourself! as we are enterprise we can not provide you with a free version, however, as we believe in free software, we allow you to check out the demo version! it's free! find us at hitme.tasteless.eu:10201

deploy the demo! as we are cloud-native, we do functions as a service. like real. one request = one container. no shared resources. this is beyond-scaling! build your server with docker build -t vaas . and run with socat -v tcp-listen:1125,reuseaddr,fork exec:"docker run --rm -i --read-only --tmpfs /revs\:rw\,noexec\,nosuid\,size=11m --user 9000\:9000 vaas". flag is in /flag.

Author: ccm

sha1 7eaf07f1e3b1c4c1ff99b45db546606d vaas
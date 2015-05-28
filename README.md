DDoS Attack Visualization System (DAVS)
====

![](https://raw.githubusercontent.com/fridary/davs/master/screen.png)

Script that shows traffic on TCP server. Written on C using libpcap to handle incoming packets and JavaScript to visualize. WebSockets are used to send data from server to browser. System shows TCP state diagram, logs (tcpdump) and states (number of connections and ips on each state).

How to use
----------
#### Backend
You need root permissions to run program on your server. Connect to your server via terminal, upload files from backend folder and compile `main.c`
```
gcc main.c -o main -pthread -lpcap
```
Then execute program with any port
```
./main 8088
```

#### Frontend
Open `frontend/js/diagram.js` and modify host and port
```
var host = "127.0.0.1",
	ws = new WebSocket("ws://" + host + ":8088/echo");
```


Now you can monitor traffic on your server. To run a lot of random traffic from your machine, open terminal and run this command
```
touch /tmp/run; while [ -f /tmp/run ]; do for i in {1..30}; do curl http://127.0.0.1/ >/dev/null 2>&1 & done; wait; echo RUN; done
```
To stop it use this command in new terminal window
```
rm /tmp/run
```
# Server-Client
Fifo protocol decoder server implemented in C.

## Run in Docker
 - `docker compose up -d --build`

## Run locally
Enter the project the folder in two separate terminals.
- `make run_server` builds and runs the server application.
- `make run_clients` builds and runs clients sending requests to the server.

Test with: 
```bash
echo -n '$$159,866344056940484,2E69,A03,,230824200543,240|8|2724|20EEF33,4.21,100,003F,1,84D81B5DFC3A:-66|8ED81B5DFC3A:-66|8AD81B5DFC3A:-67|AC233FC0D496:-68|3C286D5FBD72:-68*55' | nc localhost 5124
```

Listen to port:
```bash
nc -l -p 5111
```

## Parameters
When runnning the server there are a couple of customizable parameters.
 - Port: Which port to listen to.
 - Pending: How many pending requests to allow before denying.
 - Max buffer size: Amount of bytes to allow to be parsed and sent.
 - Reuse port: Set to 1 when rerunning frequently to get past socket lingering time.
 - Forward url: Which address to send the decoded data to, skip to disable forwarding.
 - Api key: Whether to add api key to the end of the forwarding url.
